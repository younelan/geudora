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

/*
 * address.c — RFC 822 address parser
 * Ported from MAC624/address.c to pure C.
 *
 * Returns char** (NULL-terminated string array, like argv).
 * Free with g_strfreev().
 */

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <gtk/gtk.h>
#include "mydefs.h"
#include "mailbox.h"
#include "features.h"
#include "gtk_prefs.h"
#include "StringUtil.h"
#include "prefdefs.h"

/* Forward declarations */
static bool addr_is_fcc(const char *addr);
static bool addr_is_newsgroup(const char *addr);

#define AddrWhite(c) ((unsigned char)(c) <= ' ')

typedef enum {
  Regular, Comma, lParen, rParen, lAngle, rAngle,
  lBrak, rBrak, dQuote, Colon, Semicolon, aDone
} AddressCharEnum;

typedef enum {
  sNoChange = -1,
  sPlain, sParen, sAngle, sBrak, sQuot, sTrail, sTDone,
  sError, sToken, sBrakE, sQuotE, incPar, decPar, sColon, sSem, sTrailB
} AddressStateEnum;

static char AddrStateTable[sTDone + 1][aDone + 1] = {
/*           .       ,       (       )       <       >       [       ]       "       :       ;      done */
/* sPlain */ {-1,  sToken, incPar, sError, sAngle, sError, sBrak,  sError, sQuot,  sColon, sSem,  sToken},
/* sParen */ {-1,  -1,     incPar, decPar, -1,     -1,     -1,     -1,     -1,     -1,     -1,    sError},
/* sAngle */ {-1,  -1,     incPar, decPar, sError, sTrailB,sBrak,  sError, sQuot,  -1,     -1,    sError},
/* sBrak  */ {-1,  -1,     -1,     -1,     -1,     -1,     sError, sBrakE, -1,     -1,     -1,    sError},
/* sQuot  */ {-1,  -1,     -1,     -1,     -1,     -1,     -1,     -1,     sQuotE, -1,     -1,    sError},
/* sTrail */ {-1,  sToken, incPar, decPar, sError, sError, -1,     -1,     sQuot,  -1,     sSem,  sToken},
/* sTDone */ {-1,  sToken, incPar, sError, sAngle, sError, sBrak,  sError, sQuot,  sColon, sSem,  sToken}
};

/*
 * Helper: grow a GPtrArray of strings, return it as char** on finish.
 */

/************************************************************************
 * SuckPtrAddresses - parse RFC 822 addresses from raw text
 *
 * Returns 0 on success, non-zero on error.
 * *addresses is set to a NULL-terminated char** array (free with g_strfreev).
 ************************************************************************/
int SuckPtrAddresses(char ***addresses, const char *text, long size,
                     bool wantComments, bool wantErrors,
                     bool wantAutoQual, void *addrSpots_unused)
{
  GPtrArray *result = g_ptr_array_new();
  const unsigned char *spot;
  short paren = 0;
  AddressStateEnum oldState, state, nextState;
  unsigned char c;
  AddressCharEnum cClass;
  char addrBuf[256];
  char *ap;
  bool sawAt;
  char *addrEnd;
  char autoQual[128];
  int err = 0;
  unsigned char lastC = 0;

  (void)addrSpots_unused; /* Not used in port */

#define AddrBufSize  ((int)sizeof(addrBuf))
#define AddrFull     (ap - addrBuf >= AddrBufSize - 2)
#define AddrEmpty    (ap == addrBuf)
#define AddrChar(ch) do { \
    if ((wantComments && (ch) != '\015' || !AddrWhite(ch)) && oldState != sTDone) { \
      *ap++ = (ch); \
      if (!AddrWhite(ch)) addrEnd = ap; \
      if ((ch) == '@') sawAt = true; \
    } \
  } while (0)
