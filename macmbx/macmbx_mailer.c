/* macmbx_mailer.c — Mail send/receive engine
 * Part of macmbx: standalone mail data management library.
 *
 * Uses crispy for SMTP/POP3, macmbx for mailbox storage.
 * Portable: no Eudora, no GTK, no GLib.
 */

#include "macmbx_mailer.h"
#include "macmbx.h"
#include "macmbx_conf.h"
#include "crispy_smtp.h"
#include "crispy_pop3.h"
#include "crispy_imap.h"
#include "crispy_transport.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <sys/stat.h>
#include <pthread.h>
#ifdef _WIN32
  #include <direct.h>
  #define mkdir_p(p) _mkdir(p)
#else
  #include <unistd.h>
  #define mkdir_p(p) mkdir(p, 0755)
#endif

/* Max simultaneous IDLE sessions (one per IMAP account) */
#define MAILER_MAX_IDLE 16

/* ================================================================
 * Internal structure
 * ================================================================ */

struct MacmbxMailer {
  MacmbxConf *conf;
  MacmbxStore *store;
  /* Callbacks */
  MacmbxMailerProgressFn progress_fn;
  void *progress_ctx;
  MacmbxMailerCredentialFn cred_fn;
  void *cred_ctx;
  MacmbxMailerCertFn cert_fn;
  void *cert_ctx;
  /* Post-receive processing */
  MacmbxFilterSet *filters;
  MacmbxJunkConfig *junk;
  /* Signature */
  char *signature;
  /* LMOS directory */
  char lmos_dir[1024];
  /* IDLE sessions (optional, one per IMAP account) */
  struct {
    int account_index;
    pthread_t thread;
    volatile bool running;     /* set false to request stop */
    volatile bool active;      /* true while thread is alive */
    MacmbxIdleCallback cb;
    void *cb_ctx;
  } idle[MAILER_MAX_IDLE];
  int idle_count;
};

/* ================================================================
 * Lifecycle
 * ================================================================ */

MacmbxMailer *macmbx_mailer_new(MacmbxConf *conf, MacmbxStore *store) {
  if (!conf || !store) return NULL;
  MacmbxMailer *m = (MacmbxMailer *)calloc(1, sizeof(MacmbxMailer));
  if (!m) return NULL;
  m->conf = conf;
  m->store = store;
  /* LMOS dir next to the mailbox store */
  snprintf(m->lmos_dir, sizeof(m->lmos_dir), "%s/../lmos", store->base_path);
  return m;
}

MacmbxConf *macmbx_mailer_get_conf(MacmbxMailer *m) { return m ? m->conf : NULL; }

void macmbx_mailer_free(MacmbxMailer *m) {
  if (!m) return;
  free(m->signature);
  free(m);
}

void macmbx_mailer_set_progress(MacmbxMailer *m, MacmbxMailerProgressFn fn, void *ctx) {
  if (m) { m->progress_fn = fn; m->progress_ctx = ctx; }
}

void macmbx_mailer_set_credentials(MacmbxMailer *m, MacmbxMailerCredentialFn fn, void *ctx) {
  if (m) { m->cred_fn = fn; m->cred_ctx = ctx; }
}

void macmbx_mailer_set_cert_callback(MacmbxMailer *m, MacmbxMailerCertFn fn, void *ctx) {
  if (m) { m->cert_fn = fn; m->cert_ctx = ctx; }
}

void macmbx_mailer_set_filters(MacmbxMailer *m, MacmbxFilterSet *filters) {
  if (m) m->filters = filters;
}

void macmbx_mailer_set_junk(MacmbxMailer *m, MacmbxJunkConfig *junk) {
  if (m) m->junk = junk;
}

void macmbx_mailer_set_signature(MacmbxMailer *m, const char *sig) {
  if (!m) return;
  free(m->signature);
  m->signature = sig ? strdup(sig) : NULL;
}

/* ================================================================
 * Helpers
 * ================================================================ */

static void progress(MacmbxMailer *m, const char *status, int cur, int total) {
  if (m->progress_fn) m->progress_fn(status, cur, total, m->progress_ctx);
}

/* Get account settings — dominant (0) or personality (1+) */
static int get_account(MacmbxMailer *m, int index, MacmbxAccount *acct) {
  if (index <= 0) return macmbx_conf_get_dominant(m->conf, acct);
  return macmbx_conf_get_account(m->conf, index, acct);
}

/* Get password — from config or callback */
static int get_password(MacmbxMailer *m, MacmbxAccount *acct, char *pw, int sz) {
  /* Try config first */
  char sec[32];
  if (acct->index > 0) snprintf(sec, sizeof(sec), "account_%d", acct->index);
  else snprintf(sec, sizeof(sec), "checking_mail");
  const char *saved = macmbx_conf_get(m->conf, sec, "saved_password", "");
  if (saved && saved[0]) {
    snprintf(pw, sz, "%s", saved);
    return 0;
  }
  /* Try callback — try username first, then email, then name */
  if (m->cred_fn) {
    const char *user = acct->username[0] ? acct->username : acct->email;
    if (m->cred_fn(user, acct->server, pw, sz, m->cred_ctx) == 0)
      return 0;
    /* Also try email if username didn't match */
    if (acct->username[0] && acct->email[0])
      if (m->cred_fn(acct->email, acct->server, pw, sz, m->cred_ctx) == 0)
        return 0;
  }
  return -1;
}

/* TLS certificate callback — called by crispy_transport when server presents a cert.
 * Returns 0 to accept, nonzero to reject.
 * If the mailer has a cert_fn callback (from Eudora UI), use it to prompt the user.
 * Otherwise accept (for headless/automated use). */
static bool mailer_cert_callback(const char *host, const char *cert_pem,
                                  const char *error_msg, void *userdata) {
  MacmbxMailer *m = (MacmbxMailer *)userdata;
  (void)cert_pem;

  if (m && m->cert_fn) {
    int result = m->cert_fn(host, error_msg ? error_msg : "Unknown certificate",
                             m->cert_ctx);
    return (result >= 0); /* true = accept */
  }

  /* No UI callback — accept all (local/automated use) */
  return true;
}

/* Create and connect SMTP session for an account */
static SmtpSession *connect_smtp(MacmbxMailer *m, MacmbxAccount *acct) {
  SmtpTransport tp = crispy_transport_new();
  SmtpSession *smtp = (SmtpSession *)calloc(1, sizeof(SmtpSession));
  if (!smtp) return NULL;

  const char *server = acct->smtp_server[0] ? acct->smtp_server : acct->server;

  /* Read SMTP-specific SSL setting from config */
  int smtp_ssl = 0;
  if (m->conf) {
    smtp_ssl = macmbx_conf_get_int(m->conf, "ssl", "ssl_smtp_mode", 0);
    /* Also check per-account */
    if (acct->index > 0) {
      char sec[32]; snprintf(sec, sizeof(sec), "account_%d", acct->index);
      smtp_ssl = macmbx_conf_get_int(m->conf, sec, "ssl_mode", smtp_ssl);
    }
  }

  /* Port: 25 default, 587 if submission port enabled */
  bool use_submission = m->conf ? macmbx_conf_get_bool(m->conf, "sending_mail", "use_submission_port", false) : false;
  int port = use_submission ? 587 : 25;

  /* SSL/TLS mode (matches Eudora ESSLSetting bitmask):
   *   0 = plain, no TLS
   *   1 = optional STARTTLS (try, fall back to plain)
   *   2 = required STARTTLS
   *   3 = required STARTTLS
   *   4 = implicit SSL (alt port)
   *   6 = implicit SSL (alt port) + required */
  SmtpSecurity security = SMTP_PLAIN;
  if (smtp_ssl & 4) { security = SMTP_SSL; port = 465; }  /* esslUseAltPort */
  else if (smtp_ssl & 2) security = SMTP_STARTTLS;         /* esslUseTLS */
  else if (smtp_ssl & 1) security = SMTP_STARTTLS;         /* esslOptional — try STARTTLS */

  fprintf(stderr, "connect_smtp: server=%s port=%d ssl=%d\n", server, port, smtp_ssl);

  /* Set up TLS certificate handling */
  crispy_transport_set_cert_callback(&tp, mailer_cert_callback, m);

  crispy_smtp_init(smtp, tp, "localhost");
  int err = crispy_smtp_connect(smtp, server, port, security);
  if (err) { free(smtp); return NULL; }

  /* Authenticate */
  char pw[256] = "";
  if (get_password(m, acct, pw, sizeof(pw)) == 0 && pw[0]) {
    const char *user = acct->username[0] ? acct->username : acct->email;
    if (crispy_smtp_auth_plain(smtp, user, pw) != 0) {
      /* Try LOGIN if PLAIN fails */
      crispy_smtp_auth_login(smtp, user, pw);
    }
  }
  return smtp;
}

