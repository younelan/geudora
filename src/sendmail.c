/* Copyright (c) 2017, Computer History Museum
All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted (subject to the limitations in the disclaimer below) provided that
the following conditions are met:
 * Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.
 * Neither the name of Computer History Museum nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission. NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S
PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE
COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#include "sendmail.h"
#include "Globals.h"
#include "MyRes.h"
#include "StringDefs.h"
#include "StringUtil.h"
#include "binhex.h"
#include "buildtoc.h"
#include "comp.h"
#include "features.h"
#include "fileutil.h"
#include <fcntl.h>
#include <sys/stat.h>
#include "legacy_shim.h"
#include "filtrun.h"
#include "gtk_dialogs.h"
#include "log.h"
#include "mailbox.h"
#include "message.h"
#include "myssl.h"
#include "pop.h"
#include "sasl.h"
#include "util.h"
#include "uudecode.h"
#include "peteglue.h"
#include "threading.h"
#include <time.h>

#define FILE_NUM 34

int ReallyDoAnAlert(int templ, int which);

/* UUCP output mode - always false in GTK port (UUCP is Unix-only legacy) */
extern bool UUPCOut;
extern bool UUPCIn;
extern bool UseFlowOut; /* use format=flowed (RFC 2646) when wrapping */
extern char *GetPOPPref(char *buffer);
extern int UUPCPrime(char *serverName);
extern void UUPCDry(TransStream stream);
/* Logging */
#ifndef LOG_SEND
#define LOG_SEND 0
#endif
/* ApproxMessageSize declared in compact.h */
extern bool ShouldSMTPAuth(void);
extern void OffsetWindow(void *winWP);
extern void MyParamText(const char *p1, const char *p2, const char *p3,
                        const char *p4);
extern int AddOutgoingMesgError(short sumNum, unsigned long uidHash, int errorCode,
                                int tmpl, ...);
extern int ExpandAliases(void **h, void *raw, int n, bool deep);
extern int GetSMTPInfo(char *host);
extern bool DotToNum(char *str, long *num);
extern bool IsFCCAddr(char *addr);
extern bool IsNewsgroupAddr(char *addr);
extern int UUPCWriteAddr(char *addr);
extern int BoxSpecByName(char * spec, char *name);
extern int HTMLPreamble(void *acc, char *subj, int n, bool b);
extern int BuildHTML(void *acc, GtkWidget *pte, void *p, long stop, long val,
                       void *p2, void *p3, int n, char *mid,
                       void *parts, char * errSpec);
extern int PeteLen(GtkWidget *pte);
extern int BuildEnriched(void *acc, GtkWidget *pte, void *p, long stop,
                           long val, void *p2, bool b);
extern void ConvertExcerpt(GtkWidget *pte, long start, long stop, void *p1,
                           void *p2);
/* PETEGetTextLen declared in peteglue.h */
extern void PeteCleanList(GtkWidget *pte);
extern short Prior2Display(short priority);
extern int HTMLPostamble(void *acc, bool b);
extern int MyHandToHand(void **h);
extern long SearchStrPtr(char *pattern, char *text,
                         long offset, long len, bool caseSens, bool wordOnly,
                         void *p);
extern long SearchPtrPtr(char *pattern, long patLen, char *text,
                         long offset, long stop, bool caseSens, bool wordOnly,
                         void *p);
extern void MyGetWTitle(void *win, char *title);
extern void SecondsToDate(uint32_t secs, DateTimeRec *dtr);
extern void GetTime(DateTimeRec *dtr);
extern void GetResInfo(void *res, short *id, unsigned int *type,
                       char *name);
/* AttachOptNumber is a macro in compact.h */
#include "compact.h"
extern void UpdateNumStat(int type, int val);
extern bool IsMailbox(char * spec);
extern MyWindowPtr GetAMessageLo(TOCType * tocH, int sumNum, GtkWidget *winWP,
                                 void *p1, bool newWin, bool *outNew);
extern int TransmitMessageForSpool(TransStream stream, MessHandle messH);
extern int FlushTOCs(bool andClose, bool canSkip);
#define MoveHHi(h) /* no-op: Mac memory compaction not needed in GTK port */
/* DirIterateMac removed - was a Mac CInfoPBRec stub, now dead code.
 * Directory iteration should use POSIX opendir/readdir or GLib g_dir_*. */
#ifndef LOG_TPUT
#define LOG_TPUT 0
#endif
#ifndef mFulErr
#define mFulErr (-108) /* Mac memory full error */
#endif
/* NumToString: converts long to C string via sprintf (see MyNumToString in fileutil.c) */
#ifndef NumToString
#define NumToString MyNumToString
extern void MyNumToString(long n, char *s);
#endif
/* WrapSendTrans: special SMTP-layer sender for line-wrapped connections; NULL
 * in GTK port */
#define WrapSendTrans NULL
#define DisposePtr(p) free(p)
/* PCatC: append a single character to a C string (replaces Pascal PCatC) */
#undef PCatC
#define PCatC(str, ch) do { \
  size_t _pcl = strlen((const char*)(str)); \
  if (_pcl < sizeof(str) - 1) { ((char*)(str))[_pcl] = (char)(ch); ((char*)(str))[_pcl+1] = '\0'; } \
} while(0)
/* Message option bits */
#ifndef OPT_STRIP
#define OPT_STRIP 0x0080
#define OPT_JUST_EXCERPT 0x0100
#define OPT_BLOAT 0x0200
#endif
#ifndef LOG_PROTO
#define LOG_PROTO 0
#endif
#ifndef OPT_RECEIPT
#define OPT_RECEIPT 0x0020
#endif
#ifndef OPT_BULK
#define OPT_BULK 0x0040
#endif
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/************************************************************************
 * functions for dealing with a sendmail (or other?) smtp server
 ************************************************************************/


typedef enum {
  smtpHelo = 1,
  smtpMail,
  smtpRcpt,
  smtpData,
  smtpRset,
  smtpSend,
  smtpSoml,
  smtpSaml,
  smtpVrfy,
  smtpExpn,
  smtpHelp,
  smtpNoop,
  smtpQuit,
  smtpTurn,
  smtpEhlo,
  smtpAuth,
  smtpStarttls
} SMTPEnum;

typedef enum { k1342LWSP, k1342Word, k1342Plain, k1342End } Enum1342;
typedef struct {
  unsigned char word[256];
  Enum1342 wordType;
} Token1342, *Token1342Ptr;

typedef struct {
  bool sawGreeting;
  long maxSize;
  bool mime8bit;
  bool pipeline;
  SASLEnum saslMech;
  bool starttls;
  unsigned char digest[256];
} EhloStuff, *EhloStuffPtr, *EhloStuffHandle;

EhloStuffHandle Ehlo;

#define RingNext(pointer, array, size)                                         \
  ((array) + (((pointer) - (array)) + 1) % (size))
#define kSpecialSendDidntPanOut -1
#define CMD_BUFFER 1024
typedef struct wdsEntry *WDSPtr;
/************************************************************************
 * declarations for private routines
 ************************************************************************/
int SendPtrHead(TransStream stream, char * label, long labelLen, char * body,
                  long bodyLen, bool allowQP, short tid);
int SendRawMIME(TransStream stream, char * spec);
int SendEnriched(TransStream stream, char *text, long textLen, DecoderFunc *encoder);
void EhloLine(char *line, long size);
int DoIntroductions(TransStream stream);
int DoSMTPAuth(TransStream stream);
int SayByeBye(TransStream stream);
int SendCmd(TransStream stream, int cmd, char *args, AccuPtr argsAcc);
int SendCmdGetReply(TransStream stream, int cmd, char *args,
                    bool chatter, MessHandle messH);
int SendHeaderLine(TransStream stream, MessHandle messH, short index,
                   bool allowQP, short tid);
int SendAttachments(TransStream stream, MessHandle messH, long flags,
                    char * boundary, short tableID, short idBase);
int SendNewsGroups(TransStream stream, AccuPtr newsGroupAcc, short tid);
int SendCID(TransStream stream, MessHandle messH, long part, short n);
int SMTPCmdError(int cmd, char *args, char *message);
void PrimeProgress(MessHandle messH);
int SendDigest(TransStream stream, char * spec);
int WannaSend(MyWindowPtr win);
short SendXSender(TransStream stream, MessHandle messH);
int SendMIMEHeaders(TransStream stream, MessHandle messH, void *enriched,
                      char * boundary, emsMIMEHandle *tlMIME, bool isRelated);
int SendContentType(TransStream stream, char *text1, long text1Len, long offset1,
                      char *text2, long text2Len, long offset2, short tableID, long *flags,
                      long *opts, char * name, emsMIMEHandle *tlMIME,
                      char * subType);
long DecideEncoding(char *text1, long text1Len, char *text2, long text2Len,
                    bool anyfunny, short etid, long flags);
bool SevenBitTable(short tableID);
bool Any2022(char *text, long textLen, long offset);
char * NameCharset(char * charset, short tid, emsMIMEHandle *tlMIME);
bool LongerThan(char *text, long textLen, short len);
bool LongerWordThan(char *text, long textLen, short len);
char *Encode1342(char *source, long len, short lineLimit,
                 short *charsOnLine, char * nl, short tid, long *outLen);
void Encode1342String(char * s, short tid);
void Next1342Word(char **startP, char *end,
                  Token1342Ptr current, char * delim, bool *wasQuote,
                  bool *encQuote);
int SendAnonFTP(TransStream stream, char * spec);
int SendSpecial(TransStream stream, char * spec, AttMapPtr amp);
int SendAddressHead(TransStream stream, PETEHandle pte, HSPtr hs,
                      bool allowQP, short tid);
int SendNormalHead(TransStream stream, PETEHandle pte, HSPtr hs, bool allowQP,
                     short tid);
int SendSubjectHead(TransStream stream, PETEHandle pte, HSPtr hs,
                      bool allowQP, short tid);
// int SendPipeRCPT(TransStream stream,char * newRecip,AccuPtr pipe,bool
// chatter);
int SendPipeRCPT(TransStream stream, char * newRecip, AccuPtr pipe,
                   bool chatter, MessHandle messH);
int PeriodEncoder(CallType callType, DecoderPBPtr pb);
long StuffPeriods(char *in, long inLen, char *out,
                  char * newLine, short *nlStatePtr);
int SendRelatedParts(TransStream stream, MessHandle messH, long flags,
                     StackHandle parts, char * boundary);
int SendAnAttachment(TransStream stream, MessHandle messH, long flags,
                     bool canQP, bool plainText, short tableID, char * boundary,
                     char * spec, short multiID, short partID);
int SendAttachmentFolder(TransStream stream, MessHandle messH, long flags,
                         bool canQP, bool plainText, short tableID,
                         char * boundary, char * folderSpec, short multiID,
                         short partID, short *partBase, CInfoPBRec *hfi);
int sErr;
void ConvertPictPart(char * origSpec, char * spec);
int FlattenAndSpool(char * spec);
int FlattenQTMovie(char * inSpec, char * outSpec);
int AllAttachOnBoard(MessHandle messH);

int TransmitMessageMixed(TransmitPBPtr pb, bool topLevel);
int TransmitMessageRelated(TransmitPBPtr pb, bool topLevel);
int TransmitMessageText(TransmitPBPtr pb, bool sigToo, bool topLevel);
int TransmitMessageBody(TransmitPBPtr pb, bool withClosure);
int TransmitMessageBodyHeaders(TransmitPBPtr pb, bool sigToo, bool topLevel);
int TransmitTopHeaders(TransmitPBPtr pb);
int TransmitMultiHeaders(TransmitPBPtr pb, short subType, char * boundary,
                           short otherParm, char * otherVal);
int TransmitMessageTextBloat(TransmitPBPtr pb, bool sigToo, bool topLevel);
int TransmitMessageTextStrip(TransmitPBPtr pb, bool sigToo, bool topLevel);
int TransmitMessageTextPlain(TransmitPBPtr pb, bool sigToo, bool topLevel);
int TransmitMessageTextRich(TransmitPBPtr pb, bool sigToo, bool topLevel);
int TransmitMessageSig(TransmitPBPtr pb);
int TransmitMessageSigBloat(TransmitPBPtr pb);
int TransmitMessageSigBody(TransmitPBPtr pb, bool withHeaders);
int TransmitMessageSigBodyPlain(TransmitPBPtr pb);

/************************************************************************
 * Public routines
 ************************************************************************/

/************************************************************************
 * StartSMTP - initiate a connection with the specified SMTP server
 ************************************************************************/
int StartSMTP(TransStream stream, char *serverName, long port) {
  g_print("StartSMTP: server='%s' port=%ld\n", (char*)serverName, port);
  if (UUPCOut)
    sErr = UUPCPrime(serverName);
  else if (!(sErr = ConnectTrans(stream, serverName, port, False,
                                 GetRLong(OPEN_TIMEOUT)))) {
    g_print("StartSMTP: connected, doing introductions\n");
    sErr = DoIntroductions(stream);
    g_print("StartSMTP: introductions done, sErr=%d\n", sErr);
  } else {
    g_print("StartSMTP: connect failed, sErr=%d\n", sErr);
  }
  return (sErr);
}

/************************************************************************
 * MySendMessage - send a message to the SMTP server
 ************************************************************************/
int MySendMessage(TransStream stream, TOCType * tocH, int sumNum,
                  CSpecHandle specList) {
  WindowPtr messWinWP;
  unsigned char buffer[256];
  unsigned char param[256];
  MessHandle messH;
  Accumulator newsGroupAcc;

  Zero(newsGroupAcc);

  /*
   * handle open, dirty windows
   */
  if (!(messH = SaveB4Send(tocH, sumNum)))
    return (1);

  messWinWP = GetMyWindowWindowPtr(messH->win);
  /*
   * Log, if we must
   */
  GetWTitle(messWinWP, buffer);
  g_debug("log: %d", SENDING, buffer);

  if (PrefIsSet(PREF_POP_SEND)) {
    long size = sizeof(buffer);
    *buffer = '+';
    if (POPCmdGetReply(stream, kpcXmit, "", buffer, &size) || *buffer != '+') {
      if (*buffer == '-')
        POPCmdError(kpcXmit, "", buffer);
      return (1);
    }
  } else {
    /*
     * reset SMTP
     */
    sErr = SendCmdGetReply(stream, smtpRset, NULL, True, messH);
    if (sErr / 100 != 2)
      return (sErr);
    sErr = 0;

    /*
     * envelope
     */
    MessReturnAddr(messH, buffer);
    if (Ehlo && *Ehlo->digest) {
      ComposeString(param, " %r=%p", EsmtpStrn + esmtpAmd5,
                    Ehlo->digest);
      g_strlcat(buffer, param, sizeof(buffer));
    }
    if (Ehlo && Ehlo->maxSize) {
      ComposeString(param, " %r=%d", EsmtpStrn + esmtpSize,
                    ApproxMessageSize(messH) K);
      g_strlcat(buffer, param, sizeof(buffer));
    }
    if (Ehlo && Ehlo->mime8bit && PrefIsSet(PREF_ALLOW_8BITMIME)) {
      ComposeRString(param, BODY_EQUALS, EsmtpStrn + esmtp8BMIME);
      g_strlcat(buffer, param, sizeof(buffer));
    }
    sErr = SendCmdGetReply(stream, smtpMail, buffer, True, messH);

    if (Ehlo && *Ehlo->digest && IsAddrErr(sErr))
      InvalidatePasswords(False, True, False);
    if (sErr / 100 != 2)
      return (sErr);
    sErr = 0;

    if (WrapWrong)
      OffsetWindow(messWinWP);

    if (sErr = DoRcptTos(stream, messH, True, specList, &newsGroupAcc))
      goto done;
  }

  messH->newsGroupAcc = newsGroupAcc;
  if (sErr = TransmitMessageHi(stream, messH, True, !PrefIsSet(PREF_POP_SEND)))
    goto done;
  free(newsGroupAcc.data); newsGroupAcc.data = NULL; newsGroupAcc.offset = newsGroupAcc.size = 0;
  Zero(messH->newsGroupAcc);

  // TimeStamp(tocH,sumNum,GMTDateTime(),ZoneSecs());

done:
  (void)g_debug("log: %d", sErr ? LOG_FAILED : LOG_SUCCEEDED, sErr);
  free(newsGroupAcc.data); newsGroupAcc.data = NULL; newsGroupAcc.offset = newsGroupAcc.size = 0;
  return (sErr);
}

/**********************************************************************
 * MessReturnAddr - get the right return addr for this message; if addr
 *  is in "Me" nickname, returns from addr.  Else returns configured addr
 **********************************************************************/
char * MessReturnAddr(MessHandle messH, char * buffer) {
  unsigned char from[256];
  unsigned char returnAddr[256];
  short template =
      MessOptIsSet(messH, OPT_RECEIPT) && MessOptIsSet(messH, OPT_BULK)
          ? EMPTY_MFROM
          : NORMAL_MFROM;

  // return address usually
  GetReturnAddr(returnAddr, False);

  // or contents of from field
  if (!CompHeadGetStr(messH, FROM_HEAD, from) &&
      !StringSame(returnAddr, from) && IsMe(from))
    g_strlcpy(returnAddr, from, sizeof(returnAddr));

  // strip <>'s
  ShortAddr(returnAddr, returnAddr);

  // and compose the argument
  ComposeRString(buffer, template, returnAddr);

  return (buffer);
}

/************************************************************************
 * EndSMTP - done talking to SMTP
 ************************************************************************/
int EndSMTP(TransStream stream) {
  if (UUPCOut)
    UUPCDry(stream);
  else {
    SilenceTrans(
        stream,
        True); // ignore all TCP/IP errors from here on out -jdboyd 030113
    if ((!sErr || (sErr < 600 && sErr >= 400)) && !(sErr = SayByeBye(stream)))
      DisTrans(stream);
    sErr = DestroyTrans(stream);
  }
  free(eSignature);
  free(RichSignature);
  free(HTMLSignature);
  free(Ehlo);
  return (sErr);
}

/************************************************************************
 * SMTPError - return the last SMTP error
 ************************************************************************/
int SMTPError(TransStream stream) { return (sErr); }

/************************************************************************
 * Private routines
 ************************************************************************/
/************************************************************************
 * DoIntroductions - take care of the beginning of the SMTP protocol
 ************************************************************************/
int DoIntroductions(TransStream stream) {
  unsigned char buffer[256];

  Ehlo = NewZH(EhloStuff);

  /*
   * get banner from the remote end
   */
  sErr = GetReply(stream, buffer, sizeof(buffer), True, False);
  if (sErr / 100 != 2)
    return (sErr);
  sErr = 0;

SayHello:
  /*
   * tell it who we are
   */
  sErr = SendCmdGetReply(stream, smtpEhlo, WhoAmI(stream, buffer), True, NULL);
  if (sErr >= 400)
    sErr = SendCmdGetReply(stream, smtpHelo, WhoAmI(stream, buffer), True, NULL);

  if (sErr / 100 == 2) {
#ifdef ESSL
    if (ShouldUseSSL(stream) && !(stream->ESSLSetting && esslSSLInUse)) {
      if (!Ehlo->starttls) {
        if (!(stream->ESSLSetting & esslOptional)) {
          sErr = 502;
          //	SMTPCmdError(starttls,NULL,GetRString(buffer,SSL_ERR_STRING)+1);
          ComposeStdAlert(Note, ALRTStringsStrn + NO_SERVER_SSL);
        }
      } else {
        OSStatus sslErr;
        int tempErr;

        tempErr =
            SendCmdGetReply(stream, smtpStarttls, NULL,
                            (stream->ESSLSetting & esslOptional) != 0, NULL);
        if (tempErr / 100 == 2) {
          sslErr = ESSLStartSSL(stream);
          if (sslErr) {
            if (!(stream->ESSLSetting & esslOptional)) {
              sErr = 554;
              SMTPCmdError(smtpStarttls, NULL,
                           GetRString(buffer, SSL_ERR_STRING));
            }
          } else if (stream->ESSLSetting & esslSSLInUse) {
            ZeroHandle(Ehlo);
            goto SayHello;
          }
        } else if (!(stream->ESSLSetting & esslOptional)) {
          sErr = tempErr;
        }
      }
    }
#endif
    if ((sErr / 100 == 2)) {
      if (!PrefIsSet(PREF_SMTP_AUTH_NOTOK)) {
        short (*authfunc)(TransStream stream) = NULL;

        if (!Ehlo->saslMech) {
          g_debug("SMTP auth not available, disabling");
          SetPref(PREF_SMTP_GAVE_530, NoStr);
          SetPref(PREF_SMTP_DOES_AUTH, NoStr);
        } else {
          SetPref(PREF_SMTP_DOES_AUTH, YesStr);
          if (PrefIsSet(PREF_KERBEROS) || *CurPers->password) {
            g_debug("SMTP auth (mech=%d) under way...",
                        Ehlo->saslMech);
            // We have everything we need to attempt authentication
            sErr = DoSMTPAuth(stream);
          } else
            g_debug("SMTP auth not being attempted, no credentials (mech=%d)",
                    Ehlo->saslMech);
        }
      } else if (Ehlo->saslMech)
        g_debug("SMTP auth (mech=%d) available but forbidden",
                    Ehlo->saslMech);
    }
  }

  return ((sErr / 100 != 2) ? sErr : (sErr = 0));
}

