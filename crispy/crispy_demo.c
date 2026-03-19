/* crispy_demo.c — Standalone demo: send and receive email
 *
 * Usage:
 *   crispy_demo send [conf_file]          — send a test message
 *   crispy_demo recv [conf_file]          — list messages via POP3
 *   crispy_demo recv N [conf_file]        — fetch message N
 *
 * Default conf_file: crispy_demo.conf
 */

#include "crispy_smtp.h"
#include "crispy_pop3.h"
#include "crispy_transport.h"
#include "crispy_headparse.h"
#include "crispy_conf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static SmtpSecurity parse_smtp_security(const char *s) {
  if (!s) return SMTP_STARTTLS;
  if (strcasecmp(s, "ssl") == 0 || strcasecmp(s, "tls") == 0) return SMTP_SSL;
  if (strcasecmp(s, "starttls") == 0) return SMTP_STARTTLS;
  return SMTP_PLAIN;
}

static void debug_print(const char *line, void *userdata) {
  (void)userdata;
  fprintf(stderr, "  %s\n", line);
}

static Pop3Security parse_pop3_security(const char *s) {
  if (!s) return POP3_SSL;
  if (strcasecmp(s, "ssl") == 0 || strcasecmp(s, "tls") == 0) return POP3_SSL;
  if (strcasecmp(s, "stls") == 0 || strcasecmp(s, "starttls") == 0) return POP3_STLS;
  return POP3_PLAIN;
}

/* Build a minimal RFC 5322 message */
static char *build_message(const char *from, const char *to,
                           const char *subject, const char *body,
                           long *outLen) {
  char date[64];
  crispy_smtp_format_date(date, sizeof(date), (long)time(NULL), 0);

  char *msg = (char *)malloc(4096);
  if (!msg) return NULL;

  int len = snprintf(msg, 4096,
    "Date: %s\r\n"
    "From: %s\r\n"
    "To: %s\r\n"
    "Subject: %s\r\n"
    "MIME-Version: 1.0\r\n"
    "Content-Type: text/plain; charset=UTF-8\r\n"
    "X-Mailer: Crispy Mail Library\r\n"
    "\r\n"
    "%s\r\n",
    date, from, to, subject, body);

  if (outLen) *outLen = len;
  return msg;
}

static int do_send(const CrispyConf *conf) {
  const char *host = crispy_conf_get(conf, "smtp_host", NULL);
  const char *user = crispy_conf_get(conf, "smtp_user", NULL);
  const char *pass = crispy_conf_get(conf, "smtp_pass", NULL);
  const char *from = crispy_conf_get(conf, "from", user);
  const char *to   = crispy_conf_get(conf, "to", NULL);
  int port = crispy_conf_int(conf, "smtp_port", 587);
  SmtpSecurity sec = parse_smtp_security(
      crispy_conf_get(conf, "smtp_security", "starttls"));

  if (!host || !user || !pass || !to) {
    fprintf(stderr, "Error: smtp_host, smtp_user, smtp_pass, to are required\n");
    return 1;
  }

  printf("Connecting to %s:%d ...\n", host, port);

  SmtpTransport tp = crispy_transport_new();
  SmtpSession s;
  crispy_smtp_init(&s, tp, "localhost");
  s.debug = debug_print;

  int err = crispy_smtp_connect(&s, host, port, sec);
  if (err) {
    fprintf(stderr, "Connect failed: %d (%s)\n", err, s.last_reply);
    crispy_smtp_close(&s);
    return 1;
  }
  printf("Connected. EHLO done.\n");
  printf("  STARTTLS: %s\n", s.caps.starttls ? "yes" : "no");
  printf("  AUTH: %s\n", s.caps.has_auth ? s.caps.auth_mechs : "none");
  printf("  8BITMIME: %s\n", s.caps.eightbitmime ? "yes" : "no");
  printf("  SIZE: %ld\n", s.caps.max_size);

  if (s.caps.has_auth) {
    printf("Authenticating as %s ...\n", user);
    err = crispy_smtp_auth_plain(&s, user, pass);
    if (err) {
      /* Try LOGIN if PLAIN fails */
      err = crispy_smtp_auth_login(&s, user, pass);
    }
    if (err) {
      fprintf(stderr, "Auth failed: %d (%s)\n", err, s.last_reply);
      crispy_smtp_close(&s);
      return 1;
    }
    printf("Authenticated.\n");
  } else {
    printf("Server does not require authentication.\n");
  }

  /* Build message */
  long msgLen;
  char *msg = build_message(from, to,
      "Test from Crispy Mail Library",
      "Hello!\r\n\r\nThis message was sent by the Crispy standalone mail library.\r\n"
      "If you're reading this, the library works!",
      &msgLen);

  printf("Sending to %s ...\n", to);
  const char *rcpts[] = { to, NULL };
  err = crispy_smtp_send(&s, from, rcpts, msg, msgLen);
  free(msg);

  if (err) {
    fprintf(stderr, "Send failed: %d (%s)\n", err, s.last_reply);
  } else {
    printf("Message sent successfully!\n");
  }

  crispy_smtp_close(&s);
  return err ? 1 : 0;
}