/* Create and connect POP3 session for an account */
static Pop3Session *connect_pop3(MacmbxMailer *m, MacmbxAccount *acct) {
  SmtpTransport tp = crispy_transport_new();
  Pop3Session *pop = (Pop3Session *)calloc(1, sizeof(Pop3Session));
  if (!pop) return NULL;

  /* Read POP-specific SSL setting from config */
  int pop_ssl = 0;
  if (m->conf) {
    pop_ssl = macmbx_conf_get_int(m->conf, "ssl", "ssl_pop_mode", 0);
    if (acct->index > 0) {
      char sec[32]; snprintf(sec, sizeof(sec), "account_%d", acct->index);
      pop_ssl = macmbx_conf_get_int(m->conf, sec, "ssl_mode", pop_ssl);
    }
  }

  /* Port: 110 default, 995 if alt port SSL */
  int port = 110;
  Pop3Security security = POP3_PLAIN;
  if (pop_ssl & 4) { security = POP3_SSL; port = 995; }  /* esslUseAltPort */
  else if (pop_ssl & 2) security = POP3_STLS;             /* esslUseTLS = STLS */
  else if (pop_ssl & 1) security = POP3_STLS;             /* esslOptional = try STLS */

  /* Set up TLS certificate handling */
  crispy_transport_set_cert_callback(&tp, mailer_cert_callback, m);

  fprintf(stderr, "connect_pop3: server=%s port=%d ssl=%d\n", acct->server, port, pop_ssl);

  crispy_pop3_init(pop, tp);
  int err = crispy_pop3_connect(pop, acct->server, port, security);
  if (err) {
    fprintf(stderr, "connect_pop3: connect failed err=%d\n", err);
    free(pop); return NULL;
  }

  char pw[256] = "";
  if (get_password(m, acct, pw, sizeof(pw)) != 0) {
    fprintf(stderr, "connect_pop3: no password for %s\n", acct->username);
    free(pop); return NULL;
  }
  const char *user = acct->username[0] ? acct->username : acct->email;
  fprintf(stderr, "connect_pop3: authenticating user='%s' pw_len=%d\n", user, (int)strlen(pw));
  err = crispy_pop3_auth(pop, user, pw);
  fprintf(stderr, "connect_pop3: auth result=%d reply='%s'\n", err, pop->last_reply ? pop->last_reply : "");
  if (err) {
    fprintf(stderr, "connect_pop3: auth failed for %s\n", user);
    crispy_pop3_close(pop); free(pop); return NULL;
  }

  return pop;
}

/* Extract bare email from "Name <addr>" or "addr" */
static void extract_bare_email(const char *addr, char *bare, int sz) {
  const char *lt = strchr(addr, '<');
  if (lt) {
    lt++;
    const char *gt = strchr(lt, '>');
    int len = gt ? (int)(gt - lt) : (int)strlen(lt);
    if (len >= sz) len = sz - 1;
    memcpy(bare, lt, len);
    bare[len] = '\0';
  } else {
    snprintf(bare, sz, "%s", addr);
  }
}

/* Collect all recipients from To/Cc/Bcc headers into a flat array */
static int collect_recipients(const char *msg, const char ***out_rcpts) {
  char *to = NULL, *cc = NULL, *bcc = NULL;
  /* Simple header extraction */
  const char *p = msg;
  while (*p) {
    if (strncasecmp(p, "To:", 3) == 0) {
      p += 3; while (*p == ' ') p++;
      const char *end = p; while (*end && *end != '\r' && *end != '\n') end++;
      to = strndup(p, end - p);
    } else if (strncasecmp(p, "Cc:", 3) == 0) {
      p += 3; while (*p == ' ') p++;
      const char *end = p; while (*end && *end != '\r' && *end != '\n') end++;
      cc = strndup(p, end - p);
    } else if (strncasecmp(p, "Bcc:", 4) == 0) {
      p += 4; while (*p == ' ') p++;
      const char *end = p; while (*end && *end != '\r' && *end != '\n') end++;
      bcc = strndup(p, end - p);
    }
    /* End of headers */
    if (*p == '\r' || *p == '\n') {
      if (p[0] == '\r' && p[1] == '\n') break;
      if (p[0] == '\n') break;
    }
    while (*p && *p != '\n') p++;
    if (*p == '\n') p++;
  }

  /* Count and collect */
  int cap = 16, count = 0;
  const char **rcpts = (const char **)calloc(cap, sizeof(char *));

  char *lists[] = { to, cc, bcc };
  for (int i = 0; i < 3; i++) {
    if (!lists[i]) continue;
    char *tok = strtok(lists[i], ",");
    while (tok) {
      while (*tok == ' ') tok++;
      if (count >= cap - 1) { cap *= 2; rcpts = realloc(rcpts, cap * sizeof(char *)); }
      char *bare = (char *)malloc(256);
      extract_bare_email(tok, bare, 256);
      rcpts[count++] = bare;
      tok = strtok(NULL, ",");
    }
  }
  rcpts[count] = NULL;
  free(to); free(cc); free(bcc);
  *out_rcpts = rcpts;
  return count;
}

static void free_rcpts(const char **rcpts) {
  if (!rcpts) return;
  for (int i = 0; rcpts[i]; i++) free((void *)rcpts[i]);
  free(rcpts);
}

/* ================================================================
 * Outbound — queue and send
 * ================================================================ */

int macmbx_mailer_queue(MacmbxMailer *m, const char *message, long len,
                          const char *from, const char **rcpts) {
  if (!m || !message) return -1;
  MacmbxNode *out_node = macmbx_store_find_special(m->store, MACMBX_TYPE_OUT);
  if (!out_node) return -1;
  MacmbxTOC *out = macmbx_toc_open(out_node->path);
  if (!out) return -1;
  int idx = macmbx_append_message(out, message, len, from, MACMBX_QUEUED, 3);
  macmbx_toc_save(out);
  return idx;
}

int macmbx_mailer_queue_as(MacmbxMailer *m, int account_index,
                             const char *message, long len,
                             const char *from, const char **rcpts) {
  int idx = macmbx_mailer_queue(m, message, len, from, rcpts);
  if (idx >= 0 && account_index > 0) {
    MacmbxAccount acct;
    if (macmbx_conf_get_account(m->conf, account_index, &acct) == 0) {
      MacmbxNode *out_node = macmbx_store_find_special(m->store, MACMBX_TYPE_OUT);
      MacmbxTOC *out = macmbx_toc_open(out_node->path);
      if (out) macmbx_tag_sending_personality(out, idx, acct.name);
    }
  }
  return idx;
}

int macmbx_mailer_send(MacmbxMailer *m) {
  if (!m) return -1;
  int total_sent = 0;

  /* Send dominant account queued messages */
  MacmbxAccount dom;
  macmbx_conf_get_dominant(m->conf, &dom);
  total_sent += macmbx_mailer_send_account(m, 0);

  /* Send each personality's queued messages */
  int n = macmbx_conf_count_accounts(m->conf);
  for (int i = 1; i <= n; i++)
    total_sent += macmbx_mailer_send_account(m, i);

  return total_sent;
}