/************************************************************************
 * DoSMTPAuth - authenticate for SMTP
 ************************************************************************/
int DoSMTPAuth(TransStream stream) {
  Accumulator chalAcc, respAcc;
  short rounds = 0;
  unsigned char service[64];
  long state;

  Zero(chalAcc);
  Zero(respAcc);

  // put auth command in initial response
  AccuAddRes(&respAcc, EsmtpStrn + esmtpAuth);
  AccuAddChar(&respAcc, ' ');

  // grab service name for kerberos
  GetRString(service, K5_SMTP_SERVICE);

  // run the mechanism
  do {
    // Build the response
    if (SASLDo(service, Ehlo->saslMech, rounds++, &state, &chalAcc,
               &respAcc))
      sErr = 601;
    else {
      // Send the response
      if (SendCmd(stream, 0, NULL, &respAcc))
        sErr = 601;
      else {
        // get the reply
        sErr = GetReplyLo(stream, NULL, 0, &respAcc, false, false);
        chalAcc.offset = 0;
        if (sErr == 334) {
          char *spot;

          // extract the challenge token
          AccuAddChar(&respAcc, 0);
          spot = respAcc.data;
          while (*spot && !IsWhite(*spot))
            spot++; // skip code

          // rest will be base64 or whitespace, the decoder won't care
          AccuAddFromHandle(
              &chalAcc, respAcc.data, spot - (char *)*respAcc.data,
              respAcc.offset - (spot - (char *)*respAcc.data) - 1);
        }
      }
    }

    // the response needs no pre-loading anymore.
    respAcc.offset = 0;
  } while (sErr / 100 == 3);

  // if the server rejects our auth, but doesn't actually
  // require auth for anything, pretend like nothing happened
  if (sErr / 100 == 5 && !ShouldSMTPAuth())
    sErr = 0;

  // Kill the accumulators
  free(chalAcc.data); chalAcc.data = NULL; chalAcc.offset = chalAcc.size = 0;
  free(respAcc.data); respAcc.data = NULL; respAcc.offset = respAcc.size = 0;

  // Let the sasl mechanism know how it all came out
  SASLDone(service, Ehlo->saslMech, rounds, &state, sErr);

  return sErr;
}

/************************************************************************
 * SayByeBye - take care of the end of the SMTP protocol
 ************************************************************************/
int SayByeBye(TransStream stream) {
  sErr = SendCmdGetReply(stream, smtpQuit, NULL, False, NULL);
  return ((sErr / 100 != 2) ? sErr : (sErr = 0));
}

/************************************************************************
 * SendCmd - send an smtp command, with optional arguments
 ************************************************************************/
int SendCmd(TransStream stream, int cmd, char *args, AccuPtr argsAcc) {
  Byte buffer[CMD_BUFFER];

  GetRString(buffer, SMTP_STRN + cmd);
  if (args && *args)
    g_strlcat(buffer, args, sizeof(buffer));
  if (cmd)
    ProgressMessage(kpMessage, buffer);

  if (!argsAcc) {
    g_strlcat(buffer, NewLine, sizeof(buffer));
    if (sErr = SendPString(stream, buffer))
      return (sErr);
  } else {
    // send the command
    if (cmd)
      PCatC(buffer, ' ');
    if (sErr = SendPString(stream, buffer))
      return (sErr);

    // add a newline to the accumulator
    AccuAddStr(argsAcc, NewLine);

    // send the data
    sErr = SendTrans(stream, argsAcc->data, argsAcc->offset, NULL);

    // erase what we did to the accumulator
    argsAcc->offset -= strlen((const char *)NewLine);

    if (sErr)
      return sErr;
  }

  return (noErr);
}

/************************************************************************
 * SMTPCmdError - report an error for an SMTP command
 ************************************************************************/
int SMTPCmdError(int cmd, char *args, char *message) {
  unsigned char theCmd[256];
  unsigned char theError[256];
  int err;

  GetRString(theCmd, 2800 + cmd);
  if (args && *args)
    g_strlcat(theCmd, args, sizeof(theCmd));
  strcpy((char *)theError, (const char *)message);
  {
    size_t len = strlen((const char *)theError);
    if (len > 0 && theError[len - 1] == '\012')
      theError[--len] = '\0';
    if (len > 0 && theError[len - 1] == '\015')
      theError[--len] = '\0';
  }
  MyParamText(theCmd, theError, "SMTP", "");
  err = ReallyDoAnAlert(PROTO_ERR_ALRT, Note);
  return (err);
}

/************************************************************************
 * SendCmdGetReply - send an smtp command, with optional arguments, and
 * wait for the reply.	Returns reply code.
 ************************************************************************/
int SendCmdGetReply(TransStream stream, int cmd, char *args,
                    bool chatter, MessHandle messH) {
  unsigned char buffer[CMD_BUFFER];

  if (sErr = SendCmd(stream, cmd, args, NULL))
    return (601); /* error in transmission */
  sErr = GetReply(stream, buffer, sizeof(buffer), False, cmd == smtpEhlo);
  if (cmd == smtpRcpt && sErr > 499 && sErr < 600)
    sErr = 550;
  if (sErr > 399 && sErr <= 600 && (IsAddrErr(sErr) || cmd != smtpRcpt) &&
      cmd != smtpEhlo && chatter)
    SMTPCmdError(cmd ? cmd : smtpAuth, (cmd && cmd != smtpAuth) ? args : NULL,
                 buffer);
  if (messH && IsAddrErr(sErr) && cmd != smtpRcpt) {
    if (strchr(buffer, '\015'))
      *strchr(buffer, '\015') = 0;
    AddOutgoingMesgError(messH->sumNum, SumOf(messH)->uidHash, sErr,
                         BAD_XMIT_ERR_TEXT, "", buffer);
  }
  return (sErr);
}

/************************************************************************
 * EhloLine - process an Ehlo return
 ************************************************************************/
void EhloLine(char *line, long size) {
  unsigned char directive[64];
  unsigned char value[256], digest;
  char *start, *end, *stop;
  long longVal;

  if (!Ehlo)
    return;

  if (Ehlo->sawGreeting) {
    Ehlo->sawGreeting = True;
    return;
  }

  // Stupid AUTH= nonsense
  GetRString(value, EsmtpStrn + esmtpAuth);
  PCatC(value, '=');
  if (start = PPtrFindSub(value, line, size))
    start[4] = ' ';

  /*
   * grab the Ehlo directive
   */
  end = line + size;
  start = line + 4;
  while (start < end && IsWhite(*start))
    start++;
  for (stop = start; stop < end && !IsWhite(*stop); stop++)
    ;
  { size_t _mpl = (stop - start); memcpy(directive, start, _mpl); ((char*)(directive))[_mpl] = '\0'; }
  { size_t _dlen = strlen((const char *)directive);
    if (_dlen > 0 && directive[_dlen - 1] == '\r')
      directive[_dlen - 1] = '\0'; }

  /*
   * and the value
   */
  while (stop < end && IsWhite(*stop))
    stop++;
  { size_t _mpl = (size - (stop - line)); memcpy(value, stop, _mpl); ((char*)(value))[_mpl] = '\0'; }

  /*
   * now what?
   */
  switch (FindSTRNIndex(EsmtpStrn, directive)) {
  case esmtpSize:
    Ehlo->maxSize = 0x7fffffff;
    if (strlen((const char *)value) <= 9) // avoid rilly big numbers
    {
      StringToNum(value, &longVal);
      if (longVal > 1024)
        Ehlo->maxSize = longVal;
      g_debug("ESMTP size %d", longVal);
    } else
      g_debug("ESMTP size invalid (%p)", value);
    break;
  case esmtp8BMIME:
    Ehlo->mime8bit = True;
    g_debug("ESMTP mime8bit");
    break;
  case esmtpAuth: {
    SASLEnum mech = Ehlo->saslMech;
    char *spot = value;
    unsigned char service[32];

    GetRString(service, K5_SMTP_SERVICE);

    while (PToken(value, directive, &spot, " \011\012\015"))
      mech = SASLFind(service, directive, mech);

    Ehlo->saslMech = mech;
    if (mech)
      g_debug("ESMTP SASL mech %d", mech);
  } break;
  case esmtpAmd5:
    SetPref(PREF_SMTP_DOES_AUTH, YesStr);
    g_strlcpy((char *)(directive), (char *)(CurPers->password), sizeof(directive));
    GenDigest(value, directive, (char *)digest);
    g_strlcpy((char *)Ehlo->digest, (const char *)digest, sizeof(Ehlo->digest));
    g_debug("ESMTP AMD5 auth found");
    break;
  case esmtpPipeline:
    Ehlo->pipeline = True;
    g_debug("ESMTP pipeline");
    break;
  case esmtpStartTLS:
    Ehlo->starttls = True;
    g_debug("ESMTP starttls");
    break;
  default:
    g_debug("ESMTP has NFI what �%p� is supposed to mean",
                directive);
    break;
  }
}

/************************************************************************
 * DoRcptTos - tell the remote sendmail who is getting the message
 ************************************************************************/
int DoRcptTos(TransStream stream, MessHandle messH, bool chatter,
              CSpecHandle fccList, AccuPtr newsGroupAcc) {
  sErr = DoRcptTosFrom(stream, messH, TO_HEAD, chatter, fccList, newsGroupAcc);
  if (sErr)
    return (sErr);
  sErr = DoRcptTosFrom(stream, messH, BCC_HEAD, chatter, fccList, newsGroupAcc);
  if (sErr)
    return (sErr);
  sErr = DoRcptTosFrom(stream, messH, CC_HEAD, chatter, fccList, newsGroupAcc);
  return (sErr);
}

/************************************************************************
 * DoRcptTosFrom - do the Rcpt to's from a particular TERec
 ************************************************************************/
int DoRcptTosFrom(TransStream stream, MessHandle messH, short index,
                  bool chatter, CSpecHandle fccSpecs, AccuPtr newsGroupAcc) {
  unsigned char toWhom[256];
  char **addresses = NULL;
  char **rawAddresses = NULL;
  char *address;
  HeadSpec hs;
  void *text;
  bool evilSendmail = PrefIsSet(PREF_EVIL_SENDMAIL);
  char *spot;
  unsigned char server[256];
  long junk;
  Accumulator pipe;
  short err;

  Zero(pipe);

  if (evilSendmail) {
    GetSMTPInfo(server);
    if (DotToNum(server, &junk)) {
      /* ip address; turn into domain literal */
      PInsert(server, sizeof(server), "[", server);
      PCatC(server, ']');
    }
  }

  sErr = 550;
  if (CompHeadFind(messH, index, &hs) &&
      !CompHeadGetText(TheBody, &hs, &text)) {
    if (!(sErr = SuckAddresses(&rawAddresses, (char **)text, False, True, False, NULL))) {
      sErr = 200;
      if (rawAddresses && rawAddresses[0] && rawAddresses[0][0]) {
        ExpandAliases((void **)&addresses, (void *)rawAddresses, 0, False);
        g_strfreev(rawAddresses); rawAddresses = NULL;
        if (!addresses) {
          AddOutgoingMesgError(
              messH->sumNum,
              messH->tocH->sums[messH->sumNum].uidHash, sErr,
              BAD_ADDRESS);
          return (sErr = 550);
        }
        for (int _ai = 0; addresses[_ai]; _ai++) { address = (char *)addresses[_ai];
          /*
           * skip groups
           */
          {
            size_t _alen = strlen((const char *)address);
            if ((_alen > 0 && address[_alen - 1] == ':') || address[0] == ';')
              continue; /* skip group identifiers */
          }

          /*
           * handle Fcc's
           */
          // Folder Carbon Copy - do not support FCC in Light
          if (HasFeature(featureFcc)) {
            if (IsFCCAddr(address)) {
              if (fccSpecs) {
                if (sErr = AddFccToList(address, fccSpecs))
                  break;
              }
              continue;
            } else if (IsNewsgroupAddr(address)) {
              if (newsGroupAcc) {
                sErr = noErr;
                if (newsGroupAcc->offset)
                  sErr = AccuAddRes(newsGroupAcc, COMMA_SPACE);
                if (!sErr)
                  sErr = AccuAddPtr(newsGroupAcc, address, strlen((const char *)address));
                if (sErr)
                  break;
              }
              continue;
            }
          }

          if (strlen((const char *)address) > MAX_ALIAS) {
            sErr = 550;
            AddOutgoingMesgError(
                messH->sumNum,
                messH->tocH->sums[messH->sumNum].uidHash, sErr,
                BAD_ADDRESS);
            break;
          }
          if (UUPCOut) {
            if (sErr = UUPCWriteAddr(address))
              break;
          } else {
            toWhom[0] = '<';
            toWhom[1] = '\0';
            if (evilSendmail && strlen((const char *)toWhom) + strlen((const char *)server) + 6 < sizeof(toWhom)) {
              for (spot = address + strlen((const char *)address) - 1; spot > address; spot--)
                if (*spot == '@')
                  *spot = '%';
            }
            g_strlcat((char *)toWhom, (const char *)address, sizeof(toWhom));
            if (evilSendmail && strlen((const char *)toWhom) + strlen((const char *)server) + 6 < sizeof(toWhom)) {
              PCatC(toWhom, '@');
              g_strlcat(toWhom, server, sizeof(toWhom));
            }
            PCatC(toWhom, '>');

            if (Ehlo && Ehlo->pipeline)
              sErr = SendPipeRCPT(stream, toWhom, &pipe, chatter, messH);
            else {
              unsigned char buffer[CMD_BUFFER];

              if (sErr = SendCmd(stream, smtpRcpt, toWhom, NULL))
                sErr = 601; /* error in transmission */
              sErr = GetReply(stream, buffer, sizeof(buffer), False, false);
              if (sErr > 499 && sErr < 600)
                sErr = 550;
              if (IsAddrErr(sErr)) {
                { size_t _blen = strlen((const char *)buffer);
                  if (_blen > 0 && buffer[_blen - 1] == '\r')
                    buffer[_blen - 1] = '\0'; }
                AddOutgoingMesgError(
                    messH->sumNum,
                    messH->tocH->sums[messH->sumNum].uidHash, sErr,
                    BAD_ADDRESS_ERR_TEXT, address, buffer);
              }
            }
            if (sErr / 100 != 2) {
              chatter = False;
              break;
            }
          }
        }
        g_strfreev(addresses); addresses = NULL;
        if (Ehlo && Ehlo->pipeline) {
          err = SendPipeRCPT(stream, NULL, &pipe, chatter, messH);
          if (sErr / 100 == 2 || !sErr)
            sErr = err;
        }
      } else
        { g_strfreev(rawAddresses); rawAddresses = NULL; }
    } else {
      sErr = 550;
      AddOutgoingMesgError(messH->sumNum,
                           messH->tocH->sums[messH->sumNum].uidHash,
                           sErr, BAD_ADDRESS);
    }
    free(text);
  }
  return (sErr / 100 != 2 ? sErr : (sErr = 0));
}

/************************************************************************
 * SendPipeRCPT - send rcpt to commands in a pipeline
 ************************************************************************/
int SendPipeRCPT(TransStream stream, char * newRecip, AccuPtr pipe,
                   bool chatter, MessHandle messH) {
  int err = 200;
  int firstErr = 200;
  unsigned char buffer[CMD_BUFFER];
  unsigned char address[256];

  /*
   * reap outstanding rcpts if need be
   */
  while (!newRecip && pipe->offset || pipe->offset > 3 K) {
    g_strlcpy(address, pipe->data, sizeof(address));
    err = GetReply(stream, buffer, sizeof(buffer), chatter, false);
    if (err > 499 && err < 600)
      err = 550;
    if (firstErr / 100 == 2)
      firstErr = err;
    if (err / 100 != 2) {
      if (chatter && IsAddrErr(err)) {
        { size_t _blen = strlen((const char *)buffer);
          if (_blen > 0 && buffer[_blen - 1] == '\r')
            buffer[_blen - 1] = '\0'; }
        AddOutgoingMesgError(messH->sumNum,
                             messH->tocH->sums[messH->sumNum].uidHash,
                             err, BAD_ADDRESS_ERR_TEXT, address, buffer);
      }
      chatter = False;
    }
    // take the first address out of the accumulator
    {
      size_t _addrSize = strlen((const char *)address) + 1;
      memmove(pipe->data, pipe->data + _addrSize, pipe->offset - _addrSize);
      pipe->offset -= _addrSize;
    }
  }

  /*
   * send current address
   */
  if (err / 100 == 2 && newRecip) {
    if (err = SendCmd(stream, smtpRcpt, newRecip, NULL))
      return (601); /* error in transmission */
    if (err = AccuAddPtr(pipe, newRecip, strlen((const char *)newRecip) + 1)) {
      WarnUser(MEM_ERR, err);
      return (601);
    }
  }
  return (firstErr);
}

/**********************************************************************
 * AddFccToList - add a mailbox to the fcc list.
 **********************************************************************/
int AddFccToList(char * fcc, CSpecHandle list) {
  FSSpec spec;
  unsigned char name[256];
  unsigned char prefix[16];
  int err;

  UseFeature(featureFcc);
  g_strlcpy((char *)(name), (char *)(fcc), sizeof(name));
  if (name[0] == '"') {
    memmove(name, name + 1, strlen((const char *)name));
  }

  TrimPrefix(name, GetRString(prefix, FCC_PREFIX));

  if (err = BoxSpecByName(&spec, name))
    return (FileSystemError(NOT_MAILBOX, name, err));

  AddSpecToList(&spec, list);

  if (err = MemError())
    WarnUser(MEM_ERR, err);
  return (err);
}

/************************************************************************
 * TransmitMessage - send a message to the remote sendmail
 ************************************************************************/
int TransmitMessage(TransStream stream, MessHandle messH, bool chatter,
                    bool mime, bool others, emsMIMEHandle *tlMIME,
                    bool sendDataCmd) {
  return TransmitMessageLo(stream, messH, chatter, mime, others, tlMIME,
                           sendDataCmd, !UUPCOut, true);
}

/************************************************************************
 * TransmitMessageLo - send a message to the remote sendmail
 ************************************************************************/