#define CmmntChar(ch) do { if (wantComments && oldState < sTDone && (ch) != '\015') *ap++ = (ch); } while (0)
#define AddrAny(ch)   do { if (oldState < sTDone) *ap++ = (ch); } while (0)
#define RestartAddr() do { ap = addrBuf; sawAt = false; addrEnd = NULL; lastC = 0; } while (0)

  *addresses = NULL;

  autoQual[0] = '\0';
  if (wantAutoQual) {
    gchar *aq = prefs_get_string("Hosts", "auto_qualify", "");
    if (aq && aq[0]) {
      if (aq[0] == '@')
        g_strlcpy(autoQual, aq, sizeof(autoQual));
      else
        snprintf(autoQual, sizeof(autoQual), "@%s", aq);
    }
    g_free(aq);
  }

  oldState = state = sPlain;
  spot = (const unsigned char *)text;
  while (spot < (const unsigned char *)text + size && AddrWhite(*spot)) spot++;

  RestartAddr();
  do {
    if (spot >= (const unsigned char *)text + size)
      cClass = aDone;
    else {
      c = *spot++;
      switch (c) {
        case ',':  cClass = Comma;     break;
        case '(':  cClass = lParen;    break;
        case ')':  cClass = rParen;    break;
        case '[':  cClass = lBrak;     break;
        case ']':  cClass = rBrak;     break;
        case '<':  cClass = lAngle;    break;
        case '>':  cClass = rAngle;    break;
        case '"':  cClass = dQuote;    break;
        case ':':
          if ((spot > (const unsigned char *)text + 1 && spot[-2] == ':') ||
              (spot < (const unsigned char *)text + size && spot[0] == ':'))
            cClass = Regular;
          else
            cClass = Colon;
          break;
        case ';':  cClass = Semicolon; break;
        default:   cClass = Regular;   break;
      }
      if (lastC == '\\') cClass = Regular;
      lastC = c;
    }
    nextState = AddrStateTable[state][cClass];
rescan:
    if (nextState == sNoChange) nextState = state;
    switch (nextState) {
      case sPlain:
        AddrChar(c);
        break;
      case sAngle:
        if (state == sAngle || wantComments)
          AddrChar(c);
        else
          RestartAddr();
        break;
      case sColon:
        AddrChar(c);
        nextState = sToken;
        goto rescan;
      case sBrak:
        if (state != sBrak) oldState = state;
        AddrAny(c);
        break;
      case sQuot:
        if (state != sQuot) oldState = state;
        if (oldState == sTrail) CmmntChar(c); else AddrAny(c);
        break;
      case sTDone:
        break;
      case sError:
        err = -1;
        goto parsePunt;
      case sSem:
      case sToken:
        /* Strip trailing whitespace */
        while (!AddrEmpty && AddrWhite(ap[-1])) ap--;
        *ap = '\0';

        if (addrBuf[0]) {
          /* Auto-qualify: insert @domain at addrEnd */
          if (wantAutoQual && autoQual[0] && !sawAt && addrEnd && addrBuf[0] != ';') {
            if (!addr_is_fcc(addrBuf) && !addr_is_newsgroup(addrBuf)) {
              size_t pos = addrEnd - addrBuf;
              size_t aqLen = strlen(autoQual);
              size_t curLen = strlen(addrBuf);
              if (pos + aqLen + (curLen - pos) < (size_t)AddrBufSize - 1) {
                memmove(addrBuf + pos + aqLen, addrBuf + pos, curLen - pos + 1);
                memcpy(addrBuf + pos, autoQual, aqLen);
              }
            }
          }
          /* Trim whitespace */
          {
            char *s = addrBuf;
            char *e = s + strlen(s);
            while (e > s && AddrWhite((unsigned char)e[-1])) e--;
            *e = '\0';
            while (*s && AddrWhite((unsigned char)*s)) s++;
            if (s > addrBuf) memmove(addrBuf, s, strlen(s) + 1);
          }

          g_ptr_array_add(result, g_strdup(addrBuf));
        }
        RestartAddr();
        oldState = sPlain;
        if (nextState == sSem) {
          AddrChar(c);
          nextState = sTDone;
        }
        nextState = sPlain;
        while (spot < (const unsigned char *)text + size && AddrWhite(*spot)) spot++;
        break;
      case sTrailB:
        nextState = sTrail;
        CmmntChar('>');
        break;
      case sBrakE:
        nextState = oldState;
        AddrChar(c);
        break;
      case sQuotE:
        nextState = oldState;
        if (oldState == sTrail) CmmntChar(c); else AddrAny(c);
        break;
      case incPar:
        if (!paren++) oldState = state;
        nextState = sParen;
        CmmntChar('(');
        break;
      case decPar:
        if (!--paren)
          nextState = oldState;
        else
          nextState = sParen;
        CmmntChar(')');
        break;
      default:
        CmmntChar(c);
        break;
    }
    state = nextState;
  } while (cClass != aDone && !AddrFull);

parsePunt:
  if (err || nextState == sError || AddrFull) {
    g_ptr_array_free(result, TRUE);
    *addresses = NULL;
    if (!err) err = -1;
    return err;
  }

  /* NULL-terminate the array */
  g_ptr_array_add(result, NULL);
  *addresses = (char **)g_ptr_array_free(result, FALSE);
  return 0;

#undef AddrBufSize
#undef AddrFull
#undef AddrEmpty
#undef AddrChar
#undef CmmntChar
#undef AddrAny
#undef RestartAddr
}

/************************************************************************
 * SuckAddresses - parse RFC 822 addresses from a text buffer (Handle)
 *
 * Legacy wrapper: text is a Handle (char**), we dereference and call
 * SuckPtrAddresses. Callers should migrate to SuckPtrAddresses directly.
 ************************************************************************/