int macmbx_mailer_send_account(MacmbxMailer *m, int account_index) {
  if (!m) return -1;
  MacmbxAccount acct;
  if (get_account(m, account_index, &acct) != 0) {
    fprintf(stderr, "mailer_send_account: get_account(%d) failed\n", account_index);
    return -1;
  }

  MacmbxNode *out_node = macmbx_store_find_special(m->store, MACMBX_TYPE_OUT);
  if (!out_node) {
    fprintf(stderr, "mailer_send_account: no Out mailbox in store\n");
    return -1;
  }
  MacmbxTOC *out = macmbx_toc_open(out_node->path);
  if (!out) {
    fprintf(stderr, "mailer_send_account: failed to open Out TOC at %s\n", out_node->path);
    return -1;
  }

  /* Count queued messages for this account */
  uint32_t pers_hash = account_index > 0 ? macmbx_personality_hash(acct.name) : 0;
  int queued = 0;
  for (int i = 0; i < out->count; i++) {
    if (out->msgs[i].state != MACMBX_QUEUED) continue;
    if (out->msgs[i].flags & MACMBX_FLAG_DELETED) continue;
    if (pers_hash && out->msgs[i].pers_id != pers_hash) continue;
    queued++;
  }
  fprintf(stderr, "mailer_send_account[%d]: acct='%s' email='%s' smtp='%s' queued=%d out->count=%d\n",
          account_index, acct.name, acct.email, acct.smtp_server, queued, out->count);
  if (queued == 0) return 0;

  /* Connect SMTP */
  progress(m, "Connecting to SMTP...", 0, queued);
  SmtpSession *smtp = connect_smtp(m, &acct);
  if (!smtp) {
    fprintf(stderr, "mailer_send_account: SMTP connect failed to %s\n", acct.smtp_server);
    return -1;
  }

  /* Send each queued message */
  int sent = 0;
  for (int i = 0; i < out->count; i++) {
    if (out->msgs[i].state != MACMBX_QUEUED) continue;
    if (out->msgs[i].flags & MACMBX_FLAG_DELETED) continue;
    if (pers_hash && out->msgs[i].pers_id != pers_hash) continue;

    progress(m, out->msgs[i].subject, sent + 1, queued);

    /* Read message from mbox */
    long msgLen = 0;
    char *raw = macmbx_read_message(out, i, &msgLen);
    if (!raw) continue;

    /* Skip From line */
    char *msg = raw;
    if (strncmp(msg, "From ", 5) == 0) {
      char *nl = strchr(msg, '\n');
      if (nl) { msg = nl + 1; msgLen = msgLen - (msg - raw); }
    }

    /* Append signature if set */
    char *withSig = NULL;
    if (m->signature && m->signature[0]) {
      long sigLen = (long)strlen(m->signature);
      withSig = (char *)malloc(msgLen + sigLen + 10);
      if (withSig) {
        memcpy(withSig, msg, msgLen);
        int pos = (int)msgLen;
        memcpy(withSig + pos, "\r\n-- \r\n", 7); pos += 7;
        memcpy(withSig + pos, m->signature, sigLen); pos += sigLen;
        withSig[pos] = '\0';
        msg = withSig;
        msgLen = pos;
      }
    }

    /* Extract from and recipients from headers */
    char fromBare[256] = "";
    const char *fromHdr = acct.email;
    extract_bare_email(fromHdr, fromBare, sizeof(fromBare));

    const char **rcpts = NULL;
    collect_recipients(msg, &rcpts);

    /* Send */
    int err = crispy_smtp_send(smtp, fromBare, rcpts, msg, msgLen);
    if (err == 0) {
      macmbx_set_state(out, i, MACMBX_SENT);
      sent++;
    }

    free_rcpts(rcpts);
    free(withSig);
    free(raw);
  }

  crispy_smtp_close(smtp);
  free(smtp);
  macmbx_toc_save(out);

  progress(m, "Send complete", sent, sent);
  return sent;
}

int macmbx_mailer_send_now(MacmbxMailer *m, const char *message, long len,
                             const char *from, const char **rcpts) {
  return macmbx_mailer_send_now_as(m, 0, message, len, from, rcpts);
}

int macmbx_mailer_send_now_as(MacmbxMailer *m, int account_index,
                                const char *message, long len,
                                const char *from, const char **rcpts) {
  if (!m || !message || !from) return -1;
  if (len < 0) len = (long)strlen(message);

  MacmbxAccount acct;
  if (get_account(m, account_index, &acct) != 0) {
    fprintf(stderr, "mailer_send_now: get_account(%d) failed\n", account_index);
    return -1;
  }

  SmtpSession *smtp = connect_smtp(m, &acct);
  if (!smtp) {
    fprintf(stderr, "mailer_send_now: SMTP connect failed to %s\n", acct.smtp_server);
    return -1;
  }

  char fromBare[256];
  extract_bare_email(from, fromBare, sizeof(fromBare));

  /* If recipients not provided, extract from message headers */
  const char **use_rcpts = rcpts;
  bool own_rcpts = false;
  if (!rcpts) {
    collect_recipients(message, &use_rcpts);
    own_rcpts = true;
  }

  int err = crispy_smtp_send(smtp, fromBare, use_rcpts, message, len);
  if (err) fprintf(stderr, "mailer_send_now: crispy_smtp_send returned %d\n", err);

  if (own_rcpts) free_rcpts(use_rcpts);
  crispy_smtp_close(smtp);
  free(smtp);
  return err;
}

int macmbx_mailer_cancel(MacmbxMailer *m, int queue_index) {
  if (!m) return -1;
  MacmbxNode *out_node = macmbx_store_find_special(m->store, MACMBX_TYPE_OUT);
  if (!out_node) return -1;
  MacmbxTOC *out = macmbx_toc_open(out_node->path);
  if (!out) return -1;
  macmbx_delete_message(out, queue_index);
  macmbx_toc_save(out);
  return 0;
}

int macmbx_mailer_queued_count(MacmbxMailer *m) {
  if (!m) return 0;
  MacmbxNode *out_node = macmbx_store_find_special(m->store, MACMBX_TYPE_OUT);
  if (!out_node) return 0;
  MacmbxTOC *out = macmbx_toc_open(out_node->path);
  if (!out) return 0;
  int count = 0;
  for (int i = 0; i < out->count; i++)
    if (out->msgs[i].state == MACMBX_QUEUED && !(out->msgs[i].flags & MACMBX_FLAG_DELETED))
      count++;
  return count;
}

/* ================================================================
 * Inbound — check and receive
 * ================================================================ */

/* Per-account check thread data */
typedef struct {
  MacmbxMailer *m;
  int account_index;
  int result;
} CheckThreadData;

#include <pthread.h>

static void *check_account_thread(void *arg) {
  CheckThreadData *d = (CheckThreadData *)arg;
  d->result = macmbx_mailer_check_account(d->m, d->account_index);
  return NULL;
}

int macmbx_mailer_check(MacmbxMailer *m) {
  if (!m) return -1;

  int acct_count = macmbx_conf_count_accounts(m->conf);
  int total_accounts = 1 + acct_count; /* dominant + personalities */

  /* Allocate thread data for all accounts */
  CheckThreadData *data = (CheckThreadData *)calloc(total_accounts, sizeof(CheckThreadData));
  pthread_t *threads = (pthread_t *)calloc(total_accounts, sizeof(pthread_t));
  if (!data || !threads) { free(data); free(threads); return -1; }

  /* Launch all checks in parallel */
  for (int i = 0; i < total_accounts; i++) {
    data[i].m = m;
    data[i].account_index = i; /* 0 = dominant, 1+ = personalities */
    data[i].result = 0;

    MacmbxAccount acct;
    if (get_account(m, i, &acct) != 0) continue;

    /* Only check enabled accounts with a server configured */
    if (!acct.enabled || !acct.server[0]) continue;

    progress(m, acct.name[0] ? acct.name : "Checking mail...", i, total_accounts);
    pthread_create(&threads[i], NULL, check_account_thread, &data[i]);
  }

  /* Wait for all to finish */
  int total = 0;
  for (int i = 0; i < total_accounts; i++) {
    if (threads[i]) {
      pthread_join(threads[i], NULL);
      if (data[i].result > 0) total += data[i].result;
    }
  }

  free(data);
  free(threads);
  return total;
}

