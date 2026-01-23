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

#define WriteZero(p, s) memset(p, 0, s)
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#ifndef bool
#define bool int
#endif
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

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

/* Eudora-specific stubs/prototypes for library compatibility */
#ifndef Handle
typedef void **Handle;
#define Handle Handle
#endif
void DisposeHandle(Handle h);
void DisposeMailboxTree(void *tree);
void UID_LL_Zap(void *list);
#ifndef OSErr
typedef short OSErr;
#define OSErr OSErr
#endif
#ifndef noErr
#define noErr 0
#endif

#define CommandPeriod 0
void IMAPSpamWatchSupported(bool supported, bool bNotify);
void MakePStr(unsigned char *p, const void *c, int len);
void StringToNum(unsigned char *p, unsigned long *num);
void AccuZap(Accumulator *pAccu);
void AccuInit(Accumulator **ppAccu);
int AccuAddPtr(Accumulator *pAccu, const void *ptr, long size);

/* Symbols for ASSERT macro in mydefs.h */
extern int RunType;
void DebugStr(unsigned char *s);
unsigned char *ComposeString(unsigned char *dst, const char *fmt, ...);

/* Extra Eudora stubs for IMAP library */
#undef TransError
#define TransError(s) (0)
struct mail_stream;
void SaveMinimalHeader(struct mail_stream *stream);

/* Resource string stub */
char *GetRString(void *dst, int id);
#define IMAP_SENT_FLAG 0 // Dummy ID

/* Mac file I/O stubs */
void AWrite(short refNum, long *count, void *buffer);
void FSWriteP(short refNum, unsigned char *str);
extern unsigned char *Cr;
int pstrincmp(unsigned char *s1, const char *s2, int n);

/* SSL preference stubs */
long GetPrefLong(int id);
long GetRLong(int id);
#define IMAP_SSL_PORT 0 // Dummy ID

/* Eudora UI stubs */
enum { Note, Stop, Caution };
void ComposeStdAlert(int type, int id);
#define ALRTStringsStrn 0
#define NO_SERVER_SSL 0
#define IMAP_SSL_FAILED 0
#define SSL_ERR_STRING 0
#define PREF_SSL_IMAP_SETTING 0

/* Redefine ASSERT to be portable */
#undef ASSERT
#define gethostid clock

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* Preference stubs for library-only build */
#define PrefIsSet(x) 0
#define PREF_IMAP_STUPID_PASSWORD 0
#define PREF_IMAP_POLITE_LOGOUT 0
#define PREF_IMAP_EXTRA_LOGGING 0
#define PREF_IMAP_EXPUNGE_EXCLUSIVITY 100
#define InvalidatePasswords(a, b, c)

#endif /* OS_UNIX_H */
