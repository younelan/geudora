/*
 * Program:	Operating-system dependent routines -- Unix version
 */

#include "mail.h"
#include "osdep.h"
#include <ctype.h>

/* fs_unix.c implementation */

void *fs_get(size_t size) {
  void *block = malloc(size ? size : (size_t)1);
  if (!block)
    fatal("Out of memory");
  memset(block, 0, size ? size : (size_t)1);
  return block;
}

void fs_resize(void **block, size_t size) {
  if (!(*block = realloc(*block, size ? size : (size_t)1)))
    fatal("Can't resize memory");
}

void fs_give(void **block) {
  if (*block)
    free(*block);
  *block = NULL;
}

/* ftl_unix.c implementation */

void fatal(char *string) {
  mm_fatal(string);
  abort();
}

/* env_unix.c implementation */

static char *myHomeDir = NULL;
static char *myLocalHost = NULL;
static char *myNewsrc = NULL;

void *env_parameters(long function, void *value) {
  switch ((int)function) {
  case SET_HOMEDIR:
    if (myHomeDir)
      fs_give((void **)&myHomeDir);
    myHomeDir = cpystr((char *)value);
    break;
  case GET_HOMEDIR:
    if (!myHomeDir) {
      char *h = getenv("HOME");
      myHomeDir = cpystr(h ? h : "");
    }
    value = (void *)myHomeDir;
    break;
  case SET_LOCALHOST:
    if (myLocalHost)
      fs_give((void **)&myLocalHost);
    myLocalHost = cpystr((char *)value);
    break;
  case GET_LOCALHOST:
    if (!myLocalHost) {
      char tmp[MAILTMPLEN];
      if (gethostname(tmp, MAILTMPLEN - 1) == 0)
        myLocalHost = cpystr(tmp);
      else
        myLocalHost = cpystr("localhost");
    }
    value = (void *)myLocalHost;
    break;
  case SET_NEWSRC:
    if (myNewsrc)
      fs_give((void **)&myNewsrc);
    myNewsrc = cpystr((char *)value);
    break;
  case GET_NEWSRC:
    if (!myNewsrc) {
      char tmp[MAILTMPLEN];
      sprintf(tmp, "%s/.newsrc",
              (char *)mail_parameters(NULL, GET_HOMEDIR, NULL));
      myNewsrc = cpystr(tmp);
    }
    value = (void *)myNewsrc;
    break;
  default:
    value = NULL;
    break;
  }
  return value;
}

void rfc822_date(char *date) {
  time_t ti = time(0);
  struct tm *t = localtime(&ti);
  strftime(date, MAILTMPLEN, "%a, %d %b %Y %H:%M:%S %z", t);
}

void internal_date(char *date) {
  time_t ti = time(0);
  struct tm *t = localtime(&ti);
  strftime(date, MAILTMPLEN, "%d-%b-%Y %H:%M:%S %z", t);
}

char *mylocalhost(void) {
  return (char *)mail_parameters(NULL, GET_LOCALHOST, NULL);
}

char *myhomedir() { return (char *)mail_parameters(NULL, GET_HOMEDIR, NULL); }

/* nl_unix.c implementation (largely copied from nl_mac.c as it was mostly
 * portable) */

unsigned long strcrlfcpy(char **dst, unsigned long *dstl, char *src,
                         unsigned long srcl) {
  long i, j;
  char c, *d = src;
  for (i = srcl, j = 0; j < srcl; j++)
    if (*d++ == '\015')
      i++;
  if (*dst && (i > *dstl))
    fs_give((void **)dst);
  if (!*dst) {
    *dst = (char *)fs_get((*dstl = i) + 1);
  }
  d = *dst;
  while (srcl--) {
    c = *d++ = *src++;
    if ((c == '\015') && (*src != '\012'))
      *d++ = '\012';
  }
  *d = '\0';
  return d - *dst;
}

unsigned long strcrlflen(STRING *s) {
  unsigned long pos = GETPOS(s);
  unsigned long i = SIZE(s);
  unsigned long j = i;
  while (j--)
    if ((SNX(s) == '\015') && ((CHR(s) != '\012') || !j))
      i++;
  SETPOS(s, pos);
  return i;
}

/* auths.c implementation */
#define server_login(user, pass, argc, argv) NIL
#define myusername() ""

#include "auth_log.c"

/* Eudora-specific stubs for library compatibility */
short RunType = 0;

void DebugStr(unsigned char *s) {}
/* ComposeString is implemented in stringutil.c */

extern long GetPrefLong(short id);
extern long GetRLong(int id);

extern void DisposeHandle(Handle h);

/* GetRString, FSWriteP, pstrincmp, Cr, UID_LL_Zap, and SaveMinimalHeader
 * have real implementations in main Eudora source - not stubbed here.
 * CramMD5Authenticator, KrbV4Authenticator, GssapiAuthenticator
 * are implemented in src/imapauth.c and linked from the main binary. */