/* Helper: connect + auth to POP3, returns 0 on success */
static int pop3_open(const CrispyConf *conf, Pop3Session *p) {
  const char *host = crispy_conf_get(conf, "pop3_host", NULL);
  const char *user = crispy_conf_get(conf, "pop3_user", NULL);
  const char *pass = crispy_conf_get(conf, "pop3_pass", NULL);
  int port = crispy_conf_int(conf, "pop3_port", 995);
  Pop3Security sec = parse_pop3_security(
      crispy_conf_get(conf, "pop3_security", "ssl"));

  if (!host || !user || !pass) {
    fprintf(stderr, "Error: pop3_host, pop3_user, pop3_pass are required\n");
    return 1;
  }

  printf("Connecting to %s:%d ...\n", host, port);

  Pop3Transport tp = crispy_transport_new();
  crispy_pop3_init(p, tp);
  p->debug = debug_print;

  int err = crispy_pop3_connect(p, host, port, sec);
  if (err) {
    fprintf(stderr, "Connect failed (%s)\n", p->last_reply);
    crispy_pop3_close(p);
    return 1;
  }
  printf("Connected. Greeting: %s\n", p->greeting);

  printf("Authenticating as %s ...\n", user);
  err = crispy_pop3_auth(p, user, pass);
  if (err) {
    fprintf(stderr, "Auth failed (%s)\n", p->last_reply);
    crispy_pop3_close(p);
    return 1;
  }
  printf("Authenticated.\n");
  return 0;
}

static int do_recv(const CrispyConf *conf, int fetchNum) {
  Pop3Session p;
  if (pop3_open(conf, &p)) return 1;
  int err;

  if (fetchNum > 0) {
    /* Need STAT for single message fetch */
    err = crispy_pop3_stat(&p);
    if (err) {
      fprintf(stderr, "STAT failed (%s)\n", p.last_reply);
      crispy_pop3_close(&p);
      return 1;
    }
    printf("Mailbox: %d messages, %ld bytes\n", p.msg_count, p.mailbox_size);

    /* Fetch a specific message */
    printf("\n--- Message %d ---\n", fetchNum);
    char *msg = NULL;
    long len = crispy_pop3_retr(&p, fetchNum, &msg);
    if (len < 0) {
      fprintf(stderr, "RETR %d failed (%s)\n", fetchNum, p.last_reply);
    } else {
      /* Parse and show headers */
      MailHeadSpec hs;
      char *val;
      if (crispy_headparse_find(msg, "From: ", &hs) &&
          crispy_headparse_get_value(msg, &hs, &val) == 0) {
        printf("From: %s\n", val);
        free(val);
      }
      if (crispy_headparse_find(msg, "Subject: ", &hs) &&
          crispy_headparse_get_value(msg, &hs, &val) == 0) {
        printf("Subject: %s\n", val);
        free(val);
      }
      if (crispy_headparse_find(msg, "Date: ", &hs) &&
          crispy_headparse_get_value(msg, &hs, &val) == 0) {
        printf("Date: %s\n", val);
        free(val);
      }
      printf("Size: %ld bytes\n", len);

      /* Show first 20 lines of body */
      char *body = strstr(msg, "\r\n\r\n");
      if (body) {
        body += 4;
        printf("\n");
        int lines = 0;
        for (char *s = body; *s && lines < 20; s++) {
          putchar(*s);
          if (*s == '\n') lines++;
        }
        if (lines >= 20) printf("...\n");
      }
      free(msg);
    }
  } else {
    /* List messages using TOP (headers only) */
    Pop3MsgInfo *msgs = NULL;
    int count = crispy_pop3_list(&p, &msgs);
    printf("Mailbox: %d messages, %ld bytes\n", p.msg_count, p.mailbox_size);
    if (count > 0 && msgs) {
      printf("\n");
      int show = count < 20 ? count : 20;
      for (int i = 0; i < show; i++) {
        char *hdrs = NULL;
        long len = crispy_pop3_top(&p, msgs[i].number, 0, &hdrs);
        if (len > 0 && hdrs) {
          MailHeadSpec hs;
          char *from = NULL, *subj = NULL;
          if (crispy_headparse_find(hdrs, "From: ", &hs))
            crispy_headparse_get_value(hdrs, &hs, &from);
          if (crispy_headparse_find(hdrs, "Subject: ", &hs))
            crispy_headparse_get_value(hdrs, &hs, &subj);

          printf("%3d  %6ld  %-30.30s  %s\n",
                 msgs[i].number, msgs[i].size,
                 from ? from : "(unknown)",
                 subj ? subj : "(no subject)");
          free(from);
          free(subj);
          free(hdrs);
        } else {
          printf("%3d  %6ld  (could not fetch headers)\n",
                 msgs[i].number, msgs[i].size);
        }
      }
      if (count > 20) printf("... and %d more\n", count - 20);
      free(msgs);
    }
  }

  crispy_pop3_close(&p);
  return 0;
}

