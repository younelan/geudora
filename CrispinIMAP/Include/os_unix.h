/*
 * Program:	Operating-system dependent routines -- Unix version
 */

#ifndef OS_UNIX_H
#define OS_UNIX_H

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* WriteZero may be defined in mailbox.h - undef if needed */
#undef WriteZero
#define WriteZero(p, s) memset(p, 0, s)
#include <sys/types.h>
#include <time.h>
#include <unistd.h>


/* Use standard SEEK_ macros */
#define L_SET SEEK_SET
#define L_INCR SEEK_CUR
#define L_XTND SEEK_END

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

/* Map dummy gmtFlags for compatibility if needed */
#define gmtFlags u

/* Prototypes for functions provided by os_unix.c or needed from library */
void *fs_get(size_t size);
void fs_resize(void **block, size_t size);
void fs_give(void **block);
void fatal(char *string);
char *cpystr(const char *string);
void *mail_parameters(struct mail_stream *stream, long function, void *value);
void mm_fatal(char *string);
void *env_parameters(long function, void *value);

/* Eudora-specific stubs/prototypes for library compatibility */
#ifndef Handle
typedef void **Handle;
#define Handle Handle
#endif

void DisposeHandle(Handle h);
/* DisposeMailboxTree is defined in imapmailboxes.h with different signature */
#ifndef OSErr
typedef short OSErr;
#define OSErr OSErr
#endif

#ifndef noErr
#define noErr 0
#endif

/* Accumulator structure - dynamic buffer used throughout Eudora */
#ifndef ACCUMULATOR_DEFINED
#define ACCUMULATOR_DEFINED
typedef struct Accumulator {
  char *data;
  long size;
  long offset;
} Accumulator, *AccuPtr, **AccuHandle;
#endif
typedef Accumulator IMAPAccumulator;

/* MailboxNodeHandle is defined in imapnetlib.h - do not redefine */

/* CommandPeriod is a thread-local macro defined in threading.h.
   CrispinIMAP can't include threading.h, so use the accessor function. */
extern short *_CommandPeriodPtr(void);
#ifndef CommandPeriod
#define CommandPeriod (*_CommandPeriodPtr())
#endif
void IMAPSpamWatchSupported(bool supported, bool bNotify);
void IMAPAccuZap(IMAPAccumulator *pAccu);
void IMAPAccuInit(IMAPAccumulator *pAccu);
int IMAPAccuAddPtr(IMAPAccumulator *pAccu, const void *ptr, long size);
/* AccuZap/AccuInit/AccuAddPtr: CrispinIMAP code uses IMAPAccuXxx directly.
   No macro redirects needed — avoids conflicts with Eudora's Accu functions. */

/* Symbols for ASSERT macro in mydefs.h */
extern short RunType;
void DebugStr(unsigned char *s);
char *ComposeString(char *dst, const char *fmt, ...);
// GetRString is declared in gtk_dialogs.h with unsigned char * signature

/* Extra Eudora stubs for IMAP library */
#undef TransError
#define TransError(s) (0)

/* Forward declarations - actual implementations in src/ */
struct mail_stream;
struct UIDNode;
/* UIDNodeHandle is defined in imapnetlib.h - use that definition */

int SaveMinimalHeader(struct mail_stream *stream);

/* Forward declarations matching actual Eudora function signatures */
void UID_LL_Zap(struct UIDNode **list);
/* GetRString is declared in gtk_dialogs.h with unsigned char * signature */
short FSWriteP(short refN, unsigned char *pString);
int pstrincmp(unsigned char *ps, const char *cs, short n);

/* Redefine ASSERT to be portable */
#define gethostid clock

/* Preference stubs for library-only build */
#define PrefIsSet(x) 0
#define PREF_IMAP_STUPID_PASSWORD 0
#define PREF_IMAP_POLITE_LOGOUT 0
#define PREF_IMAP_EXTRA_LOGGING 0
#define PREF_IMAP_EXPUNGE_EXCLUSIVITY 100
#ifndef PREF_SSL_IMAP_SETTING
#define PREF_SSL_IMAP_SETTING 350
#endif
long GetPrefLong(short id);
long GetRLong(int id);
void SetPrefLong(short prefId, long value);
/* Timeout resource IDs (values from StringDefs.h; GetRLong returns 0 in stubs) */
#ifndef OPEN_TIMEOUT
#define OPEN_TIMEOUT 6804
#endif
#ifndef RECV_TIMEOUT
#define RECV_TIMEOUT 7017
#endif

/* Forward declarations for Eudora functions called from imap4r1.c */
void InvalidatePasswords(bool pwGood, bool auxpwGood, bool all);
/* MailboxNodeHandle declared via imapnetlib.h */
void DisposeMailboxTree(MailboxNodeHandle *tree);

/* File I/O */
short AWrite(short refN, long *count, unsigned char *buf);

/* Use plain `char *` for textual parameters to match portable PStr */
void MyStringToNum(char *string, long *num);
#ifndef StringToNum
#define StringToNum(a, b) MyStringToNum(a, b)
#endif

/* Alert/dialog functions */
#ifndef Note
#define Note 1
#endif
short ComposeStdAlert(int alertType, int msgResId, ...);

/* String resource IDs used in imap4r1.c (not in the standard include chain) */
#ifndef IMAP_SENT_FLAG
#define IMAP_SENT_FLAG   13714  /* Resource ID: IMAP sent flag string */
#endif
#ifndef IMAP_SSL_PORT
#define IMAP_SSL_PORT    32404  /* Resource ID: IMAPS port number */
#endif
#ifndef SSL_ERR_STRING
#define SSL_ERR_STRING   32406  /* Resource ID: SSL negotiation failed */
#endif
/* ALRTStringsStrn and NO_SERVER_SSL are defined in StrnDefs.h (included via mydefs.h) */

#endif /* OS_UNIX_H */
