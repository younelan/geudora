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
#include "crispy_transport.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <sys/stat.h>
#ifdef _WIN32
  #include <direct.h>
  #define mkdir_p(p) _mkdir(p)
#else
  #include <unistd.h>
  #define mkdir_p(p) mkdir(p, 0755)
#endif

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
  /* Try callback */
  if (m->cred_fn)
    return m->cred_fn(acct->name, acct->server, pw, sz, m->cred_ctx);
  return -1;
}

/* Create and connect SMTP session for an account */
static SmtpSession *connect_smtp(MacmbxMailer *m, MacmbxAccount *acct) {
  SmtpTransport tp = crispy_transport_new();
  SmtpSession *smtp = (SmtpSession *)calloc(1, sizeof(SmtpSession));
  if (!smtp) return NULL;

  const char *server = acct->smtp_server[0] ? acct->smtp_server : acct->server;
  int port = 587;
  SmtpSecurity security = SMTP_STARTTLS;
  if (acct->ssl_mode == 1) { security = SMTP_SSL; port = 465; }
  else if (acct->ssl_mode == 0) { security = SMTP_PLAIN; port = 25; }

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

  int port = 110;
  Pop3Security security = POP3_PLAIN;
  if (acct->ssl_mode == 1) { security = POP3_SSL; port = 995; }

  crispy_pop3_init(pop, tp);
  int err = crispy_pop3_connect(pop, acct->server, port, security);
  if (err) { free(pop); return NULL; }

  char pw[256] = "";
  if (get_password(m, acct, pw, sizeof(pw)) != 0) { free(pop); return NULL; }
  const char *user = acct->username[0] ? acct->username : acct->email;
  err = crispy_pop3_auth(pop, user, pw);
  if (err) { crispy_pop3_close(pop); free(pop); return NULL; }

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
  if (get_account(m, account_index, &acct) != 0) return -1;

  MacmbxNode *out_node = macmbx_store_find_special(m->store, MACMBX_TYPE_OUT);
  if (!out_node) return -1;
  MacmbxTOC *out = macmbx_toc_open(out_node->path);
  if (!out) return -1;

  /* Count queued messages for this account */
  uint32_t pers_hash = account_index > 0 ? macmbx_personality_hash(acct.name) : 0;
  int queued = 0;
  for (int i = 0; i < out->count; i++) {
    if (out->msgs[i].state != MACMBX_QUEUED) continue;
    if (out->msgs[i].flags & MACMBX_FLAG_DELETED) continue;
    if (pers_hash && out->msgs[i].pers_id != pers_hash) continue;
    queued++;
  }
  if (queued == 0) return 0;

  /* Connect SMTP */
  progress(m, "Connecting to SMTP...", 0, queued);
  SmtpSession *smtp = connect_smtp(m, &acct);
  if (!smtp) return -1;

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
  if (!m || !message || !from || !rcpts) return -1;
  if (len < 0) len = (long)strlen(message);

  MacmbxAccount acct;
  if (get_account(m, account_index, &acct) != 0) return -1;

  SmtpSession *smtp = connect_smtp(m, &acct);
  if (!smtp) return -1;

  char fromBare[256];
  extract_bare_email(from, fromBare, sizeof(fromBare));

  int err = crispy_smtp_send(smtp, fromBare, rcpts, message, len);
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

int macmbx_mailer_check(MacmbxMailer *m) {
  if (!m) return -1;
  int total = 0;

  /* Check dominant */
  int n = macmbx_mailer_check_dominant(m);
  if (n > 0) total += n;

  /* Check each personality */
  int acct_count = macmbx_conf_count_accounts(m->conf);
  for (int i = 1; i <= acct_count; i++) {
    n = macmbx_mailer_check_account(m, i);
    if (n > 0) total += n;
  }
  return total;
}

int macmbx_mailer_check_dominant(MacmbxMailer *m) {
  return macmbx_mailer_check_account(m, 0);
}

int macmbx_mailer_check_account(MacmbxMailer *m, int account_index) {
  if (!m) return -1;
  MacmbxAccount acct;
  if (get_account(m, account_index, &acct) != 0) return -1;
  if (!acct.enabled) return 0;

  /* Only POP for now (IMAP would use crispy_imap) */
  if (strcasecmp(acct.type, "POP") != 0 && strcasecmp(acct.type, "pop") != 0)
    return 0;

  progress(m, "Connecting to POP3...", 0, 0);
  Pop3Session *pop = connect_pop3(m, &acct);
  if (!pop) return -1;

  /* Get message count */
  int msg_count = crispy_pop3_stat(pop);
  if (msg_count <= 0) {
    crispy_pop3_close(pop);
    progress(m, msg_count == 0 ? "No new mail" : "STAT failed", 0, 0);
    return msg_count == 0 ? 0 : -1;
  }

  /* UIDL for LMOS tracking — use TOP to get Message-ID as fallback */
  bool use_lmos = acct.leave_on_server;

  /* Open inbox */
  MacmbxNode *in_node = macmbx_store_find_special(m->store, MACMBX_TYPE_IN);
  if (!in_node) { crispy_pop3_close(pop); return -1; }
  MacmbxTOC *inbox = macmbx_toc_open(in_node->path);
  if (!inbox) { crispy_pop3_close(pop); return -1; }

  int downloaded = 0;
  for (int i = 1; i <= msg_count; i++) {
    progress(m, "Downloading...", i, msg_count);

    /* Download message */
    char *msg = NULL;
    long msgLen = crispy_pop3_retr(pop, i, &msg);
    if (msgLen <= 0 || !msg) { free(msg); continue; }

    /* Use Message-ID as LMOS key */
    if (use_lmos) {
      int32_t hash = 0;
      /* Extract Message-ID hash for LMOS tracking */
      const char *p = msg;
      while (*p) {
        if (strncasecmp(p, "Message-ID:", 11) == 0 || strncasecmp(p, "Message-Id:", 11) == 0) {
          p += 11; while (*p == ' ') p++;
          const char *id = p; if (*id == '<') id++;
          const char *end = id; while (*end && *end != '>' && *end != '\r' && *end != '\n') end++;
          char uidl[256];
          int len = (int)(end - id); if (len >= 256) len = 255;
          memcpy(uidl, id, len); uidl[len] = '\0';
          if (macmbx_mailer_lmos_seen(m, account_index, uidl)) {
            free(msg); goto next_msg;
          }
          break;
        }
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
        if (*p == '\r' || *p == '\n') break;
      }
    }

    /* Dedup check */
    if (macmbx_is_duplicate(inbox, msg, msgLen) >= 0) { free(msg); continue; }

    /* Append to inbox */
    int idx = macmbx_append_message(inbox, msg, msgLen, NULL, MACMBX_UNREAD, 3);
    if (idx >= 0) {
      if (account_index > 0)
        macmbx_tag_personality(inbox, idx, acct.name);
      downloaded++;

      /* Mark LMOS with Message-ID */
      if (use_lmos) {
        const char *p2 = msg;
        while (*p2) {
          if (strncasecmp(p2, "Message-ID:", 11) == 0 || strncasecmp(p2, "Message-Id:", 11) == 0) {
            p2 += 11; while (*p2 == ' ') p2++;
            const char *id = p2; if (*id == '<') id++;
            const char *end = id; while (*end && *end != '>' && *end != '\r') end++;
            char uidl[256]; int len = (int)(end - id); if (len >= 256) len = 255;
            memcpy(uidl, id, len); uidl[len] = '\0';
            macmbx_mailer_lmos_mark(m, account_index, uidl);
            break;
          }
          while (*p2 && *p2 != '\n') p2++;
          if (*p2 == '\n') p2++;
          if (*p2 == '\r' || *p2 == '\n') break;
        }
      }
    }
    free(msg);

    if (!use_lmos)
      crispy_pop3_dele(pop, i);

    next_msg:;
  }

  crispy_pop3_close(pop);
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

void macmbx_mailer_disconnect(MacmbxMailer *m) {
  (void)m; /* Sessions are created and closed per operation */
}