int TransmitMessageLo(TransStream stream, MessHandle messH, bool chatter,
                      bool mime, bool others, emsMIMEHandle *tlMIME,
                      bool sendDataCmd, bool finishSMTP, bool doTopLevel) {
  TransmitPB pb;
  FSSpec spec;
  unsigned char scratch[256];
  FSSpec errSpec;

  Zero(pb);
  pb.messH = messH;
  pb.mime = mime;
  pb.others = others;
  pb.receipt =
      MessOptIsSet(messH, OPT_BULK) && MessOptIsSet(messH, OPT_RECEIPT);
  pb.parts = NULL;
  pb.isRelated = false;
  pb.hasAttachments = 1 != GetIndAttachment(messH, 1, &spec, &pb.hs);
  pb.flags = SumOf(messH)->flags;
  pb.opts = SumOf(messH)->opts;
  pb.stream = stream;
  pb.tlMIME = tlMIME;
  pb.html = MessOptIsSet(messH, OPT_HTML);
  pb.rich = MessFlagIsSet(messH, FLAG_RICH);
  pb.allLWSP =
      !pb.hasAttachments && !pb.html && !pb.rich && IsAllLWSPMess(messH);
  pb.hasSig = !pb.allLWSP && (SumOf(messH)->sigId != -1) &&
              !MessOptIsSet(messH, OPT_INLINE_SIG) && eSignature &&
              *eSignature && GetHandleSize(eSignature);
  pb.strip = !pb.allLWSP && MessOptIsSet(messH, OPT_STRIP) ||
             MessOptIsSet(messH, OPT_JUST_EXCERPT);
  pb.bloat = !pb.allLWSP && MessOptIsSet(messH, OPT_BLOAT);
  if (pb.allLWSP)
    pb.flags &= ~FLAG_WRAP_OUT;

  Zero(pb.enriched);
  if (tlMIME && NewTLMIME(tlMIME))
    goto fail;

  PrimeProgress(messH);

  // Make sure we have all the attachments
  if (sErr = AllAttachOnBoard(messH))
    goto fail;

  CompHeadFind(messH, 0, &pb.hs);

  // Add back in the inline sig if removed...
  pb.hs.stop = PeteLen(TheBody);

  /*
   * rich?
   */
  if (pb.mime && !pb.strip && pb.html) {
    if (sErr = AccuInit(&pb.enriched))
      goto fail;
    g_strlcpy((char *)scratch, (const char *)SumOf(messH)->subj, sizeof(scratch));
    if (sErr = HTMLPreamble(&pb.enriched, scratch, 0,
                            False))
      goto fail;
    if (sErr = StackInit(sizeof(FSSpec), &pb.parts))
      goto fail;
    Zero(errSpec);
    if (sErr =
            BuildHTML(&pb.enriched, TheBody, NULL, pb.hs.stop, pb.hs.value, NULL,
                      NULL, 1, CompGetMID(messH, scratch), pb.parts, &errSpec)) {
      if (errSpec[0]) {
        //	Report error with graphic file
        AddOutgoingMesgError(messH->sumNum,
                             messH->tocH->sums[messH->sumNum].uidHash,
                             sErr, GRAPHIC_FILE_ERR, sErr, spec_name(errSpec));
      }
      sErr = 543;
      goto fail;
    }
    AccuTrim(&pb.enriched);
    pb.isRelated = pb.parts && pb.parts->elCount > 0;
  } else if (pb.mime && !pb.strip && pb.rich) {
    if (sErr = AccuInit(&pb.enriched))
      goto fail;
    if (BuildEnriched(&pb.enriched, TheBody, NULL, pb.hs.stop, pb.hs.value, NULL,
                      False))
      goto fail;
  }

  if (sendDataCmd) {
    sErr = SendCmdGetReply(stream, smtpData, NULL, True, messH);
    if (sErr && sErr / 100 != 3)
      goto fail;
  }

  if (pb.hasAttachments)
    sErr = TransmitMessageMixed(&pb, doTopLevel);
  else if (pb.isRelated)
    sErr = TransmitMessageRelated(&pb, doTopLevel);
  else
    sErr = TransmitMessageText(&pb, true, doTopLevel);

  if (sErr)
    goto fail;

  if (pb.mime)
    if (finishSMTP)
      sErr = FinishSMTP(stream, pb.messH);

done:
  if (pb.enriched.data) { free(pb.enriched.data); pb.enriched.data = NULL; };
  free(pb.parts);
  return (sErr / 100 != 2 ? sErr : (sErr = 0));

fail:
  if (IsAddrErr(sErr))
    SumOf(messH)->state = MESG_ERR;
  else
    sErr = 600;
  if (tlMIME)
    ZapTLMIME(*tlMIME);
  goto done;
}

/************************************************************************
 * AllAttachOnBoardLo - do we have all the attachments? No error reporting.
 ************************************************************************/
int AllAttachOnBoardLo(MessHandle messH, bool errReport) {
  short index;
  FSSpec spec;
  int err = noErr;

  for (index = 1; !err; index++) {
    if (err = GetIndAttachment(messH, index, &spec, NULL))
      if ((err != 1) && errReport) {
        AddOutgoingMesgError(messH->sumNum,
                             messH->tocH->sums[messH->sumNum].uidHash,
                             err, ATTACH_MESS_ERR, err, spec_name(spec));
        FileSystemError(BINHEX_OPEN, spec_name(spec), err);
      }
  }
  if (err == 1)
    err = noErr;
  else if (err)
    err = 543;
  return (err);
}

/************************************************************************
 * AllAttachOnBoard - do we have all the attachments?
 ************************************************************************/
int AllAttachOnBoard(MessHandle messH) {
  return AllAttachOnBoardLo(messH, true);
}

/************************************************************************
 * TransmitMessageMixed - transmit a multipart/mixed message
 ************************************************************************/
int TransmitMessageMixed(TransmitPBPtr pb, bool topLevel) {
  int err;
  char boundary[128];

  // build the boundary
  BuildBoundary(pb->messH, boundary, "");

  // mime-version first of all
  if (topLevel)
    if (err = TransmitMimeVersion(pb))
      return (err);

  // send the non-mime headers
  if (topLevel)
    if (err = TransmitTopHeaders(pb))
      return (err);

  // send the mime headers
  if (pb->mime &&
      (err = TransmitMultiHeaders(pb, MIME_MIXED, boundary, 0, NULL)))
    return (err);

  if (pb->mime) {
    // send the header/body separator
    if (err = SendPString(pb->stream, NewLine))
      return (err);

    // send the first boundary
    if (err = SendBoundary(pb->stream))
      return (err);

    // send the message itself
    if (pb->isRelated)
      err = TransmitMessageRelated(pb, false);
    else
      err = TransmitMessageText(pb, false, false);
    if (err)
      return (err);

    // send the attachments
    err = SendAttachments(pb->stream, pb->messH, pb->flags, boundary,
                          SumOf(pb->messH)->tableId,
                          pb->isRelated ? pb->parts->elCount : 0);
    if (err)
      return (err);

    // signature?
    if (pb->hasSig) {
      // attachment/signature boundary
      if (err = SendBoundary(pb->stream))
        return (err);
      if (err = TransmitMessageSig(pb))
        return (err);
    }

    // and the final boundary
    g_strlcat(boundary, "--", sizeof(boundary));
    if (err = SendBoundary(pb->stream))
      return (err);
  }

  return (noErr);
}

/************************************************************************
 * TransmitMessageRelated - transmit a multipart/related message
 ************************************************************************/
int TransmitMessageRelated(TransmitPBPtr pb, bool topLevel) {
  int err;
  char boundary[128];
  unsigned char textSlashHtml[32];

  // build the boundary
  BuildBoundary(pb->messH, boundary, "mr");

  // mime-version first of all
  if (topLevel)
    if (err = TransmitMimeVersion(pb))
      return (err);

  // send the non-mime headers
  if (topLevel)
    if (err = TransmitTopHeaders(pb))
      return (err);

  // send the mime headers
  if (pb->mime)
    if (err = TransmitMultiHeaders(
            pb, MIME_RELATED, boundary, AttributeStrn + aType,
            ComposeRString(textSlashHtml, THING_SLASH_THING, MIME_TEXT,
                           HTMLTagsStrn + htmlTag)))
      return (err);

  if (pb->mime) {
    // send the header/body separator
    if (err = SendPString(pb->stream, NewLine))
      return (err);

    // send the first boundary
    if (err = SendBoundary(pb->stream))
      return (err);

    // send the message itself
    err = TransmitMessageText(pb, !pb->hasAttachments, false);
    if (err)
      return (err);

    // send the parts
    err =
        SendRelatedParts(pb->stream, pb->messH, pb->flags, pb->parts, boundary);
    if (err)
      return (err);

    // and the final boundary
    g_strlcat(boundary, "--", sizeof(boundary));
    if (err = SendBoundary(pb->stream))
      return (err);
  }

  return (noErr);
}

/************************************************************************
 * TransmitMessageText - transmit body and sig, with or without bloat
 ************************************************************************/
int TransmitMessageText(TransmitPBPtr pb, bool sigToo, bool topLevel) {
  int err;

  if (pb->bloat && !pb->strip && (pb->html || pb->rich)) {
    // The user wants multipart/alternative
    if (err = TransmitMessageTextBloat(pb, sigToo, topLevel))
      return (err);
  } else if (!pb->strip && (pb->html || pb->rich)) {
    // The user wants the rich version
    if (err = TransmitMessageTextRich(pb, sigToo, topLevel))
      return (err);
  } else {
    // The user is chicken
    if (err = TransmitMessageTextStrip(pb, sigToo, topLevel))
      return (err);
  }
  return (noErr);
}

/************************************************************************
 * TransmitMessageTextBloat - transmit body and possibly sig with bloat
 ************************************************************************/
int TransmitMessageTextBloat(TransmitPBPtr pb, bool sigToo, bool topLevel) {
  int err;
  char boundary[128];

  // we'll need a boundary
  BuildBoundary(pb->messH, boundary, "ma");

  // mime-version first of all
  if (topLevel)
    if (err = TransmitMimeVersion(pb))
      return (err);

  // send the non-mime headers
  if (topLevel)
    if (err = TransmitTopHeaders(pb))
      return (err);

  // send the part headers, header/body separator, and initial boundary
  if (pb->mime)
    if (err = TransmitMultiHeaders(pb, MIME_ALTERNATIVE, boundary, 0, NULL))
      return (err);

  if (pb->mime) {
    // now the header/body separator and the initial boundary
    if (err = SendPString(pb->stream, NewLine))
      return (err);
    if (err = SendBoundary(pb->stream))
      return (err);

    // send the stripped version, not top-level
    if (err = TransmitMessageTextStrip(pb, sigToo, false))
      return (err);

    // now the mid boundary
    if (err = SendBoundary(pb->stream))
      return (err);

    // send the rich version, not top-level
    if (err = TransmitMessageTextRich(pb, sigToo, false))
      return (err);

    // and the final boundary
    g_strlcat(boundary, "--", sizeof(boundary));
    if (err = SendBoundary(pb->stream))
      return (err);
  }

  return (noErr);
}

/************************************************************************
 * TransmitMessageTextStrip - strip styles before sending
 ************************************************************************/
int TransmitMessageTextStrip(TransmitPBPtr pb, bool sigToo, bool topLevel) {
  bool oldFlat = Flatten != NULL;
  int err;

  if ((pb->opts & OPT_BLOAT) || MessOptIsSet(pb->messH, FLAG_WRAP_OUT))
    pb->flags |= FLAG_WRAP_OUT; // force wrapping on plain part of m/a
  if ((pb->opts && OPT_BLOAT) && !Flatten)
    Flatten = GetFlatten(); // force flattening
  ConvertExcerpt(pb->messH->bodyPTE, pb->hs.value, 0x7fffffff, NULL,
                 NULL); // and convert the excerpts
  pb->hs.stop = PETEGetTextLen(PETE, pb->messH->bodyPTE);
  PeteCleanList(pb->messH->bodyPTE);
  pb->messH->win->isDirty = false;

  // send it
  if (err = TransmitMessageTextPlain(pb, sigToo, topLevel))
    return (err);

  // put flatten back
  if (!oldFlat)
    ZapPtr(Flatten);
  return (err);
}

/************************************************************************
 * TransmitMessageTextPlain - send plaintext version of message & sig
 ************************************************************************/
int TransmitMessageTextPlain(TransmitPBPtr pb, bool sigToo, bool topLevel) {
  int err;
  // save off some stuff
  bool oldStrip = pb->strip; // save the old strip value
  long oldOpts = pb->opts;
  long oldFlags = pb->flags;

  // we be strippin'
  pb->strip = true;
  pb->opts |= OPT_STRIP;

  // mime-version first of all
  if (topLevel)
    if (err = TransmitMimeVersion(pb))
      return (err);

  // send the non-mime headers
  if (topLevel)
    if (err = TransmitTopHeaders(pb))
      return (err);

  // send the mime headers
  if (pb->mime && !pb->receipt)
    if (err = TransmitMessageBodyHeaders(pb, sigToo, topLevel))
      return (err);

  if (pb->mime) {
    // send the header/body separator
    if (!pb->receipt)
      if (err = SendPString(pb->stream, NewLine))
        return (err);

    // send the message itself
    err = TransmitMessageBody(pb, false);
    if (err)
      return (err);

    // signature?  Do not generate headers.
    if (sigToo && pb->hasSig)
      if (err = TransmitMessageSigBody(pb, false))
        return (err);
  }

  // put stuff back
  pb->strip = oldStrip;
  pb->opts = oldOpts;
  pb->flags = oldFlags;

  return (noErr);
}

/************************************************************************
 * TransmitMessageTextRich - send rich version of message & sig
 ************************************************************************/
int TransmitMessageTextRich(TransmitPBPtr pb, bool sigToo, bool topLevel) {
  int err;

  // we don't wrap rich text
  pb->flags &= ~FLAG_WRAP_OUT;

  // mime-version first of all
  if (topLevel)
    if (err = TransmitMimeVersion(pb))
      return (err);

  // send the non-mime headers
  if (topLevel)
    if (err = TransmitTopHeaders(pb))
      return (err);

  // send the mime headers
  if (pb->mime)
    if (err = TransmitMessageBodyHeaders(pb, sigToo, topLevel))
      return (err);

  if (pb->mime) {
    // send the header/body separator
    if (err = SendPString(pb->stream, NewLine))
      return (err);

    // send the message itself; close out html if no signature
    err = TransmitMessageBody(pb, !pb->hasSig);
    if (err)
      return (err);

    // signature?  Do not generate headers & preamble
    if (sigToo && pb->hasSig)
      if (err = TransmitMessageSigBody(pb, false))
        return (err);
  }
  return (noErr);
}

/************************************************************************
 * TransmitMimeVersion - transmit the silly mime-version header
 ************************************************************************/
int TransmitMimeVersion(TransmitPBPtr pb) {
  int err;

  if (err =
          ComposeRTrans(pb->stream, MIME_V_FMT, InterestHeadStrn + hMimeVersion,
                        MIME_VERSION, NewLine))
    return (err);

  return (noErr);
}

/************************************************************************
 * TransmitTopHeaders - transmit the non-MIME headers at the top of the message
 ************************************************************************/
int TransmitTopHeaders(TransmitPBPtr pb) {
  int sErr = noErr;
  unsigned char buffer[256];
  unsigned char scratch[256];
  short header;
  short tid = EffectiveTID(SumOf(pb->messH)->tableId);
  short priority;
  Accumulator newsGroupAcc;

  if (!pb->others)
    return (noErr);

  /*
   * The dreaded X-Sender:
   */
  if (sErr = SendXSender(pb->stream, pb->messH))
    return (sErr);

  /*
   * extra headers saved with message
   */
  BufferSendRelease(pb->stream);
  if (pb->messH->extras.data)
    SendExtras(pb->stream, pb->messH->extras.data,
               (pb->flags & FLAG_CAN_ENC) != 0, tid);
  BSCLOSE(pb->stream, 0);

  /*
   * newsgroups
   */
  newsGroupAcc = pb->messH->newsGroupAcc;
  if (newsGroupAcc.offset) {
    BufferSendRelease(pb->stream);
    SendNewsGroups(pb->stream, &newsGroupAcc, tid);
    BSCLOSE(pb->stream, 0);
  }

  /*
   * Return Receipts
   */
  if (MessFlagIsSet(pb->messH, FLAG_RR) &&
      !MessOptIsSet(pb->messH, OPT_RECEIPT)) {
    UseFeature(featureReturnReceiptTo);
    if (PrefIsSet(PREF_RRT)) {
      if (sErr = ComposeRTrans(pb->stream, RRT_FMT,
                               GetReturnAddr(scratch, true), NewLine))
        return (sErr);
    }
    if (sErr = ComposeRTrans(pb->stream, MIME_P_FMT, InterestHeadStrn + hMDN,
                             GetReturnAddr(scratch, true), NewLine))
      return (sErr);
  }

  /*
   * Bulk?
   */
  if (MessOptIsSet(pb->messH, OPT_BULK))
    if (sErr = ComposeRTrans(pb->stream, IMPORTANCE_FMT,
                             TOCHeaderStrn + tchPrecedence, BULK, NewLine))
      return (sErr);

  /*
   * extra static headers
   */
  if (GetResource_('STR#', EX_HEADERS_STRN)) {
    for (header = 1;; header++) {
      if (*GetRString(buffer, EX_HEADERS_STRN + header)) {
        if (sErr = SendTrans(pb->stream, buffer, strlen((const char *)buffer), NewLine, strlen((const char *)NewLine), NULL))
          return (sErr);
      } else
        break;
    }
  }

  /*
   * Registration headers (commercial Eudora only - not in open-source GTK port)
   */
#ifdef HAVE_REGISTRATION
  if (MessOptIsSet(pb->messH, OPT_SEND_REGINFO)) {
    UserStateType state = GetNagState();

    BufferSendRelease(pb->stream);

    GetRegFirst(state, scratch);
    GetRString(buffer, RegCodeHeadStrn + hRegFirst);
    strcat((char *)buffer, ":");
    SendPtrHead(pb->stream, buffer, strlen((const char *)buffer), scratch, strlen((const char *)scratch),
                (pb->flags & FLAG_CAN_ENC) != 0, tid);

    GetRegLast(state, scratch);
    GetRString(buffer, RegCodeHeadStrn + hRegLast);
    strcat((char *)buffer, ":");
    SendPtrHead(pb->stream, buffer, strlen((const char *)buffer), scratch, strlen((const char *)scratch),
                (pb->flags & FLAG_CAN_ENC) != 0, tid);

    GetRegCode(state, scratch);
    GetRString(buffer, RegCodeHeadStrn + hRegCode);
    strcat((char *)buffer, ":");
    SendPtrHead(pb->stream, buffer, strlen((const char *)buffer), scratch, strlen((const char *)scratch),
                (pb->flags & FLAG_CAN_ENC) != 0, tid);

    BSCLOSE(pb->stream, 0);
  }
#endif /* HAVE_REGISTRATION */

  /*
   * real headers
   */
  // priority
  priority = SumOf(pb->messH)->priority;
  priority = Prior2Display(priority);
  if (priority != 3) {
    if (!PrefIsSet(PREF_SUP_PRIORITY)) {
      PriorityHeader(buffer, priority);
      if (sErr = SendTrans(pb->stream, buffer, strlen((const char *)buffer), NewLine, strlen((const char *)NewLine), NULL))
        return (sErr);
    }

    if (PrefIsSet(PREF_GEN_IMPORTANCE)) {
      ComposeRString(buffer, IMPORTANCE_FMT, TOCHeaderStrn + tchImportance,
                     ImportanceOutStrn + priority, NewLine);
      if (SendPString(pb->stream, buffer))
        return (600);
    }
  }

  // date
  BuildDateHeader(buffer, SumOf(pb->messH)->seconds);
  if (*buffer &&
      SendTrans(pb->stream, buffer, strlen((const char *)buffer), NewLine, strlen((const char *)NewLine), NULL))
    return (600);

  BufferSendRelease(pb->stream);

  // finally what the user thinks of as the headers...
  for (header = 1; header < BODY_HEAD; header++)
    if (header != ATTACH_HEAD &&
        (PrefIsSet(PREF_POP_SEND) || header != BCC_HEAD))
      if (sErr = SendHeaderLine(pb->stream, pb->messH, header,
                                (pb->flags & FLAG_CAN_ENC) != 0, tid))
        return (sErr);
  BSCLOSE(pb->stream, 0);

done:
  return (sErr / 100 != 2 ? sErr : 0);
}

/************************************************************************
 * TransmitMultiHeaders - transmit the headers for multipart/something
 ************************************************************************/