int macmbx_mailer_check_dominant(MacmbxMailer *m) {
  return macmbx_mailer_check_account(m, 0);
}

/* ================================================================
 * IMAP check — full sync ported from CrispinIMAP DoFetchNewMessages
 *
 * Sync algorithm (from imapdownload.c:664-900):
 * 1. Build local UID list from TOC (≡ SpecToUIDList)
 * 2. Get highest local UID (≡ GetLocalHighestUid)
 * 3. Connect, SELECT mailbox
 * 4. Check UIDVALIDITY — if changed, redownload all
 * 5. If remote has 0 messages → delete all local
 * 6. If fetchFlags (or redownloadAll): FetchAllFlags for ALL messages
 *    else: fetch UIDs for range lastUID+1:*
 * 7. MergeUidLists: compare local vs remote →
 *      toDelete (local only), toFetch (remote only), toUpdate (flag changes)
 * 8. Delete local messages removed from server
 * 9. Fetch new messages
 * 10. Update flags on existing local messages
 * 11. Save state (UIDVALIDITY + highest UID)
 * ================================================================ */

/* Map IMAP flags to Eudora state.
 * Original CrispinIMAP stored flags in UIDNode then applied them
 * through the delivery pipeline. We do it directly. */
static uint8_t imap_flags_to_state(ImapFlags f) {
  if (f.answered) return MACMBX_REPLIED;
  if (f.draft)    return MACMBX_UNSENT;
  if (f.seen)     return MACMBX_READ;
  return MACMBX_UNREAD;
}

/* Build sorted UID list from local TOC (≡ SpecToUIDList).
 * Returns count, allocates *out_uids (caller frees). */
static int build_local_uid_list(MacmbxTOC *toc, unsigned long **out_uids) {
  if (!toc || toc->count == 0) {
    *out_uids = NULL;
    return 0;
  }
  unsigned long *uids = calloc(toc->count, sizeof(unsigned long));
  if (!uids) { *out_uids = NULL; return 0; }
  int n = 0;
  for (int i = 0; i < toc->count; i++) {
    unsigned long uid = toc->msgs[i].uid_hash;
    if (uid == 0) continue;
    uids[n++] = uid;
  }
  /* Sort ascending (≡ UID_LL_OrderedInsert building sorted list) */
  for (int i = 1; i < n; i++) {
    unsigned long key = uids[i];
    int j = i - 1;
    while (j >= 0 && uids[j] > key) { uids[j+1] = uids[j]; j--; }
    uids[j+1] = key;
  }
  *out_uids = uids;
  return n;
}

/* Get highest UID from sorted list (≡ GetLocalHighestUid) */
static unsigned long get_local_highest_uid(unsigned long *uids, int count) {
  if (!uids || count == 0) return 0;
  return uids[count - 1];
}

/* MergeUidLists — ported from imapdownload.c:1124-1172
 *
 * Both lists must be sorted ascending by UID.
 * After merge:
 *   local_uids  → contains UIDs no longer on server (to delete locally)
 *   remote_uids → contains UIDs that need to be fetched (new on server)
 *   update_uids + update_flags → contains flag updates for local msgs still on server
 *
 * Counts are updated in place. */
static void merge_uid_lists(
    unsigned long *local_uids,  int *local_count,
    unsigned long *remote_uids, ImapFlags *remote_flags, int *remote_count,
    unsigned long **out_update_uids, ImapFlags **out_update_flags, int *out_update_count)
{
  int lc = *local_count;
  int rc = *remote_count;

  /* Allocate update list (at most min(lc, rc) entries) */
  int max_updates = lc < rc ? lc : rc;
  unsigned long *upd_uids = max_updates > 0 ? calloc(max_updates, sizeof(unsigned long)) : NULL;
  ImapFlags *upd_flags = max_updates > 0 ? calloc(max_updates, sizeof(ImapFlags)) : NULL;
  int upd_count = 0;

  /* Walk both sorted lists simultaneously (≡ MergeUidLists logic) */
  int li = 0, ri = 0;
  int new_local = 0, new_remote = 0;

  while (li < lc && ri < rc) {
    if (local_uids[li] < remote_uids[ri]) {
      /* Local UID not on server → keep in local list (to delete) */
      local_uids[new_local++] = local_uids[li++];
    } else if (local_uids[li] > remote_uids[ri]) {
      /* Remote UID not local → keep in remote list (to fetch) */
      remote_uids[new_remote] = remote_uids[ri];
      remote_flags[new_remote] = remote_flags[ri];
      new_remote++;
      ri++;
    } else {
      /* Same UID — exists on both sides → update flags, remove from both lists */
      if (upd_uids && upd_flags) {
        upd_uids[upd_count] = remote_uids[ri];
        upd_flags[upd_count] = remote_flags[ri];
        upd_count++;
      }
      li++;
      ri++;
    }
  }

  /* Remaining locals not on server → to delete */
  while (li < lc) local_uids[new_local++] = local_uids[li++];

  /* Remaining remotes not local → to fetch */
  while (ri < rc) {
    remote_uids[new_remote] = remote_uids[ri];
    remote_flags[new_remote] = remote_flags[ri];
    new_remote++;
    ri++;
  }

  *local_count = new_local;
  *remote_count = new_remote;
  *out_update_uids = upd_uids;
  *out_update_flags = upd_flags;
  *out_update_count = upd_count;
}

/* Find TOC index by uid_hash */
static int find_toc_index_by_uid(MacmbxTOC *toc, unsigned long uid) {
  for (int i = 0; i < toc->count; i++)
    if (toc->msgs[i].uid_hash == uid) return i;
  return -1;
}