static int do_dele(const CrispyConf *conf, int msgNum) {
  Pop3Session p;
  if (pop3_open(conf, &p)) return 1;

  int err = crispy_pop3_stat(&p);
  if (err) { crispy_pop3_close(&p); return 1; }
  printf("Mailbox: %d messages\n", p.msg_count);

  if (msgNum > p.msg_count) {
    fprintf(stderr, "Message %d does not exist (only %d)\n", msgNum, p.msg_count);
    crispy_pop3_close(&p);
    return 1;
  }

  printf("Deleting message %d ...\n", msgNum);
  err = crispy_pop3_dele(&p, msgNum);
  if (err) {
    fprintf(stderr, "DELE failed (%s)\n", p.last_reply);
  } else {
    printf("Marked for deletion. QUIT commits it.\n");
  }

  crispy_pop3_close(&p);
  return err ? 1 : 0;
}

/* top N [lines] — fetch headers + first N lines of body (leave on server) */
static int do_top(const CrispyConf *conf, int msgNum, int lines) {
  Pop3Session p;
  if (pop3_open(conf, &p)) return 1;

  printf("\n--- TOP %d %d ---\n", msgNum, lines);
  char *msg = NULL;
  long len = crispy_pop3_top(&p, msgNum, lines, &msg);
  if (len < 0) {
    fprintf(stderr, "TOP failed (%s)\n", p.last_reply);
    crispy_pop3_close(&p);
    return 1;
  }

  /* Parse and show key headers */
  MailHeadSpec hs;
  char *val;
  const char *hdrs[] = { "From: ", "To: ", "Subject: ", "Date: ", NULL };
  for (int i = 0; hdrs[i]; i++) {
    if (crispy_headparse_find(msg, hdrs[i], &hs) &&
        crispy_headparse_get_value(msg, &hs, &val) == 0) {
      printf("%s%s\n", hdrs[i], val);
      free(val);
    }
  }

  /* Show body portion */
  char *body = strstr(msg, "\r\n\r\n");
  if (body) {
    body += 4;
    if (*body) {
      printf("\n%s", body);
      /* Ensure trailing newline */
      if (body[strlen(body)-1] != '\n') printf("\n");
    }
  }
  if (lines == 0)
    printf("(headers only — message left on server)\n");

  free(msg);
  crispy_pop3_close(&p);
  return 0;
}

/* rset — undo all pending deletions in the session */
static int do_rset(const CrispyConf *conf) {
  Pop3Session p;
  if (pop3_open(conf, &p)) return 1;

  /* Mark some for deletion then undo */
  int err = crispy_pop3_rset(&p);
  if (err) {
    fprintf(stderr, "RSET failed (%s)\n", p.last_reply);
  } else {
    printf("All deletions undone (RSET).\n");
  }

  crispy_pop3_close(&p);
  return err ? 1 : 0;
}

static void usage(void) {
  fprintf(stderr,
    "Usage:\n"
    "  crispy_demo send [config]         Send a test message\n"
    "  crispy_demo recv [config]         List messages via POP3\n"
    "  crispy_demo recv N [config]       Fetch full message N\n"
    "  crispy_demo top N [lines] [conf]  Fetch headers + N lines (leave on server)\n"
    "  crispy_demo dele N [config]       Delete message N\n"
    "\n"
    "Default config: crispy_demo.conf\n");
}

/* Parse: cmd [N] [N2] [config] — returns config file path */
static const char *parse_args(int argc, char *argv[], int *num1, int *num2) {
  const char *conf = "crispy_demo.conf";
  int argi = 2;

  if (argi < argc && argv[argi][0] >= '0' && argv[argi][0] <= '9') {
    *num1 = atoi(argv[argi++]);
  }
  if (argi < argc && argv[argi][0] >= '0' && argv[argi][0] <= '9') {
    *num2 = atoi(argv[argi++]);
  }
  if (argi < argc) conf = argv[argi];
  return conf;
}

int main(int argc, char *argv[]) {
  if (argc < 2) { usage(); return 1; }

  const char *cmd = argv[1];
  int num1 = 0, num2 = 0;
  const char *confFile = parse_args(argc, argv, &num1, &num2);

  CrispyConf conf;
  if (crispy_conf_load(&conf, confFile) != 0) {
    fprintf(stderr, "Cannot open config file: %s\n", confFile);
    return 1;
  }

  if (strcmp(cmd, "send") == 0)
    return do_send(&conf);
  else if (strcmp(cmd, "recv") == 0)
    return do_recv(&conf, num1);
  else if (strcmp(cmd, "top") == 0) {
    if (num1 < 1) { fprintf(stderr, "Usage: crispy_demo top N [lines]\n"); return 1; }
    return do_top(&conf, num1, num2);
  }
  else if (strcmp(cmd, "dele") == 0) {
    if (num1 < 1) { fprintf(stderr, "Usage: crispy_demo dele N\n"); return 1; }
    return do_dele(&conf, num1);
  }
  else if (strcmp(cmd, "rset") == 0)
    return do_rset(&conf);
  else { usage(); return 1; }
}