int TransmitMultiHeaders(TransmitPBPtr pb, short subType, char * boundary,
                           short otherParam, char * otherVal) {
  int err;
  unsigned char scratch[256];
  MessHandle messH = pb->messH;
  void *headerContent = NULL;

  // The multipart header
  if (err =
          ComposeRTrans(pb->stream, MIME_MP_FMT,
                        InterestHeadStrn + hContentType, MIME_MULTIPART,
                        subType, AttributeStrn + aBoundary, boundary, NewLine))
    return (err);
  if (otherParam && (err = ComposeRTrans(pb->stream, MIME_CT_ANNOTATE,
                                         otherParam, otherVal, NewLine)))
    return (err);

  if (!GetRHeaderAnywhere(messH, PLUGIN_INFO, &headerContent)) {
    { size_t _mpl = (GetHandleSize(headerContent) - 2); memcpy(scratch, headerContent + 2, _mpl); ((char*)(scratch))[_mpl] = '\0'; } // 2 adjusts for colon-space
    free(headerContent);
    if (err = ComposeRTrans(pb->stream, MIME_CT_ANNOTATE, PLUGIN_INFO, scratch,
                            NewLine))
      return err;
  }

  // Let the translators know
  if (pb->tlMIME) {
    AddTLMIME(*pb->tlMIME, TLMIME_TYPE, GetRString(scratch, MIME_MULTIPART),
              NULL);
    AddTLMIME(*pb->tlMIME, TLMIME_SUBTYPE, GetRString(scratch, subType), NULL);
    AddTLMIME(*pb->tlMIME, TLMIME_PARAM,
              GetRString(scratch, AttributeStrn + aBoundary), boundary);
  }

  return (noErr);
}

/************************************************************************
 * TransmitMessageBodyHeaders - send the header for the body
 ************************************************************************/
int TransmitMessageBodyHeaders(TransmitPBPtr pb, bool withSignature,
                                 bool topLevel) {
  MessHandle messH = pb->messH; // keep macros happy
  void *text;
  void *sig;
  int err;

  if (pb->strip) {
    PETEGetRawText(PETE, TheBody, &text);
    sig = withSignature ? eSignature : NULL;
    { long _tlen = PETEGetTextLen(PETE, TheBody);
      long _slen = sig ? (long)GetHandleSize_((void *)sig) : 0;
      err = SendContentType(pb->stream, (char *)text, _tlen, BodyOffset((char *)text),
                            sig ? (char *)sig : NULL, _slen, 0,
                            SumOf(messH)->tableId, &pb->flags, &pb->opts, NULL,
                            topLevel ? pb->tlMIME : NULL, NULL); }
  } else {
    sig = withSignature ? eSignature : NULL;
    { long _slen = sig ? (long)GetHandleSize_((void *)sig) : 0;
      err = SendContentType(pb->stream, pb->enriched.data, pb->enriched.offset, 0,
                            sig ? (char *)sig : NULL, _slen, 0,
                            SumOf(messH)->tableId, &pb->flags, &pb->opts, NULL,
                            topLevel ? pb->tlMIME : NULL, NULL); }
  }
  pb->encoder =
      (pb->receipt || 0 == (pb->flags & FLAG_ENCBOD)) ? NULL : QPEncoder;
  return (noErr);
}

/************************************************************************
 * TransmitMessageBody - send the message's body
 ************************************************************************/
int TransmitMessageBody(TransmitPBPtr pb, bool withClosure) {
  int sErr = noErr;
  void *body;

  if (pb->strip) {
    // send the plain text
    PETEGetRawText(PETE, pb->messH->bodyPTE, &body);
    sErr = SendBodyLines(pb->stream, (char *)body, pb->hs.stop, pb->hs.value, pb->flags,
                         True, NULL, 0, False, pb->encoder);
  } else {
    if (pb->html) {
      // gotta close out the html?
      if (withClosure)
        if (sErr = HTMLPostamble(&pb->enriched, False))
          return (sErr);

      // send the html
      sErr = SendBodyLines(pb->stream, pb->enriched.data, pb->enriched.offset,
                           0, pb->flags, True, NULL, 0, False, pb->encoder);
    } else if (pb->rich) {
      // send the enriched
      sErr = SendEnriched(pb->stream, pb->enriched.data, pb->enriched.offset, pb->encoder);
    }

    // done with that...
    pb->enriched.offset = 0;
    AccuTrim(&pb->enriched);
  }

  BSCLOSE(pb->stream, pb->encoder);
  if (withClosure)
    pb->encoder = NULL;

done:
  return (sErr);
}

/************************************************************************
 * TransmitMessageSig - transmit the sig as its own part, possibly bloated
 ************************************************************************/
int TransmitMessageSig(TransmitPBPtr pb) {
  int err;

  if (pb->bloat && SigStyled && !pb->strip && (pb->rich || pb->html))
    err = TransmitMessageSigBloat(pb);
  else if (SigStyled && !pb->strip)
    err = TransmitMessageSigBody(pb, true);
  else
    err = TransmitMessageSigBodyPlain(pb);
  return (err);
}

/************************************************************************
 * TransmitMessageSigBodyPlain - send the sig, forcing to plain
 ************************************************************************/
int TransmitMessageSigBodyPlain(TransmitPBPtr pb) {
  int err;
  // save off some stuff
  bool oldStrip = pb->strip; // save the old strip value
  long oldOpts = pb->opts;
  long oldFlags = pb->flags;
  bool oldSigStyled = SigStyled;

  // we be strippin'
  pb->strip = true;
  pb->opts |= OPT_STRIP;
  SigStyled = false;

  // Send the darn thing
  err = TransmitMessageSigBody(pb, true);

  // put stuff back
  pb->strip = oldStrip;
  pb->opts = oldOpts;
  pb->flags = oldFlags;
  SigStyled = oldSigStyled;
  return (err);
}

/************************************************************************
 * TransmitMessageSigBloat - Send the message's signature, possibly with m/a
 ************************************************************************/
int TransmitMessageSigBloat(TransmitPBPtr pb) {
  int err;
  char boundary[128];
  bool oldFlat = Flatten != NULL;

  // we'll need a boundary
  BuildBoundary(pb->messH, boundary, "ma");

  // send the part headers, header/body separator, and initial boundary
  if (err = TransmitMultiHeaders(pb, MIME_ALTERNATIVE, boundary, 0, NULL))
    return (err);

  // now the header/body separator and the initial boundary
  if (err = SendPString(pb->stream, NewLine))
    return (err);
  if (err = SendBoundary(pb->stream))
    return (err);

  // send the plain version
  pb->flags |= FLAG_WRAP_OUT; // force wrapping on plain part
  if (!Flatten)
    Flatten = GetFlatten(); // force flattening
  if (err = TransmitMessageSigBodyPlain(pb))
    return (err);
  if (!oldFlat)
    ZapPtr(Flatten);

  // now the mid boundary
  if (err = SendBoundary(pb->stream))
    return (err);

  // send the rich version
  if (err = TransmitMessageSigBody(pb, true))
    return (err);

  // and the final boundary
  g_strlcat(boundary, "--", sizeof(boundary));
  if (err = SendBoundary(pb->stream))
    return (err);

  return (noErr);
}

/************************************************************************
 * TransmitMessageSigBody - Send the message's signature
 ************************************************************************/
int TransmitMessageSigBody(TransmitPBPtr pb, bool withHeaders) {
  void *sigSrc = NULL;
  char *sigBuf = NULL;
  long sigLen = 0;
  unsigned char scratch[64];

  // which sig do we want?
  if (!SigStyled && withHeaders || pb->strip || !(pb->rich || pb->html))
    sigSrc = eSignature;
  else if (pb->html)
    sigSrc = HTMLSignature;
  else if (pb->rich)
    sigSrc = RichSignature;

  // copy the sig into a flat buffer
  if (sigSrc) {
    sigLen = (long)GetHandleSize_((void *)sigSrc);
    sigBuf = malloc(sigLen);
    if (!sigBuf) { sErr = memFullErr; goto done; }
    memcpy(sigBuf, sigSrc, sigLen);
  }

  // send headers if we must
  if (withHeaders) {
    if (sErr = SendContentType(pb->stream, sigBuf, sigLen, 0, NULL, 0, 0,
                               SumOf(pb->messH)->tableId, &pb->flags, &pb->opts,
                               NULL, NULL, NULL))
      goto done;
    pb->encoder =
        (pb->receipt || 0 == (pb->flags & FLAG_ENCBOD)) ? NULL : QPEncoder;

    // header/body separator
    if (withHeaders)
      if (sErr = SendPString(pb->stream, NewLine))
        goto done;

    // preamble if html
    if (!pb->strip && pb->html) {
      // generate the preamble
      free(pb->enriched.data); pb->enriched.data = NULL; pb->enriched.offset = pb->enriched.size = 0;
      if (sErr = AccuInit(&pb->enriched))
        goto done;
      sErr =
          HTMLPreamble(&pb->enriched, GetRString(scratch, SIGNATURE), 0, False);

      // add the signature to it
      if (!sErr && sigBuf)
        sErr = AccuAddPtr(&pb->enriched, sigBuf, sigLen);
      if (sErr)
        goto done;
      AccuTrim(&pb->enriched);

      // replace sigBuf with the enriched data
      free(sigBuf);
      sigLen = pb->enriched.offset;
      sigBuf = malloc(sigLen);
      if (!sigBuf) { sErr = memFullErr; goto done; }
      memcpy(sigBuf, pb->enriched.data, sigLen);
      free(pb->enriched.data); pb->enriched.data = NULL; pb->enriched.offset = pb->enriched.size = 0;
    }
  }

  // and finally, send it
  if (sErr = SendBodyLines(pb->stream, sigBuf, sigLen, 0,
                           pb->flags, True, NULL, 0, False, pb->encoder))
    goto done;
  BSCLOSE(pb->stream, pb->encoder);
  pb->encoder = NULL;

done:
  free(sigBuf);
  return (sErr);
}

/**********************************************************************
 * SendEnriched - send a block of text as text/enriched
 **********************************************************************/
int SendEnriched(TransStream stream, char *text, long textLen, DecoderFunc *encoder) {
  char *start, *space, *spot, *end;
  long soft = GetRLong(ENRICHED_SOFT_LINE);
  bool wasNl = True;
  long lastLen, lastC;
  short nofill = 0;
  unsigned char dir[32];

  end = text + textLen;

  for (start = text; start < end; start = spot + 1) {
    for (space = spot = start; spot < end; spot++) {
      if (*spot == '\015')
        break;
      if (*spot == ' ')
        space = spot;
      if (!nofill && space > start && spot - start > soft)
        break;
    }

    if (!nofill && spot - start > soft && space > start)
      spot = space;
    if (!UUPCOut && *start == '.')
      BS(stream, encoder, ".", 1);
    BS(stream, encoder, start, spot - start);

    if (*start == '<' && spot[-1] == '>') {
      if (start[1] == '/')
        { size_t _mpl = (spot - start - 3); memcpy(dir, start + 2, _mpl); ((char*)(dir))[_mpl] = '\0'; }
      else
        { size_t _mpl = (spot - start - 2); memcpy(dir, start + 1, _mpl); ((char*)(dir))[_mpl] = '\0'; }

      if (EqualStrRes(dir, EnrichedStrn + enNoFill)) {
        if (start[1] == '/')
          nofill = MAX(nofill - 1, 0);
        else
          nofill++;
      }
    }
    BS(stream, encoder, NewLine, strlen((const char *)NewLine));
    if (space > start && spot == space && spot < end - 1 && spot[1] == '\015')
      spot++; // we already sent a newline, don't want to send another.
  }

done:
  return (sErr);
}

/**********************************************************************
 * SMTPFinish - close out the SMTP session
 **********************************************************************/
int FinishSMTP(TransStream stream, MessHandle messH) {
  unsigned char buffer[256];

  if (!UUPCOut) {
    ComposeString(buffer, ".%p", NewLine);
    if (sErr = SendTrans(stream, buffer, strlen((const char *)buffer), NULL))
      return (600);
  }

  if (!UUPCOut) {
    sErr = GetReply(stream, buffer, sizeof(buffer), False, False);
    if (sErr > 399)
      SMTPCmdError(smtpData, "", buffer);
    if (IsAddrErr(sErr)) {
      if (*buffer)
        buffer[strlen(buffer) - 1] = 0;
      AddOutgoingMesgError(messH->sumNum,
                           messH->tocH->sums[messH->sumNum].uidHash,
                           sErr, BAD_XMIT_ERR_TEXT, "", buffer);
    }
  }

  return (sErr / 100 != 2 ? sErr : (sErr = 0));
}

/************************************************************************
 * SendHeaderLine - send a line of header information to sendmail
 ************************************************************************/
int SendHeaderLine(TransStream stream, MessHandle messH, short header,
                   bool allowQP, short tid) {
  unsigned char note[64];
  unsigned char label[64];
  HeadSpec hs;

  if (!CompHeadFind(messH, header, &hs)) {
    if (header != TO_HEAD)
      return (noErr);
    else
      // something wrong.  We didn't find the header we should have.  This
      // message is toast
      return fnfErr;
  }

  if (hs.stop == hs.value) {
    if (header == TO_HEAD && *GetRString(note, BCC_ONLY)) {
      GetRString(label, HEADER_STRN + TO_HEAD);
      return (sErr = SendPtrHead(stream, label, strlen((const char *)label), note, strlen((const char *)note),
                                 allowQP, tid));
    } else
      return (noErr);
  }

  /*
   * is it an address header?
   */
  else if (IsAddressHead(header))
    sErr = SendAddressHead(stream, TheBody, &hs, allowQP, tid);
  else
    sErr = SendNormalHead(stream, TheBody, &hs, allowQP, tid);

done:
  return (sErr);
}

/************************************************************************
 * SendAddressHead - send an address header
 ************************************************************************/
int SendAddressHead(TransStream stream, PETEHandle pte, HSPtr hs,
                      bool allowQP, short tid) {
  char *start;
  int lineLimit = GetRLong(WRAP_SPOT) - 2;
  char *fix = NULL;
  long fixLen = 0;
  bool high, wasHigh;
  bool first = True;
  short lastLen, lastC;
  char **addresses = NULL;
  char **rawAddresses = NULL;
  short inGroup = 0;
  void *text;
  short charsOnLine = 0;
  int err;
  bool popSend = PrefIsSet(PREF_POP_SEND);
  bool wasGroup = False;
  unsigned char dontHide[32];

  PETEGetRawText(PETE, pte, &text);
  GetRString(dontHide, GROUP_DONT_HIDE);
  { char *tp = (char *)text;

  /*
   * start by sending the label
   */
  BS(stream, NULL, tp + hs->start, hs->value - hs->start - 1);
  charsOnLine = hs->value - hs->start;

  SuckPtrAddresses(&rawAddresses, (const char *)tp + hs->value, hs->stop - hs->value, True,
                   True, False, NULL);
  if (rawAddresses && rawAddresses[0] && rawAddresses[0][0]) {
    err = ExpandAliases((void **)&addresses, (void *)rawAddresses, 0, True);
    g_strfreev(rawAddresses); rawAddresses = NULL;
    if (err)
      return (err);

    /*
     * now we have the fully-expanded address list
     */
    if (addresses) {
      for (int _ai = 0; addresses[_ai]; _ai++) { start = (char *)addresses[_ai];
        // Folder Carbon Copy - do no support FCC in Light
        if (HasFeature(featureFcc)) {
          if (IsFCCAddr(start))
            continue;
          if (IsNewsgroupAddr(start))
            continue;
        }
        if (inGroup && start[0] == ';')
          inGroup--;
        if (popSend || !inGroup) {
          if (Flatten)
            TransLit(start, strlen((const char *)start), Flatten);
          high = allowQP && AnyHighBits(start, strlen((const char *)start));

          /*
           * put out a return if we need to start a new line, or a comma if we
           * just need a separator.  If it's the very first address, it goes on
           * the first line, period.  If it's the trailing semicolon, it goes on
           * this line, period.
           */
          if (first || strlen((const char *)start) == 1 && start[0] == ';')
            first = False;
          else {
            if (!wasGroup && start[strlen((const char *)start) - 1] != ';') {
              BS(stream, NULL, ",", 1);
              charsOnLine++;
            } /* send the comma separator */
            if ((charsOnLine &&
                 (long)strlen((const char *)start) + charsOnLine > lineLimit) || /* overflow */
                (high || wasHigh)) /* 1522 stuff gets own line always */
            {
              BS(stream, NULL, NewLine, strlen((const char *)NewLine));
              charsOnLine = 0;
            }
          }

          /*
           * space
           */
          if (start[strlen((const char *)start) - 1] != ';') {
            BS(stream, NULL, " ", 1);
            charsOnLine++;
          }

          /*
           * hey!  we can finally send this!
           */
          wasHigh = high; /* make sure 1522 stuff gets own line next time */

          /*
           * need to use RFC 1522
           */
          if (high) {
            fix = Encode1342(start, strlen((const char *)start), lineLimit, &charsOnLine,
                             NewLine, tid, &fixLen);
            if (fix)
              BS(stream, NULL, fix, fixLen);
          }

          /*
           * RFC 1522 not used
           */
          if (!fix) {
            { size_t _slen = strlen((const char *)start);
              BS(stream, NULL, start, _slen);
              charsOnLine += _slen; }
          }
          free(fix); fix = NULL; fixLen = 0;
        }
        if (wasGroup = start[strlen((const char *)start) - 1] == ':')
          if (!PFindSub(dontHide, start))
            inGroup++;
      }

      /*
       * finish off header with newline
       */
      BS(stream, NULL, NewLine, strlen((const char *)NewLine));
    }
    g_strfreev(addresses); addresses = NULL;
  } else
    { g_strfreev(rawAddresses); rawAddresses = NULL; }

done:
  } /* end tp block */
  free(fix);
  return (sErr);
}

/************************************************************************
 * SendNormalHead - send a normal header
 ************************************************************************/
int SendNormalHead(TransStream stream, PETEHandle pte, HSPtr hs, bool allowQP,
                     short tid) {
  void *text;
  unsigned char kiran[16];

  if (hs->index == SUBJ_HEAD && *GetRString(kiran, JUST_FOR_KIRAN))
    return (SendSubjectHead(stream, pte, hs, allowQP, tid));

  PETEGetRawText(PETE, pte, &text);

  sErr = SendPtrHead(stream, (char *)text + hs->start, hs->value - hs->start - 1,
                     (char *)text + hs->value, hs->stop - hs->value, allowQP, tid);

  return (sErr);
}

/************************************************************************
 * SendSubjectHead - send the subject header
 ************************************************************************/
int SendSubjectHead(TransStream stream, PETEHandle pte, HSPtr hs,
                      bool allowQP, short tid) {
  void *text;
  unsigned char kiran[16];
  long offset;
  long len;
  long stop;
  long colon;
  long k;

  GetRString(kiran, JUST_FOR_KIRAN);

  PETEGetRawText(PETE, pte, &text);

  { char *tp = (char *)text;
  len = hs->stop;
  offset = hs->start;

  do {
    // find delimitter
    k = SearchStrPtr(kiran, tp, offset, len, true, false, NULL);
    // if not found, pretend at end
    stop = k < 0 ? len : k;

    if (k >= 0)
      tp[k] = '\015'; // replace first delim with newline 'cuz sendptrhead
                      // expects it

    // find colon
    colon = SearchPtrPtr(":", 1, tp, offset, stop, true, false, NULL);
    if (colon < 0)
      sErr = SendPtrHead(stream, " ", 1, tp + offset, stop - offset, allowQP,
                         tid);
    else
      sErr = SendPtrHead(stream, tp + offset, colon - offset + 1,
                         tp + colon + 2, stop - colon - 2, allowQP, tid);
    // skip delimitter
    offset = stop + strlen((const char *)kiran);

    if (k >= 0)
      tp[k] = kiran[0]; // put char back.

  } while (stop < len && !sErr); }


  return (sErr);
}

/************************************************************************
 * SendPtrHead - send a normal header with label and text
 ************************************************************************/