static int macmbx_mailer_check_imap(MacmbxMailer *m, int account_index,
                                      MacmbxAccount *acct) {
  /* Read IMAP SSL setting */
  int imap_ssl = 0;
  if (m->conf) {
    imap_ssl = macmbx_conf_get_int(m->conf, "ssl", "ssl_imap_mode", 0);
    if (acct->index > 0) {
      char sec[32]; snprintf(sec, sizeof(sec), "account_%d", acct->index);
      imap_ssl = macmbx_conf_get_int(m->conf, sec, "ssl_mode", imap_ssl);
    }
  }

  int port = 143;
  ImapSecurity security = IMAP_PLAIN;
  if (imap_ssl & 4) { security = IMAP_SSL; port = 993; }
  else if (imap_ssl & 2) security = IMAP_STARTTLS;
  else if (imap_ssl & 1) security = IMAP_STARTTLS;

  fprintf(stderr, "check_imap[%d]: server=%s port=%d ssl=%d\n",
          account_index, acct->server, port, imap_ssl);

  /* --- Step 1: Build local UID list from TOC (≡ SpecToUIDList) --- */
  MacmbxNode *in_node = macmbx_store_find_special(m->store, MACMBX_TYPE_IN);
  if (!in_node) return -1;
  MacmbxTOC *inbox = macmbx_toc_open(in_node->path);
  if (!inbox) return -1;

  unsigned long *local_uids = NULL;
  int local_count = build_local_uid_list(inbox, &local_uids);

  /* --- Step 2: Get highest local UID (≡ GetLocalHighestUid) --- */
  unsigned long local_highest = get_local_highest_uid(local_uids, local_count);

  /* --- Step 3: Connect + SELECT (≡ GetIMAPConnection + IMAPOpenMailbox) --- */
  ImapTransport tp = crispy_transport_new();
  crispy_transport_set_cert_callback(&tp, mailer_cert_callback, m);

  ImapSession imap;
  crispy_imap_init(&imap, tp);
  int err = crispy_imap_connect(&imap, acct->server, port, security);
  if (err) {
    fprintf(stderr, "check_imap[%d]: connect failed\n", account_index);
    free(local_uids);
    return -1;
  }

  char pw[256] = "";
  if (get_password(m, acct, pw, sizeof(pw)) != 0) {
    crispy_imap_close(&imap);
    free(local_uids);
    return -1;
  }
  const char *user = acct->username[0] ? acct->username : acct->email;
  err = crispy_imap_login(&imap, user, pw);
  if (err) {
    fprintf(stderr, "check_imap[%d]: login failed for %s\n", account_index, user);
    crispy_imap_close(&imap);
    free(local_uids);
    return -1;
  }

  err = crispy_imap_select(&imap, "INBOX");
  if (err) {
    fprintf(stderr, "check_imap[%d]: SELECT INBOX failed\n", account_index);
    crispy_imap_close(&imap);
    free(local_uids);
    return -1;
  }
  fprintf(stderr, "check_imap[%d]: INBOX selected, %ld msgs, uidvalidity=%lu\n",
          account_index, imap.selected.exists, imap.selected.uidvalidity);

  /* --- Step 4: Load saved UIDVALIDITY, check for change --- */
  unsigned long saved_uidvalidity = 0;
  {
    const char *lmos_path = macmbx_mailer_lmos_path(m, account_index);
    if (lmos_path) {
      FILE *f = fopen(lmos_path, "r");
      if (f) {
        char line[128];
        if (fgets(line, sizeof(line), f)) saved_uidvalidity = strtoul(line, NULL, 10);
        fclose(f);
      }
    }
  }
  unsigned long server_uidvalidity = imap.selected.uidvalidity;
  bool redownload_all = false;
  if (saved_uidvalidity != 0 && server_uidvalidity != saved_uidvalidity) {
    fprintf(stderr, "check_imap[%d]: UIDVALIDITY changed (%lu -> %lu), full resync\n",
            account_index, saved_uidvalidity, server_uidvalidity);
    redownload_all = true;
  }

  /* --- Step 5: If remote has 0 messages → delete all local IMAP msgs --- */
  int downloaded = 0;
  int deleted = 0;
  int updated = 0;

  if (imap.selected.exists == 0) {
    /* ≡ delivery->td = MSUM_DELETE_ALL */
    fprintf(stderr, "check_imap[%d]: remote empty, deleting all %d local msgs\n",
            account_index, local_count);
    /* Delete local messages with IMAP UIDs (backwards to preserve indices) */
    for (int i = inbox->count - 1; i >= 0; i--) {
      if (inbox->msgs[i].uid_hash != 0)
        macmbx_delete_message(inbox, i);
    }
    deleted = local_count;
    goto save_state;
  }

  /* --- Steps 6-7: Fetch flags or just new UIDs --- */
  /*
   * ≡ CrispinIMAP logic (imapdownload.c:786-837):
   * if redownloadAll or fetchFlags → FetchAllFlags(1:*)
   * else → FetchFlags(lastUID+1:*)
   *
   * fetchFlags is true when we want full sync (flag changes, deletes).
   * We always fetch flags to get the complete picture — CrispinIMAP
   * does this on manual check and periodically.
   */
  bool fetch_flags = redownload_all || (local_count > 0);
  /* If this is first sync with no local messages, just fetch UIDs */
  if (local_count == 0 && !redownload_all) fetch_flags = false;

  unsigned long *remote_uids = NULL;
  ImapFlags *remote_flags = NULL;
  int remote_count = 0;

  if (fetch_flags) {
    /* ≡ FetchAllFlags(imapStream, &remoteList) — fetch flags for ALL messages */
    int fc = 0;
    err = crispy_imap_fetch_flags(&imap, "1:*", &remote_uids, &remote_flags, &fc);
    if (err || fc <= 0) {
      fprintf(stderr, "check_imap[%d]: FETCH FLAGS 1:* failed\n", account_index);
      free(local_uids); crispy_imap_close(&imap);
      return -1;
    }
    remote_count = fc;
    fprintf(stderr, "check_imap[%d]: fetched flags for %d remote msgs\n",
            account_index, remote_count);
  } else {
    /* ≡ FetchFlags(imapStream, "localHighest+1:*", &remoteList)
     * Only look at new messages, don't track deletes/flags this time */
    if (local_highest > 0) {
      char range[64];
      snprintf(range, sizeof(range), "%lu:*", local_highest + 1);
      int fc = 0;
      err = crispy_imap_fetch_flags(&imap, range, &remote_uids, &remote_flags, &fc);
      if (err) { free(local_uids); crispy_imap_close(&imap); return -1; }
      remote_count = fc;
      /* Remove local_highest itself if returned (UID ranges are inclusive) */
      if (remote_count > 0 && remote_uids[0] == local_highest) {
        memmove(remote_uids, remote_uids + 1, (remote_count - 1) * sizeof(unsigned long));
        memmove(remote_flags, remote_flags + 1, (remote_count - 1) * sizeof(ImapFlags));
        remote_count--;
      }
    } else {
      /* First sync — get all */
      int fc = 0;
      err = crispy_imap_fetch_flags(&imap, "1:*", &remote_uids, &remote_flags, &fc);
      if (err || fc <= 0) { free(local_uids); crispy_imap_close(&imap); return -1; }
      remote_count = fc;
    }
    fprintf(stderr, "check_imap[%d]: %d new UIDs (range %lu:*)\n",
            account_index, remote_count, local_highest + 1);
  }

  /* --- Step 6b: If redownloadAll, zap local list (≡ UID_LL_Zap(&localList)) --- */
  if (redownload_all) {
    /* ≡ CrispinIMAP: UID_LL_Zap(&localList) + delivery->td = MSUM_DELETE_ALL
     * Delete all local messages, then fetch everything */
    fprintf(stderr, "check_imap[%d]: UIDVALIDITY resync — deleting %d local msgs\n",
            account_index, inbox->count);
    for (int i = inbox->count - 1; i >= 0; i--) {
      if (inbox->msgs[i].uid_hash != 0)
        macmbx_delete_message(inbox, i);
    }
    deleted = local_count;
    local_count = 0;
    /* All remote messages are now "to fetch" — skip merge */
  }

  /* --- Step 7: MergeUidLists (≡ imapdownload.c:1124-1172) --- */
  unsigned long *update_uids = NULL;
  ImapFlags *update_flags = NULL;
  int update_count = 0;

  if (!redownload_all && fetch_flags && local_count > 0) {
    /* Full merge: compare local vs remote → toDelete, toFetch, toUpdate */
    merge_uid_lists(local_uids, &local_count,
                    remote_uids, remote_flags, &remote_count,
                    &update_uids, &update_flags, &update_count);
    fprintf(stderr, "check_imap[%d]: merge: %d to_delete, %d to_fetch, %d to_update\n",
            account_index, local_count, remote_count, update_count);
  } else if (!fetch_flags) {
    /* ≡ CrispinIMAP: if !fetchFlags, zap localList (no deletes),
     * remoteList stays as-is (all to fetch), no updates.
     * "if we didn't fetch new uid's, then we're not going to change
     *  the local cache at all. Delete the local list so we don't
     *  end up deleting anything locally." */
    local_count = 0; /* nothing to delete */
  }

  /* --- Step 8: Delete local messages removed from server --- */
  /* ≡ delivery->td = UIDNodeList2Handle(&localList) then ProcessDelivery removes them */
  if (local_count > 0) {
    fprintf(stderr, "check_imap[%d]: deleting %d msgs removed from server\n",
            account_index, local_count);
    for (int d = 0; d < local_count; d++) {
      int idx = find_toc_index_by_uid(inbox, local_uids[d]);
      if (idx >= 0) {
        macmbx_delete_message(inbox, idx);
        deleted++;
      }
    }
  }

  /* --- Step 9: Fetch new messages (≡ UIDFetchMessages) --- */
  if (remote_count > 0) {
    fprintf(stderr, "check_imap[%d]: fetching %d new messages\n",
            account_index, remote_count);
    for (int i = 0; i < remote_count; i++) {
      progress(m, "Downloading...", i + 1, remote_count);

      long msgLen = 0;
      char *msg = crispy_imap_fetch_message(&imap, remote_uids[i], &msgLen);
      if (!msg || msgLen <= 0) { free(msg); continue; }

      fprintf(stderr, "check_imap[%d]: FETCH UID %lu -> %ld bytes\n",
              account_index, remote_uids[i], msgLen);

      int idx = macmbx_append_message(inbox, msg, msgLen, NULL,
                                       imap_flags_to_state(remote_flags[i]), 3);
      if (idx >= 0) {
        /* Store UID in the summary so we can track it next sync */
        inbox->msgs[idx].uid_hash = (uint32_t)remote_uids[i];
        if (account_index > 0)
          macmbx_tag_personality(inbox, idx, acct->name);
        downloaded++;
      }
      free(msg);
    }
  }

  /* --- Step 10: Update flags on existing local messages (≡ delivery->tu) --- */
  /* ≡ CrispinIMAP: updateList contains new flags for messages still on server.
   * ProcessDelivery walks tu and updates each summary's state/flags. */
  if (update_count > 0) {
    fprintf(stderr, "check_imap[%d]: updating flags on %d existing msgs\n",
            account_index, update_count);
    for (int u = 0; u < update_count; u++) {
      int idx = find_toc_index_by_uid(inbox, update_uids[u]);
      if (idx < 0) continue;

      uint8_t new_state = imap_flags_to_state(update_flags[u]);
      uint8_t cur_state = inbox->msgs[idx].state;

      /* Only update if the flag-derived state differs from current.
       * Don't downgrade states set locally (e.g. REPLIED stays REPLIED
       * unless server says otherwise). */
      if (new_state != cur_state) {
        macmbx_set_state(inbox, idx, new_state);
        updated++;
      }
    }
  }

save_state:
  /* --- Step 11: Save UIDVALIDITY + highest UID for next check --- */
  {
    unsigned long highest = 0;
    for (int i = 0; i < inbox->count; i++) {
      unsigned long u = inbox->msgs[i].uid_hash;
      if (u > highest) highest = u;
    }
    /* Also consider fetched UIDs in case some weren't appended */
    for (int i = 0; i < remote_count; i++)
      if (remote_uids && remote_uids[i] > highest) highest = remote_uids[i];

    const char *lmos_path = macmbx_mailer_lmos_path(m, account_index);
    if (lmos_path) {
      struct stat st; if (stat(m->lmos_dir, &st) != 0) mkdir_p(m->lmos_dir);
      FILE *f = fopen(lmos_path, "w");
      if (f) {
        fprintf(f, "%lu\n%lu\n", server_uidvalidity, highest);
        fclose(f);
      }
    }
    fprintf(stderr, "check_imap[%d]: saved state uidvalidity=%lu highest_uid=%lu\n",
            account_index, server_uidvalidity, highest);
  }

  free(local_uids);
  free(remote_uids);
  free(remote_flags);
  free(update_uids);
  free(update_flags);
  crispy_imap_close(&imap);

  fprintf(stderr, "check_imap[%d]: done — %d downloaded, %d deleted, %d flag-updated\n",
          account_index, downloaded, deleted, updated);
  macmbx_toc_save(inbox);

  /* Post-receive: run filters on inbox */
  if (m->filters && downloaded > 0) {
    macmbx_filter_apply_all(m->filters, inbox, MACMBX_WHEN_INCOMING,
                              m->store, NULL, NULL);
    macmbx_toc_save(inbox);
  }

  /* Post-receive: junk scoring */
  if (m->junk && downloaded > 0) {
    macmbx_junk_score_box(m->junk, inbox);
    macmbx_junk_move_spam(m->junk, inbox, m->store);
    macmbx_toc_save(inbox);
  }

  return downloaded;
}