int SuckAddresses(char ***addresses, char **text,
                  bool wantComments, bool wantErrors,
                  bool wantAutoQual, void *spots)
{
  if (!text || !*text) {
    *addresses = NULL;
    return -1;
  }
  /* text is a Handle (char**) — *text is the C string data */
  return SuckPtrAddresses(addresses, *text, strlen(*text),
                          wantComments, wantErrors, wantAutoQual, spots);
}

/************************************************************************
 * ShortAddr - return just the short form of an address (user@host)
 * Input/output: C strings
 ************************************************************************/
char *ShortAddr(char *shortAddr, const char *longAddr)
{
  char **addrs = NULL;

  SuckPtrAddresses(&addrs, longAddr, strlen(longAddr),
                   false, false, true, NULL);

  if (addrs && addrs[0] && addrs[0][0])
    g_strlcpy(shortAddr, addrs[0], 256);
  else
    g_strlcpy(shortAddr, longAddr, 256);

  g_strfreev(addrs);
  return shortAddr;
}

/************************************************************************
 * CountAddresses - count addresses in a char** array
 ************************************************************************/
short CountAddresses(char **addresses, short atLeast)
{
  short count = 0;
  if (!addresses) return 0;
  for (int i = 0; addresses[i]; i++) {
    count++;
    if (atLeast && count >= atLeast) return count;
  }
  return count;
}

/************************************************************************
 * IsFCCAddr / IsNewsgroupAddr
 ************************************************************************/
static bool addr_is_fcc(const char *addr)
{
  return (addr && (unsigned char)addr[0] == 0xC7);
}

static bool addr_is_newsgroup(const char *addr)
{
  return (addr && (unsigned char)addr[0] == 0xC7);
}

bool IsFCCAddr(unsigned char *addr)
{
  return addr_is_fcc((const char *)addr);
}

bool IsNewsgroupAddr(unsigned char *addr)
{
  return addr_is_newsgroup((const char *)addr);
}

/************************************************************************
 * GetRealname - get the user's real name from preferences
 ************************************************************************/
unsigned char *GetRealname(unsigned char *name)
{
  if (!name) return NULL;
  gchar *rn = prefs_get_string(PREFS_GROUP_SENDING_MAIL, "real_name", "");
  g_strlcpy((char *)name, rn, 256);
  g_free(rn);
  return name[0] ? name : NULL;
}

/************************************************************************
 * GetReturnAddr - get the user's return address
 * All C strings, no Pascal, no Handles.
 ************************************************************************/
unsigned char *GetReturnAddr(unsigned char *addr, bool comments)
{
  char host[256];

  /* Try configured email address */
  gchar *ret = prefs_get_string(PREFS_GROUP_SENDING_MAIL, "email_address", "");
  g_strlcpy((char *)addr, ret, 256);
  g_free(ret);

  /* If empty, build from POP account: user@host */
  if (!addr[0]) {
    char user[256];
    extern void GetPOPInfo(void *u, void *h);
    GetPOPInfo(user, host);
    snprintf((char *)addr, 256, "%s@%s", user, host);
  }

  /* Validate with SuckPtrAddresses */
  char **canon = NULL;
  if (!SuckPtrAddresses(&canon, (const char *)addr, strlen((char *)addr),
                        comments, true, false, NULL)) {
    if (canon && canon[0] && canon[0][0]) {
      if (!comments && canon[0][0] != '<')
        snprintf((char *)addr, 256, "<%s>", canon[0]);
      else
        g_strlcpy((char *)addr, canon[0], 256);
    }
    g_strfreev(canon);
  }

  if (comments) {
    char rn[256];
    if (GetRealname((unsigned char *)rn) && rn[0]) {
      char buf[512];
      snprintf(buf, sizeof(buf), "\"%s\" <%s>", rn, (char *)addr);
      g_strlcpy((char *)addr, buf, 256);
    }
  }

  return addr;
}

/************************************************************************
 * GetReturnAddrC - convenience wrapper
 ************************************************************************/
unsigned char *GetReturnAddrC(unsigned char *addr)
{
  return GetReturnAddr(addr, false);
}

/************************************************************************
 * AutoCheckOK - check if automatic mail checking is allowed
 ************************************************************************/
bool AutoCheckOK(void)
{
  extern bool Offline;
  if (Offline) return false;
  return true;
}

/************************************************************************
 * GetTrashTOC - get the Trash mailbox TOC
 ************************************************************************/
extern TOCType *GetSpecialTOC(short nameId);

TOCType *GetTrashTOC(void)
{
  return GetSpecialTOC(3 /* MBX_TRASH */);
}

/************************************************************************
 * IsMe - check if an address matches the current user
 ************************************************************************/
bool IsMe(unsigned char *address)
{
  char shortAddr[256], myAddr[256], myShort[256];

  if (!address || !address[0]) return false;

  ShortAddr(shortAddr, (const char *)address);
  GetReturnAddr((unsigned char *)myAddr, false);
  ShortAddr(myShort, myAddr);

  return (g_ascii_strcasecmp(shortAddr, myShort) == 0);
}