int SendPtrHead(TransStream stream, char * label, long labelLen, char * body,
                  long bodyLen, bool allowQP, short tid) {
  char *start, *stop, *end, *space, *limit;
  int lineLimit = GetRLong(WRAP_SPOT) - 2;
  char *fix = NULL;
  long fixLen = 0;
  bool first = True;
  short lastLen, lastC;
  short charsOnLine;

  /*
   * start by sending the label
   */
  BS(stream, NULL, label, labelLen);
  charsOnLine = labelLen;

  /*
   * send it, a line at a time
   * prepend a ' ' to each line, ala RFC 822
   */
  start = body;
  stop = start + bodyLen;

  // trim trailing spaces
  if (stop > start && IsWhite(stop[-1])) {
    while (stop > start && IsWhite(stop[-1]))
      --stop;
    bodyLen = stop - start;
    start[bodyLen] = '\015'; // pretend we have newline there
  }

  if (Flatten)
    TransLit(
        start, bodyLen,
        Flatten); /* yes, tromps on in-memory copy;
                                                                                                                                                   doesn't matter, will get pitched at close anyway */
  if (allowQP && AnyHighBits(start, stop - start)) {
    if (fix = Encode1342(start, stop - start, lineLimit, NULL, NewLine, tid, &fixLen)) {
      BS(stream, NULL, " ", 1);
      BS(stream, NULL, fix, fixLen);
      start = stop;
    }
  }
  for (; start < stop; start = end + 1) {
    bool shortLine;
  restart:
    limit = start + lineLimit - charsOnLine;
    if ((shortLine = stop < limit))
      limit = stop;
    for (space = end = start; end < limit && *end != '\015'; end++)
      if (IsSpace(*end))
        space = end;
    if (!shortLine && space == start && charsOnLine && *end != '\015') {
      BS(stream, NULL, NewLine, strlen((const char *)NewLine));
      charsOnLine = 0; /* charsOnLine used only for header label */
      goto restart;
    }
    charsOnLine = 0; /* charsOnLine used only for header label */
    if (space > start && end >= limit && limit < stop)
      end = space;
    ByteProgress(NULL, -1, 0);
    BS(stream, NULL, " ", 1);
    BS(stream, NULL, start, end - start);
    if (end < stop) {
      BS(stream, NULL, NewLine, strlen((const char *)NewLine));
      if (!IsSpace(*end))
        end--;
    }
  }
  BS(stream, NULL, NewLine, strlen((const char *)NewLine));
done:
  free(fix);
  return (sErr);
}

/************************************************************************
 * SendExtras - send extra headers
 ************************************************************************/
int SendExtras(TransStream stream, void *extras, bool allowQP, short tid) {
  char *start, *stop, *limit;
  char *labelStart, *labelStop;
  unsigned char label[32];
  unsigned char uglyStupidHackForWindowsIMAP[32];

  GetRString(uglyStupidHackForWindowsIMAP, PLUGIN_INFO);

  start = (unsigned char *)extras;
  limit = start + GetHandleSize_(extras);

  for (; start < limit; start = stop + 1) {
    labelStart = start;
    if (*start == ' ' || *start == 9) {
      labelStop = labelStart;
    } else {
      /*
       * find the colon
       */
      for (stop = start; stop < limit && *stop != ':'; stop++)
        ;
      if (stop == limit)
        break;
      labelStop = stop + 1;
      if (labelStop - labelStart > 2)
        { size_t _mpl = (labelStop - labelStart - 1); memcpy(label, labelStart, _mpl); ((char*)(label))[_mpl] = '\0'; }
      else
        *label = 0;

      /*
       * find the newline
       */
      start = stop + 1;
      if (*start == ' ')
        start++;
    }
    for (stop = start; stop < limit && *stop != '\015'; stop++)
      ;

    /*
     * send the header
     */
    if (!StringSame(label, uglyStupidHackForWindowsIMAP))
      if (sErr = SendPtrHead(stream, labelStart, labelStop - labelStart, start,
                             stop - start, allowQP, tid))
        break;
  }
  return (sErr);
}

/************************************************************************
 * SendNewsGroups - send the NewsGroups header
 ************************************************************************/
int SendNewsGroups(TransStream stream, AccuPtr newsGroupAcc, short tid) {
  unsigned char headerName[64];

  AccuAddChar(newsGroupAcc, '\015');
  AccuTrim(newsGroupAcc);
  GetRString(headerName, NEWSGROUPS);
  return (sErr = SendPtrHead(stream, headerName, strlen((const char *)headerName),
                             newsGroupAcc->data,
                             newsGroupAcc->offset - 1, false, tid));
}

/************************************************************************
 * SendXSender - construct and send the X-Sender header
 ************************************************************************/
short SendXSender(TransStream stream, MessHandle messH) {
  char **popCanon = NULL, **returnCanon = NULL;
  unsigned char buffer[256];
  unsigned char from[256];
  short err = 0;

  if (CurPers->popSecure) {
    CompHeadGetStr(messH, FROM_HEAD, from);
    /* figure out what the return addr means */
    SuckPtrAddresses(&returnCanon, (const char *)from, strlen((const char *)from), False, True, False, NULL);

    /* grab the POP account, and figure out what it means */
    GetPOPPref(buffer);
    SuckPtrAddresses(&popCanon, (const char *)buffer, strlen((const char *)buffer), False, True, False, NULL);
  }

  /* if different or no password, send Sender field */
  if (!UUPCOut && (!CurPers->popSecure || !popCanon || !returnCanon ||
                   !popCanon[0] || !returnCanon[0] ||
                   !StringSame(popCanon[0], returnCanon[0]))) {
    if (!CurPers->popSecure)
      GetRString(buffer, UNVERIFIED);
    else
      *buffer = 0;

    err = ComposeRTrans(stream, XSENDER_FMT, UUPCIn ? 0 : GetPOPPref(from),
                        buffer, NewLine);
  }

  g_strfreev(popCanon); popCanon = NULL;
  g_strfreev(returnCanon); returnCanon = NULL;
  return (err);
}

/************************************************************************
 * SendBodyLines - send the actual body of the message
 *	Don't look at this; it's a mess.
 *	text				void *to the text to send
 *	length			length of same
 *  offset			offset at which to begin
 *	flags				message flags
 *	forceLines	should I force a newline at the end of the text?
 *	lineStarts	pointer to array for determining wrap (may be NULL)
 *	nLines			length of same
 *	partial			should I listen to the partial information?
 ************************************************************************/
int SendBodyLines(TransStream stream, char *text, long length, long offset,
                  long flags, bool forceLines, short *lineStarts, short nLines,
                  bool partial, DecoderFunc *encoder) {
  char *start; /* the beginning of the text left to be sent */
  char *stop;  /* the end of the entire text block */
  char *end;   /* one past the last character of a line of text to be
                           sent   for complete lines, this will be a return */
  char *space; /* the last space before end */
  int lineLimit;        /* # of chars at which to wrap */
  int hardLimit; /* limit for hard returns; don't wrap if para < hardLimit */
  static short quoteLevel; /* the # of quote chars at start of line */
  Byte suspendChar;        /* the quote character */
  static short
      partialSize;         /* the size of the last line output, if it
                                                                                                            was an incomplete line */
  static bool softNewline; /* was the last newline added by us? */
  unsigned char scratch[32];
  short i;
  char *nl;
  bool doWrap = 0 != (flags & FLAG_WRAP_OUT);
  short lastC, lastLen;
  short maxQuote = GetRLong(MAX_QUOTE);
  short curLen;
  short tab = GetRLong(TAB_DISTANCE);
  bool flowed = doWrap && UseFlowOut;
  bool withSpace;
  int smtpMaxLen = GetRLong(MAX_SMTP_LINE);
  unsigned char spaceNewLine[16];

  if (flowed) {
    *spaceNewLine = 0;
    PCatC(spaceNewLine, ' ');
    g_strlcat(spaceNewLine, NewLine, sizeof(spaceNewLine));
  }

  if (!partial) {
    softNewline = False; /* the caller has told us not to */
    partialSize = 0;     /* bother with partial processing */
  }
  start = text + offset;
  stop = text + length;

  /*
   * gather up important info for wrap calculations
   */
  if (doWrap) {
    suspendChar =
        (GetRString(scratch, UseFlowOut ? FLOWED_QUOTE : QUOTE_PREFIX))[1];
    withSpace = UseFlowOut ? scratch[0] > 1 : false;
    lineLimit = GetRLong(
        UseFlowOut ? (encoder ? ENCODED_FLOWED_WRAP_SPOT : FLOW_WRAP_SPOT)
                   : WRAP_SPOT);
    hardLimit = GetRLong(UseFlowOut ? FLOW_WRAP_THRESH : WRAP_THRESH);

    /*
     * if this is a new line, count the quote level
     */
    if (!partialSize) {
      for (end = start; end < stop && *end == suspendChar && end - start < 10;
           end++)
        ;
      quoteLevel = end - start;
      if (quoteLevel > maxQuote)
        quoteLevel = 0;
    }
  } else
    lineLimit = REAL_BIG;

  /*
   * main loop; loop through the buffer, sending one line at a time
   */
  for (; start < stop; start = end + 1) {
    /* calculate the spot before which we should wrap */
    if (doWrap)
      curLen = partialSize; // limit = start + lineLimit - partialSize;

    /* if we don't want wrapping, or there is less text than the wrap limit */
    // if (!doWrap) limit = stop;	/* no need to wrap */

    /*
     * look through the buffer, from start to the calculated line limit
     * keep track of the last space we see, since it's a potential wrap point
     * if we find a return, we have a whole line, and can send it
     */
    if (doWrap) {
      if (softNewline)
        while (*start == ' ' && start < stop)
          start++; /* skip leading spaces after a soft newline */
      for (space = nl = start; nl < stop && *nl != '\015'; nl++) {
        if ((curLen < lineLimit || space == start && curLen < smtpMaxLen) &&
            (*nl == ' ' || *nl == '\t') && (nl == stop - 1 || nl[1] != '>'))
          space = nl;
        if (*nl == '\t')
          curLen = tab * ((curLen + tab) / tab);
        else
          curLen++;
      }

      // adjust for lines ending in whitespace
      if (nl < stop) {
        // special case for "-- "
        if (nl - start == 3 && start[0] == start[1] && start[0] == '-' &&
            start[2] == ' ')
          ; // leave it alone
        else
          for (end = nl - 1; end > start; end--) {
            if (*end == ' ')
              curLen--;
            else if (*end == '\t')
              curLen = tab * ((curLen - tab) / tab);
            else
              break;
          }
      }
      end = nl;

      if (curLen > lineLimit) /* we went over the wrap limit */
      {
        if (space > start) /* and we found a space */
          end = space;     /* Wrap it! */
        else if (curLen > smtpMaxLen)
          end = start + smtpMaxLen;
      }
    } else {
      for (nl = start; nl < stop && *nl != '\015'; nl++)
        ; /* just look for newlines */
      end = nl;
    }

    /*
     * make special allowance for lines >wrap limit but < 80
     */
    if (!softNewline && end < stop && curLen < hardLimit)
      end = nl;

    /* are we adding the newline?  We'll want to know for the next line. */
    if (end < stop)
      softNewline = *end != '\015';

    /*
     * at this point, start points at the beginning of the line to send,
     * and end points one character past the end of the line to send
     */

    // Protect a few things that are liable to transport damage
    if (!encoder && !partialSize && end > start &&
        ((void *)SendTrans) != ((void *)WrapSendTrans)) {
      /* escape initial periods, if need be */
      if (!UUPCOut && *start == '.')
        BS(stream, encoder, ".", 1);
      /* if doing f=f, space-stuff "From " */
      else if (flowed && *start == 'F' && end - start > 5 &&
               *(uint32_t *)(start + 1) == 'rom ')
        BS(stream, encoder, " ", 1);
    }

    /*
     * find last non-space character on line,
     * unless it's the end of the buffer, or not being wrapped,
     * in which case we'd best not drop spaces
     */
    space = end;
    // special case for "-- "
    if (space - start == 3 && start[0] == start[1] && start[0] == '-' &&
        start[2] == ' ')
      ; // leave it alone
    else if (doWrap && end < stop)
      while (space > start && (space[-1] == '\t' || space[-1] == ' '))
        space--;

    /* if there is data to send, send it */
    if (space > start) {
      // Ok, we might need to insert an extra space if doing f=f
      if (flowed && !partialSize && quoteLevel && space - start > quoteLevel &&
          start[quoteLevel] == ' ' &&
          ((void *)SendTrans) != ((void *)WrapSendTrans)) {
        BS(stream, encoder, start, quoteLevel);
        BS(stream, encoder, " ", 1);
        BS(stream, encoder, start + quoteLevel, space - start - quoteLevel);
      } else {
        /* if doing f=f, space-stuff initial space */
        if (!partialSize && flowed && *start == ' ')
          BS(stream, encoder, " ", 1);
        /* Now send the line */
        BS(stream, encoder, start, space - start);
      }
    }

    /*
     * send the newline, unless we've run out of characters and so don't know
     * if this should be a complete line or not
     */
    if (forceLines || end < stop) {
      if (softNewline && flowed)
        BS(stream, encoder, spaceNewLine,
           strlen((const char *)spaceNewLine)); // indicate that the newline is soft
      else
        BS(stream, encoder, NewLine, strlen((const char *)NewLine));
    }

    /*
     * We just put out a line, so we know we're starting fresh for
     * the next one, if there is a next one
     */
    partialSize = 0;

    /*
     * quoted line processing, if there are any chars left
     */
    if (end < stop)
      /*
       * if we sent out a complete line, peek at the next line to see how
       * many quote characters it has
       */
      if (*end == '\015') {
        char *p;
        for (p = end + 1; p < stop && *p == suspendChar; p++)
          ;
        quoteLevel = p - end - 1;
        if (quoteLevel > maxQuote)
          quoteLevel = 0;
      } else /* if we wrapped it, prequote the next line */
      {
        for (i = 0; i < quoteLevel; i++)
          BS(stream, encoder, &suspendChar, 1);
        partialSize = quoteLevel; /* guess we have a partial line after all */
        if (flowed && *end == '>')
          BS(stream, encoder, " ", 1); // space-stuff initial >
        if (quoteLevel && withSpace) {
          BS(stream, encoder, " ", 1);
          partialSize++;
        }
      }

    /*
     * normally, end points at a newline (for complete lines) or space
     * (for wrapped ones).  So, we normally skip the character end points
     * to.  However, long solid lines or Rong-wrapped lines might not obey
     * this behavior; adjust end back by one to make up for the increment
     * we'll do in just a few cycles...
     */
    if (end < stop && *end != ' ' && *end != '\015')
      end--;
  }

  /*
   * all done with that buffer.  If the last character is a newline,
   * we don't have much to do.  Otherwise, we may (forceLines) wish to
   * newline-terminate, else we want to remember how long the line
   * fragment we sent was
   */
  { size_t _nllen = strlen((const char *)NewLine);
    if (_nllen == 0 || lastC != NewLine[_nllen - 1]) {
      if (forceLines)
        BS(stream, encoder, NewLine, strlen((const char *)NewLine));
      else
        partialSize = lastLen;
    }
  }

done:
  return (sErr);
}



/************************************************************************
 * PrimeProgress - get the progress window started.
 ************************************************************************/
void PrimeProgress(MessHandle messH) {
  unsigned char buff[256];

  MyGetWTitle(GetMyWindowWindowPtr(messH->win), buff);
  //	ByteProgress(buff,0,CountCompBytes(messH));
  Progress(NoChange, NoChange, NULL, NULL, buff);
}

/************************************************************************
 * WannaSend - find out of the user wants to send a dirty window
 ************************************************************************/
int WannaSend(MyWindowPtr win) {
  unsigned char title[256];

  MyGetWTitle(GetMyWindowWindowPtr(win), title);
  AlertStr(WANNA_SEND_ALRT, Stop, title);
  return 1; /* assume user confirmed send in GTK port */
}

/************************************************************************
 * SendAttachments - send the files the user has attached to his message.
 ************************************************************************/
int SendAttachments(TransStream stream, MessHandle messH, long flags,
                    char * boundary, short tableID, short idBase) {
  FSSpec spec;
  short index;
  short err = noErr;
  bool plainText = 0 == (flags & FLAG_BX_TEXT);
  bool canQP = 0 != (flags & FLAG_CAN_ENC);
  bool isUU;
  CInfoPBRec hfi;
  unsigned char name[32];
  short aType;

  aType = AttachOptNumber(flags);
  isUU = aType + 1 == atmUU;
  hfi.hFileInfo.ioNamePtr = name;
  for (index = 1; !err; index++) {
    if (err = GetIndAttachment(messH, index, &spec, NULL))
      if (err == 1)
        break;
      else
        return (FileSystemError(BINHEX_OPEN, spec_name(spec), err));
    IsAlias(&spec, &spec);
    struct stat st_2873;
  if (stat(spec, &st_2873) == 0 && S_ISDIR(st_2873.st_mode))
      err = SendAttachmentFolder(stream, messH, flags, canQP, plainText,
                                 tableID, boundary, &spec, 0,
                                 idBase + index - 1, &idBase, &hfi);
    else
      err = SendAnAttachment(stream, messH, flags, canQP, plainText, tableID,
                             boundary, &spec, 0, idBase + index - 1);
  }
  if (index - 1)
    UpdateNumStat(ksStatSentAttach, index - 1);
  if (err == 1)
    err = noErr;
  return (err);
}

/************************************************************************
 * SendRelatedParts - send the files the user has put in his html
 ************************************************************************/
int SendRelatedParts(TransStream stream, MessHandle messH, long flags,
                     StackHandle stack, char * boundary) {
  FSSpec origSpec, spec;
  short index;
  short err = noErr;

  for (index = stack->elCount; !err && index--;) {
    if (err = StackItem(&origSpec, index, stack))
      err = 1;
    else {
      IsAlias(&origSpec, &spec);
      ConvertPictPart(&origSpec, &spec);
      if (err = SendAnAttachment(stream, messH, flags, true, true, 0, boundary,
                                 &spec, 1, stack->elCount - index - 1))
        break;
    }
  }
  if (err == 1)
    err = noErr;
  return (err);
}

/************************************************************************
 * ConvertPictPart - convert any PICT HTML parts to something more universal
 ************************************************************************/
void ConvertPictPart(char * origSpec, char * spec) {
#ifdef HAVE_QUICKTIME
  static OSType exportType;
  unsigned char scratch[256];
  unsigned char token[32];
  GraphicsExportComponent exCI = NULL;
  int err;

  if (PrefIsSet(PREF_NO_PICT_CONVERSION) || //	User doesn't want conversion
      exportType == -1 ||                   //	Exporter not found
#if TARGET_RT_MAC_CFM
      !HaveQuickTime(0x0400) ||
      !GraphicsExportDoExport || //	Make sure we have version 4 or greater
                                 // and QT library
#else
      !HaveQuickTime(0x0400) ||
#endif
      FileTypeOf(spec) != 'PICT')
    return; //	Don't convert this one

  if (!exportType) {
    //	Find best exporter
    if (GetRString(scratch, EXPORT_PICT_LIST)) {
      OSType thisType;
      char *spot;

      for (spot = scratch; PToken(scratch, token, &spot, ",");) {
        if (strlen((const char *)token) == sizeof(thisType)) {
          memcpy(&thisType, token, sizeof(thisType));
          if (exCI = OpenDefaultComponent(GraphicsExporterComponentType,
                                          thisType)) {
            //	Found one!
            exportType = thisType;
            break;
          }
        }
      }
    }

    if (!exportType) {
      //	Exporter not found
      exportType = -1;
      return;
    }
  } else
    exCI = OpenDefaultComponent(GraphicsExporterComponentType, exportType);

  if (exCI) {
    //	Do conversion
    GraphicsImportComponent imCI;

    if (!GetGraphicsImporterForFile(spec, &imCI)) {
      FSSpec tempSpec;

      g_strlcpy(tempSpec, origSpec, sizeof(tempSpec));
      UniqueSpec(&tempSpec, 31);
      GraphicsExportSetOutputFile(exCI, &tempSpec);
      GraphicsExportSetInputGraphicsImporter(exCI, imCI);
      if (exportType == 'PNGf') {
        //	Don't allow alpha channel. QuickTime PNG exporter messes up
        // alpha channel with 	PICT vector images rendering them invisible when
        // not viewing with QuickTime
        GraphicsExportSetDepth(exCI, 24);
      }
      err = GraphicsExportDoExport(exCI, NULL);
      CloseComponent(imCI);
      CloseComponent(exCI);
      if (!err) {
        //	Replace old PICT file (or alias)
        unlink(origSpec);
        // FSpRename(tempSpec, name) renames tempSpec to a new name in the same dir
        char newPath[1024];
        char *lastSlash = strrchr(tempSpec, '/');
        if (lastSlash) {
          int dirLen = lastSlash - tempSpec + 1;
          strncpy(newPath, tempSpec, dirLen);
          strcpy(newPath + dirLen, (char *)spec_name(origSpec) + 1);
        } else {
          strcpy(newPath, (char *)spec_name(origSpec) + 1);
        }
        rename(tempSpec, newPath);
        *spec = *origSpec;
      }
    } else
      CloseComponent(exCI);
  }
#else
  (void)origSpec;
  (void)spec; /* QuickTime PICT conversion not supported in GTK port */
#endif /* HAVE_QUICKTIME */
}