int macmbx_mailer_check_account(MacmbxMailer *m, int account_index) {
  if (!m) return -1;
  MacmbxAccount acct;
  if (get_account(m, account_index, &acct) != 0) return -1;
  if (!acct.enabled) return 0;

  /* Dispatch by account type */
  if (strcasecmp(acct.type, "IMAP") == 0 || strcasecmp(acct.type, "imap") == 0)
    return macmbx_mailer_check_imap(m, account_index, &acct);
  if (strcasecmp(acct.type, "POP") != 0 && strcasecmp(acct.type, "pop") != 0)
    return 0;

  progress(m, "Connecting to POP3...", 0, 0);
  Pop3Session *pop = connect_pop3(m, &acct);
  if (!pop) return -1;

  /* Get message count */
  int err2 = crispy_pop3_stat(pop);
  int msg_count = (err2 == 0) ? pop->msg_count : -1;
  fprintf(stderr, "check_account[%d]: %s@%s STAT err=%d count=%d\n",
          account_index, acct.username, acct.server, err2, msg_count);
  if (msg_count <= 0) {
    crispy_pop3_close(pop);
    free(pop);
    progress(m, msg_count == 0 ? "No new mail" : "STAT failed", 0, 0);
    return msg_count == 0 ? 0 : -1;
  }

  bool use_lmos = acct.leave_on_server;

  /* Get UIDL list for LMOS tracking (proper POP3 way) */
  Pop3MsgInfo *msg_list = NULL;
  int list_count = 0;
  if (use_lmos) {
    list_count = crispy_pop3_list(pop, &msg_list);
    fprintf(stderr, "check_account[%d]: UIDL list returned %d entries\n",
            account_index, list_count);
  }

  /* Open inbox */
  MacmbxNode *in_node = macmbx_store_find_special(m->store, MACMBX_TYPE_IN);
  if (!in_node) { free(msg_list); crispy_pop3_close(pop); free(pop); return -1; }
  MacmbxTOC *inbox = macmbx_toc_open(in_node->path);
  if (!inbox) { free(msg_list); crispy_pop3_close(pop); free(pop); return -1; }

  int downloaded = 0;
  for (int i = 1; i <= msg_count; i++) {
    progress(m, "Downloading...", i, msg_count);

    /* LMOS check — skip if already seen (by UIDL) */
    if (use_lmos && msg_list && i <= list_count && msg_list[i-1].uidl[0]) {
      if (macmbx_mailer_lmos_seen(m, account_index, msg_list[i-1].uidl)) {
        fprintf(stderr, "check_account[%d]: msg %d UIDL '%s' already seen, skip\n",
                account_index, i, msg_list[i-1].uidl);
        continue;
      }
    }

    /* Download message */
    char *msg = NULL;
    long msgLen = crispy_pop3_retr(pop, i, &msg);
    fprintf(stderr, "check_account[%d]: RETR %d len=%ld\n", account_index, i, msgLen);
    if (msgLen <= 0 || !msg) { free(msg); continue; }

    /* Dedup check */
    if (macmbx_is_duplicate(inbox, msg, msgLen) >= 0) {
      fprintf(stderr, "check_account[%d]: msg %d is duplicate, skip\n", account_index, i);
      /* Still mark LMOS so we don't re-download */
      if (use_lmos && msg_list && i <= list_count && msg_list[i-1].uidl[0])
        macmbx_mailer_lmos_mark(m, account_index, msg_list[i-1].uidl);
      free(msg); continue;
    }

    /* Append to inbox */
    int idx = macmbx_append_message(inbox, msg, msgLen, NULL, MACMBX_UNREAD, 3);
    fprintf(stderr, "check_account[%d]: append msg %d → idx=%d\n", account_index, i, idx);
    if (idx >= 0) {
      if (account_index > 0)
        macmbx_tag_personality(inbox, idx, acct.name);
      downloaded++;

      /* Mark LMOS with UIDL */
      if (use_lmos && msg_list && i <= list_count && msg_list[i-1].uidl[0])
        macmbx_mailer_lmos_mark(m, account_index, msg_list[i-1].uidl);
    }
    free(msg);

    if (!use_lmos)
      crispy_pop3_dele(pop, i);
  }

  free(msg_list);
  crispy_pop3_close(pop);
  free(pop);
  fprintf(stderr, "check_account[%d]: downloaded %d messages, saving TOC\n",
          account_index, downloaded);
  macmbx_toc_save(inbox);

  /* Post-receive: run filters */
  if (m->filters && downloaded > 0) {
    macmbx_filter_apply_all(m->filters, inbox, MACMBX_WHEN_INCOMING,
                              m->store, NULL, NULL);
    macmbx_toc_save(inbox);
  }

  /* Post-receive: junk scoring */
  if (m->junk && downloaded > 0) {
    macmbx_junk_score_box(m->junk, inbox);
    macmbx_junk_move_spam(m->junk, inbox, m->store);
    macmbx_toc_save(inbox);
  }

  progress(m, "Check complete", downloaded, downloaded);
  return downloaded;
}