/************************************************************************
 * SendAttachmentFolder - send a folder full of files
 ************************************************************************/
int SendAttachmentFolder(TransStream stream, MessHandle messH, long flags,
                         bool canQP, bool plainText, short tableID,
                         char * boundary, char * folderSpec, short multiID,
                         short partID, short *partBase, CInfoPBRec *hfi) {
  short err = noErr;
  short index;
  FSSpec spec;
  char ourBoundary[128];
  long dirId;

  // start by sending a boundary for the outer multipart
  if (err = SendBoundary(stream))
    return err;
  // and a content-id, why not?
  if (err = SendCID(stream, messH, multiID, partID))
    return (err);

  // now, let's compose our boundary
  NumToString(partID, spec_name(spec));
  BuildBoundary(NULL, ourBoundary, spec_name(spec));
  /*
   * send the multipart header
   */
  if (err = ComposeRTrans(stream, MIME_MP_FMT, InterestHeadStrn + hContentType,
                          MIME_MULTIPART, MIME_X_FOLDER,
                          AttributeStrn + aBoundary, ourBoundary, NewLine))
    return (err);

  /*
   * content-disposition
   */
  if (!err)
    err = ComposeRTrans(stream, MIME_CD_FMT,
                        InterestHeadStrn + hContentDisposition, ATTACHMENT,
                        AttributeStrn + aFilename, spec_name(folderSpec), NewLine);

  // header/body separator
  if (err = SendPString(stream, NewLine))
    return (err);

  // get our dirID
  g_strlcpy(spec, folderSpec, sizeof(spec));
  dirId = SpecDirId(folderSpec); // keep for now if used later
  if (!spec[0])
    return fnfErr;

  // TODO: Implement POSIX directory iteration for attachment folders.
  // The old Mac CInfoPBRec/DirIterateMac loop has been removed.
  // When implemented, iterate over spec with opendir/readdir or
  // g_dir_open/g_dir_read_name, calling SendAnAttachment for files
  // and recursing with SendAttachmentFolder for subdirectories.
  (void)index;
  (void)hfi;

  // we need to send our final boundary
  if (!err)
    err = SendTrans(stream, "--", 2, ourBoundary, strlen((const char *)ourBoundary), "--", 2,
                    NewLine, strlen((const char *)NewLine), NULL);

  return err;
}

/************************************************************************
 * SendAnAttachment - send a single file
 ************************************************************************/
int SendAnAttachment(TransStream stream, MessHandle messH, long flags,
                     bool canQP, bool plainText, short tableID, char * boundary,
                     char * spec, short multiID, short partID) {
  short err = noErr;
  bool isUU;
  CInfoPBRec hfi;
  unsigned char s[256];
  unsigned char name[32];
  short aType;
  AttMap am;
  int64_t startTime_us;
  bool flat = false;
  FSSpec local;
  bool noRFork;

  aType = AttachOptNumber(flags);
  isUU = aType + 1 == atmUU;
  hfi.hFileInfo.ioNamePtr = name;
  struct stat st_3084;
  if (stat(spec, &st_3084) == 0)
    err = noErr;
  else
    err = ioErr;
  if (err)
    return (FileSystemError(BINHEX_OPEN, spec_name(spec), err));
  ComposeRString(s, BINHEX_PROG_FMT, spec_name(spec));
  Progress(NoChange, NoChange, NULL, NULL, s);
  noRFork = hfi.hFileInfo.ioFlRLgLen == 0;

  startTime_us = g_get_monotonic_time();
  if (err = SendBoundary(stream))
    return (err);
  if (err = SendCID(stream, messH, multiID, partID))
    return (err);
  if (err = FindAttMap(spec, &am))
    return (err);

  if (am.mm.specialId == 'flat') {
    g_strlcpy(local, spec, sizeof(local));
    if (!FlattenAndSpool(&local)) {
      spec = &local; // send the spooled copy
      flat = true;
    }
    am.mm.specialId = 0; // special processing done
  }

  if (plainText && am.mm.specialId && am.mm.specialId != '    ' ||
      am.mm.specialId == 'MiME')
    err = SendSpecial(stream, spec, &am);
  else
    err = kSpecialSendDidntPanOut;
  if (err != kSpecialSendDidntPanOut)
    ;
  else if (plainText && IsMailbox(spec) && hfi.hFileInfo.ioFlLgLen)
    err = SendDigest(stream, spec);
  else if (plainText && (am.isPostScript && !canQP ||
                         EqualStrRes(am.mm.mimetype, MIME_TEXT) && am.isBasic))
    err = SendPlain(stream, spec, flags, tableID, &am);
  else if (!isUU && plainText && (am.isBasic || noRFork))
    err = SendDataFork(stream, spec, flags, tableID, &am);
  else {
    switch (aType + 1) {
    case atmDouble:
      err = SendDouble(stream, spec, flags, tableID, &am);
      break;
    case atmSingle:
      err = SendSingle(stream, spec, True, &am);
      break;
    case atmUU:
      err = SendUU(stream, spec, &am);
      break;
    default:
      err = SendBinHex(stream, spec, &am);
      break;
    }
  }
  //		if (!err) {Progress(100,NoBar,NULL,NULL,NULL);PopProgress(False);}
  {
    int64_t elapsed_ms = (g_get_monotonic_time() - startTime_us) / 1000;
    if (elapsed_ms < 1) elapsed_ms = 1;
    long rate = (10 * (hfi.hFileInfo.ioFlLgLen + hfi.hFileInfo.ioFlRLgLen)) /
                (elapsed_ms * 1024);
    g_debug("%p: %d %d.%d KBps", spec_name(spec), aType,
                rate / 10, rate % 10);
  }

  if (flat)
    unlink(spec);
  return (err);
}

/************************************************************************
 * SendCID - send a content-id
 ************************************************************************/
int SendCID(TransStream stream, MessHandle messH, long part, short n) {
  unsigned char mid[256], cid[256];
  int err = noErr;

  if (*CompGetMID(messH, mid)) {
    // compose
    BuildContentID(cid, mid, part, n);
    // send
    err = ComposeRTrans(stream, CID_SEND_FMT, InterestHeadStrn + hContentId,
                        cid, NewLine);
  }
  return (err);
}

/************************************************************************
 * BuildContentID - build a content-id, without <>'s or header or newline
 ************************************************************************/
char * BuildContentID(char * into, char * mid, long part, short i) {
  return (ComposeRString(into, CID_ONLY_FMT, mid, part, i));
}

/**********************************************************************
 * SendDigest - send a mailbox, as a digest
 **********************************************************************/
int SendDigest(TransStream stream, char * spec) {
  TOCType * tocH = TOCBySpec(spec);
  unsigned char boundary[256];
  unsigned char date[64];
  short i;
  MyWindowPtr win;
  bool newWin;

  if (!tocH)
    return (1);

  /*
   * build the boundary
   */
  BuildBoundary(NULL, boundary, "d");

  sErr = ComposeRTrans(stream, MIME_MP_FMT, InterestHeadStrn + hContentType,
                       MIME_MULTIPART, MIME_DIGEST, AttributeStrn + aBoundary,
                       boundary, NewLine);
  if (!sErr)
    sErr = ComposeRTrans(stream, MIME_CD_FMT,
                         InterestHeadStrn + hContentDisposition, ATTACHMENT,
                         AttributeStrn + aFilename, spec_name(spec), NewLine);
  struct stat st_3202;
  stat(spec, &st_3202);
  if (!sErr && *R822Date(date, st_3202.st_mtime - ZoneSecs()))
    sErr = ComposeRTrans(stream, MIME_CT_ANNOTATE, AttributeStrn + aModDate,
                         date, NewLine);
  if (!sErr)
    sErr = SendPString(stream, NewLine);

  if (!sErr)
    for (i = 0; i < tocH->count; i++) {
      void *tSig, *tRSig, *tHSig;

      tSig = eSignature;
      tRSig = RichSignature;
      tHSig = HTMLSignature;

      eSignature = NULL;
      RichSignature = NULL;
      HTMLSignature = NULL;

      /*
       * send a boundary
       */
      if (sErr = SendBoundary(stream))
        break;

      win = GetAMessageLo(tocH, i, NULL, NULL, false, &newWin);
      if (win) {
        WindowPtr winWP = GetMyWindowWindowPtr(win);

        sErr = TransmitMessageForSpool(stream, Win2MessH(win));
        if (newWin)
          CloseMyWindow(winWP);
        else
          NotUsingWindow(winWP);
      } else
        sErr = mFulErr;

      BSCLOSE(stream, 0); /* unfortunate, but gotta do it because of how
                             SendBoundary works */

      eSignature = tSig;
      RichSignature = tRSig;
      HTMLSignature = tHSig;
    }

done:
  FlushTOCs(True, False);

  /*
   * and the terminal boundary
   */
  if (!sErr) {
    g_strlcat(boundary, "--", sizeof(boundary));
    sErr = SendBoundary(stream);
  }
  return (sErr);
}

/************************************************************************
 * SendSpecial - send a special attachment type
 ************************************************************************/
int SendSpecial(TransStream stream, char * spec, AttMapPtr amp) {
  int err;

  switch (amp->mm.specialId) {
  case 'AURL':
    err = SendAnonFTP(stream, spec);
    break;
  case 'MiME':
    err = SendRawMIME(stream, spec);
    break;
  default:
    WarnUser(INVALID_MAP, 0);
    err = 1;
    break;
  }
  return (err);
}

/************************************************************************
 * GetFlatten - Copy the flatten table into a pointer
 ************************************************************************/
char *GetFlatten(void) {
  void *flatH;
  char *flatten;

  flatten = NuPtr(256);
  flatH = GetResource_('taBL', ktFlatten);
  if (flatH)
    memmove(flatten, flatH, 256);
  else
    ZapPtr(flatten);

  return (flatten);
}

/************************************************************************
 * SendAnonFTP - send an 'AURL' doc as an anonymous ftp thingie
 ************************************************************************/
int SendAnonFTP(TransStream stream, char * spec) {
  int err;
  void *text;
  char type[128], ftp[128], host[128], dir[128], name[128], token[128];
  unsigned char data[256];
  char *spot;
  short size;

  if (err = Snarf(spec, &text, 254))
    FileSystemError(BINHEX_READ, spec_name(spec), err);
  else {
    size = GetHandleSize_(text);
    { size_t _mpl = (size); memcpy(data, (char *)text, _mpl); ((char*)(data))[_mpl] = '\0'; }

    /*
     * parse the string
     */
    spot = data;
    if (PToken(data, type, &spot, " ") && PToken(data, ftp, &spot, ":")) {
      while (*spot == '/')
        spot++;
      if (PToken(data, host, &spot, "/")) {
        *name = *dir = 0;
        while (PToken(data, token, &spot, "/")) {
          {
            size_t _dlen = strlen((const char *)dir);
            if (_dlen == 0 || dir[_dlen - 1] != '/')
              PCatC(dir, '/');
          }
          g_strlcat((char *)dir, (const char *)token, sizeof(dir));
          g_strlcpy((char *)name, (const char *)token, sizeof(name));
        }
        if (*name) {
          {
            size_t _dlen = strlen((const char *)dir);
            size_t _nlen = strlen((const char *)name);
            if (_dlen >= 1 + _nlen)
              dir[_dlen - 1 - _nlen] = '\0';
          }

          /*
           * hey; that all parsed
           */
          if (EqualStrRes(type, ANARCHIE_GET) ||
              EqualStrRes(type, ANARCHIE_TXT))
            if (EqualStrRes(ftp, ANARCHIE_FTP) &&
                !PPtrFindSub("@", host, strlen((const char *)host))) {
              /*
               * and it is even something we can handle.  Hurrah!
               */
              err = ComposeRTrans(stream, MIME_TEXTPLAIN,
                                  InterestHeadStrn + hContentType, MIME_MESSAGE,
                                  EXTERNAL_BODY, "", NewLine);
              if (!err)
                err = ComposeRTrans(stream, NQ_ANNOTATE,
                                    AttributeStrn + aAccessType,
                                    GetRString(data, ANON_FTP), NewLine);
              if (!err)
                err = ComposeRTrans(stream, MIME_CT_ANNOTATE,
                                    AttributeStrn + aSite, host, NewLine);
              if (*dir && !err)
                err = ComposeRTrans(stream, MIME_CT_ANNOTATE,
                                    AttributeStrn + aDirectory, dir, NewLine);
              if (!err)
                err = ComposeRTrans(stream, MIME_CT_ANNOTATE,
                                    AttributeStrn + aName, name, NewLine);
              if (!err)
                err = ComposeRTrans(
                    stream, MIME_CT_ANNOTATE, AttributeStrn + aMode,
                    GetRString(data,
                               EqualStrRes(type, ANARCHIE_TXT) ? ASCII : IMAGE),
                    NewLine);

              if (!err)
                err = SendPString(stream, NewLine);
              if (!err)
                err = ComposeRTrans(stream, MIME_MP_FMT,
                                    InterestHeadStrn + hContentType,
                                    MIME_APPLICATION, MIME_OCTET_STREAM,
                                    AttributeStrn + aName, name, NewLine);
              if (!err)
                err = ComposeRTrans(
                    stream, MIME_CD_FMT, InterestHeadStrn + hContentDisposition,
                    ATTACHMENT, AttributeStrn + aFilename, name, NewLine);
              return (err);
            }
        }
      }
    }
    return (-1);
  }
  return (err);
}
/************************************************************************
 * SendRawMIME - send a raw MIME document
 ************************************************************************/
int SendRawMIME(TransStream stream, char * spec) {
  long size = FSpDFSize(spec);
  char *buffer = NULL;
  long bSize;
  long count;
  long sendCount;
  int err = noErr;
  short refN;
  DecoderFunc *encoder = UUPCOut ? NULL : PeriodEncoder;
  bool needNL = false; // we do not need to add a newline

  bSize = MIN(size, GetRLong(BUFFER_SIZE));

  struct stat st_3397;
  stat(spec, &st_3397);
  size = st_3397.st_size;
  refN = 0;
  err = noErr;

  if (size > 0) {
    buffer = malloc(bSize);
    if (!buffer) {
      WarnUser(MEM_ERR, err = memFullErr);
      return (err);
    }
    refN = open(spec, O_RDWR);
    if (refN >= 0) {
      err = noErr;
    } else {
      err = ioErr;
    }
    if (!err) {
      // sniff the end of the file for crlf
      GetEOF(refN, &count);
      if (count < 2)
        needNL = true;
      else {
        SetFPos(refN, fsFromLEOF, -2);
        count = 2;
        ARead(refN, &count, buffer);
        needNL = buffer[0] != '\015' || buffer[1] != '\012';
        SetFPos(refN, fsFromStart, 0);
      }

      // send the file
      while (!err && size > 0) {
        count = MIN(size, bSize);
        if (!(err = ARead(refN, &count, buffer))) {
          if (strlen((const char *)NewLine) == 1 && NewLine[0] == '\015')
            sendCount = RemoveChar('\012', buffer, count);
          else if (strlen((const char *)NewLine) == 1 && NewLine[0] == '\012')
            sendCount = RemoveChar('\015', buffer, count);
          else
            sendCount = count;
          err = BufferSend(stream, encoder, buffer, sendCount, True);
        } else
          FileSystemError(BINHEX_READ, spec_name(spec), err);
        size -= count;
      }
      // if the file didn't end with a newline, add one
      if (!err && needNL)
        err = BufferSend(stream, encoder, NewLine, strlen((const char *)NewLine), True);

      // send any remainder
      if (!err)
        err = BufferSend(stream, NULL, NULL, 0, False);
      BufferSendRelease(stream);
      close(refN);
    }
    free(buffer);
  } else
    WarnUser(BINHEX_READ, err = MemError());
  return (err);
}

/************************************************************************
 * PeriodEncoder - encode periods
 ************************************************************************/
int PeriodEncoder(CallType callType, DecoderPBPtr pb) {
  static short nlState;

  if (pb) {
    switch (callType) {
    case kDecodeInit:
      nlState = strlen((const char *)NewLine);
      break;

    case kDecodeDone:
      break;

    case kDecodeDispose:
      break;

    case kDecodeData:
      if (pb->inlen)
        pb->outlen =
            StuffPeriods(pb->input, pb->inlen, pb->output, NewLine, &nlState);
      else
        pb->outlen = 0;
      break;
    }
  }
  return (noErr);
}

/************************************************************************
 * StuffPeriods - byte-stuff periods for SMTP
 ************************************************************************/
long StuffPeriods(char *in, long inLen, char *out,
                  char * newLine, short *nlStatePtr) {
  short nlState = *nlStatePtr;
  char *end = in + inLen;
  short newLineLen = strlen((const char *)newLine);
  char *origOut = out;

  for (end = in + inLen; in < end; in++) {
    if (*in == '.' && nlState == newLineLen) // found period we need to stuff
    {
      *out++ = '.';
      nlState = 0;
    } else {
      if (nlState == newLineLen)
        nlState = 0; // start over again

      if (*in == newLine[nlState])
        nlState++; // found one of the newline chars
      else
        nlState = 0; // start over again
    }
    *out++ = *in;
  }

  *nlStatePtr = nlState;
  return (out - origOut);
}

/************************************************************************
 * IsPostScript - is a file a PostScript file?
 ************************************************************************/
bool IsPostScript(char * spec) {
  short refN;
  unsigned char psMagic[32];
  unsigned char fileMagic[32];
  long count;
  bool result = False;

  refN = open(spec, O_RDONLY);
  if (refN >= 0) {
    GetRString(psMagic, PS_MAGIC);
    count = strlen((const char *)psMagic);
    if (!ARead(refN, &count, fileMagic)) {
      fileMagic[count] = '\0';
      result = StringSame(fileMagic, psMagic);
    }
    close(refN);
  }
  return (result);
}

/************************************************************************
 * SendPlain - send a plain text file
 ************************************************************************/
short SendPlain(TransStream stream, char *spec, long flags, short tableId,
                AttMapPtr amp) {
  int err;
  DecoderFunc *encoder = NULL;
  void *taste = NULL;
  unsigned char scratch[32];
  FInfo info;

  /*
   * send header
   */
  if (amp->isPostScript) {
    if ((flags & FLAG_CAN_ENC)) {
      encoder = B64Encoder;
      flags |= FLAG_ENCBOD;
    }
    struct stat st_3560;
    stat(spec, &st_3560);
    if (err = MIMEFileHeader(stream, amp, POSTSCRIPT, st_3560.st_mtime))
      goto done;
    if (err = ComposeRTrans(stream, MIME_V_FMT,
                            InterestHeadStrn + hContentEncoding,
                            encoder ? MIME_BASE64 : MIME_BINARY, NewLine))
      goto done;
    DontTranslate = True;
    flags &= ~FLAG_WRAP_OUT;
  } else {
    flags &=
        ~FLAG_ENCBOD; /* we may not need to encode this; we'll find out later */
    Snarf(spec, &taste, GetRLong(TEXT_QP_TASTE));
    { long _tlen = taste ? (long)GetHandleSize_((void *)taste) : 0;
      if (err = SendContentType(stream, taste ? (char *)taste : NULL, _tlen, 0,
                                NULL, 0, 0, tableId, &flags, NULL,
                                ATT_MAP_NAME(amp), NULL, amp->mm.subtype))
        goto done;
    }
    free(taste);
    encoder = flags & FLAG_ENCBOD ? QPEncoder : NULL;
    if (err = ComposeRTrans(
            stream, MIME_CD_FMT, InterestHeadStrn + hContentDisposition,
            ATTACHMENT, AttributeStrn + aFilename, ATT_MAP_NAME(amp), NewLine))
      goto done;
    struct stat st_3581;
    stat(spec, &st_3581);
    if (*R822Date(scratch, st_3581.st_mtime - ZoneSecs()) &&
        (err = ComposeRTrans(stream, MIME_CT_ANNOTATE, AttributeStrn + aModDate,
                             scratch, NewLine)))
      goto done;
  }
  // FSpGetFInfo is no-op
  if (!err && !amp->suppressXMac)
    err = ComposeRTrans(stream, MIME_CT_ANNOTATE, AttributeStrn + aMacType,
                        Long2Hex(scratch, info.fdType), NewLine);
  if (!err && !amp->suppressXMac)
    err = ComposeRTrans(stream, MIME_CT_ANNOTATE, AttributeStrn + aMacCreator,
                        Long2Hex(scratch, info.fdCreator), NewLine);

  if (err = SendPString(stream, NewLine))
    goto done;

  err = SendTextFile(stream, spec, flags, encoder);

done:
  DontTranslate = False;
  BufferSendRelease(stream);
  return (err);
}

/**********************************************************************
 * SendTextFile - send text from a file
 **********************************************************************/
int SendTextFile(TransStream stream, char * spec, long flags,
                   DecoderFunc *encoder) {
  short refN = 0;
  char *dataBuffer = NULL;
  long dataSize;
  int err;
  long fileSize, sendSize, readSize;
  bool partial = False;

  /*
   * allocate the buffers
   */
  dataSize = GetRLong(BUFFER_SIZE);
  if (!(dataBuffer = malloc(dataSize))) {
    WarnUser(MEM_ERR, err = memFullErr);
    goto done;
  }

  /*
   * open it
   */
  refN = open(spec, O_RDONLY);
  if (refN < 0) {
    err = ioErr;
  } else {
    err = noErr;
  }
  if (err) {
    FileSystemError(BINHEX_OPEN, spec_name(spec), err);
    goto done;
  }
  if (err = GetEOF(refN, &fileSize)) {
    FileSystemError(BINHEX_OPEN, spec_name(spec), err);
    goto done;
  }

  /*
   * send it
   */
  for (; fileSize; fileSize -= readSize) {
    readSize = MIN(dataSize, fileSize);
    sendSize = readSize;
    if (err = ARead(refN, &sendSize, dataBuffer)) {
      FileSystemError(BINHEX_READ, spec_name(spec), err);
      goto done;
    }
    if (err = SendBodyLines(stream, (void *)dataBuffer, sendSize, 0, flags, False, NULL,
                            0, partial, encoder))
      goto done;
    partial = dataBuffer[sendSize - 1] != '\015';
  }
  if (!err)
    err = BufferSend(stream, encoder, NULL, 0, True);
  if (!err && partial)
    SendPString(stream, NewLine);

done:
  DontTranslate = False;
  BufferSendRelease(stream);
  if (refN)
    close(refN);
  free(dataBuffer);
  return (err);
}

/************************************************************************
 * BuildDateHeader - build an RFC 822 date header
 ************************************************************************/
void BuildDateHeader(char *buffer, long seconds) {
  unsigned char date[64];
  if (*R822Date(date, seconds))
    ComposeRString(buffer, DATE_HEADER, HeaderStrn + DATE_HEAD, date);
  else
    *buffer = 0;
  return;
}

/************************************************************************
 * BuildDateHeader - build an RFC 822 date header
 ************************************************************************/

/************************************************************************
 * SaveB4Send - grab an outgoing message, saving if necessary
 ************************************************************************/
MessHandle SaveB4Send(TOCType * tocH, short sumNum) {
  short which;
  MessHandle messH = (MessHandle)tocH->sums[sumNum].messH;

  if (messH && messH->win->isDirty) {
    which = WannaSend(messH->win);
    if (which == CANCEL_ITEM || which == WANNA_SAVE_CANCEL)
      return (NULL);
    else if (which == WANNA_SAVE_SAVE && !SaveComp(messH->win))
      return (NULL);
    else if (which == WANNA_SAVE_DISCARD) {
      messH->win->isDirty = False;
      CloseMyWindow(GetMyWindowWindowPtr(messH->win));
      messH = NULL;
    }
  }
  if (!messH) {
    MyWindowPtr winResult;

    MyThreadBeginCritical(); // Make sure OpenComp doesn't switch threads
    winResult = OpenComp(tocH, sumNum, NULL, NULL, False, False);
    MyThreadEndCritical();
    if (!winResult)
      return (NULL);
    messH = (MessHandle)tocH->sums[sumNum].messH;
  }
  return (messH);
}

/************************************************************************
 * BuildBoundary - build a boundary line for a message
 ************************************************************************/
void BuildBoundary(MessHandle messH, char * boundary, char * middle) {
(void)messH;
  ComposeRString(boundary, MIME_BOUND1_FMT, GMTDateTime(), middle, MIME_BOUND2);
}

/************************************************************************
 * SendContentType - deduce and send the appropriate content-type
 *  (and CTE) for two blocks of text (body and signature, typically)
 *	text1 - one block of text (may be NULL)
 *  text2 - second block of text (may be NULL)
 *  tableID - xlate table id
 *  flags - message flags
 ************************************************************************/
int SendContentType(TransStream stream, char *text1, long text1Len, long offset1,
                      char *text2, long text2Len, long offset2, short tableID, long *flags,
                      long *opts, char * name, emsMIMEHandle *tlMIME,
                      char * subtype) {
  short etid = EffectiveTID(tableID);
  char scratch[128];
  unsigned char flowed[64];
#ifdef ETL
  unsigned char s2[64];
#endif
  bool anyfunny;
  bool any2022;
  short err;
  short encId;
  bool strip = 0 != (opts && (*opts && OPT_STRIP));
  bool rich = !strip && 0 != (flags && (*flags && FLAG_RICH));
  bool html = !strip && 0 != (opts && (*opts && OPT_HTML));
  short computedSubType =
      html ? HTMLTagsStrn + htmlTag : (rich ? MIME_RICHTEXT : MIME_PLAIN);

  if (rich || html)
    *flags &= ~FLAG_WRAP_OUT;

  anyfunny =
      !text1 || AnyFunny(text1, text1Len, offset1) || text2 && AnyFunny(text2, text2Len, offset2);
  any2022 = !PrefIsSet(PREF_NO_2022) && (text1 && Any2022(text1, text1Len, offset1) ||
                                         text2 && Any2022(text2, text2Len, offset2));

  /*
   * figure out proper charset
   */
  if (anyfunny)
    NameCharset(scratch, etid, tlMIME);
  else if (any2022)
    NameCharset(scratch, kt2022, tlMIME);
  else
    NameCharset(scratch, ktMacUS, tlMIME);

  if (0 != (*flags && FLAG_WRAP_OUT) && UseFlowOut) {
    if (tlMIME)
      AddTLMIME(*tlMIME, TLMIME_PARAM,
                GetRString(flowed, AttributeStrn + aFormat),
                GetRString(s2, FORMAT_FLOWED));
    ComposeRString(flowed, MIME_CT_ANNOTATE, AttributeStrn + aFormat,
                   GetRString(s2, FORMAT_FLOWED), "");
    g_strlcat(scratch, flowed, sizeof(scratch));
  }

  /*
   * send the content type
   */
  if (!name) {
    if (subtype)
      err = ComposeRTrans(stream, MIME_TEXTNOTPLAIN,
                          InterestHeadStrn + hContentType, MIME_TEXT, subtype,
                          scratch, NewLine);
    else
      err =
          ComposeRTrans(stream, MIME_TEXTPLAIN, InterestHeadStrn + hContentType,
                        MIME_TEXT, computedSubType, scratch, NewLine);
#ifdef ETL
    if (tlMIME) {
      AddTLMIME(*tlMIME, TLMIME_TYPE, GetRString(scratch, MIME_TEXT), NULL);
      AddTLMIME(*tlMIME, TLMIME_SUBTYPE,
                subtype ? subtype : GetRString(scratch, computedSubType), NULL);
    }
#endif
  } else {
    g_strlcat(scratch, NewLine, sizeof(scratch));
    if (subtype)
      err = ComposeRTrans(stream, MIME_TEXT_SUBTYPE_FMT,
                          InterestHeadStrn + hContentType, MIME_TEXT, subtype,
                          AttributeStrn + aName, name, scratch);
    else
      err = ComposeRTrans(stream, MIME_MP_FMT, InterestHeadStrn + hContentType,
                          MIME_TEXT, computedSubType, AttributeStrn + aName,
                          name, scratch);
#ifdef ETL
    if (tlMIME) {
      AddTLMIME(*tlMIME, TLMIME_TYPE, GetRString(scratch, MIME_TEXT), NULL);
      AddTLMIME(*tlMIME, TLMIME_SUBTYPE,
                subtype ? subtype : GetRString(scratch, computedSubType), NULL);
    }
#endif
  }
  if (err)
    return (err);

  /*
   * content-transfer-encoding, if any
   */
  if (0 == (*flags & FLAG_ENCBOD)) /* set manually? */
    *flags = DecideEncoding(text1, text1Len, text2, text2Len, anyfunny, etid,
                            *flags); /* determine automatically */

  if (0 != (*flags & FLAG_ENCBOD)) {
    if ((*flags && FLAG_CAN_ENC) && (UUPCOut || (Ehlo && !Ehlo->mime8bit) ||
                                    !PrefIsSet(PREF_ALLOW_8BITMIME)))
      encId = MIME_QP;
    else {
      encId = MIME_8BIT;
      *flags &= ~FLAG_ENCBOD;
    }
    if (err =
            ComposeRTrans(stream, MIME_V_FMT,
                          InterestHeadStrn + hContentEncoding, encId, NewLine))
      return (err);
#ifdef ETL
    if (tlMIME) {
      AddTLMIME(*tlMIME, TLMIME_TYPE, GetRString(scratch, MIME_TEXT), NULL);
      AddTLMIME(*tlMIME, TLMIME_PARAM, GetRString(scratch, MIME_CTE),
                GetRString(s2, encId));
    }
#endif
  }
  return (noErr);
}

/************************************************************************
 * DecideEncoding - decide which encoding (if any) to use
 ************************************************************************/
long DecideEncoding(char *text1, long text1Len, char *text2, long text2Len,
                    bool anyfunny, short etid, long flags) {
  if (etid == ktMacUS)
    anyfunny = False;

  if (anyfunny)
    flags |= FLAG_ENCBOD; /* encode funny chars in QP */
  else if (0 == (flags && FLAG_WRAP_OUT) && !(flags && FLAG_RICH)) {
    if (!text1 || LongerThan(text1, text1Len, GetRLong(MAX_SMTP_LINE)) ||
        text2 && LongerThan(text2, text2Len, GetRLong(MAX_SMTP_LINE)))
      flags |= FLAG_ENCBOD;
  } else if (0 == (flags && FLAG_ENCBOD) && (flags && FLAG_RICH)) {
    if (!text1 || LongerWordThan(text1, text1Len, GetRLong(ENRICHED_MAX_WORD)) ||
        text2 && LongerWordThan(text1, text1Len, GetRLong(ENRICHED_MAX_WORD)))
      flags |= FLAG_ENCBOD;
  }
  return (flags);
}

/************************************************************************
 * SevenBitTable - is the table in question full of only 7-bit chars?
 ************************************************************************/
bool SevenBitTable(short tableID) {
  void *table;
  char *spot, *end;

  if (!tableID || !(table = GetResource_('taBL', tableID)))
    return (False);

  for (spot = (char *)table + 127, end = (char *)table + 256; spot < end; spot++)
    if (*spot > 126)
      return (False);

  return (True);
}

/************************************************************************
 * AnyFunny - does a block of text contain funny chars?
 ************************************************************************/
bool AnyFunny(char *text, long textLen, long offset) {
  char *spot, *end;
  unsigned char line[256];

  if (!text)
    return (True);

  // check for high bits
  if (Flatten) {
    for (spot = text + offset, end = text + textLen;
         spot < end; spot++)
      if (Flatten[(unsigned char)*spot] > 126)
        return (True);
  } else {
    for (spot = text + offset, end = text + textLen;
         spot < end; spot++)
      if ((unsigned char)*spot > 126)
        return (True);
  }

  // check for uucp envelopes
  for (spot = text + offset; spot < end; spot++) {
    if (*spot == '\015')
      break;
  }
  if (spot < end) {
    { size_t _mpl = (spot - text - offset); memcpy(line, text + offset, _mpl); ((char*)(line))[_mpl] = '\0'; }
    if (line[0] && IsFromLine(line))
      return true;
  }

  // all quiet on the western front
  return (False);
}

/************************************************************************
 * Any2022 - does a block of text contain 2022?
 ************************************************************************/
bool Any2022(char *text, long textLen, long offset) {
  char *spot, *end;

  if (!text)
    return (True);

  for (spot = text + offset, end = text + textLen;
       spot < end; spot++)
    if (spot[0] == escChar && spot[1] == '$')
      return (True);

  return (False);
}

/************************************************************************
 * EffectiveTID - what is the effective table id?
 ************************************************************************/
short EffectiveTID(short tid) {
  unsigned char pTable[32];

  if (!NewTables)
    return (TransOutTablID());
  if (tid == NO_TABLE)
    return (0);
  if (tid == DEFAULT_TABLE)
    return (*GetPref(pTable, PREF_OUT_XLATE) ? GetPrefLong(PREF_OUT_XLATE)
                                             : TransOutTablID());
  else
    return (tid);
}

/************************************************************************
 * TransOutTablID - return the translit table name to use for high-bit chars
 *   when not using the new table support.  Pretty hacky, I'm afraid.
 ************************************************************************/
short TransOutTablID(void) {
  unsigned char name[256];
  GetPref(name, PREF_SEND_CSET);
  if (EqualStrRes(name, MIME_ISO_LATIN1))
    return TRANS_OUT_TABL_8859_1;
  if (EqualStrRes(name, MIME_ISO_LATIN15))
    return ktMacISO15;
  if (EqualStrRes(name, MIME_WIN_1252))
    return ktMacWindows;
  if (EqualStrRes(name, MIME_MAC))
    return ktIdendity;
  return TRANS_OUT_TABL_8859_1; // Oh well, pref is junk...
}

/************************************************************************
 * TransOutTablName - return the translit table name to use for high-bit chars
 *   when not using the new table support.  Pretty hacky, I'm afraid.
 ************************************************************************/
char * TransOutTablName(char * name) {
  GetPref(name, PREF_SEND_CSET);
  if (EqualStrRes(name, MIME_ISO_LATIN1))
    return name;
  if (EqualStrRes(name, MIME_ISO_LATIN15))
    return name;
  if (EqualStrRes(name, MIME_WIN_1252))
    return name;
  if (EqualStrRes(name, MIME_MAC))
    return name;
  return GetRString(name, MIME_ISO_LATIN1); // Oh well, pref is junk...
}

/************************************************************************
 * NameCharset - build a charset= parameter
 ************************************************************************/
char * NameCharset(char * charset, short tid, emsMIMEHandle *tlMIME) {
  unsigned char scratch[64];
#ifdef ETL
  unsigned char header[64];
#endif

  if (*SimpleNameCharset(scratch, tid)) {
    ComposeRString(charset, MIME_CSET, scratch);
#ifdef ETL
    if (tlMIME)
      AddTLMIME(*tlMIME, TLMIME_PARAM, GetRString(header, MIME_CHARSET),
                scratch);
#endif
  } else
    *charset = 0;

  return (charset);
}

/**********************************************************************
 * SimpleNameCharset - return just the name of a charset
 **********************************************************************/
char * SimpleNameCharset(char * name, short tid) {
  void *res;
  short id;
  ResType type;

  if (!tid)
    GetRString(name, MIME_MAC);
  else if (tid == ktMacUS)
    GetRString(name, MIME_USASCII);
  else if (tid == TransOutTablID())
    TransOutTablName(name);
  else if (tid == TRANS_OUT_TABL_8859_1 || tid == ktMacISO || tid == ktISOMac ||
           tid == TRANS_IN_TABL)
    GetRString(name, MIME_ISO_LATIN1);
  else if (tid == kt2022)
    GetRString(name, ISO_2022_JP);
  else {
    if (!GetTableCName(tid - 1, name)) {
      res = GetResource_('taBL', tid);
      if (res) {
        GetResInfo(res, &id, &type, name);
      } else
        *name = 0;
    }
  }
  return (name);
}

/************************************************************************
 * LongerThan - is there a line longer than some number of chars?
 ************************************************************************/
bool LongerThan(char *text, long textLen, short len) {
  char *spot, *end, *nl;

  spot = text;
  end = spot + textLen;
  nl = spot - 1;

  for (; spot < end; spot++) {
    if (*spot == '\015') {
      if (spot - nl - 1 > len)
        return (True);
      nl = spot;
    }
  }
  return (False);
}

/************************************************************************
 * LongerWordThan - is there a "word" longer than some number of chars?
 ************************************************************************/
bool LongerWordThan(char *text, long textLen, short len) {
  char *spot, *end, *nl;

  spot = text;
  end = spot + textLen;
  nl = spot - 1;

  for (; spot < end; spot++) {
    if (*spot == '\015' || *spot == ' ') {
      if (spot - nl - 1 > len)
        return (True);
      nl = spot;
    }
  }
  return (False);
}

/************************************************************************
 * Next1342Word - parse a word from a 1342 stream
 ************************************************************************/
void Next1342Word(char **startP, char *end,
                  Token1342Ptr current, char * delim, bool *wasQuote,
                  bool *encQuote) {
  unsigned char word[64];
  short wordLim = 48;
  Enum1342 wordType;
  Byte c;
  char *source = *startP;
  bool justSpace;
  bool newWasQuote;
  char *qSpot;

  /*
   * are we off the end?
   */
  if (source >= end) {
    wordType = k1342End;
    word[0] = '\0';
  } else {
    size_t wlen;
    c = *source++;
    word[0] = c;
    word[1] = '\0';
    if (*wasQuote || c == '"') /* collect a quote */
    {
      if (!*wasQuote) { /* search to end of quote to see if quote contains any
                           high bits */
        for (qSpot = source; qSpot < end; qSpot++)
          if (qSpot[0] == '"' && qSpot[-1] != '\\')
            break;
        *encQuote = AnyHighBits(source, qSpot - source);
      }
      newWasQuote = True;
      while (source < end && (wlen = strlen((const char *)word)) < wordLim) {
        PCatC(word, *source);
        if (*source >= 0x80)
          wordLim -= 2; /* allow space for encoding */
        if (*source++ == '"' && wlen >= 2 && word[wlen - 1] != '\\') {
          newWasQuote = False;
          break; /* closing " */
        }
      }
      wordType = *encQuote ? k1342Word : k1342Plain;
      *wasQuote = newWasQuote;
    } else if (strchr((const char *)delim, c)) /* collect a string of delimiters */
    {
      justSpace = c == ' ';
      while (source < end && strlen((const char *)word) < wordLim)
        if (*source != '"' && strchr((const char *)delim, *source)) {
          PCatC(word, *source);
          if (*source != ' ')
            justSpace = False;
          source++;
        } else
          break;
      wordType = justSpace ? k1342LWSP : k1342Plain;
    } else /* collect a regular word */
    {
      char *whichDelim;
      char *lastSP = NULL;
      short oldWordLim;
      short oldWordSize;

      while (source < end && (wlen = strlen((const char *)word)) < wordLim)
        if (whichDelim = (char *)strchr((const char *)delim, *source)) {
          if (*whichDelim == ' ') {
            if (!AnyHighBits(word, wlen))
              break;
            // We've seen some stuff we need to encode, and now we've
            // seen a space.  Remember where we saw it.  If we overflow
            // our buffer, then we'll truncate to before here so we always
            // have integral words in our encoding.  With luck, this will
            // allow us to make less stupid choices about encoding words by
            // sometimes including spaces in encoded words rather than encoding
            // space runs
            lastSP = source;
            oldWordLim = wordLim;
            oldWordSize = wlen;
            PCatC(word, *source++);
          } else
            break;
        } else {
          if (*source >= 0x80)
            wordLim -= 2; /* allow space for encoding */
          PCatC(word, *source);
          source++;
        }

      // did we end with chars left? If not, and if we have a prior space,
      // back up to that prior space
      wlen = strlen((const char *)word);
      if (wlen >= wordLim && lastSP) {
        source = lastSP;
        wordLim = oldWordLim;
        word[oldWordSize] = '\0';
        wlen = oldWordSize;
      }

      wordType = AnyHighBits(word, wlen) ? k1342Word : k1342Plain;
      // trim trailing spaces from encoded words
      if (wordType == k1342Word)
        while (wlen > 0 && word[wlen - 1] == ' ') {
          word[--wlen] = '\0';
          --source;
        }
    }
  }

  g_strlcpy(current->word, word, sizeof(current->word)); /* copy word and type into current buffer */
  current->wordType = wordType;
  *startP = source; /* mark new position in string */
}