/* ================================================================
 * LMOS — Leave Mail On Server
 * ================================================================ */

const char *macmbx_mailer_lmos_path(MacmbxMailer *m, int account_index) {
  static char path[1024];
  MacmbxAccount acct;
  if (get_account(m, account_index, &acct) != 0) return NULL;
  snprintf(path, sizeof(path), "%s/lmos_%s@%s.txt",
           m->lmos_dir, acct.username, acct.server);
  return path;
}

bool macmbx_mailer_lmos_seen(MacmbxMailer *m, int account_index,
                               const char *uidl) {
  if (!m || !uidl) return false;
  const char *path = macmbx_mailer_lmos_path(m, account_index);
  if (!path) return false;
  FILE *f = fopen(path, "r");
  if (!f) return false;
  char line[256];
  while (fgets(line, sizeof(line), f)) {
    size_t len = strlen(line);
    while (len > 0 && (line[len-1] == '\r' || line[len-1] == '\n')) line[--len] = '\0';
    if (strcmp(line, uidl) == 0) { fclose(f); return true; }
  }
  fclose(f);
  return false;
}

void macmbx_mailer_lmos_mark(MacmbxMailer *m, int account_index,
                               const char *uidl) {
  if (!m || !uidl) return;
  /* Ensure lmos directory exists */
  struct stat st;
  if (stat(m->lmos_dir, &st) != 0)
    mkdir_p(m->lmos_dir);
  const char *path = macmbx_mailer_lmos_path(m, account_index);
  if (!path) return;
  FILE *f = fopen(path, "a");
  if (!f) return;
  fprintf(f, "%s\n", uidl);
  fclose(f);
}

/* ================================================================
 * Connection test
 * ================================================================ */

int macmbx_mailer_test_connection(MacmbxMailer *m, int account_index) {
  if (!m) return -1;
  MacmbxAccount acct;
  if (get_account(m, account_index, &acct) != 0) return -1;

  /* Test SMTP */
  SmtpSession *smtp = connect_smtp(m, &acct);
  if (!smtp) return -1;
  crispy_smtp_close(smtp);
  free(smtp);

  /* Test POP3 */
  if (strcasecmp(acct.type, "POP") == 0) {
    Pop3Session *pop = connect_pop3(m, &acct);
    if (!pop) return -2;
    crispy_pop3_close(pop);
    free(pop);
  }

  return 0;
}

/* ================================================================
 * IMAP IDLE — optional push notification
 *
 * Thread per IMAP account. Keeps connection open, enters IDLE,
 * wakes on server push or 29-min timeout, runs sync, fires callback.
 * Falls back to NOOP + sleep if server doesn't support IDLE.
 * ================================================================ */

typedef struct {
  MacmbxMailer *mailer;
  int slot;           /* index into mailer->idle[] */
  int account_index;
} IdleThreadArg;

static void *idle_thread_func(void *arg) {
  IdleThreadArg *a = (IdleThreadArg *)arg;
  MacmbxMailer *m = a->mailer;
  int slot = a->slot;
  int account_index = a->account_index;
  free(a);

  MacmbxAccount acct;
  if (get_account(m, account_index, &acct) != 0) {
    m->idle[slot].active = false;
    return NULL;
  }

  /* Connect */
  int imap_ssl = 0;
  if (m->conf) {
    imap_ssl = macmbx_conf_get_int(m->conf, "ssl", "ssl_imap_mode", 0);
    if (acct.index > 0) {
      char sec[32]; snprintf(sec, sizeof(sec), "account_%d", acct.index);
      imap_ssl = macmbx_conf_get_int(m->conf, sec, "ssl_mode", imap_ssl);
    }
  }
  int port = 143;
  ImapSecurity security = IMAP_PLAIN;
  if (imap_ssl & 4) { security = IMAP_SSL; port = 993; }
  else if (imap_ssl & 2) security = IMAP_STARTTLS;
  else if (imap_ssl & 1) security = IMAP_STARTTLS;

  ImapTransport tp = crispy_transport_new();
  crispy_transport_set_cert_callback(&tp, mailer_cert_callback, m);

  ImapSession imap;
  crispy_imap_init(&imap, tp);

  while (m->idle[slot].running) {
    /* (Re)connect if needed */
    if (!imap.connected) {
      if (crispy_imap_connect(&imap, acct.server, port, security) != 0) {
        fprintf(stderr, "idle[%d]: connect failed, retry in 60s\n", account_index);
        for (int w = 0; w < 60 && m->idle[slot].running; w++) sleep(1);
        continue;
      }
      char pw[256] = "";
      if (get_password(m, &acct, pw, sizeof(pw)) != 0) {
        crispy_imap_close(&imap);
        fprintf(stderr, "idle[%d]: no password, retry in 60s\n", account_index);
        for (int w = 0; w < 60 && m->idle[slot].running; w++) sleep(1);
        continue;
      }
      const char *user = acct.username[0] ? acct.username : acct.email;
      if (crispy_imap_login(&imap, user, pw) != 0) {
        crispy_imap_close(&imap);
        fprintf(stderr, "idle[%d]: login failed, retry in 60s\n", account_index);
        for (int w = 0; w < 60 && m->idle[slot].running; w++) sleep(1);
        continue;
      }
      if (crispy_imap_select(&imap, "INBOX") != 0) {
        crispy_imap_close(&imap);
        fprintf(stderr, "idle[%d]: SELECT failed, retry in 60s\n", account_index);
        for (int w = 0; w < 60 && m->idle[slot].running; w++) sleep(1);
        continue;
      }
      fprintf(stderr, "idle[%d]: connected, INBOX selected, cap_idle=%d\n",
              account_index, imap.cap_idle);
    }

    if (!m->idle[slot].running) break;

    /* Enter IDLE (or NOOP fallback).
     * 29 minutes — RFC 2177 recommends re-issuing before 30 min. */
    int res = crispy_imap_idle(&imap, 29 * 60 * 1000);
    if (res < 0) {
      /* Connection lost — will reconnect next iteration */
      fprintf(stderr, "idle[%d]: connection lost, reconnecting\n", account_index);
      crispy_imap_close(&imap);
      continue;
    }

    if (!m->idle[slot].running) break;

    /* IDLE returned — server sent an update or we timed out.
     * Check imap.reply for EXISTS/EXPUNGE/FETCH to know if something changed.
     * Either way, run a full sync to get the definitive state. */
    bool has_changes = false;
    if (strstr(imap.reply, "EXISTS") || strstr(imap.reply, "EXPUNGE") ||
        strstr(imap.reply, "FETCH")) {
      has_changes = true;
      fprintf(stderr, "idle[%d]: server push: %s\n", account_index, imap.reply);
    }

    if (has_changes) {
      /* Run full sync — reuse the open connection's state but
       * we need to do it via check_account which opens its own connection.
       * Close this one, sync, then reconnect for next IDLE cycle. */
      crispy_imap_close(&imap);

      int new_msgs = macmbx_mailer_check_account(m, account_index);
      int downloaded = new_msgs > 0 ? new_msgs : 0;

      /* Fire callback */
      if (m->idle[slot].cb) {
        m->idle[slot].cb(account_index, downloaded, 0, 0, m->idle[slot].cb_ctx);
      }
      /* Connection closed — will reconnect at top of loop */
    }
    /* If no changes (timeout), loop re-enters IDLE as keepalive */
  }

  crispy_imap_close(&imap);
  m->idle[slot].active = false;
  fprintf(stderr, "idle[%d]: stopped\n", account_index);
  return NULL;
}