/************************************************************************
 * Encode1342 - encode a header line ala RFC 1342 (1522, actually)
 ************************************************************************/
char *Encode1342(char *source, long len, short lineLimit,
                 short *charsOnLine, char * nl, short tid, long *outLen) {
  Token1342 tokens[3];
  Token1342Ptr prev, curr, next;
  unsigned char dl1342[32];
  bool continueQuote = False;
  bool encQuote = False;
  char *spot = source;
  short line;
  char *encoded = NULL;
  long encodedLen = 0;
  long encodedCap = 0;
  bool wrapped;
  Byte c;

  if (outLen) *outLen = 0;

  /*
   * grab delimiter list
   */
  GetRString(dl1342, RFC1342_DELIMS);

  /*
   * prime initial line length
   */
  line = charsOnLine ? *charsOnLine : 0;

  /*
   * initialize token buffer
   */
  tokens[0].wordType = k1342End;
  *tokens[0].word = 0;

  /*
   * read first token
   */
  Next1342Word(&spot, source + len, tokens + 1, dl1342, &continueQuote,
               &encQuote);

  /*
   * main loop
   */
  for (curr = tokens + 1; curr->wordType != k1342End; curr = next) {
    next = RingNext(curr, tokens, 3);
    prev = RingNext(next, tokens, 3);
    Next1342Word(&spot, source + len, next, dl1342, &continueQuote, &encQuote);

    /*
     * whitespace between two encoded words gets elided.  Therefore, if we
     * are looking at whitespace and both the previous and next words are
     * encoded, we must encode the current word.
     *
     * if either the previous or next words are not encoded, then we don't
     * need to do anything special
     */
    if (curr->wordType == k1342LWSP && prev->wordType == k1342Word &&
        next->wordType == k1342Word)
      curr->wordType = k1342Word;

    /*
     * if we are encoding, get with it
     */
    if (curr->wordType == k1342Word)
      Encode1342String(curr->word, tid);

    /*
     * worry about line wrapping
     */
    wrapped = (prev->wordType != k1342Plain || curr->wordType != k1342Plain) &&
              (lineLimit && line + (short)strlen((const char *)curr->word) >= lineLimit);
    if (wrapped) {
      size_t _nllen = strlen((const char *)nl);
      if (encodedLen + (long)_nllen + 1 > encodedCap) {
        char *_tmp; encodedCap += _nllen + 256;
        _tmp = realloc(encoded, encodedCap);
        if (!_tmp) goto fail; encoded = _tmp;
      }
      memcpy(encoded + encodedLen, nl, _nllen); encodedLen += _nllen;
      encoded[encodedLen++] = ' ';
      line = 1;
    }

    /*
     * if we are outputting an encoded word and the previous thing was an
     * encoded word, or if the previous thing was a regular word that did
     * NOT end with a space or a ')', then we must prepend a space to the
     * encoded word
     */
    { size_t _pwlen = strlen((const char *)prev->word); c = _pwlen ? prev->word[_pwlen - 1] : 0; }
    if (!wrapped && curr->wordType == k1342Word &&
        (prev->wordType == k1342Word ||
         prev->wordType == k1342Plain && c != ' ' && c != ')')) {
      { size_t _cwlen = strlen((const char *)curr->word);
        memmove(curr->word + 1, curr->word, _cwlen + 1);
        curr->word[0] = ' '; }
    }

    /*
     * if we are outputting an encoded word and the next thing is a
     * plain word, then we must append a space to the encoded word
     */
    c = next->word[0];
    if (curr->wordType == k1342Word && next->wordType == k1342Plain)
      PCatC(curr->word, ' ');

    /*
     * stick the word on the end
     */
    { size_t _cwlen2 = strlen((const char *)curr->word);
      if (encodedLen + (long)_cwlen2 + 1 > encodedCap) {
        char *_tmp; encodedCap += _cwlen2 + 256;
        _tmp = realloc(encoded, encodedCap);
        if (!_tmp) goto fail; encoded = _tmp;
      }
      memcpy(encoded + encodedLen, curr->word, _cwlen2);
      encodedLen += _cwlen2;
      line += _cwlen2; }
  }

  if (encoded) encoded[encodedLen] = '\0';
  if (outLen) *outLen = encodedLen;
  if (charsOnLine)
    *charsOnLine = line;
  return (encoded);

fail:
  free(encoded);
  return (NULL);
}

/************************************************************************
 * Encode1342String - encode a string in 1342-speak
 ************************************************************************/
void Encode1342String(char * s, short tid) {
  unsigned char encoded[256];
  unsigned char name[64];
  char *from, *to, *end;
  Byte c;

  /*
   * first, we translit to ISO-latin1
   */
  if (tid)
    TransLitRes(s, strlen((const char *)s), tid);

  /*
   * now, we encode
   */
  to = encoded;
  end = s + strlen((const char *)s);
  for (from = s; from < end; from++) {
    c = *from;
    if ('a' <= c && c <= 'z' || 'A' <= c && c <= 'Z' || '0' <= c && c <= '9')
      *to++ = c;
    else if (c == ' ')
      *to++ = '_';
    else {
      *to++ = '=';
      Bytes2Hex(&c, 1, to);
      to += 2;
    }
    if (to > encoded + sizeof(encoded) - 20)
      return; /* OVERFLOW */
  }

  *to = '\0';
  ComposeRString(s, RFC1342_FMT, SimpleNameCharset(name, tid), encoded);
}


/************************************************************************
 * SendPString - send a pascal string
 * TODO: convert to C strings when ComposeString/GetRString are ported
 ************************************************************************/
int SendPString(TransStream stream, char * string) {
  return (SendTrans(stream, string, strlen((const char *)string), NULL));
}

/************************************************************************
 * TimeStamp - put a time stamp on a message
 ************************************************************************/
void TimeStamp(TOCType * tocH, short sumNum, uint32_t when, long delta) {
  PtrTimeStamp(tocH->sums + sumNum, when, delta);
  (void)0; /* was  - no-op */
#ifdef NEVER
  CalcSumLengths(tocH, sumNum);
#endif
  InvalSum(tocH, sumNum);
  TOCSetDirty(tocH, true);
}

/************************************************************************
 * PtrTimeStamp - timestamp, but into a sum directly
 ************************************************************************/
void PtrTimeStamp(MSumPtr sum, uint32_t when, long delta) {
  sum->seconds = when;
  sum->origZone = delta / 60;
}

char * FormatZone(char * string, long delta) {
  bool neg = delta < 0;

  if (neg)
    delta *= -1;
  delta /= 60; /* minutes*/
  ComposeString(string, " %c%d%d%d%d", neg ? '-' : '+', delta / 600,
                (delta % 600) / 60, (delta % 60) / 10, delta % 10);
  return (string);
}

/************************************************************************
 * BufferSend - send a buffer of (possibly encoded) data
 *  encoder - function to call for encoding
 *	data - data to encode/send (or NULL to send remaining data and close)
 *	dataLen - length of data to encode/send
 ************************************************************************/
int BufferSend(TransStream stream, DecoderFunc *encoder, char *data,
                 long dataLen, bool text) {
  short err = noErr;
  static long used;
  long consumed;
  char *spot, *end;
  long bSize;
  long progBytes;

  if (EncoderGlobalsOldEncoder && EncoderGlobalsOldEncoder != encoder) {
    BufferSendRelease(stream);
    EncoderGlobalsOldEncoder = encoder;
  }

  /*
   * are we being fed data?
   */
  if (data) {
    /*
     * do we need to initialize?
     */
    if (!EncoderGlobalsBuffers[0]) {
      bSize = 3 * GetRLong(BUFFER_SIZE);
      while (!(EncoderGlobalsBuffers[0] = NuHTempOK(bSize)) && bSize > 256)
        bSize /= 2;
      if (!EncoderGlobalsBuffers[0]) {
        WarnUser(MEM_ERR, err = MemError());
        return (err);
      }
#ifdef DEBUG
      if (!BUG5)
#endif
        if (AsyncSendTrans)
          EncoderGlobalsBuffers[1] = NuHTempOK(bSize);
      EncoderGlobalsBuffer = EncoderGlobalsBuffers[0];
      if (encoder)
        err = (*encoder)(kDecodeInit, &EncoderGlobalsPb);
      EncoderGlobalsPb.text = text;
      if (err) {
        BufferSend(stream, encoder, NULL, 0, 0);
        return (err);
      }
      used = 0;
    }

    bSize = GetHandleSize_(EncoderGlobalsBuffer);

    if (!DontTranslate && Flatten)
      for (spot = data, end = data + dataLen; spot < end; spot++)
        *spot = Flatten[*spot];

    if (!DontTranslate && TransOut)
      for (spot = data, end = data + dataLen; spot < end; spot++)
        *spot = TransOut[*spot];

    while (dataLen) {
      progBytes = consumed = 0;

      /*
       * encode?
       */
      if (encoder) {
        if ((!used || bSize - used > 4 * dataLen)) {
          EncoderGlobalsPb.output = (unsigned char *)EncoderGlobalsBuffer + used;
          consumed = EncoderGlobalsPb.inlen = MIN((bSize - used) / 4, dataLen);
          EncoderGlobalsPb.input = data;
          err = (*encoder)(kDecodeData, &EncoderGlobalsPb);
          if (err)
            return (err);
          used += EncoderGlobalsPb.outlen;
          progBytes = EncoderGlobalsPb.outlen;
        }
      }
      /*
       * no, just copy
       */
      else {
        consumed = MIN(bSize - used, dataLen);
        memmove((unsigned char *)EncoderGlobalsBuffer + used, data, consumed);
        used += consumed;
      }

      /*
       * send
       */
      if (consumed < dataLen) {
        if (AsyncSendTrans && EncoderGlobalsBuffers[1]) {
          err = AsyncSendTrans(stream, EncoderGlobalsBuffer, used);
          EncoderGlobalsBuffer =
              EncoderGlobalsBuffer == EncoderGlobalsBuffers[0]
                  ? EncoderGlobalsBuffers[1]
                  : EncoderGlobalsBuffers[0];
        } else {
          err = SendTrans(stream, EncoderGlobalsBuffer, used, NULL);
        }
        if (err)
          return (err);
        used = 0;
      }

      dataLen -= consumed;
      data += consumed;
      if (progBytes)
        ByteProgress(NULL, -progBytes, 0);
    }
  }

  /*
   * no data; clear out encoder
   */
  else {
    if (AsyncSendTrans)
      err = AsyncSendTrans(stream, NULL, -1);
    if (!err && used && !dataLen && EncoderGlobalsBuffer)
      err = SendTrans(stream, EncoderGlobalsBuffer, used, NULL);

    if (encoder && EncoderGlobalsPb.refCon) {
      if (!err) {
        EncoderGlobalsPb.output = EncoderGlobalsBuffer;
        err = (*encoder)(kDecodeDone, &EncoderGlobalsPb);
        if (!err && EncoderGlobalsPb.outlen && !dataLen)
          err = SendTrans(stream, EncoderGlobalsBuffer,
                          EncoderGlobalsPb.outlen, NULL);
      }
      (*encoder)(kDecodeDispose, &EncoderGlobalsPb);
    }
    WriteZero(&EncoderGlobalsPb, sizeof(EncoderGlobalsPb));
    used = 0;
    free(EncoderGlobalsBuffers[0]);
    free(EncoderGlobalsBuffers[1]);
    EncoderGlobalsBuffer = NULL;
  }

  return (err);
}

/************************************************************************
 * GetIndAttachment - get a particular attacment
 ************************************************************************/
int GetIndAttachment(MessHandle messH, short index, char * spec,
                       HSPtr where) {
  int err = 1;
  void *text = NULL;
  HeadSpec hs;

  if (CompHeadFind(messH, ATTACH_HEAD, &hs)) {
    if (!(err = PETEGetRawText(PETE, TheBody, &text)))
      err = GetIndAttachmentLo(text, index, spec, where, &hs);
  }
  return (err);
}

/************************************************************************
 * GetIndAttachmentLo - get a particular attacment
 ************************************************************************/
int GetIndAttachmentLo(void *text, short index, char * spec, HSPtr where,
                         HeadSpec *hs) {
  short colons[4];
  short onColon;
  unsigned char name[32];
  unsigned char volName[32];
  long id;
  int onChar;
  int err;

  onColon = 0;
  for (onChar = hs->value; onChar < hs->stop; onChar++)
    if (((char *)text)[onChar] == ':') {
      colons[onColon] = onChar;
      if (++onColon == sizeof(colons) / sizeof(short)) {
        index--;
        onColon = 0;
        if (!index) {
          { short _len = colons[1] - colons[0];
          memmove(volName, (char *)text + colons[0] + 1, _len);
          volName[_len] = 0; }
          id = Atoi((char *)text + colons[1] + 1);
          { short _len = colons[3] - colons[2] - 1;
          memmove(name, (char *)text + colons[2] + 1, _len);
          name[_len] = 0; }
          if (where) {
            where->start = where->value = colons[0];
            where->stop = colons[3] + 1;
          }
          if (err = spec_for(Root.path, (const char *)name, spec)) {
            // This file probably wasn't found. Go ahead and
            // build spec manually. May need name later on.
            spec_make(Root.path, (const char *)name, spec);
          }
          hs->value = onChar + 1;
          return err;
        }
      }
    }
  hs->value = onChar;
  return (1); /* no more files */
}

/************************************************************************
 * PriorityHeader: Build a priority header
 ************************************************************************/
char *PriorityHeader(char *buffer, Byte priority) {
  return (ComposeRString(buffer, PRIORITY_FMT, HEADER_STRN + PRIORITY_HEAD,
                         priority, PRIOR_STRN + priority));
}

/************************************************************************
 * GetReply - get a reply to an SMTP command
 ************************************************************************/
int GetReplyLo(TransStream stream, char *buffer, int size,
               AccuPtr bufAcc, bool verbose, bool isEhlo) {
  long rSize;
  char scratch[128];
  char *cp;
  short err;
  unsigned char tempBuffer[256];

  // if a buffer was not passed in...
  if (!buffer) {
    buffer = tempBuffer;
    size = 255;
  }

  if (PrefIsSet(PREF_POP_SEND)) {
    rSize = size;
    err = POPCmdGetReply(stream, -1, "", buffer, &rSize);
    if (err)
      return (602);
    if (*buffer == '+')
      memcpy(buffer, "200 ", 4);
    else
      memcpy(buffer, "550 ", 4);
    cp = buffer;
  } else
    do {
      bool partialBuffer;

      rSize = size;
      if (bufAcc)
        bufAcc->offset = 0;
      if (CommandPeriod || (sErr = RecvLine(stream, buffer, &rSize)))
        return (602); /* error receiving */
      if (bufAcc)
        AccuAddPtr(bufAcc, buffer, rSize);
      partialBuffer = rSize && buffer[rSize - 1] != '\r';

      // if we've been given an accumulator,
      // accumulate the whole response if this one is incomplete
      if (bufAcc && partialBuffer) {
        while (buffer[rSize - 1] != '\r') {
          rSize = size;
          sErr = RecvLine(stream, buffer, &rSize);
          if (CommandPeriod || sErr)
            return 602;
          AccuAddPtr(bufAcc, buffer, rSize);
        }

        // pretend that what we got was just that first bufferful
        rSize = MIN(size, bufAcc->offset);
        memmove(buffer, bufAcc->data, rSize);
      } else if (partialBuffer) {
        // Ick - not all of the reply will fit in the buffer, and we
        // weren't given an accumulator to keep it in.  Throw stuff away until
        // we find the end of the line
        unsigned char dumpBuffer[64];
        long dumpSize;

        do {
          dumpSize = sizeof(dumpBuffer);
          sErr = RecvLine(stream, dumpBuffer, &dumpSize);
          if (CommandPeriod || sErr)
            return 602;
          g_debug("log: %d", DISCARD_LOG_FMT, dumpBuffer, dumpSize);
        } while (dumpBuffer[dumpSize - 1] != '\r');
      }

      if (verbose) {
        { size_t _cplen = MIN(127, rSize);
          strncpy((char *)scratch, (const char *)buffer, _cplen);
          scratch[_cplen] = '\0'; }
        ProgressMessage(kpMessage, scratch);
      }
      for (cp = buffer; cp < buffer + rSize && (*cp < ' ' || *cp > '~'); cp++)
        ;
      if (isEhlo && cp[0] == '2' && cp[1] == '5' && cp[2] == '0')
        EhloLine(buffer, rSize);
      rSize -= cp - buffer;
    } while (rSize < 3 || !isdigit(cp[0]) || !isdigit(cp[1]) ||
             !isdigit(cp[2]) || rSize > 3 && cp[3] == '-');
  cp[rSize] = 0;
  err = Atoi(cp);
  if (verbose && err > 399 && err < 600)
    SMTPCmdError(0, NULL, buffer);
  if (err == 505 || err == 530)
    SetPref(PREF_SMTP_GAVE_530, YesStr);
  if (err == 452)
    err = 552; // Goddam stupid SMTP spec gave a 4xx series response
               // to a permanent error code
  return (err);
}

/************************************************************************
 * FlattenAndSpool - flatten and spool a movie
 * Changes the filespec passed to it!
 ************************************************************************/
int FlattenAndSpool(char * spec) {
  FSSpec tempSpec;
  int err = NewTempSpec(0, 0, spec_name(spec), &tempSpec);

  if (!err) {
    if (err = FlattenQTMovie(spec, &tempSpec))
      unlink(tempSpec);
    else {
      unlink(tempSpec);
    }
    // FSpKillRFork is no-op
  }
  return (err);
}

/**********************************************************************
 * FlattenQTMovie - put movie in data fork of new file
 **********************************************************************/
int FlattenQTMovie(char * inSpec, char * outSpec) {
#ifdef HAVE_QUICKTIME
  short movieResFile;
  Movie theMovie, tempMovie;
  int err = noErr;

  if (!HaveQuickTime(0x0100))
    return cantOpenHandler; //	Don't have QuickTime

  if (!QTMoviesInited) {
    if (!EnterMovies()) //	Need to do this once
      QTMoviesInited = true;
    err = GetMoviesError();
  }

  if (QTMoviesInited &&
      !(err = OpenMovieFile(inSpec, &movieResFile, fsRdPerm))) {
    short movieResID = 0; /* want first movie */
    Boolean wasChanged;

    err = NewMovieFromFile(&theMovie, movieResFile, &movieResID, NULL, 0,
                           &wasChanged);
    CloseMovieFile(movieResFile);

    if (!err) {
      tempMovie = FlattenMovieData(theMovie, flattenAddMovieToDataFork, outSpec,
                                   FileCreatorOf(inSpec), 0,
                                   createMovieFileDeleteCurFile);
      err = GetMoviesError();
      if (tempMovie)
        DisposeMovie(tempMovie);
      DisposeMovie(theMovie);
    }
  }

  return err;
#else
  (void)inSpec;
  (void)outSpec;
  return -4 /* unimpErr */; /* QuickTime movie flattening not available in GTK
                               port */
#endif /* HAVE_QUICKTIME */
}

char * R822Date(char * date, long seconds) {
  long delta = ZoneSecs();
  bool negative;
  struct tm tmBuf;
  time_t t;

  if (delta == -1) {
    *date = 0;
    return date;
  }
  if (seconds) {
    t = (time_t)(seconds + delta);
    localtime_r(&t, &tmBuf);
  } else {
    t = time(NULL);
    localtime_r(&t, &tmBuf);
  }
  if ((negative = (delta < 0)))
    delta *= -1;
  delta /= 60;
  return ComposeRString(date, R822_DATE_FMT,
                        WEEKDAY_STRN + tmBuf.tm_wday + 1,
                        tmBuf.tm_mday,
                        MONTH_STRN + tmBuf.tm_mon + 1,
                        tmBuf.tm_year + 1900,
                        tmBuf.tm_hour / 10, tmBuf.tm_hour % 10,
                        tmBuf.tm_min / 10, tmBuf.tm_min % 10,
                        tmBuf.tm_sec / 10, tmBuf.tm_sec % 10,
                        negative ? '-' : '+',
                        delta / 600, (delta % 600) / 60,
                        (delta % 60) / 10, delta % 10);
}