int macmbx_mailer_idle_start(MacmbxMailer *m, int account_index,
                               MacmbxIdleCallback cb, void *ctx) {
  if (!m) return -1;

  /* Verify this is an IMAP account */
  MacmbxAccount acct;
  if (get_account(m, account_index, &acct) != 0) return -1;
  if (strcasecmp(acct.type, "IMAP") != 0 && strcasecmp(acct.type, "imap") != 0)
    return -1;

  /* Check if already running */
  for (int i = 0; i < m->idle_count; i++) {
    if (m->idle[i].account_index == account_index && m->idle[i].active)
      return 0; /* already running */
  }

  if (m->idle_count >= MAILER_MAX_IDLE) return -1;

  /* Run initial sync before entering IDLE */
  macmbx_mailer_check_account(m, account_index);

  int slot = m->idle_count++;
  m->idle[slot].account_index = account_index;
  m->idle[slot].running = true;
  m->idle[slot].active = true;
  m->idle[slot].cb = cb;
  m->idle[slot].cb_ctx = ctx;

  IdleThreadArg *arg = calloc(1, sizeof(IdleThreadArg));
  arg->mailer = m;
  arg->slot = slot;
  arg->account_index = account_index;
  pthread_create(&m->idle[slot].thread, NULL, idle_thread_func, arg);

  return 0;
}

void macmbx_mailer_idle_stop(MacmbxMailer *m, int account_index) {
  if (!m) return;
  for (int i = 0; i < m->idle_count; i++) {
    if (account_index >= 0 && m->idle[i].account_index != account_index)
      continue;
    if (!m->idle[i].active) continue;

    m->idle[i].running = false;
    /* Thread will exit on next IDLE timeout or error.
     * We could also close the socket to force immediate wakeup,
     * but for now just wait. */
    pthread_join(m->idle[i].thread, NULL);
    m->idle[i].active = false;
  }
}

bool macmbx_mailer_idle_active(MacmbxMailer *m, int account_index) {
  if (!m) return false;
  for (int i = 0; i < m->idle_count; i++) {
    if (m->idle[i].account_index == account_index && m->idle[i].active)
      return true;
  }
  return false;
}

void macmbx_mailer_disconnect(MacmbxMailer *m) {
  if (!m) return;
  /* Stop all IDLE sessions */
  macmbx_mailer_idle_stop(m, -1);
}

/* ================================================================
 * On-demand fetch — headers-only / leave-on-server mode
 * ================================================================ */


static const char *mailer_basename(const char *path) {
  const char *p = strrchr(path, '/');
  if (!p) p = strrchr(path, '\\');
  return p ? p + 1 : path;
}

/* Check if a message is a stub (no body downloaded yet) */
bool macmbx_mailer_is_stub(MacmbxMailer *m, MacmbxTOC *toc, int index) {
  (void)m;
  if (!toc || index < 0 || index >= toc->count) return false;
  MacmbxMsgSum *msg = &toc->msgs[index];
  /* A stub has a summary but zero-length body in the mbox */
  return (msg->flags & MACMBX_FLAG_BX_TEXT) == 0 && msg->length <= msg->body_offset;
}

/* Connect IMAP for the account that owns this mailbox */
static ImapSession *connect_imap_for_toc(MacmbxMailer *m, MacmbxTOC *toc) {
  if (!m || !toc) return NULL;

  /* Determine which account owns this mailbox from personality tag */
  MacmbxAccount acct;
  if (macmbx_conf_get_dominant(m->conf, &acct) != 0) return NULL;
  if (strcasecmp(acct.type, "IMAP") != 0) return NULL;

  ImapTransport tp = crispy_transport_new();
  ImapSession *imap = (ImapSession *)calloc(1, sizeof(ImapSession));
  if (!imap) return NULL;

  int port = 143;
  ImapSecurity security = IMAP_STARTTLS;
  if (acct.ssl_mode == 1) { security = IMAP_SSL; port = 993; }
  else if (acct.ssl_mode == 0) { security = IMAP_PLAIN; port = 143; }

  crispy_imap_init(imap, tp);
  int err = crispy_imap_connect(imap, acct.server, port, security);
  if (err) { free(imap); return NULL; }

  char pw[256] = "";
  if (get_password(m, &acct, pw, sizeof(pw)) != 0) {
    crispy_imap_close(imap); free(imap); return NULL;
  }
  const char *user = acct.username[0] ? acct.username : acct.email;
  err = crispy_imap_login(imap, user, pw);
  if (err) { crispy_imap_close(imap); free(imap); return NULL; }

  return imap;
}

/* Ensure message body is fully downloaded */
int macmbx_mailer_ensure_body(MacmbxMailer *m, MacmbxTOC *toc, int index) {
  if (!m || !toc || index < 0 || index >= toc->count) return -1;
  if (!macmbx_mailer_is_stub(m, toc, index)) return 0; /* already have it */

  MacmbxMsgSum *msg = &toc->msgs[index];

  ImapSession *imap = connect_imap_for_toc(m, toc);
  if (!imap) return -1;

  /* Select the mailbox on the server */
  const char *mbox_name = mailer_basename(toc->mbox_path);
  int err = crispy_imap_select(imap, mbox_name);
  if (err) { crispy_imap_close(imap); free(imap); return -1; }

  /* Fetch the full message by UID */
  long body_len = 0;
  char *body = crispy_imap_fetch_message(imap, msg->uid_hash, &body_len);
  if (!body || body_len == 0) {
    free(body);
    crispy_imap_close(imap); free(imap);
    return -1;
  }

  /* Replace the stub in the local mbox with the full message */
  FILE *fp = fopen(toc->mbox_path, "r+b");
  if (fp) {
    fseek(fp, 0, SEEK_END);
    long new_offset = ftell(fp);

    char from_line[256];
    macmbx_write_from_line(from_line, sizeof(from_line),
                            msg->from[0] ? msg->from : "unknown");
    fputs(from_line, fp);
    fwrite(body, 1, body_len, fp);
    fputs("\n", fp);

    long new_length = ftell(fp) - new_offset;
    fclose(fp);

    /* Update summary */
    msg->offset = new_offset;
    msg->length = new_length;
    msg->body_offset = 0;
    long from_len = (long)strlen(from_line);
    for (long i = 0; i < body_len - 1; i++) {
      if (body[i] == '\n' && body[i+1] == '\n') {
        msg->body_offset = (int)(i + 2 + from_len); break;
      }
      if (body[i] == '\r' && body[i+1] == '\n' &&
          i+3 < body_len && body[i+2] == '\r' && body[i+3] == '\n') {
        msg->body_offset = (int)(i + 4 + from_len); break;
      }
    }
    toc->dirty = true;
    macmbx_toc_save(toc);
  }

  free(body);
  crispy_imap_close(imap);
  free(imap);
  return 0;
}

/* Fetch a specific MIME attachment by part ID */
int macmbx_mailer_fetch_attachment(MacmbxMailer *m, MacmbxTOC *toc,
                                     int index, const char *part_id,
                                     char *out_path, int path_size) {
  if (!m || !toc || index < 0 || index >= toc->count || !part_id) return -1;

  MacmbxMsgSum *msg = &toc->msgs[index];

  ImapSession *imap = connect_imap_for_toc(m, toc);
  if (!imap) return -1;

  const char *mbox_name = mailer_basename(toc->mbox_path);
  int err = crispy_imap_select(imap, mbox_name);
  if (err) { crispy_imap_close(imap); free(imap); return -1; }

  /* Fetch the MIME section */
  long part_len = 0;
  char *part_data = crispy_imap_fetch_section(imap, msg->uid_hash, part_id, &part_len);
  if (!part_data || part_len == 0) {
    free(part_data);
    crispy_imap_close(imap); free(imap);
    return -1;
  }

  /* Save to Attachments directory */
  char attach_dir[PATH_MAX];
  snprintf(attach_dir, sizeof(attach_dir), "%s/../Attachments",
           m->store->root_path);
  mkdir_p(attach_dir);

  snprintf(out_path, path_size, "%s/%s_part_%s",
           attach_dir, msg->subject[0] ? msg->subject : "message", part_id);

  FILE *fp = fopen(out_path, "wb");
  if (fp) {
    fwrite(part_data, 1, part_len, fp);
    fclose(fp);
  } else {
    err = -1;
  }

  free(part_data);
  crispy_imap_close(imap);
  free(imap);
  return err;
}
