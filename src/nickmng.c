#include "../gEditCtrl/geditctrl.h"
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

#include "nickmng.h"
#include "comp.h"
/* sendmail.h removed — crispy handles SMTP */
#include "Globals.h"
#include "StringDefs.h"
#include "StringUtil.h"
#include "fileutil.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "lineio.h"
#include "sort.h"
#include "util.h"
/* imapdownload.h removed — crispy_imap handles IMAP */
#include "threading.h"
#include <gtk/gtk.h>
#include "gtk_dialogs.h"
#include "macmbx.h"
#include "crispy_rfc822.h"
#include "macmbx_mailer.h"
#include "idle_scheduler.h"
#define FILE_NUM 2

// Forward declarations for functions implemented elsewhere
extern void SetPrefLong(short prefId, long value);
extern void SetResLoad(bool load);
extern int PeteGetTextAndSelection(void *pte, void **text, long *start, long *end);
extern void ReplyDefaults(short modifiers, bool *all, bool *self, bool *quote);
extern void *DoReplyMessage(void *win, bool all, bool self, bool f1, bool f2, int i,
                     bool f3, bool f4, bool f5);
/* JunkSetScore declaration removed — use macmbx_junk_set_score from macmbx.h */
extern void EnsureFromHash(MacmbxTOC * tocH, short sumNum);
extern void *DupHandle(void *h);
extern short ComposeStdAlert(int alertType, int msgResId, ...);
/* HGetState/HSetState provided by legacy_shim.h */
// ParseAttributeValuePair has 6 parameters in actual usage
unsigned char *ParseAttributeValuePair(unsigned char *ptr, long size,
                                       unsigned char **attr, long *attrLen,
                                       unsigned char **val, long *valLen) {
  return NULL;
}
unsigned char *ParseAttributeValuePairStr(unsigned char *ptr, long size,
                                          unsigned char *attr,
                                          unsigned char *val) {
  return NULL;
}
bool AddAttributeValuePair(void **h, unsigned char *attr, unsigned char *val,
                           long valLen) {
  return false;
}
long Munger(void **h, long offset, void *ptr1, long len1, void *ptr2,
            long len2) {
  return 0;
}
/* NumToString provided by legacy_shim.h as static inline */
/* GetPrefLong provided by legacy_shim.h */
/* SetPrefLong is implemented in gtk_dialogs.c */
/* SetResLoad is implemented in imapmailboxes.c */
long GetResourceSizeOnDisk(void **h) { return strlen((char *)h); }
void ReadPartialResource(void **h, long offset, void *dest,
                         long len) { /* stub */ }
long Random(void) { return random(); }
bool IsNickname(unsigned char *name, short which) { return false; }
void NewNick(void **addresses, int flags) { /* stub */ }
void NoteUser(int msgId, int arg) { /* stub - show message to user */ }
/* PeteGetTextAndSelection is implemented in gEditCtrl/pete_compat.c */
int HandToHand(void **h) {
  if (!h || !*h)
    return -1;
  void **copy = malloc(strlen((char *)h));
  if (!copy)
    return -1;
  memcpy(*copy, *h, strlen((char *)h));
  *h = *copy;
  return 0;
}
int HandAndHand(void **h1, void **h2) {
  if (!h1 || !*h1 || !h2 || !*h2)
    return -1;
  return (buf_append(*h2, NULL, *h1, strlen((char *)*h1)) != NULL) ? 0 : -1;
}
bool IsVCardAvailable(void) { return false; }
/* FindAnAttachment declared in message.h (7 params) */
bool IsVCardFile(char *spec) { return false; }
/* ReplyDefaults and DoReplyMessage are implemented in message.c */
/* GetAMessage declared in message.h */
/* IsWindowVisible provided by mailbox.h as static inline */
void ABClean(void) { /* stub */ }
void InvalCachedNicknameData(void) { /* stub */ }
int ParseURLPtr(unsigned char *url, long size, unsigned char *proto,
                unsigned char *host, char **query, long *queryLen) {
  return -1; /* stub */
}
void FixURLPtr(char *query, long *queryLen) { /* stub */ }
bool AliasWinIsOpen(void) { return false; /* stub */ }
void AliasWinRefresh(void) { /* stub */ }
int ExpandAliases(void **h, void *raw, int n, bool deep) {
  (void)raw;
  (void)n;
  (void)deep;
  return 0; /* stub */
}
void GetTime(unsigned long *time) { *time = 0; /* stub */ }
char *CopyBytesAndMovePtr(char *dest, void *src, long len) {
  memcpy(dest, src, len);
  return dest + len; /* stub */
}
void ReleaseResource(void **h) { /* stub */ }
bool ValidAddressBook(short which) { return true; /* stub */ }
/* EnsureFromHash is implemented in message.c */
bool ValidHash(unsigned long h) { return h != 0; /* stub */ }
/* JunkSetScore is implemented in junk.c */
bool AppearsInAliasFile(unsigned char *addr, short which) {
  return false; /* stub */
}
void NickSuggest(unsigned char *name, unsigned char *addr) { /* stub */ }
int NewNickLow(short which, unsigned char *name, void **addr, void **notes,
               int flags) {
  return 0; /* stub */
}
void **CreateSimpleNotes(unsigned char *name, unsigned char *addr) {
  return NULL; /* stub */
}
bool NickWinIsOpen(void) { return false; /* stub */ }
void ABTickleHardEnoughToMakeYouPuke(void) { /* stub */ }
int AddAddressHashUniq(void *hash, void *acc) { return 0; /* stub */ }
/* CountAddresses: real implementation in address.c */
/* DupHandle is implemented in imapmailboxes.c */
#define featureMultipleNicknameFiles 1
#define TAG_MAP_TYPE 'TGMP'
#define HEADER_STRN 1000
#define JUNK_BECAUSE_WHITE 1
#define nrNone 0

// Real implementations using existing code
bool GetPhotoSpec(char *spec, int ab, int nick, bool *alreadyExists) {
  // TODO: Implement using file utilities
  *alreadyExists = false;
  return false;
}

unsigned char *MakeFileURL(unsigned char *url, char *spec, int unused) {
  // Convert FSSpec path to file:// URL
  if (spec && url) {
    sprintf((char *)url + 1, "file://%s", spec);
    url[0] = strlen((char *)url + 1);
  }
  return url;
}

/* ComposeStdAlert is implemented in gtk_dialogs.c */

// Constants
#define MINI_MASK 0
#define MAILBOX_TYPE 'TOC '
#define readErr (-19) // Mac file read error


// Note: buf_append is already defined in fileutil.c and returns Handle (void
// **) The macro buf_append wraps it. No need to redefine here.

// Helper for ContainsMultipleAddresses - checks if char** array has multiple addresses
static inline bool ContainsMultipleAddressesHelper(char **addrs) {
  if (!addrs || !addrs[0])
    return false;
  return addrs[1] != NULL;
}

// Redefine the macro to use our helper
#undef ContainsMultipleAddresses
#define ContainsMultipleAddresses(addrs)                                       \
  ContainsMultipleAddressesHelper(addrs)

// Mac type compatibility
typedef long Size;
typedef unsigned char Str32[33];

// Constants
#define ktFlatten 1

// GTK replacement for Mac AlertStr
void AlertStr(short alertId, short type, const char *str) {
  GtkWidget *dialog;
  GtkAlertDialog *alert;
  GtkMessageType msgType = GTK_MESSAGE_WARNING;

  // Convert Mac alert type to GTK message type
  const char *title = "Eudora";
  if (type == 2)
    title = "Warning"; // Caution
  else if (type == 0)
    title = "Information"; // Note
  else if (type == 1)
    title = "Error"; // Stop

  // Copy C string
  char message[256];
  if (str && str[0] != '\0') {
    strcpy(message, (const char *)str);
  } else {
    strcpy(message, "Alert");
  }

  // GTK4 uses GtkAlertDialog instead of deprecated GtkMessageDialog
  alert = gtk_alert_dialog_new("%s", message);
  gtk_alert_dialog_set_modal(alert, TRUE);

  // For now, just show synchronously (in real app would use async callback)
  // This is a simplified version - full implementation would need proper event
  // loop
  g_print("Alert [%s]: %s\n", title, message);

  g_object_unref(alert);
}

void SetHandleBig(void **h, long size) { h = realloc(h, size); }

// Mac type compatibility
typedef long Size;

// Alert constants
#define OK_ALRT 1000
#define Caution 2

// Creator code for Eudora
#define CREATOR 'CSOm'

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */
/************************************************************************
 * routines to manage the nickname list
 ************************************************************************/
#pragma segment NickMng

/************************************************************************
 * 7/17/96	ALB
 *		Performance update for very large nickname files.
 *		- Each nickname is no longer stored in a separate handle. They
 *			are all stored in one handle with an offset to each one
 *		- Addresses, notes, and view data are no longer kept in a
 *separate handle
 ************************************************************************/

/************************************************************************
 * The aliases list has the following structure:
 * <length byte>name-of-alias<2 length bytes>expansion-of-alias...
 * The aliases file contains lines of the form:
 * "alias" name-of-alias expansion of alias<newline>
 * Newlines may be escaped with "\".
 * Lines not beginning with "alias" will be ignored
 ************************************************************************/

/************************************************************************
 * private functions
 ************************************************************************/

typedef struct {
  char name[32];
  long offset;
} AliasSortType, *AliasSortPtr, *AliasSortHandle;

typedef struct {
  long hashName;
  long hashAddress;
  //	long	createDate;		(eventually)
  //	long	modDate;			(eventually)
  //	long	usageDate;		(eventually)
  long cacheDate;
  long addressOffset;
  long notesOffset;
  NicknameFlagsType flags; // New for 5.0
} NickTOCStruct;

typedef struct {
  long offset;
  short nickIndex;
} NickOffSetSortType;

bool NeatenLine(char * line, long *len);
void CheckForNicknameBogosity(short which);
bool SaveIndNickFile(short which, bool saveChangeBits);
bool SaveFileFast(short which, bool saveChangeBits);
int NickOffsetCompare(NickOffSetSortType *n1, NickOffSetSortType *n2);
void NickOffsetSwap(NickOffSetSortType *n1, NickOffSetSortType *n2);
char * StripQualifiersAndHonorifics(char * name, char * strippedName);
int AddHandleToAddressHashes(TextAddrHandle sourceHandle, AccuPtr a);
int SortAddrNameCompare(char * *s1, char * *s2);

#define This Aliases[which]

short TotalNumOfNicks;

int ReadNicknames(short which);
bool SplitNicknames(short ab);
int KillNickTOC(char * spec);
int ReadNickTOC(short which);
int WriteNickTOC(short which);
bool NickFileOkForFastSave(short which);
void SaveDirtyPictures(short ab);
long NickMatchFoundLo(NickStructHandle theNicknames, long theNicknamesLen,
                      long hashName, char * theName, short which);

/************************************************************************
 * ReadNicknames - read the list of aliases
 *  Can be called either to read the nicknames initially or simply to make
 *  sure they're all in memory
 ************************************************************************/
int ReadNicknames(short which) {
  char **shortAddress;
  FSSpec spec;
  int err = 0;
  char line[256];
  short type;
  bool exLine = false;
  long expOffset = 0;
  long len;
  char aliasCmd[32], noteCmd[32];
  char currentCmd[32], tempName[32];
  LineIOD lid;
  long i, count;
  long currNickIndex;
  long hashName;
  bool doingAlias, doingNote;
  int theMemErr = 0;
  long nameOffset = 0;
  Accumulator nameAcc, lineAcc;
  /* Binary NickStruct array — grown directly, not via Accumulator */
  NickStructHandle dataArr = This.theData;
  int dataCount = This.theDataCount;
  char lookingFor;
  long firstOffset;
  long lineOffset;
  long curOffset;
  bool firstRead =
      This.hNames == nil; // are we reading for the first time?  (or filling in)
  bool lastExLine;

  /*
   * Setup section
   */
  Zero(nameAcc);
  Zero(lineAcc);
  // put existing name buffer into accumulator for growing
  nameAcc.data = This.hNames;
  if (nameAcc.data)
    nameAcc.size = nameAcc.offset = This.hNamesSize;

  // Find the file, open it for reading, get the command names
  g_strlcpy(spec, This.spec, sizeof(spec));
  This.collapsed = FindSTRNIndex(NickFileCollapseStrn, spec_name(spec)) > 0;

  GetRString(aliasCmd, ALIAS_CMD);
  GetRString(noteCmd, NOTE_CMD);
  if (err = FSpOpenLine(&spec, O_RDONLY, &lid))
    return (err == ENOENT ? 0
                          : FileSystemError(OPEN_ALIAS, spec_name(spec), err));

  // accumulator to hold the line we're working on
  if (theMemErr = AccuInit(&lineAcc))
    goto hitMemError;

  /*
   * the main reading loop; read lines until no more
   */
  curOffset = firstOffset = lastExLine = 0;
  while ((type = GetLine(line, sizeof(line), &len, &lid)) > 0) {
    lastExLine = exLine;
    lineOffset = curOffset; // offset of beginning of line
    curOffset += len;       // this is the offset of the next line
    exLine = line[len - 1] != '\015';

    // Record the offset where we first start collecting data
    if (lineAcc.offset == 0)
      firstOffset = lineOffset;

    // if the last line was escaped or we're looking at a partial line, just
    // accumulate
    if (lastExLine ||
        exLine) // If the line was escaped or we're in the middle of a line
    {
      // first, fix up the line
      exLine = NeatenLine(line, &len) || exLine;

      // if the previous line was escaped and this line doesn't begin
      // with a space, add one now
      if (lastExLine && type != LINE_MIDDLE && !issep(*line))
        if (theMemErr = AccuAddChar(&lineAcc, ' '))
          goto hitMemError;

      // if this line is further extended, append it now
      // if it's not, it will get appended inside the next if
      if (exLine && (theMemErr = AccuAddPtr(&lineAcc, line, len)))
        goto hitMemError;
    }

    if (!exLine) {
      // Previous line was not escaped.  Is this one?
      exLine = NeatenLine(line, &len) || exLine;

      // and append the current line to the line accumulator
      if (theMemErr = AccuAddPtr(&lineAcc, line, len))
        goto hitMemError;

      // if the line was escaped, just go round again; we want to deal with
      // full lines only
      if (exLine)
        continue;

      // ok, now we know that we have a full line
      // if not empty, do parsing and add nickname here
      if (lineAcc.offset) {
        char *lineData = lineAcc.data;
        // first string of non-space characters is the command name
        for (i = 0; i < sizeof(currentCmd) - 1; i++)
          if (lineData[i] == ' ')
            break;
        { size_t _mpl = ( i); memcpy(currentCmd,  lineData, _mpl); ((char*)(currentCmd))[_mpl] = '\0'; }

        /*
         * find the nickname
         */
        // first, skip over spaces
        while (i < lineAcc.offset && lineData[i] == ' ')
          i++;

        // if the character we found is not a quote, then back up one,
        // because we want i to be the position just BEFORE the nickname
        if (lineData[i] != '"')
          i--;

        // If we found a quote, we'll be looking for a quote to finish the
        // nickname
        lookingFor = lineData[i];
        for (count = i + 1; count < lineAcc.offset; count++) {
          if (lineData[count] == lookingFor)
            break;
        }
        // Eureka!  Copy the name into a pascal string and find its hash
        { size_t _mpl = ( count - i - 1); memcpy(tempName,  lineData + i + 1, _mpl); ((char*)(tempName))[_mpl] = '\0'; }
        SanitizeFN(tempName, tempName, NICK_STORED_BAD_CHAR,
                   NICK_STORED_REP_CHAR, false);
        hashName = NickHash(tempName);
        if (lookingFor == '"')
          count++; // skip the quote later when looking
                   // at the body of the line

        /*
         * process the contents
         */
        // Is this an alias command or a note command?
        doingAlias = StringSame(aliasCmd, currentCmd);
        doingNote = doingAlias ? false : StringSame(noteCmd, currentCmd);

        if ((doingAlias || doingNote) &&
            *tempName) // We have a valid command and a valid nickname
        {
          // Add to toc if necessary
          if ((currNickIndex = NickMatchFoundLo(dataArr, dataCount * sizeof(NickStruct),
                                                hashName, tempName, which)) <
              0) // Scan list for nickname
          {
            ASSERT(firstRead);
            if (firstRead) {
              // Nickname doesn't exist...create new one and clear it out
              // If it doesn't exist, add it
              NickStruct nickInfo;

              // Append the name
              if (theMemErr = AccuAddPtr(&nameAcc, &tempName, *tempName + 1))
                goto hitMemError;

              // Set nickname info
              nickInfo.hashName = hashName;
              nickInfo.hashAddress = -1;
              nickInfo.addressesDirty = false;
              nickInfo.notesDirty = false;
              nickInfo.pornography = false;
              nickInfo.deleted = false;
              nickInfo.group = false;
              nickInfo.theAddresses = nil;
              nickInfo.theNotes = nil;
              nickInfo.nameTOCOffset = nameOffset;
              nameOffset += *tempName + 1;

              if (doingAlias) // Command is alias
              {
                // Put offset of address in file into nickname
                nickInfo.addressOffset = firstOffset + 1;
                // Initialize notes offset to something not valid
                nickInfo.notesOffset = -1;

                // Hash the addresses in semi-complicated fashion
                shortAddress = nil;
                // As a last check... see if the address is already present in a
                // nickname file
                if (!SuckPtrAddresses(
                        &shortAddress, (char *)(lineAcc.data) + count + 1,
                        lineAcc.offset - count - 1, false, true, false, nil))
                  nickInfo.hashAddress = NickHashString(shortAddress[0]);

                nickInfo.group = ContainsMultipleAddresses(shortAddress);

                g_strfreev(shortAddress); shortAddress = NULL;
              }
              if (doingNote) // Command is note
              {
                // Put offset of notes in file into nickname
                nickInfo.notesOffset = firstOffset + 1;
                // Initialize address offset to something not valid
                nickInfo.addressOffset = -1;
              }

              //	Add nickname to array
              { NickStructHandle _p = realloc(dataArr, (dataCount + 1) * sizeof(NickStruct));
                if (!_p) { theMemErr = ENOMEM; goto hitMemError; }
                dataArr = _p;
                dataArr[dataCount] = nickInfo;
                dataCount++;
              }
              currNickIndex = dataCount - 1;
            }
          }

          // fill in data if necessary
          if (This.theData) {
            if (doingAlias &&
                (This.theData[currNickIndex].addressOffset == -1)) {
              // Put offset of address in file into nickname
              This.theData[currNickIndex].addressOffset = firstOffset + 1;

              // trim off the command and alias
              memmove(lineAcc.data, lineAcc.data + count + 1, lineAcc.offset - count - 1);
              lineAcc.offset -= count + 1;
              AccuTrim(&lineAcc);

              // And fill in the data
              This.theData[currNickIndex].theAddresses = lineAcc.data;

              Zero(lineAcc);
            }
            if (doingNote &&
                (This.theData[currNickIndex].notesOffset == -1)) {
              // Put offset of notes in file into nickname
              This.theData[currNickIndex].notesOffset = firstOffset + 1;

              // trim off the command and alias
              memmove(lineAcc.data, lineAcc.data + count + 1, lineAcc.offset - count - 1);
              lineAcc.offset -= count + 1;
              AccuTrim(&lineAcc);

              // And fill in the data
              This.theData[currNickIndex].theNotes = lineAcc.data;
              Zero(lineAcc);
            }
          }
        }
        lineAcc.offset = 0;
      }
    }
  }

  /*
   * cleanup
   */

  // close out a little stuff
  free(lineAcc.data); lineAcc.data = NULL; lineAcc.offset = lineAcc.size = 0;
  CloseLine(&lid);

  // report any errors
  if (type || err) {
    free(dataArr); dataArr = NULL; dataCount = 0;
    free(nameAcc.data); nameAcc.data = NULL; nameAcc.offset = nameAcc.size = 0;
    return (err ? WarnUser(ALLO_ALIAS, err)
                : FileSystemError(READ_ALIAS, spec_name(spec), type));
  }

  // Success!! install the toc and names handles
  if (firstRead) {
    AccuTrim(&nameAcc);
    This.hNames = nameAcc.data;
    This.hNamesSize = nameAcc.offset;
    This.theData = dataArr;
    This.theDataCount = dataCount;
    CheckForNicknameBogosity(which);
  }

  return (0);

hitMemError:
  err = WarnUser(ALLO_ALIAS, theMemErr);
  CloseLine(&lid);
  free(lineAcc.data); lineAcc.data = NULL; lineAcc.offset = lineAcc.size = 0;
  // kill these only on first read; on reread leave existing handles alone!
  if (firstRead) {
    free(dataArr); dataArr = NULL; dataCount = 0;
    free(nameAcc.data); nameAcc.data = NULL; nameAcc.offset = nameAcc.size = 0;
  }

  return (err);
}

void CheckForNicknameBogosity(short which)

{
  char beautifulName[256];
  short count, index;
  bool bogus;

  bogus = false;
  count =
      This.theDataCount;
  for (index = 0; !bogus && index < count; ++index) {
    crispy_rfc822_beautify_from((char *)GetNicknameNamePStr(which, index, beautifulName));
    if (!This.theData[index].deleted &&
        This.theData[index].hashName != NickHash(beautifulName))
      bogus = true;
  }
  This.containsBogusNicks = bogus;
}

/************************************************************************
 * NickMatchFound - find a given nickname in a nickname structure,
 * i.e. a nickname file
 * //SD - I rearranged things a bit for efficiency
 ************************************************************************/
long NickMatchFound(NickStructHandle theNicknames, int nickCount, long hashName,
                    char *theName, short which) {
  return (NickMatchFoundLo(theNicknames, nickCount * sizeof(NickStruct), hashName,
                           theName, which));
}

/************************************************************************
 * NickMatchFoundLo - find a given nickname in a nickname structure,
 * i.e. a nickname file
 * //SD - I rearranged things a bit for efficiency
 ************************************************************************/
long NickMatchFoundLo(NickStructHandle theNicknames, long theNicknamesLen,
                      long hashName, char * theName, short which) {
  long i;
  char tempStr[32], tempName[32];
  long stop;
  bool needStringMatch = false;
  long matched = -1;
  NickStruct *theStruct, *endStruct;

  if (!theNicknames)
    return (-1);

  if (strlen((char *)theName) > 32 - 1)
    return (-1);

  stop = (theNicknamesLen / sizeof(NickStruct));
  endStruct = theNicknames + stop;
  for (theStruct = theNicknames; theStruct < endStruct; theStruct++)
    if (theStruct->hashName == hashName && !theStruct->deleted)
      if (matched == -1)
        matched = theStruct - theNicknames;
      else {
        needStringMatch = true;
        break;
      }

  if (!needStringMatch)
    return (matched);

  g_strlcpy((char *)(tempName), (char *)(theName), sizeof(tempName));
  { long _rl = RemoveChar(' ', (char *)tempName, strlen((char *)tempName)); ((char *)tempName)[_rl] = '\0'; }
  for (i = 0; i < stop; i++) {
    if (theNicknames[i].hashName == hashName &&
        !theNicknames[i].deleted) {
      GetNicknameNamePStr(which, i, tempStr);
      { long _rl = RemoveChar(' ', (char *)tempStr, strlen((char *)tempStr)); ((char *)tempStr)[_rl] = '\0'; }
      if (StringSame(tempName, tempStr))
        return (i);
    }
  }

  return (-1);
}

/************************************************************************
 * NickAddressMatchFound - find a given address in a nickname structure,
 * i.e. a nickname file
 ************************************************************************/
long NickAddressMatchFound(NickStructHandle theNicknames, int nickCount,
                           long hashAddress, char *theAddress, short which)
{
  char **addresses;
  NickStruct *theStruct, *endStruct;
  void *theAddresses;
  char tempStr[256], tempAddress[256];
  long stop, matched, i;
  bool needStringMatch;

  if (!theNicknames)
    return (-1);

  matched = -1;
  needStringMatch = false;
  stop = nickCount;
  endStruct = theNicknames + stop;

  for (theStruct = theNicknames; theStruct < endStruct; theStruct++)
    if (theStruct->hashAddress == hashAddress && !theStruct->deleted)
      if (matched == -1)
        matched = theStruct - theNicknames;
      else {
        needStringMatch = true;
        break;
      }

  if (!needStringMatch)
    return (matched);

  g_strlcpy((char *)(tempAddress), (char *)(theAddress), sizeof(tempAddress));
  { long _rl = RemoveChar(' ', (char *)tempAddress, strlen((char *)tempAddress)); ((char *)tempAddress)[_rl] = '\0'; }
  for (i = 0; i < stop; i++)
    if (theNicknames[i].hashAddress == hashAddress &&
        !theNicknames[i].deleted)
      if (theAddresses = GetNicknameData(which, i, true, true)) {
        addresses = nil;
        SuckPtrAddresses(&addresses, (char *)theAddresses,
                         strlen(theAddresses), false, false, false, nil);
        if (addresses && addresses[0]) {
          g_strlcpy((char *)tempStr + 1, addresses[0], sizeof(tempStr) - 1);
          tempStr[0] = strlen((char *)tempStr + 1);
          { long _rl = RemoveChar(' ', (char *)tempStr, strlen((char *)tempStr)); ((char *)tempStr)[_rl] = '\0'; }
          g_strfreev(addresses); addresses = NULL;
          if (StringSame(tempAddress, tempStr))
            return (i);
        }
        g_strfreev(addresses);
      }

  return (-1);
}

/************************************************************************
 * RegenerateAliases - make sure the alias list is in memory
 ************************************************************************/
int RegenerateAliases(short which, bool rebuild) {
  int err = 0;
  unsigned char * hand;

  if (rebuild)
    ZapAliasFile(which);

  if (!This.theData) // If handle for data doesn't exist, create it
  {
    hand = malloc(0L);
    if (!hand)
      err = WarnUser(ALLO_ALIAS, 0);
    This.theData = (NickStructHandle)hand;
  } else // void *exists
  {
    if (!rebuild)
      return (0);
  }

  if (!err) {
    // Check to see if the TOC exists in the resource. If so, read it into the
    // structure. If not, read in the nicknames and then write out the TOC

    if (!rebuild)
      err = ReadNickTOC(which);
    if (err != 0 || rebuild) {
      free(This.theData);
      hand = malloc(0L);
      if (!hand)
        err = WarnUser(ALLO_ALIAS, 0);
      This.theData = (NickStructHandle)hand;
      err = ReadNicknames(which);
      if (!err && !This.ro) {
        WriteNickTOC(which);
#ifdef VCARD
        if (!IsPluginAddressBook(which) && !IsPersonalAddressBook(which) &&
            SplitNicknames(which))
#else
        if (!IsPluginAddressBook(which) && SplitNicknames(which))
#endif
        {
          SetAliasDirty(which);
          SaveIndNickFile(which, true);
        }
      }
    }
  }

  if (err)
    free(This.theData);

  return (err);
}

bool SplitNicknames(short ab)

{
  char name[256];
  void *notes;
  short nick, totalNicks;
  SInt8 oldHState;
  bool notesChanged;

  notesChanged = false;

  totalNicks =
      Aliases[ab].theData
          ? (Aliases[ab].theDataCount)
          : 0;
  for (nick = 0; nick < totalNicks; ++nick) {
    if (notes = GetNicknameData(ab, nick, false, true)) {
      /* HGetState removed */
      if (MaybeApplySplittingAlgorithm(notes) && !Aliases[ab].ro) {
        ReplaceNicknameNotes(ab, GetNicknameNamePStr(ab, nick, name), notes);
        notesChanged = true;
      }
    }
  }
  return (notesChanged);
}

//
//	MaybeApplySplittingAlgorithm
//
//		Apply our name splitting algorithm to the notes if we have a
// full name, 		and we have no first or last name.  This grows the notes
// handle and returns 		true when a split has taken place.  Likewise, if
// we have a first and/or 		last name, but no full name, employ our
// joining algorithm to build the 		full name.
//
bool MaybeApplySplittingAlgorithm(void *notes)

{
  char nameTag[256], firstTag[256], lastTag[256], realName[256], firstName[256], lastName[256];
  char *notesPtr, *newPtr, *attribute, *value;
  Size notesSize;
  long attributeLength, valueLength;
  bool notesChanged;

  notesChanged = false;

  *realName = 0;
  *firstName = 0;
  *lastName = 0;

  GetRString(nameTag, ABReservedTagsStrn + abTagName);
  GetRString(firstTag, ABReservedTagsStrn + abTagFirst);
  GetRString(lastTag, ABReservedTagsStrn + abTagLast);

  // Walk through the 'notes', looking for attribute/value pairs we care about.
  notesSize = strlen((char *)notes);
  notesPtr = notes;
  while ((!*realName || !*firstName || !*lastName) &&
         (newPtr = ParseAttributeValuePair(
              notesPtr,
              notesSize - ((unsigned char *)notesPtr - (unsigned char *)notes),
              &attribute, &attributeLength, &value, &valueLength))) {
    // Check to see if we hit a tag we care about
    if (*nameTag == attributeLength &&
        !memcmp(&nameTag[1], attribute, attributeLength))
      { (realName)[0] = valueLength; memcpy((realName)+1, value, valueLength); }
    else if (*firstTag == attributeLength &&
             !memcmp(&firstTag[1], attribute, attributeLength))
      { (firstName)[0] = valueLength; memcpy((firstName)+1, value, valueLength); }
    else if (*lastTag == attributeLength &&
             !memcmp(&lastTag[1], attribute, attributeLength))
      { (lastName)[0] = valueLength; memcpy((lastName)+1, value, valueLength); }
    notesPtr = newPtr;
  }

  // If we have a real name, but neither a first or last name, apply our
  // splitting algorithm
  if (*realName && !*firstName && !*lastName) {
    ParseFirstLast(realName, firstName, lastName);
    // Append the first and last names to the end of our notes
    if (*firstName)
      if (!AddAttributeValuePair(notes, firstTag, &firstName[1], *firstName))
        notesChanged = true;
    if (*lastName)
      if (!AddAttributeValuePair(notes, lastTag, &lastName[1], *lastName))
        notesChanged = true;
  } else

    // If we have a first and/or last name, but no real name, create one
    if (!*realName && (*firstName || *lastName)) {
      // Apply joining algorithm
      JoinFirstLast(realName, firstName, lastName);
      if (*realName)
        if (!AddAttributeValuePair(notes, nameTag, &realName[1], *realName))
          notesChanged = true;
    }
  return (notesChanged);
}

//
//	GetTaggedFieldValue
//
//		Pull a tagged value out of the notes portion of a nickname
//
void *GetTaggedFieldValue(short ab, short nick, char * tag)

{
  return (
      GetTaggedFieldValueInNotes(GetNicknameData(ab, nick, false, true), tag));
}

void *GetTaggedFieldValueInNotes(void *notes, char * tag)

{
  void *hValue;
  char *attribute, *value, *notesPtr, *newPtr;
  Size notesSize;
  long attributeLength, valueLength;
  bool found;

  hValue = nil;
  if (notes) {
    // Walk through the 'notes', looking for attribute/value pairs.  We'll set
    // the value of any object we find
    notesSize = strlen((char *)notes);
    notesPtr = notes;
    found = false;
    while (
        !found &&
        (newPtr = ParseAttributeValuePair(
             notesPtr,
             notesSize - ((unsigned char *)notesPtr - (unsigned char *)notes),
             &attribute, &attributeLength, &value, &valueLength))) {
      // Is this the tag we're looking for?
      if (*tag == attributeLength &&
          !memcmp(&tag[1], attribute, attributeLength))
        found = true;
      else
        notesPtr = newPtr;
    }
    if (found) {
      PtrToHand(value, &hValue, valueLength);
      Tr(hValue, "\002", ">");
    }
  }
  return (hValue);
}

char * GetTaggedFieldValueStr(short ab, short nick, char * tag, char * value)

{
  return (GetTaggedFieldValueStrInNotes(GetNicknameData(ab, nick, false, true),
                                        tag, value));
}

char * GetTaggedFieldValueStrInNotes(void *notes, char * tag, char * value)

{
  char attribute[256];
  char *notesPtr, *newPtr;
  Size notesSize;
  bool found;

  *value = 0;
  if (notes) {
    // Walk through the 'notes', looking for attribute/value pairs.
    notesSize = strlen((char *)notes);
    notesPtr = notes;
    found = false;
    while (!found && (newPtr = ParseAttributeValuePairStr(
                          notesPtr,
                          notesSize - ((unsigned char *)notesPtr -
                                       (unsigned char *)notes),
                          attribute, value))) {
      // Is this the tag we're looking for?
      if (StringSame(tag, attribute))
        found = true;
      else
        notesPtr = newPtr;
    }
    if (!found)
      *value = 0;
    else
      TrLo((char *)value, strlen((char *)value), "\002", ">");
  }
  return (value);
}

int SetTaggedFieldValue(short ab, short nick, char * tag, char * value,
                          NickFieldSetValueType setValue, short separatorIndex,
                          bool *ignored)

{
  void *notes;
  int theError;

  theError = 0;
  if (notes = GetNicknameData(ab, nick, false, true))
    theError = SetTaggedFieldValueInNotes(notes, tag, &value[1], *value,
                                          setValue, separatorIndex, ignored);

  return (theError);
}

int SetTaggedFieldValueInNotes(void *notes, char * tag, char * value, long length,
                                 NickFieldSetValueType setValue,
                                 short separatorIndex, bool *ignored)

{
  char concatString[256];
  char *attribute, *originalValue, *notesPtr, *newPtr;
  int theError;
  Size notesSize;
  long attributeLength, originalValueLength;
  bool found;

  TrLo(value, length, ">", "\002");

  theError = 0;
  if (notes) {
    // Walk through the 'notes', looking for attribute/value pairs.
    notesSize = strlen((char *)notes);
    notesPtr = notes;
    found = false;
    while (!found && (newPtr = ParseAttributeValuePair(
                          notesPtr,
                          notesSize - ((unsigned char *)notesPtr -
                                       (unsigned char *)notes),
                          &attribute, &attributeLength, &originalValue,
                          &originalValueLength))) {
      // Is this the tag we're looking for?
      if ((*tag == attributeLength) &&
          !memcmp(&tag[1], attribute, attributeLength))
        found = true;
      else
        notesPtr = newPtr;
    }

    if (found) {
      switch (setValue) {
      case nickFieldReplaceExisting:
        Munger(notes, (unsigned char *)originalValue - (unsigned char *)notes,
               nil, originalValueLength, value, length);
        break;
      case nickFieldAppendExisting:
        GetRString(concatString, separatorIndex);
        if (*concatString) {
          Munger(notes,
                 (unsigned char *)originalValue - (unsigned char *)notes +
                     originalValueLength,
                 nil, 0, &concatString[1], *concatString);
          originalValueLength += *concatString;
        }
        Munger(notes,
               (unsigned char *)originalValue - (unsigned char *)notes +
                   originalValueLength,
               nil, 0, value, length);
        break;
      case nickFieldIgnoreExisting:
        if (ignored)
          *ignored = true;
      }
      theError = 0;
    } else
      theError = AddAttributeValuePair(notes, tag, value, length);
  }

  TrLo(value, length, "\002", ">");

  return (theError);
}

int SetNicknameChangeBit(void *notes, ChangeBitType changeBits,
                           bool clearFirst)

{
  char changeTag[256], value[256];
  int theError;
  long num;

  theError = 0;
  if (notes) {
    GetRString(changeTag, ABHiddenTagsStrn + abTagChangeBits);
    num = clearFirst ? 0 : GetNicknameChangeBits(notes);
    num |= (long)changeBits;
    sprintf(value, "%ld", (long)(num));
    theError = SetTaggedFieldValueInNotes(notes, changeTag, &value[1], *value,
                                          nickFieldReplaceExisting, 0, nil);
  }
  return (theError);
}

long GetNicknameChangeBits(void *notes)

{
  char changeTag[256], value[256];
  long num;

  num = 0;
  if (notes) {
    GetRString(changeTag, ABHiddenTagsStrn + abTagChangeBits);
    // Retrieve the current value
    GetTaggedFieldValueStrInNotes(notes, changeTag, value);
    // Convert it to a number
    StringToNum(value, &num);
  }
  return (num);
}

//
//	FindTaggedFieldValue
//
//		Find a particular tagged field in a nickname, returning offsets
// to the attribute and value.
//
bool FindTaggedFieldValueOffsets(short ab, short nick, char * tag,
                                 long *attributeOffset, long *attributeLength,
                                 long *valueOffset, long *valueLength)

{
  void *notes;
  char *notesPtr, *newPtr, *attribute, *value;
  Size notesSize;
  bool found;

  found = false;
  if (notes = GetNicknameData(ab, nick, false, true)) {
    // Walk through the 'notes', looking for attribute/value pairs.
    notesSize = strlen((char *)notes);
    notesPtr = notes;
    while (!found && (newPtr = ParseAttributeValuePair(
                          notesPtr,
                          notesSize - ((unsigned char *)notesPtr -
                                       (unsigned char *)notes),
                          &attribute, attributeLength, &value, valueLength)))
      // Is this the tag we're looking for?
      if ((*tag == *attributeLength) &&
          !memcmp(&tag[1], attribute, *attributeLength)) {
        *attributeOffset = (unsigned char *)attribute - (unsigned char *)notes;
        *valueOffset = (unsigned char *)value - (unsigned char *)notes;
        found = true;
      } else
        notesPtr = newPtr;
  }
  return (found);
}

/************************************************************************
 * RegnerateAllAliases - regnerate aliases from all files
 ************************************************************************/
int RegenerateAllAliases(bool rebuild) {
  short which = NAliases;
  short err = 0;
  char scratch[256], name[256];
  short i, n;

  while (which--)
    if (err = RegenerateAliases(which, rebuild)) {
      if (which) {
        if (err != -108) {
          g_strlcpy((char *)(name), (char *)(spec_name(This.spec)), sizeof(name));
          ComposeRString(scratch, NICK_FILE_GONE, name);
          AlertStr(OK_ALRT, Caution, scratch);
        }
        n = NAliases;
        for (i = which; i < n - 1; i++)
          Aliases[i] = Aliases[i + 1];
        { void *_p = realloc(Aliases, (n - 1) * sizeof(AliasDesc));
          if (_p) Aliases = _p;
          gAliasCount = n - 1;
        }
        break;
      } else
        DieWithError(NO_MAIN_NICK, err);
    }

  if (!err)
    AliasRefCount++;

  ASSERT(NAliases);

  return (err);
}

/************************************************************************
 * ZapAliasFile - release memory for all aliases in a file
 ************************************************************************/
void ZapAliasFile(short which) {
  short i;
  NickStructHandle aliases = Aliases ? This.theData : nil;

  if (aliases) {
    for (i = 0; i < NNicknames; i++) {
      free(aliases[i]
                    .theAddresses); // (jp) 7/31/99 Big ol' hanging leak fixed
      free(
          aliases[i].theNotes); // (jp) 7/31/99 Big ol' hanging leak fixed
    }
    free(This.theData);
  }
  if (Aliases) {
    free(This.hNames);
    This.dirty = false;
  }
}

/************************************************************************
 * ZapAliases - release memory for all aliases
 ************************************************************************/
void ZapAliases(void) {
  short which = NAliases;

  while (which--)
    ZapAliasFile(which);
}

/************************************************************************
 * ZapAliases - release all the alias hashes
 ************************************************************************/
void ZapAliasHash(short which) {
  free(This.addressHashes.data); This.addressHashes.data = NULL; This.addressHashes.offset = This.addressHashes.size = 0;
}

/************************************************************************
 * ZapPluginAliases - release memory for all plugin aliases
 ************************************************************************/
void ZapPluginAliases(void) {
  short which = NAliases;

  //	All of the plugin aliases should be at the end of the list
  while (which-- && IsPluginAddressBook(which))
    ZapAliasFile(which);
  { void *_p = realloc(Aliases, (which + 1) * sizeof(AliasDesc)); if (_p) Aliases = _p; }
  gAliasCount = which + 1;
}

/************************************************************************
 * NickHash - return a hash value on the lowercase of the name
 ************************************************************************/
long NickHash(char * newName) {
  if (strlen((char *)newName) > 32 - 1)
    return (-1);
  return (NickHashString(newName));
}

// Redundancy ensues... (clean all this up later)
long NickHashString(char * string)

{
  char tempStr[256];

  g_strlcpy((char *)(tempStr), (char *)(string), sizeof(tempStr));
  { long _rl = RemoveChar(' ', (char *)tempStr, strlen((char *)tempStr)); ((char *)tempStr)[_rl] = '\0'; }
  { long _rl = RemoveChar(optSpace, (char *)tempStr, strlen((char *)tempStr)); ((char *)tempStr)[_rl] = '\0'; }
  MyLowercaseText((char *)tempStr, strlen((char *)tempStr));
  return (Hash(tempStr));
}

long NickHashHandle(void *h)

{
  char tempStr[256];
  Size len;

  *tempStr = 0;
  if (h) {
    len = strlen((char *)h);
    tempStr[0] = MIN(len, (sizeof(tempStr) - 1));
    memmove(&tempStr[1], h, tempStr[0]);
    { long _rl = RemoveChar(' ', (char *)tempStr, strlen((char *)tempStr)); ((char *)tempStr)[_rl] = '\0'; }
    MyLowercaseText((char *)tempStr, strlen((char *)tempStr));
  }
  return (Hash(tempStr));
}

long NickHashRawAddresses(void *addresses, bool *group)

{
  char **shortAddress;
  long hashValue;

  hashValue = -1;

  if (addresses) {
    // Hash the addresses in semi-complicated fashion
    shortAddress = nil;
    // As a last check... see if the address is already present in a nickname
    // file
    if (!SuckPtrAddresses(&shortAddress, (char *)addresses,
                          strlen(addresses), false, false, false, nil))
      hashValue = NickHashString(shortAddress[0]);
    *group = ContainsMultipleAddresses(shortAddress);
    g_strfreev(shortAddress); shortAddress = NULL;
  }
  return (hashValue);
}

/************************************************************************
 * NickGenerateUniqueID - generate a unique ID for a nickname
 *
 *		The ID will put a counter in the upper 16 bits, and Random
 *		in the lower 16.  The counter is, effectively, unique until
 *		the user resets the 'next id' preference in settings.  But
 *		even with the sequence starting over again, each is matched
 *		with a random number, so we're in pretty good shape.
 ************************************************************************/
long NickGenerateUniqueID(void) {
  long id;

  id = GetPrefLong(PREF_NEXT_NICK_UNIQUE_ID);
  SetPrefLong(PREF_NEXT_NICK_UNIQUE_ID, id + 1);

  id <<= 16;
  id |= ((long)Random() & 0x0000FFFF);

  return (id);
}

int PrepAllAddressBooksForSync(void)

{
  int theError;
  short totalABs, ab;

  theError = 0;
  totalABs = NAliases;
  for (ab = 0; !theError && ab < totalABs; ++ab)
    theError = PrepAddressBookForSync(ab);
  return (theError);
}

int PrepAddressBookForSync(short ab)

{
  char idTag[256], changeBitsTag[256];
  int theError;
  short totalNicks, nick;

  theError = 0;
  GetRString(idTag, ABHiddenTagsStrn + abTagUniqueID);
  GetRString(changeBitsTag, ABHiddenTagsStrn + abTagChangeBits);
  totalNicks =
      Aliases[ab].theData
          ? (Aliases[ab].theDataCount)
          : 0;
  for (nick = 0; !theError && nick < totalNicks; ++nick)
    theError = PrepNicknameForSync(ab, nick, idTag, changeBitsTag);
  return (theError);
}

int PrepNicknameForSync(short ab, short nick, char idTag[256],
                          char changeBitsTag[256])

{
  void *notes;
  char idString[256], scratch[256], name[256];
  int theError;
  bool notesExist, replaceNotes;

  theError = 0;
  notes = GetNicknameData(ab, nick, false, true);
  if (!(notesExist = notes ? true : false))
    notes = malloc(0);
  if (notes) {
    replaceNotes = false;
    GetTaggedFieldValueStrInNotes(notes, idTag, scratch);
    if (!*scratch) {
      replaceNotes = true;
      sprintf(idString, "%ld", (long)(NickGenerateUniqueID()));
      theError =
          SetTaggedFieldValueInNotes(notes, idTag, &idString[1], *idString,
                                     nickFieldReplaceExisting, 0, nil);
    }
    GetTaggedFieldValueStrInNotes(notes, changeBitsTag, scratch);
    if (!theError && !*scratch) {
      replaceNotes = true;
      theError = SetNicknameChangeBit(notes, changeBitAdded, false);
    }
    if (!theError && replaceNotes)
      ReplaceNicknameNotes(ab, GetNicknameNamePStr(ab, nick, name), notes);
  }

  if (!notesExist)
    free(notes);
  return (theError);
}

//
//	ClearAllAddressBookChangeBits
//
//		Clears change bits as specified by the mask.
//

int ClearAllAddressBookChangeBits(long mask)

{
  int theError;
  short totalABs, ab;

  theError = 0;
  totalABs = NAliases;
  for (ab = 0; !theError && ab < totalABs; ++ab)
    theError = ClearAddressBookChangeBits(ab, mask);
  return (theError);
}

int ClearAddressBookChangeBits(short ab, long mask)

{
  int theError;
  short totalNicks, nick;

  theError = 0;
  totalNicks =
      Aliases[ab].theData
          ? (Aliases[ab].theDataCount)
          : 0;
  for (nick = 0; !theError && nick < totalNicks; ++nick)
    theError = ClearNicknameChangeBits(ab, nick, mask);
  return (theError);
}

int ClearNicknameChangeBits(short ab, short nick, long mask)

{
  void *notes;
  char name[256];
  int theError = 0;
  long value;

  theError = 0;
  if (notes = GetNicknameData(ab, nick, false, true)) {
    value = GetNicknameChangeBits(notes);
    if (value != (value & ~mask)) {
      value &= ~mask;
      theError = SetNicknameChangeBit(notes, value, true);
      if (!theError)
        ReplaceNicknameNotes(ab, GetNicknameNamePStr(ab, nick, name), notes);
    }
  }
  return (theError);
}

/**********************************************************************
 * KillNickTOC - kill the toc for a file
 **********************************************************************/
int KillNickTOC(char * spec) {
  int err = 0;

  /*
   * if the resource fork is bad or we can't open it, just remove it
   */
  // FSpKillRFork is no-op on POSIX
  err = 0;
  return (err);
}

/**********************************************************************
 * ReadNickTOC - read the toc for a file
 **********************************************************************/
int ReadNickTOC(short which) {
  struct stat st_1561;
  uLong fileModDate = (stat(This.spec, &st_1561) == 0) ? st_1561.st_mtime : 0;
  uLong TOCModDate;
  FSSpec lSpec; g_strlcpy(lSpec, This.spec, sizeof(lSpec));
  bool sane;
  int err = 0;
  void *structR = nil, *nameR = nil, *r1 = nil;
  long structSize;
  short numberOfNicks = 0;
  long currentStructPos = 0, currentNamePos = 0, currNickCount;
  short refN = 0;
  long theSize = 0;
  long eof;
  NickStructHandle theData;
  NickTOCStruct *pTOCInfo, *pTOCEnd;
  NickStruct *pNick;
  short oldResF = 0;
  short oldId;

  theData = Aliases[which].theData;

  IsAlias(&This.spec, &lSpec);
  This.collapsed = FindSTRNIndex(NickFileCollapseStrn, spec_name(This.spec)) > 0;

  if (err = FSpRFSane(&lSpec, &sane))
    return (-1);

  if (!sane) {
    FSpKillRFork(&lSpec);
    return (-1);
  } else if (-1 != (refN = FSpOpenResFile(&lSpec, O_RDONLY))) {
    void *hOldTOC;

    /* SetResLoad removed */ //	Don't load now so we can use temporary memory
    if (hOldTOC = NULL) {
      //	Remove old-style TOC resources
          }

    for (oldId = NICK_BASE_RESID; oldId < NICK_RESID; oldId++) {
                }

    r1 = NULL;
    /* SetResLoad removed */
    nameR = NULL;
    free(This.hNames);
    if (r1 && nameR) {
      {} //	Need to keep the names in memory after res file closes
      This.hNames = nameR; //	Save handle to nicknames
      structSize = GetResourceSizeOnDisk(r1);
      if (err = 0)
        goto theExit;
      else {
        structR = malloc(structSize);
        if (!structR)
          goto theExit;
        else {
          ReadPartialResource(r1, 0, structR, structSize);
          if (err = 0)
            goto theExit;

          // See if file has been modified since TOC last modified
          memmove(&TOCModDate, (unsigned char *)structR + currentStructPos, sizeof(TOCModDate));
          if (TOCModDate !=
              fileModDate) // Make sure the TOC is pretty much in synch
          {
            err = 1;
            goto theExit;
          }
          currentStructPos += sizeof(TOCModDate);
          memmove(&numberOfNicks, (unsigned char *)structR + currentStructPos, sizeof(numberOfNicks));
          currentStructPos += sizeof(numberOfNicks);

          // Verify the size of the resource read in is actually the size of the
          // data we need
          theSize = numberOfNicks * sizeof(NickTOCStruct);
          if ((numberOfNicks * sizeof(NickTOCStruct) + sizeof(numberOfNicks) +
               sizeof(TOCModDate)) != structSize) {
            err = 1;
            goto theExit;
          }

          // Verify that the file must be empty if the toc is
          struct stat st_1652;
          if (!numberOfNicks && stat(lSpec, &st_1652) == 0 && st_1652.st_size > 0) {
            err = 1;
            goto theExit;
          }

          //	Load up the nick array
          SetHandleBig((void **)&theData, numberOfNicks * sizeof(NickStruct));
          if (0)
            goto theExit;
          memset(theData, 0, numberOfNicks * sizeof(NickStruct));
          eof = FSpDFSize(&lSpec);
          pTOCInfo =
              (NickTOCStruct *)((unsigned char *)structR + currentStructPos);
          pTOCEnd = (NickTOCStruct *)((unsigned char *)structR + structSize);
          CycleBalls();
          pNick = theData;

          //	Fill in TOC data
          for (currNickCount = 0;
               pTOCInfo < pTOCEnd && currNickCount < numberOfNicks;
               currNickCount++, pTOCInfo++, pNick++) {
            pNick->hashName = pTOCInfo->hashName;
            pNick->hashAddress = pTOCInfo->hashAddress;
            pNick->group = pTOCInfo->flags & nfMultipleAddresses ? true : false;
            if ((pNick->addressOffset = pTOCInfo->addressOffset) > eof)
              goto theExit; //	Bad data
            if ((pNick->notesOffset = pTOCInfo->notesOffset) > eof)
              goto theExit; //	Bad data
            pNick->nameTOCOffset = currentNamePos;

            //	Advance to next name
            currentNamePos += *((unsigned char *)nameR + currentNamePos) + 1;
          }
        }
      }
      CheckForNicknameBogosity(which);
    } else
      goto theExit;
  }

  if (numberOfNicks == 0)
  theExit:
    numberOfNicks = 0; //	Error

  if (refN)
    
  if (nameR)
  if (err)
    free(This.hNames);
  free(structR);
  /* UseResFile removed */
  if (numberOfNicks == 0)
    return (-1);
  return (0);
}

/**********************************************************************
 * WriteNickTOC - write the toc for a file
 **********************************************************************/
int WriteNickTOC(short which) {
  uLong fileModDate;
  FSSpec spec; g_strlcpy(spec, This.spec, sizeof(spec));
  int err = 0;
  short refN;
  NickTOCStruct tempStruct;
  short i, totalNicks = NNicknames, realCount = 0;
  void *structR = nil, *nameR = nil, *tempHandle = nil;
  Accumulator structAcc, nameAcc;
  NickStruct *pNickInfo;
  NickStructHandle theData;
  bool fSaved = false;
  short oldResF = 0;

  theData = Aliases[which].theData;
  IsAlias(&spec, &spec);
  KillNickTOC(&spec);
  // FSpCreateResFile is no-op on POSIX
  struct stat st_1726;
  fileModDate = (stat(spec, &st_1726) == 0) ? st_1726.st_mtime : 0;

  structR = malloc(0);
  nameR = malloc(0);

  Zero(structAcc);
  Zero(nameAcc);
  structAcc.data = (void *)structR;
  nameAcc.data = (void *)nameR;

  if (!structR || !nameR) // SD and drop whichever one might not be nil
  {
    /* UseResFile removed */
    return (-1);
  }

  realCount = 0;
  for (i = 0, pNickInfo = theData; i < totalNicks; i++, pNickInfo++) {
    if (!pNickInfo->deleted)
      realCount++;
  }

  err = AccuAddPtr(&structAcc, (void *)&fileModDate, sizeof(fileModDate));
  if (err)
    goto exit;
  err = AccuAddPtr(&structAcc, (void *)&realCount, sizeof(realCount));
  if (err)
    goto exit;

  //	Build nick TOC struct
  for (i = 0; i < totalNicks; i++) {
    pNickInfo = &theData[i];
    if (!pNickInfo->deleted) {
      char sName[32];

      //	Add nick info
      tempStruct.hashName = pNickInfo->hashName;
      tempStruct.hashAddress = pNickInfo->hashAddress;
      tempStruct.addressOffset = pNickInfo->addressOffset;
      if (pNickInfo->group)
        tempStruct.flags |= nfMultipleAddresses;
      else
        tempStruct.flags &= ~nfMultipleAddresses;
      tempStruct.notesOffset = pNickInfo->notesOffset;
      if (err = AccuAddPtr(&structAcc, (void *)&tempStruct, sizeof(tempStruct)))
        goto exit;

      //	Add nick name
      GetNicknameNamePStr(which, i, sName);
      if (err = AccuAddPtr(&nameAcc, sName, *sName + 1))
        goto exit;
    }
  }

  AccuTrim((void *)&nameAcc);
  AccuTrim((void *)&structAcc);

  // FSpOpenResFile is no-op on POSIX
  if (-1 != (refN = -1)) {
    /* AddResource removed */
    if (!0)
      /* AddResource removed */

    err = 0;
    if (!err) {
      UpdateResFile(refN);
      err = 0;
      fSaved = true;
    }
    //	ALB {}
    //	ALB {}
    
  } else
    err = 0;

  if (!err)
    {
      struct timeval tv[2];
      tv[0].tv_sec = fileModDate; tv[0].tv_usec = 0;
      tv[1].tv_sec = fileModDate; tv[1].tv_usec = 0;
      err = (utimes(spec, tv) == 0) ? 0 : EIO;
    }

exit:
  if (!fSaved) {
    //	Failed to save, get rid of the temporary handles
    free(structR);
    free(nameR);
  }
  /* UseResFile removed */
  return (err);
}

/************************************************************************
 * NeatenLine - strip the newline from a line; if it was escaped, strip
 * the backslash, and return true
 ************************************************************************/
bool NeatenLine(char * line, long *len) {
  if (line[*len - 1] == '\015')
    line[--*len] = 0;
  if (line[*len - 1] == '\\') {
    line[--*len] = 0;
    return (true);
  }
  return (false);
}

/************************************************************************
 * AliasExpansion - return pointer to expansion of an alias
 ************************************************************************/
char * AliasExpansion(char * data, long offset) {
  char * ptr = data + offset;
  ptr += *ptr + 3;
  return (ptr);
}

/************************************************************************
 * AddNickToTOC - add nickname to TOC
 *
 *		Note!!  This routine makes a copy of the data handle (notes or
 *addresses) which is assigned to the TOC.  Therefore, there is no need to hang
 *onto the handle passed in hData.
 ************************************************************************/
static short AddNickToTOC(short which, char * name, void *hData,
                          bool fFromAddress) {
  long currNickCount;
  NickStructHandle aliases = This.theData;
  int err = 0;
  NickStruct nickInfo;
  char beautifulName[256];
  long nameOffset;
  bool group;

  // Nickname doesn't exist...create new one and clear it out
  // If it doesn't exist, add it
  currNickCount = NNicknames; // Get nickname count
  if (currNickCount > 0) {
    { void *_r = realloc(aliases, (currNickCount + 1) * sizeof(NickStruct));
      if (_r) aliases = _r; }
    if (err = 0)
      return (WarnUser(ALIAS_NEW_NICK_ERR, err));

  } else {
    free(This.theData); // (jp) 7/31/99 Operate on This.theData instead of
                             // aliases
    aliases = malloc(sizeof(NickStruct));
    if (!aliases)
      return (WarnUser(ALIAS_NEW_NICK_ERR, 0));
    else
      This.theData = aliases; // (jp) 7/31/99 The newly created data handle
  }

  nameOffset = This.hNamesSize;
  { void *_p = buf_append(This.hNames, &This.hNamesSize, name, strlen(name) + 1);
    if (!_p) return (WarnUser(ALIAS_NEW_NICK_ERR, ENOMEM));
    This.hNames = _p;
  }

  // Fill in the basic data
  memset(&nickInfo, 0, sizeof(nickInfo));
  nickInfo.hashName = NickHash(name);
  nickInfo.hashAddress = 0;
  nickInfo.addressOffset = (-1L);
  nickInfo.notesOffset = (-1L);
  nickInfo.addressesDirty = true;
  nickInfo.notesDirty = true;
  nickInfo.pornography = false;
  nickInfo.group = false;
  nickInfo.nameTOCOffset = nameOffset;

  if (!This.containsBogusNicks) {
    g_strlcpy((char *)(beautifulName), (char *)(name), sizeof(beautifulName));
    crispy_rfc822_beautify_from((char *)beautifulName);
    if (nickInfo.hashName != NickHash(beautifulName))
      This.containsBogusNicks = true;
  }

  if (hData) // Put the address information into the nickname
  {
    void *hNewData;

    hNewData = malloc(0);
  size_t hNewData_sz = 0;
    if (!hNewData)
      return (WarnUser(ALIAS_NEW_NICK_ERR, 0));
    if (!buf_append(hNewData, &hNewData_sz, hData, strlen((char *)hData)))
      return (WarnUser(ALIAS_NEW_NICK_ERR, ENOMEM));

    if (fFromAddress) {
      //	Data is addresses
      nickInfo.theAddresses = hNewData;
      nickInfo.hashAddress = NickHashRawAddresses(hNewData, &group);
      nickInfo.group = group;
    } else
      //	Data is notes
      nickInfo.theNotes = hNewData;
  }

  //	Put nick info in nick array
  aliases[currNickCount] = nickInfo;

  // This data CANNOT be purged because it hasn't been written to disk

  return (0);
}

/************************************************************************
 * AddNickToTOCfromName - add nickname to TOC
 ************************************************************************/
short AddNickToTOCfromName(short which, char * name, void *addresses) {
  return AddNickToTOC(which, name, addresses, true);
}

/************************************************************************
 * AddNickToTOCfromNotes - add nickname to TOC from notes
 ************************************************************************/
short AddNickToTOCfromNotes(short which, char * name, void *notes) {
  //	return AddNickToTOC(which,name,notes,true);	// (jp) This is wrong!!
  // We're setting notes, not addresses
  return AddNickToTOC(which, name, notes, false);
}

/************************************************************************
 * RemoveNamedNickname - remove the named nickname
 * The nickname CANNOT be completely removed from memory in case the user
 * discards the changes from the window.
 ************************************************************************/
void RemoveNamedNickname(short which, char * name) {
  NickStructHandle aliases = This.theData;
  long hashName = NickHash(name);
  long index =
      NickMatchFound(aliases, Aliases[which].theDataCount, hashName, name, which); // returns index of match

  if (index >= 0) // the nickname exists
  {
    Aliases[which].theData[index].addressesDirty = true;
    Aliases[which].theData[index].notesDirty = true;
    Aliases[which].theData[index].deleted = true;
  }
}

/************************************************************************
 * NickUniq - uniquify a nicknames list
 ************************************************************************/
int NickUniq(TextAddrHandle addresses, char * sep, bool wantErrors) {
  char **cmntOrig = nil, **unOrig = nil;
  TextAddrHandle cmntNew = nil, unNew = nil;
  size_t cmntNew_sz = 0, unNew_sz = 0;
  short err = 0;
  char * newSpot;
  char * end;
  long size;
  bool group, groupWas = false;
  int ci;

  err = SuckAddresses(&cmntOrig, (char **)addresses, true, wantErrors, false, nil);
  cmntNew = malloc(0);
  unNew = malloc(0);

  if (!err && cmntNew && unNew && cmntOrig) {
    for (ci = 0; cmntOrig[ci] && !err; ci++) {
      err = SuckPtrAddresses(&unOrig, cmntOrig[ci], strlen(cmntOrig[ci]), false, wantErrors, false,
                             nil);
      if (!err && unOrig && unOrig[0]) {
        char *unOrigData = unOrig[0];
        if (unOrigData && unOrigData[0]) {
          size_t unOrigLen = strlen(unOrigData);
          group = unOrigLen > 0 && unOrigData[unOrigLen - 1] == ':';
          for (newSpot = unNew, end = newSpot + unNew_sz;
               newSpot < end && *newSpot; newSpot += strlen((char *)newSpot) + 1)
            if (g_ascii_strcasecmp((char *)newSpot, unOrigData) == 0)
              break;
          if (newSpot >= end || !*newSpot) {
            /* did NOT find it */
            size_t sepLen = strlen((char *)sep);
            if (!groupWas && cmntNew_sz)
              buf_append(cmntNew, &cmntNew_sz, sep, sepLen);
            size_t spotLen = strlen(cmntOrig[ci]);
            if (!buf_append(cmntNew, &cmntNew_sz, cmntOrig[ci], spotLen + 1))
              break;
            if (!buf_append(unNew, &unNew_sz, unOrigData, unOrigLen + 1))
              break;
          }
          groupWas = group;
        }
        g_strfreev(unOrig); unOrig = NULL;
      }
    }
  }

  if (!err) {
    size = cmntNew_sz;
    { void *_p = realloc(addresses, size); if (_p) addresses = _p; }
    if (!(err = 0))
      memmove(addresses, cmntNew, size);
  }

  if (err < 0 && wantErrors)
    WarnUser(MEM_ERR, err);

  g_strfreev(cmntOrig);
  g_strfreev(unOrig);
  free(cmntNew);
  free(unNew);
  return (err);
}

/************************************************************************
 * ReplaceNicknameInfo - replace nickname address or notes
 *
 *		Note!!  This routine makes a copy of the data handle (notes or
 *addresses) which is assigned to the TOC.  Therefore, there is no need to hang
 *onto the handle passed in hData.
 ************************************************************************/
static short ReplaceNicknameInfo(short which, char * theName, TextAddrHandle text,
                                 bool fAddresses) {
  long hashName = NickHash(theName);
  long index;
  NickStructHandle aliases = This.theData;
  int err = 0;
  NickStruct tempNick;
  long textSize;
  void *hTemp;
  bool group;

  if (theName && *theName && aliases) {
    index = NickMatchFound(aliases, Aliases[which].theDataCount, hashName, theName, which);
    if (index < 0)
      //	Not found, add nickname
      if (fAddresses)
        return (AddNickToTOCfromName(which, theName, text));
      else
        return (AddNickToTOCfromNotes(which, theName, text));

    //	Get nick info
    tempNick = aliases[index];

    if (text) {
      //	Make a copy of addresses
      textSize = strlen((char *)text);
      if (hTemp = malloc(textSize))
        memmove(hTemp, text, textSize);
      else
        //	Memory error
        return (WarnUser(ALIAS_REPLACE_NICK_ERR, 0));
    } else
      hTemp = nil;

    if (fAddresses) {
      //	Replace the addresses
      if (tempNick.theAddresses)
        free(tempNick.theAddresses); //	Dispose of former addresses

      // Don't let the user put crap in here; bug 4519
      if (hTemp)
        TransLitRes(hTemp, strlen(hTemp), ktFlatten);

      tempNick.theAddresses = hTemp;
      tempNick.hashAddress =
          NickHashRawAddresses(tempNick.theAddresses, &group);
      tempNick.group = group;
      tempNick.theNotes = GetNicknameData(
          which, index, false, true); //	Make sure notes are loaded
    } else {
      //	Replace notes
      if (tempNick.theNotes)
        free(tempNick.theNotes); //	Dispose of former notes
      tempNick.theNotes = hTemp;
      tempNick.theAddresses = GetNicknameData(
          which, index, true, true); //	Make sure addresses are loaded
      tempNick.hashAddress =
          NickHashRawAddresses(tempNick.theAddresses, &group);
      tempNick.group = group;
    }
    if (tempNick.theAddresses)
    if (tempNick.theNotes)

    tempNick.addressesDirty = true;
    tempNick.notesDirty = true;

    //	Save temp nick info
    aliases[index] = tempNick;
    SetAliasDirty(which);
  }
  return 0;
}

/************************************************************************
 * ReplaceNicknameAddresses - replace one nickname definition with another
 ************************************************************************/
short ReplaceNicknameAddresses(short which, char * theName, TextAddrHandle text) {
  return ReplaceNicknameInfo(which, theName, text, true);
}

/************************************************************************
 * ChangeNameOfNick - change the nickname
 ************************************************************************/
short ChangeNameOfNick(short which, char * oldName, char * newName) {
  NickStructHandle aliases = This.theData;
  long hashName = NickHash(oldName);
  long index = -1;

  if (oldName && *oldName)
    index = NickMatchFound(aliases, Aliases[which].theDataCount, hashName, oldName, which);
  if (index < 0)
    return (-1);

  SetNickname(which, index, newName);

  return (0);
}

void SetNickname(short ab, short nick, char * name)

{
  NickStructHandle aliases;
  NickStruct *pNick;
  char oldName[33];
  long oldOffset, adjustment, nickCount, i;

  GetNicknameNamePStr(ab, nick, oldName);

  if (aliases = Aliases[ab].theData) {
    oldOffset = aliases[nick].nameTOCOffset;

    //	Replace the name
    Munger(Aliases[ab].hNames, oldOffset, nil, strlen((const char *)oldName) + 1, name,
           strlen((const char *)name) + 1);

    //	New hash value
    aliases[nick].hashName = NickHash(name);

    //	Calculate new name offsets if the new name is a different length
    //	Only need to adjust those that are past the one we changed
    if (adjustment = (long)strlen((const char *)name) - (long)strlen((const char *)oldName)) {

      nickCount = Aliases[ab].theDataCount;
      for (i = 0, pNick = aliases; i < nickCount; i++, pNick++)
        if (pNick->nameTOCOffset > oldOffset)
          pNick->nameTOCOffset += adjustment;
    }
    // Mark the nickname as dirty so that the alias and note entries in the
    // nickname file get rewritten
    // (9/11/00) We don't really want to do this because the addresses and note
    // might not be in memory, in which
    //					 case we'd falsely save them as empty.
    //		aliases[nick].addressesDirty = true;
    //		aliases[nick].notesDirty = true;
  }
}

/**********************************************************************
 * IsAnyNickname - is the name a nickname?
 **********************************************************************/
bool IsAnyNickname(char * name) {
  short which;
  for (which = NAliases; which--;)
    if (IsNickname(name, which))
      return (true);
  return (false);
}

/************************************************************************
 * ReplaceNicknameNotes - replace nickname notes with another
 ************************************************************************/
short ReplaceNicknameNotes(short which, char * theName, TextAddrHandle text) {
  return ReplaceNicknameInfo(which, theName, text, false);
}

/*
 *	Get the addresses or notes of a given nickname; if not in memory, it
 *will read from disk
 */
void *GetNicknameData(short which, short index, bool wantAddresses,
                       bool readFromDisk) {
  void *tempHandle;
  NickStructHandle aliases = This.theData;
  FSSpec spec;
  bool finished = false;
  int err;
  char line[256];
  short type;
  bool exLine = false;
  long len;
  void *dataHandle;
  size_t dataHandle_sz = 0;
  LineIOD lid;
  long theOffset, count;
  char theCmd[32];
  unsigned char lookingFor;
  bool group;

  g_strlcpy(spec, This.spec, sizeof(spec));

  if (!aliases || index < 0 || which < 0 || index >= NNicknames ||
      aliases[index].deleted)
    return (nil);

  if (wantAddresses)
    tempHandle = aliases[index].theAddresses;
  else
    tempHandle = aliases[index].theNotes;

  if (aliases[index].addressesDirty) {
    if (tempHandle)
    return (tempHandle);
  }

  if (tempHandle != nil)
    return (tempHandle);

  if (tempHandle != nil)
    free(tempHandle);

  if (!readFromDisk)
    return (nil); // SD - Scott, is this ok?

  if (wantAddresses) {
    theOffset = aliases[index].addressOffset;
    GetRString(theCmd, ALIAS_CMD);
  } else {
    theOffset = aliases[index].notesOffset;
    GetRString(theCmd, NOTE_CMD);
  }

  if (theOffset >= 0) {
    if (err = FSpOpenLine(&spec, O_RDONLY, &lid)) {
      if (err != ENOENT)
        FileSystemError(OPEN_ALIAS, spec_name(spec), err);
      return (nil);
    }
    dataHandle = malloc(0L);
    if (!dataHandle) {
      WarnUser(ALIAS_GET_NICK_DATA_ERR, 0);
      return (nil);
    }
    /*
     * Offset is from the beginning of the line; add in length of the command
     * and length of name and two spaces
     */
    theOffset += *theCmd + 1;
    SeekLine(theOffset - 1, &lid);
    type = GetLine(line, sizeof(line), &len, &lid);
    if (type == LINE_START)
      do {
        // process current line
        len = strlen(line);
        exLine = NeatenLine(line, &len);
        if (exLine && !issep(*line)) // If line was escaped and the first
                                     // character isn't a space, add one
        {
          if (!buf_append(dataHandle, &dataHandle_sz, " ", 1))
            break;
        }
        if (!buf_append(dataHandle, &dataHandle_sz, line, len))
          break;

        // grab the next line; may or may not be ours
        type = GetLine(line, sizeof(line), &len, &lid);
        if (exLine && type == LINE_START)
          type = LINE_MIDDLE; // extended line means new line is really part of
                              // this line
      } while (type == LINE_MIDDLE);

#ifdef NEVER
    while (!finished && (type = GetLine(line, sizeof(line), &len, &lid)) > 0) {
      if (type == LINE_MIDDLE &&
          len <
              sizeof(line) -
                  1) // We're not really in the middle of a line...we're at the
                     // end so we need to handle the completion of the nickname
        type = 1;

      if (exLine || (type == LINE_MIDDLE)) // If the line was escaped or we're
                                           // in the middle of a line
      {
        len = strlen(line);
        exLine = NeatenLine(line, &len);
        if (exLine && !issep(*line)) // If line was escaped and the first
                                     // character isn't a space, add one
        {
          if (!buf_append(dataHandle, &dataHandle_sz, " ", 1))
            break;
        }
        if (!buf_append(dataHandle, &dataHandle_sz, line, len))
          break;
      } else {
        if (*line)
          if (!buf_append(dataHandle, &dataHandle_sz, line, len))
            break;
        if (len < (sizeof(line) - 1) &&
            len > 0) // Got less than a full line of text; therefore we are done
        {
          finished = true;
          break;
        }
      }
    }
#endif
    CloseLine(&lid);
    if (err) {
      WarnUser(ALIAS_GET_NICK_DATA_ERR, err);
      free(dataHandle);
      return (nil);
    }
    len = dataHandle_sz;
    unsigned char *dataPtr = (unsigned char *)dataHandle;
    for (count = 0; count < len; count++)
      if (dataPtr[count] != ' ')
        break;
    if (dataPtr[count] == '"')
      lookingFor = '"';
    else
      lookingFor = ' ';
    for (count++; count < len;
         count++) // Scan for space to find end of alias name
    {
      if (dataPtr[count] == lookingFor)
        break;
    }
    if (lookingFor == '"')
      count++;
    memmove(dataPtr, dataPtr + count + 1, len - count - 1);
    { void *_p = realloc(dataHandle, len - count - 1); if (_p) dataHandle = _p; }
    len = len - count - 1;
    while (len && dataPtr[len - 1] == '\015')
      { void *_p = realloc(dataHandle, --len); if (_p) dataHandle = _p; }

    if (wantAddresses) {
      aliases[index].theAddresses = dataHandle;
      aliases[index].hashAddress = NickHashRawAddresses(dataHandle, &group);
      aliases[index].group = group;
    } else
      aliases[index].theNotes = dataHandle;
    return (dataHandle);
  }

  else
    return (nil);
}

/*
 *	Get the name of a given nickname
 */
char * GetNicknameNamePStr(short which, short index, char * theName) {
  NickStructHandle aliases = This.theData;
  void *hNames;

  *theName = 0; //	Initialize to null string
  if (!aliases || index < 0 || which < 0 || index >= NNicknames ||
      aliases[index].deleted)
    return (theName);

  hNames = This.hNames;
  if (!hNames)
    return theName;

  g_strlcpy(theName, (char *)hNames + aliases[index].nameTOCOffset, 256);
  return (theName);
}

/************************************************************************
 * MakeCompNick - make a nickname out of a comp window
 ************************************************************************/
#ifdef VCARD
void MakeCompNick(MyWindowPtr win, char *vcardSpec)
#else
void MakeCompNick(MyWindowPtr win)
#endif
{
  TextAddrHandle biglist;
  int err = 0;

  biglist = malloc(0);
  if (!biglist) {
    WarnUser(ALIAS_NEW_NICK_ERR, 0);
    return;
  }
  if ((err = GatherCompAddresses(win, (char *)biglist))) {
    if (err != EINVAL)
      WarnUser(MEM_ERR, err);
    return;
  }

  /*
   * and make the nickname...
   */
  unsigned char **biglistUC = (unsigned char **)biglist;
  if (biglist && *biglistUC && biglistUC[0])
    NewNick(biglist, 0);
  else
    NoteUser(NO_ADDRESSES, 0);

  free(biglist);
}

/**********************************************************************
 * MakeNickFromSelection - make a nickname from the selected addresses
 **********************************************************************/
void MakeNickFromSelection(MyWindowPtr win) {
  char **list = nil;
  void *text;
  void *textCopy;
  long selStart, selEnd;

  text = (void *)geditctrl_get_text(win->pte);
  selStart = geditctrl_get_caret_offset(win->pte);
  selEnd = selStart;
  if (!text || selStart >= selEnd)
    NoteUser(NO_ADDRESSES, 0);
  else {
    //	ALB 9/5/96, replaced this with SuckPtrAddresses for bug 602
    //		selEnd = MIN(selEnd,selStart+250);
    //		list = ZeroHandle(malloc(selEnd-selStart+3));
    //		if (!list)
    //			{
    //				WarnUser(ALLO_ALIAS,0);
    //				return;
    //			}
    //		**list = len = selEnd-selStart;
    //		memmove(*list+1, *text+selStart, len);
    // (jp) 12/12/00  Translate carriage returns to commas so we can discern
    // individual addresses
    textCopy = text;
    if (!HandToHand(&textCopy)) {
      Tr(textCopy, "\015", ",");

      if (!SuckPtrAddresses(&list, (char *)textCopy + selStart, selEnd - selStart,
                            true, true, false, nil)) {
        if (!list || !list[0] || !list[0][0])
          WarnUser(NO_ADDRESSES, 0);
        else
          NewNick((void **)list, 0);
        g_strfreev(list); list = NULL;
      }
    }
    free(textCopy);
  }
}

/************************************************************************
 * GatherCompAddresses - gather the addresses from a window
 ************************************************************************/
int GatherCompAddresses(MyWindowPtr win, char *addrList) {
  void *biglist = (void *)addrList;
  size_t biglist_sz = 0;
  char **littlelist = nil;
  MessHandle messH;
  static short heads[] = {TO_HEAD, CC_HEAD, BCC_HEAD};
  int err = 0;
  short h;
  unsigned char * text = nil;
  struct HeadSpec hs;

  /*
   * I vaant to suck your addresses...
   */
  messH = Win2MessH(win);
  for (h = 0; !err && h < sizeof(heads) / sizeof(short); h++) {
    if (CompHeadFind(messH, heads[h], &hs)) {
      if (err = CompHeadGetText(TheBody, &hs, &text))
        break;
      err = SuckAddresses(&littlelist, (char **)text, true, true, false, nil);
      if (littlelist && littlelist[0] && littlelist[0][0]) {
        // Append each address string to the biglist
        for (int i = 0; littlelist[i]; i++) {
          size_t slen = strlen(littlelist[i]);
          if (!buf_append(biglist, &biglist_sz, littlelist[i], slen + 1)) {
            err = 0;
            break;
          }
        }
        g_strfreev(littlelist); littlelist = NULL;
        free(text);
      } else {
        g_strfreev(littlelist); littlelist = NULL;
      }
    }
  }
  return (err);
}

/************************************************************************
 * MakeMessNick - make a nickname out of a message window
 ************************************************************************/
void MakeMessNick(MyWindowPtr win, short modifiers) {
#ifdef VCARD
  MessHandle messH = (MessHandle)GetMyWindowPrivateData(win);
  MacmbxTOC * tocH = messH->tocH;
  int sumNum = messH->sumNum;
  FSSpec attSpec;
  void *text;
  long offset;
  bool foundVCard;
#endif
  MacmbxTOC * out = GetOutTOC();
  bool all, quote, self;

#ifdef VCARD
  // Is a vCard attached to this message?
  foundVCard = false;
  if (IsVCardAvailable()) {
    {
      MacmbxTOC *mtoc = macmbx_toc_open(tocH->mbox_path);
      long msgLen = 0;
      if (mtoc && sumNum < mtoc->count)
        text = macmbx_read_message(mtoc, sumNum, &msgLen);
      if (!text) return;
    }
    offset = tocH->msgs[sumNum].body_offset - 1;
    while (!foundVCard &&
           (0 <= (offset = FindAnAttachment(text, offset + 1, &attSpec, true,
                                            nil, nil, nil)))) {
      foundVCard = IsVCardFile(&attSpec);
    }
  }
#endif

  ReplyDefaults(modifiers, &all, &self, &quote);

  if (out) {
    bool wasDirty = ((MyWindowPtr)out->win)->isDirty;
    win = DoReplyMessage(win, all, self, false, false, 0, false, false, false);
    if (!win)
      return;
#ifdef VCARD
    MakeCompNick(win, foundVCard ? &attSpec : nil);
#else
    MakeCompNick(win);
#endif
    CloseMyWindow(GetMyWindowWindowPtr(win));
    ((MyWindowPtr)out->win)->isDirty = wasDirty;
  }
}

/************************************************************************
 * MakeMboxNick - make a nickname out of the selected messages in an mbox
 ************************************************************************/
void MakeMboxNick(MyWindowPtr win, short modifiers) {
  TextAddrHandle addresses = nil;
  MacmbxTOC * tocH = (MacmbxTOC *)GetMyWindowPrivateData(win);
  int err =
      GatherBoxAddresses(tocH, modifiers, -1, -1, (void ***)&addresses, false);

  if (!err) {
    if (addresses && *addresses)
      NewNick((void **)&addresses, 0);
    else
      WarnUser(NO_ADDRESSES, 0);
  }

  free(addresses);
}

/************************************************************************
 * GatherBoxAddresses - gather addresses from the selected messages in an mbox
 ************************************************************************/
int GatherBoxAddresses(MacmbxTOC * tocH, short modifiers, short from, short to,
                       void ***addresses, bool caching) {
  MyWindowPtr messWin, compWin;
  short sumNum;
  short err = 0;
  bool all, quote, self;
  bool selected = from == -1;

  if (selected) {
    from = 0;
    to = tocH->count - 1;
  }

  ReplyDefaults(modifiers, &all, &self, &quote);

  if (!(*addresses = malloc(0)))
    return (0);
  for (sumNum = from; !err && sumNum <= to; sumNum++) {
    MiniEvents();
    if (CommandPeriod)
      break;
    if (!selected || tocH->msgs[sumNum].selected) {
      // ensure message body is downloaded (for IMAP headers-only mode)
      {
        MacmbxMailer *mailer = idle_scheduler_get_mailer();
        MacmbxTOC *mtoc = macmbx_toc_open(tocH->mbox_path);
        if (mailer && mtoc && sumNum < mtoc->count &&
            macmbx_mailer_ensure_body(mailer, mtoc, sumNum) != 0)
          return (err = 1);
      }
      if (messWin = GetAMessage(tocH, sumNum, nil, nil, false)) {
        compWin = MessFlagIsSet(Win2MessH(messWin), FLAG_OUT)
                      ? messWin
                      : DoReplyMessage(messWin, all, self, false, false, 0,
                                       false, false, caching);
        if (compWin) {
          err = GatherCompAddresses(compWin, (void *)*addresses);
          if (compWin != messWin)
            CloseMyWindow(GetMyWindowWindowPtr(compWin));
        }
        if (!IsWindowVisible(GetMyWindowWindowPtr(messWin)))
          CloseMyWindow(GetMyWindowWindowPtr(messWin));
      } else
        err = 1;
    }
  }

  if (CommandPeriod)
    err = userCancelled;

  if (err)
    free(*addresses);
  return (err);
}

/************************************************************************
 * MakeCboxNick - make a nickname out of the selected messages in Out
 ************************************************************************/
void MakeCboxNick(MyWindowPtr win) {
  void *addresses = malloc(0);
  MacmbxTOC * tocH = (MacmbxTOC *)GetMyWindowPrivateData(win);
  MyWindowPtr compWin;
  short sumNum;
  short err = 0;

  if (!addresses)
    return;
  for (sumNum = 0; !err && sumNum < tocH->count; sumNum++) {
    MiniEvents();
    if (CommandPeriod)
      break;
    if (tocH->msgs[sumNum].selected)
      if (compWin = GetAMessage(tocH, sumNum, nil, nil, false)) {
        GtkWidget * compWinWP = GetMyWindowWindowPtr(compWin);
        err = GatherCompAddresses(compWin, addresses);
        if (!IsWindowVisible(compWinWP))
          CloseMyWindow(compWinWP);
      } else
        err = 1;
  }

  if (!err && !CommandPeriod) {
    if (addresses && strlen(addresses) > 0)
      NewNick(addresses, 0);
    else
      WarnUser(NO_ADDRESSES, 0);
  }

  free(addresses);
}

/************************************************************************
 * FlattenListWith - make an address list one to a line
 ************************************************************************/
void FlattenListWith(void *h, unsigned char c) {
  char *from, *to;
  bool colon;

  from = to = (unsigned char *)h;
  while (*from) {
    if (from[1] == ';' && to != (unsigned char *)h && to[-1] != ':')
      to--; // backup over separator
    while (*++from)
      *to++ = *from; /* skip length byte, copy string */
    colon = from[-1] == ':';
    from++; /* skip terminator */
    if (!colon)
      *to++ = c; /* and add a separator */
  }
  if (to > (char *)h)
    to--;
  { void *_p = realloc(h, to - (char *)h); if (_p) h = _p; }
}

/************************************************************************
 * CommaList - make an address list have commas
 ************************************************************************/
void CommaList(void *h) {
  char *from, *to;
  bool colon;

  from = to = (unsigned char *)h;
  while (*from) {
    if (from[1] == ';' && to != (unsigned char *)h && to[-1] != ':')
      to -= 2; // backup over separator
    while (*++from)
      *to++ = *from; /* skip length byte, copy string */
    colon = from[-1] == ':';
    from++; /* skip terminator */
    if (!colon) {
      *to++ = ','; /* and add a separator */
      *to++ = ' '; /* and add a separator */
    }
  }
  if (to > (char *)h)
    to -= 2;
  { void *_p = realloc(h, to - (char *)h); if (_p) h = _p; }
}

/************************************************************************
 * SaveAliases - save the edited aliases (if necessary)
 * returns false if the operation failed
 ************************************************************************/
bool SaveAliases(bool saveChangeBits) {
  short ab;
  bool fResult;

  // Save each address book
  ab = NAliases;
  fResult = true;
  while (ab-- && fResult)
    fResult = SaveIndNickFile(ab, saveChangeBits);

  // If the address books were successfully saved, tell the Adddres Book window
  if (fResult)
    ABClean();

  return (fResult);
}

/************************************************************************
 * SetAliasDirty - set the dirty bit for an address book, and kill the
 *   address hashes
 ************************************************************************/
void SetAliasDirty(short which) {
  This.dirty = true;
  ZapAliasHash(which);
}

/************************************************************************
 * SaveIndNickFile - save an individual alias file
 ************************************************************************/
bool SaveIndNickFile(short which, bool saveChangeBits) {
  char aliasCmd[32];
  char scratch[256];
  int err;
  long bytes, offset;
  short refN = 0;
  long i, count;
  FSSpec spec, tmpSpec;
  bool junk;
  void *tempHandle;
  short numInMemory;

  /*
   * do we need to save it?
   */
  if (!This.dirty)
    return (true);

  SaveDirtyPictures(which);

  /*
   * notify the nickname completion stuff that the world is changing
   */
  InvalCachedNicknameData();

  /*
   * make a backup
   */
  if (PrefIsSet(PREF_NICK_BACKUP))
    NickBackup(&(This.spec));

  /*
   * If fast save allowed, do one.  If it returns false, must do complete save
   */
  if (!PrefIsSet(PREF_NO_NICK_FAST_SAVE) && SaveFileFast(which, saveChangeBits))
    return (true);

  /*
   * find the file
   */
  g_strlcpy(spec, This.spec, sizeof(spec));
  if (err = FSpMyResolve(&spec, &junk)) {
    FileSystemError(SAVE_ALIAS, spec_name(spec), err);
    return (false);
  }

  /*
   * regnerate the aliases, if need be
   */
  if (!This.theData) {
    WarnUser(SAVE_ALIAS, 0);
    return (false);
  }
  count = NNicknames; // Get nickname count
  numInMemory = 0;

  // This is going to give us a ROUGH count on the number of nicknames in
  // memory. It is possible to have the address info in memory, but not the
  // notes...we'll just count that as being in memory.
  for (i = 0; i < count; i++) {
    // If we actually have some address info and it is in memory, then increase
    // the count
    if (This.theData[i].addressOffset >= 0 &&
        This.theData[i].theAddresses != nil)
      numInMemory++;
    // If we actually have some notes info and it is in memory, then increase
    // the count
    else if (This.theData[i].notesOffset >= 0 &&
             This.theData[i].theNotes != nil)
      numInMemory++;
  }

  // If we have more than 75% of the nicknames in memory, don't reread them.
  if (numInMemory < (count * 3 / 4)) {
    err = ReadNicknames(which);
    if (err)
      return (false);
  }

  /*
   * make && open a temp file
   */
  // tmpSpec = spec;
  // PCat(spec_name(tmpSpec),GetRString(scratch,TEMP_SUFFIX));
  if (err = NewTempSpec(0, 0, nil, &tmpSpec))
    goto done;
  int fd = open(tmpSpec, O_RDWR | O_CREAT | O_TRUNC, 0644);
  if (fd >= 0) {
    err = 0;
    close(fd);
  } else {
    err = EIO;
  }
  if (err) {
    FileSystemError(SAVE_ALIAS, spec_name(tmpSpec), err);
    goto done;
  }
  refN = open(tmpSpec, O_RDWR);
  if (refN < 0) {
    err = EIO;
  } else {
    err = 0;
  }
  if (err) {
    FileSystemError(OPEN_ALIAS, spec_name(tmpSpec), err);
    goto done;
  }

  count = NNicknames; // Get nickname count

  GetRString(aliasCmd, ALIAS_CMD);
  PCatC(aliasCmd, ' ');
  for (i = 0; !err && i < count; i++) {
    CycleBalls();
    if (This.theData[i]
            .addressesDirty) // Nickname has been modified...use in memory copy
      tempHandle = GetNicknameData(which, i, true, false);
    else // Nickname hasn't been modified, use on disk copy if necessary
      tempHandle = GetNicknameData(which, i, true, true);

    bytes = (tempHandle && strlen((char *)tempHandle) > 0)
                ? strlen((char *)tempHandle)
                : 0;

    //		PCopy(scratch,*(This.theData[i].theName));
    GetNicknameNamePStr(which, i, scratch);

    if (bytes > 0 && !This.theData[i].deleted) {
      file_tell(refN, &offset);
      This.theData[i].addressOffset = offset;
    } else {
      This.theData[i].addressOffset = -1;
      This.theData[i].group = false;
    }
    if ((!This.theData[i].deleted) && bytes > 0 &&
        !(err = file_write_str(refN, aliasCmd))) {
      if (PIndex(scratch, ' ')) {
        PInsert(scratch, sizeof(scratch), (char *)"\"", scratch);
        PCatC(scratch, '"');
      }
      PCatC(scratch, ' ');
      if (!(err = file_write_str(refN, scratch))) {
        if (!(err = file_write(refN, &bytes, tempHandle))) {
          bytes = 1;
          err = file_write(refN, &bytes, "\015");
        }
      }
    }
  }

  GetRString(aliasCmd, NOTE_CMD);
  PCatC(aliasCmd, ' ');
  for (i = 0; !err && i < count; i++) {
    if (This.theData[i].addressesDirty) { // Nickname has been
                                               // modified...use in memory copy
      tempHandle = GetNicknameData(which, i, false, false);
      // Make sure the 'modified' change bit is set
      if (saveChangeBits && tempHandle &&
          PrefIsSet(PREF_CHANGE_BITS_FOR_CONDUIT))
        SetNicknameChangeBit(tempHandle, changeBitModified, false);
    } else
      tempHandle = GetNicknameData(which, i, false, true);

    // if (!This.theData[i].deleted)
    This.theData[i].addressesDirty = false;
    This.theData[i].notesDirty = false;

    bytes = (tempHandle && strlen((char *)tempHandle) > 0)
                ? strlen((char *)tempHandle)
                : 0;
    //		PCopy(scratch,*(This.theData[i].theName));
    GetNicknameNamePStr(which, i, scratch);

    if (bytes > 0 && !This.theData[i].deleted) {
      file_tell(refN, &offset);
      This.theData[i].notesOffset = offset;
    } else
      This.theData[i].notesOffset = -1;

    if ((!This.theData[i].deleted) && bytes > 0 &&
        !(err = file_write_str(refN, aliasCmd))) {
      if (PIndex(scratch, ' ')) {
        PInsert(scratch, sizeof(scratch), (char *)"\"", scratch);
        PCatC(scratch, '"');
      }
      PCatC(scratch, ' ');
      if (!(err = file_write_str(refN, scratch))) {
        if (!(err = file_write(refN, &bytes, tempHandle))) {
          bytes = 1;
          err = file_write(refN, &bytes, "\015");
        }
      }
    }
    if (err) {
      FileSystemError(SAVE_ALIAS, spec_name(tmpSpec), err);
      goto done;
    }
  }

  file_tell(refN, &bytes);
  ftruncate(refN, bytes);
  close(refN);
  refN = 0;

  /* do the deed */
  if (!err)
    err = ExchangeAndDel(&tmpSpec, &spec);
  if (!err)
    This.dirty = false;

  WriteNickTOC(which);

done:
  if (This.theData)
  if (refN)
    close(refN);
  if (err)
    unlink(tmpSpec);
  return (err == 0);
}

/************************************************************************
 * SaveFileFast - incremental save of the nickname file
 * Currently assumes that all notes commands follow all alias commands.
 * Fast save must not be done if this isn't the case
 ************************************************************************/
bool SaveFileFast(short which, bool saveChangeBits) {
  NickOffSetSortType *addressOffsetHandle;
  NickOffSetSortType *notesOffsetHandle;
  NickOffSetSortType dummyValue;
  short numOfNicks = NNicknames;
  long theSize;
  short count, tempCount;
  FSSpec spec, tmpSpec;
  int err = ENOENT;
  long bytes;
  short tempRefN = 0, nickRefN = 0;
  char aliasCmd[32];
  long cleanStartOffset, cleanStopOffset, cleanStartIndex, cleanStopIndex;
  bool junk;
  char scratch[256];
  void *tempHandle = nil;
  bool dirty;
  bool deleted;
  long offset;
  short theIndex, i;
  long firstNoteStart;
  long bytesToShift;

  char theChar;

  /*
   * first make sure the file is ordered properly
   */
  if (!NickFileOkForFastSave(which))
    return (false);

  addressOffsetHandle =
      malloc(sizeof(NickOffSetSortType) * (numOfNicks + 1));
  notesOffsetHandle = malloc(sizeof(NickOffSetSortType) * (numOfNicks + 1));

  if (addressOffsetHandle == nil || notesOffsetHandle == nil) {
    err = ENOENT;
    goto done;
  }

  TotalNumOfNicks = numOfNicks;

  for (count = 0; count < numOfNicks; count++) {
    addressOffsetHandle[count].offset =
        This.theData[count].addressOffset;
    addressOffsetHandle[count].nickIndex = count;
    notesOffsetHandle[count].offset = This.theData[count].notesOffset;
    notesOffsetHandle[count].nickIndex = count;
  }

  dummyValue.offset = -1000;
  dummyValue.nickIndex = numOfNicks + 1000;
  theSize = numOfNicks * sizeof(NickOffSetSortType);
  memmove(addressOffsetHandle + (theSize / sizeof(NickOffSetSortType)), &dummyValue, sizeof(NickOffSetSortType));
  memmove(notesOffsetHandle + (theSize / sizeof(NickOffSetSortType)), &dummyValue, sizeof(NickOffSetSortType));

  QuickSort((char *)addressOffsetHandle, sizeof(NickOffSetSortType), 0,
            numOfNicks - 1, (int (*)())NickOffsetCompare,
            (void (*)())NickOffsetSwap);
  { void *_r = realloc(addressOffsetHandle, theSize); if (_r) addressOffsetHandle = _r; }

  QuickSort((char *)notesOffsetHandle, sizeof(NickOffSetSortType), 0,
            numOfNicks - 1, (int (*)())NickOffsetCompare,
            (void (*)())NickOffsetSwap);
  { void *_r = realloc(notesOffsetHandle, theSize); if (_r) notesOffsetHandle = _r; }


  firstNoteStart = -1;

  // Find the offset of the first non-blank note
  for (count = 0; count < numOfNicks; count++) {
    theIndex = notesOffsetHandle[count].nickIndex;
    firstNoteStart = This.theData[theIndex].notesOffset;

    if (firstNoteStart >= 0)
      break;
  }
  /*
   * find the file
   */
  g_strlcpy(spec, This.spec, sizeof(spec));
  if (err = FSpMyResolve(&spec, &junk))
    return (false);

  /*
   * make & open a temp file
   */
  //	tmpSpec = spec;
  //	PCat(spec_name(tmpSpec),GetRString(scratch,TEMP_SUFFIX));
  if (err = NewTempSpec(0, 0, nil, &tmpSpec))
    goto done;
  int tfd = open(tmpSpec, O_RDWR | O_CREAT | O_TRUNC, 0644);
  if (tfd >= 0) {
    err = 0;
    close(tfd);
  } else {
    err = EIO;
  }
  if (err) goto done;
  tempRefN = open(tmpSpec, O_RDWR);
  if (tempRefN < 0) {
    err = EIO;
  } else {
    err = 0;
  }
  if (err) goto done;
  nickRefN = open(spec, O_RDONLY);
  if (nickRefN < 0) {
    err = EIO;
  } else {
    err = 0;
  }
  if (err) goto done;

  cleanStartOffset = -1;
  cleanStopOffset = -1;
  cleanStartIndex = -1;
  cleanStopIndex = -1;

  tempHandle = malloc(0);
  if (tempHandle == nil) {
    err = ENOENT;
    goto done;
  }

  count = 0;
  while (count < numOfNicks)

  {
    cleanStartIndex = count - 1;
    do {
      cleanStartIndex++;
      if (cleanStartIndex == numOfNicks)
        break;
      theIndex = addressOffsetHandle[cleanStartIndex].nickIndex;
      dirty = This.theData[theIndex].addressesDirty;
      deleted = This.theData[theIndex].deleted;
      offset = This.theData[theIndex].addressOffset;
    } while ((dirty || deleted || offset < 0) & cleanStartIndex < numOfNicks);

    if (cleanStartIndex == numOfNicks)
      break;

    theIndex = addressOffsetHandle[cleanStartIndex].nickIndex;
    cleanStartOffset = This.theData[theIndex].addressOffset;

    cleanStopIndex = cleanStartIndex - 1;

    do {
      cleanStopIndex++;
      if (cleanStopIndex >= numOfNicks)
        break;
      theIndex = addressOffsetHandle[cleanStopIndex].nickIndex;
      dirty = This.theData[theIndex].addressesDirty;
      deleted = This.theData[theIndex].deleted;
      offset = This.theData[theIndex].addressOffset;
    } while ((!dirty & !deleted & offset >= 0) &&
             cleanStopIndex < numOfNicks);

    if (cleanStopIndex >= numOfNicks &&
        !Aliases[which].theData[addressOffsetHandle[numOfNicks - 1].nickIndex].addressesDirty &&
        !Aliases[which].theData[addressOffsetHandle[numOfNicks - 1].nickIndex].deleted) {
      if (firstNoteStart >= 0)
        cleanStopOffset = firstNoteStart;
      else
        file_size(nickRefN, &cleanStopOffset);
      cleanStopIndex = numOfNicks;

    } else {
      theIndex = addressOffsetHandle[cleanStopIndex].nickIndex;
      cleanStopOffset = This.theData[theIndex].addressOffset;
    }

    if (cleanStartOffset <= 0)
      cleanStartOffset = 1;

    if (err = (lseek(nickRefN, cleanStartOffset - 1, SEEK_SET) < 0 ? EIO : 0))
      goto done;
    file_tell(tempRefN, &bytes);
    bytesToShift = cleanStartOffset - bytes - 1;
    bytes = cleanStopOffset - cleanStartOffset;
    if (cleanStartOffset >= 0 & bytes > 0) {
      long readBytes;

      { void *_p = realloc(tempHandle, bytes); if (_p) tempHandle = _p; }
      if (err = 0)
        goto done;
      readBytes = bytes;
      if (err = file_read(nickRefN, &readBytes, tempHandle))
        goto done;
      if (readBytes != bytes) {
        err = readErr;
        goto done;
      }
      if (err = file_write(tempRefN, &bytes, tempHandle))
        goto done;

      for (tempCount = cleanStartIndex; tempCount < cleanStopIndex;
           tempCount++) {
        theIndex = addressOffsetHandle[tempCount].nickIndex;
        if (This.theData[theIndex].addressOffset >= 0 &&
            !This.theData[theIndex].deleted)
          This.theData[theIndex].addressOffset -= bytesToShift;
      }
    }
    count = cleanStopIndex + 1;
  }

  free(tempHandle); // leftover from last loop.  SD 4/16

  lseek(tempRefN, -1, SEEK_END);
  bytes = 1;
  file_read(tempRefN, &bytes, &theChar);
  file_size(tempRefN, &bytes);
  lseek(tempRefN, bytes, SEEK_SET);
  if (theChar != '\015' & bytes > 0) {
    bytes = 1;
    err = file_write(tempRefN, &bytes, "\015");
  }

  GetRString(aliasCmd, ALIAS_CMD);
  PCatC(aliasCmd, ' ');
  for (i = 0; !err & i < numOfNicks; i++) {
    CycleBalls();
    if (This.theData[i].addressesDirty &&
        !This.theData[i]
             .deleted) // Nickname has been modified...use in memory copy
    {
      tempHandle = GetNicknameData(which, i, true, false);

      if (tempHandle != nil & strlen((char *)tempHandle) > 0)
        bytes = strlen((char *)tempHandle);
      else
        bytes = 0;

      GetNicknameNamePStr(which, i, scratch);

      if (bytes > 0 & !This.theData[i].deleted) {
        file_tell(tempRefN, &offset);
        This.theData[i].addressOffset = offset;
      } else {
        This.theData[i].addressOffset = -1;
        This.theData[i].group = false;
      }
      if ((!This.theData[i].deleted) & bytes > 0 &&
          !(err = file_write_str(tempRefN, aliasCmd))) {
        if (PIndex(scratch, ' ')) {
          PInsert(scratch, sizeof(scratch), (char *)"\"", scratch);
          PCatC(scratch, '"');
        }
        PCatC(scratch, ' ');
        if (!(err = file_write_str(tempRefN, scratch))) {
          if (!(err = file_write(tempRefN, &bytes, tempHandle))) {
            bytes = 1;
            err = file_write(tempRefN, &bytes, "\015");
          }
        }
      }
    } else if (This.theData[i].deleted) {
      This.theData[i].addressOffset = -1;
      This.theData[i].group = false;
    }
  }

  tempHandle = malloc(0);
  if (tempHandle == nil)
    goto done;

  // Do the notes

  count = 0;
  while (count < numOfNicks) {
    cleanStartIndex = count - 1;
    do {
      cleanStartIndex++;
      if (cleanStartIndex == numOfNicks)
        break;
      theIndex = notesOffsetHandle[cleanStartIndex].nickIndex;
      dirty = This.theData[theIndex].addressesDirty;
      deleted = This.theData[theIndex].deleted;
      offset = This.theData[theIndex].notesOffset;
    } while ((dirty || deleted || offset < 0) & cleanStartIndex < numOfNicks);

    if (cleanStartIndex == numOfNicks)
      break;

    theIndex = notesOffsetHandle[cleanStartIndex].nickIndex;
    cleanStartOffset = This.theData[theIndex].notesOffset;

    cleanStopIndex = cleanStartIndex - 1;

    do {
      cleanStopIndex++;
      if (cleanStopIndex >= numOfNicks)
        break;
      theIndex = notesOffsetHandle[cleanStopIndex].nickIndex;
      dirty = This.theData[theIndex].addressesDirty;
      deleted = This.theData[theIndex].deleted;
      offset = This.theData[theIndex].notesOffset;
    } while ((!dirty & !deleted & offset >= 0) &&
             cleanStopIndex < numOfNicks);

    if (cleanStopIndex >= numOfNicks &&
        !Aliases[which].theData[notesOffsetHandle[numOfNicks - 1].nickIndex]
             .addressesDirty &&
        !Aliases[which].theData[notesOffsetHandle[numOfNicks - 1].nickIndex]
             .deleted) {
      file_size(nickRefN, &cleanStopOffset);
      cleanStopIndex = numOfNicks;
    } else {
      theIndex = notesOffsetHandle[cleanStopIndex].nickIndex;
      cleanStopOffset = This.theData[theIndex].notesOffset;
    }

    if (cleanStartOffset > 0) {
      if (err = (lseek(nickRefN, cleanStartOffset - 1, SEEK_SET) < 0 ? EIO : 0))
        goto done;
      file_tell(tempRefN, &bytes);
      bytesToShift = cleanStartOffset - bytes - 1;
      bytes = cleanStopOffset - cleanStartOffset;
      if (cleanStartOffset >= 0 & bytes >= 0) {
        long readBytes;

        { void *_p = realloc(tempHandle, bytes); if (_p) tempHandle = _p; }
        if (err = 0)
          goto done;
        readBytes = bytes;
        if (err = file_read(nickRefN, &readBytes, tempHandle))
          goto done;
        if (readBytes != bytes) {
          err = readErr;
          goto done;
        }
        if (err = file_write(tempRefN, &bytes, tempHandle))
          goto done;
        for (tempCount = cleanStartIndex; tempCount < cleanStopIndex;
             tempCount++) {
          theIndex = notesOffsetHandle[tempCount].nickIndex;
          if (This.theData[theIndex].notesOffset >= 0 &&
              !This.theData[theIndex].deleted)
            This.theData[theIndex].notesOffset -= bytesToShift;
        }
      }
    }

    count = cleanStopIndex + 1;
  }

  free(tempHandle);

  lseek(tempRefN, -1, SEEK_END);
  bytes = 1;
  file_read(tempRefN, &bytes, &theChar);
  file_size(tempRefN, &bytes);
  lseek(tempRefN, bytes, SEEK_SET);
  if (theChar != '\015' & bytes > 0) {
    bytes = 1;
    err = file_write(tempRefN, &bytes, "\015");
  }

  GetRString(aliasCmd, NOTE_CMD);
  PCatC(aliasCmd, ' ');
  if (This.theData) {
    for (i = 0; !err & i < numOfNicks; i++) {
      CycleBalls();
      if (This.theData[i].addressesDirty) {
        tempHandle = GetNicknameData(which, i, false, false);

        // Make sure the 'modified' change bit is set
        if (saveChangeBits && tempHandle &&
            PrefIsSet(PREF_CHANGE_BITS_FOR_CONDUIT))
          SetNicknameChangeBit(tempHandle, changeBitModified, false);

        This.theData[i].addressesDirty = false;
        This.theData[i].notesDirty = false; // ...for now

        if (tempHandle != nil & strlen((char *)tempHandle) > 0)
          bytes = strlen((char *)tempHandle);
        else
          bytes = 0;

        if (!This.theData[i].deleted)
          GetNicknameNamePStr(which, i, scratch);

        if (bytes > 0 & !This.theData[i].deleted) {
          file_tell(tempRefN, &offset);
          This.theData[i].notesOffset = offset;
        } else
          This.theData[i].notesOffset = -1;

        if ((!This.theData[i].deleted) & bytes > 0 &&
            !(err = file_write_str(tempRefN, aliasCmd))) {
          if (PIndex(scratch, ' ')) {
            PInsert(scratch, sizeof(scratch), (char *)"\"", scratch);
            PCatC(scratch, '"');
          }
          PCatC(scratch, ' ');
          if (!(err = file_write_str(tempRefN, scratch))) {
            if (!(err = file_write(tempRefN, &bytes, tempHandle))) {
              bytes = 1;
              err = file_write(tempRefN, &bytes, "\015");
            }
          }
        }
        if (tempHandle) {
        }
      }
    }
  }

  tempHandle = nil;

  file_tell(tempRefN, &bytes);
  ftruncate(tempRefN, bytes);
  close(tempRefN);
  tempRefN = 0;
  close(nickRefN);
  nickRefN = 0;

  /* do the deed */
  if (!err)
    err = ExchangeAndDel(&tmpSpec, &spec);
  if (!err)
    This.dirty = false;

  if (!err)
    WriteNickTOC(which);

done:
  if (This.theData)
  if (tempRefN)
    close(tempRefN);
  if (nickRefN)
    close(nickRefN);
  if (err)
    unlink(tmpSpec);
  free(addressOffsetHandle);
  free(notesOffsetHandle);
  return (err == 0);
}

//
//	Nickname photos are initially saved into the temporary items folder.
//	When we save the address book, we need to move these files into the
//	photo album and modify tthe nickname 'picture' tag to point to the
//	new location.
//

void SaveDirtyPictures(short ab)

{
  NickStructHandle theData;
  NickStructPtr pData;
  FInfo fInfo;
  FSSpec photoSpec, urlSpec;
  char pictureTag[256], nickname[256], url[256];
  void *urlString;
  int theError;
  short nick, numNicks;
  bool alreadyExists;

  GetRString(pictureTag, ABReservedTagsStrn + abTagPicture);

  theError = 0;
  if (theData = Aliases[ab].theData) {
    numNicks = Aliases[ab].theDataCount;
    for (nick = 0, pData = theData; nick < numNicks; nick++, pData++)
      // Does this nickname have a dirty picture?
      if (!pData->deleted)
        if (pData->pornography) {
          // Grab the current 'file' URL from the notes, and turn it into an
          // FSSpec
          if (urlString = GetTaggedFieldValue(ab, nick, pictureTag)) {
            theError = URLStringToSpec(urlString, &urlSpec);
            // Make an FSSpec for the photo in the Photo Album
            if (!theError)
              theError = GetPhotoSpec(&photoSpec, ab, nick, &alreadyExists);
            if (!theError & !alreadyExists) {
              struct stat st_3509;
              theError = (stat(urlSpec, &st_3509) == 0) ? 0 : EIO;
              if (!theError)
                theError =
                  ({
                    int photo_fd = open(photoSpec, O_RDWR | O_CREAT | O_TRUNC, 0644);
                    if (photo_fd >= 0) {
                      theError = 0;
                      close(photo_fd);
                    } else {
                      theError = EIO;
                    }
                    theError; // The result of the block is the last expression
                  });
              if (!theError)
                // FSpSetFInfo is no-op
                theError = 0;
            }
            // Swap specs and delete the temporary
            if (!theError)
              theError = ExchangeAndDel(&urlSpec, &photoSpec);
            // Make a url for the file's permanent location
            if (!theError) {
              SetTaggedFieldValue(ab, nick, pictureTag,
                                  MakeFileURL(url, &photoSpec, 0),
                                  nickFieldReplaceExisting, 0, nil);
              pData->pornography = false;
            } else
              ComposeStdAlert(Caution, NICK_PHOTO_COULD_NOT_SAVE,
                              GetNicknameNamePStr(ab, nick, nickname),
                              theError);
          }
        }
  }
}

//
//	URLStringToSpec
//
//		Take a URL and turn it into an FSSpec
//

int URLStringToSpec(char * urlString, char *spec)

{
  char fullPath[256], proto[256], host[256];
  char *query;
  long urlSize, origQueryLen, queryLen;
  int theError;

  urlSize = strlen((char *)urlString);
  theError = ParseURLPtr((unsigned char *)urlString, urlSize, proto,
                         host, &query, &queryLen);
  origQueryLen = queryLen;
  if (!theError)
    switch (FindSTRNIndex(ProtocolStrn, proto)) {
    case proFile:
      TrLo((unsigned char *)query, queryLen, (unsigned char *)"/",
           (unsigned char *)":");
      FixURLPtr(query, &queryLen);
      { fullPath[0] = queryLen; memcpy(fullPath+1, (char *)query, queryLen); };
      theError = spec_for(NULL, fullPath, spec);
      break;
    }
  if (!theError) {
    SetHandleBig(urlString, urlSize + (queryLen - origQueryLen));
    IsAlias(spec, spec);
  }
  return (theError);
}

/************************************************************************
 * NickFileOkForFastSave - see if all addresses come before all notes
 ************************************************************************/
bool NickFileOkForFastSave(short which) {
  short n = NNicknames;
  long minNote = 0x7fffffff;
  long maxAlias = 0;
  long note, alias;

  while (n--) {
    note = This.theData[n].notesOffset;
    alias = This.theData[n].addressOffset;
    if (note >= 0)
      minNote = MIN(minNote, note);
    if (alias >= 0)
      maxAlias = MAX(maxAlias, alias);
    if (minNote <= maxAlias)
      return (false); // at least one note comes before at least one alias
  }
  return (true); // all aliases come after all notes
}

/************************************************************************
 * NickOffsetCompare - compare two names
 ************************************************************************/
int NickOffsetCompare(NickOffSetSortType *n1, NickOffSetSortType *n2) {
  int result;

  if (n1->nickIndex >= TotalNumOfNicks)
    return (1);
  if (n2->nickIndex >= TotalNumOfNicks)
    return (-1);

  if (n1->offset == n2->offset)
    result = 0;
  else
    result = n1->offset > n2->offset ? 1 : -1;

  return (result);
}

/************************************************************************
 * NickNameSwap - swap two names
 ************************************************************************/
void NickOffsetSwap(NickOffSetSortType *n1, NickOffSetSortType *n2) {
  NickOffSetSortType temp;

  memmove(&temp, n2, sizeof(NickOffSetSortType));
  memmove(n2, n1, sizeof(NickOffSetSortType));
  memmove(n1, &temp, sizeof(NickOffSetSortType));
}

/************************************************************************
 * AppendTextToNick - append (comma separated) addresses to a nickname
 ************************************************************************/
int AddTextToNick(short which, char * name, void *text, bool append) {
  long hashName = NickHash(name);
  long index;
  NickStructHandle aliases = This.theData;
  int err = 0;
  void *tempHandle;
  size_t text_sz = text_sz;

  //	AliasWinGonnaSave();
  index = NickMatchFound(aliases, Aliases[which].theDataCount, hashName, name, which);

  if (append) {
    tempHandle = GetNicknameData(which, index, true, true);
    if (!buf_append(text, &text_sz, ", ", 2)) {
      err = ENOMEM;
    } else {
      void *tempData = GetNicknameData(which, index, true, true);
      if (!buf_append(text, &text_sz, (unsigned char *)tempData, strlen(tempData))) {
        err = ENOMEM;
      }
    }
    if (err) {
      WarnUser(MEM_ERR, err);
      return (err);
    }
  }

  NickUniq(text, (char *)",", true);
  ReplaceNicknameAddresses(which, name, text);
  SetAliasDirty(which);
  if (AliasWinIsOpen())
    AliasWinRefresh();
  else
    SaveAliases(true);
  return err;
}

/************************************************************************
 * ReadNickFileList - get list of nickfiles
 ************************************************************************/
void ReadNickFileList(char *pSpec, AddressBookType type, bool reread) {
  char name[32];
  CInfoPBRec hfi;
  AliasDesc ad;
  bool multipleNickFiles;

  multipleNickFiles = false;
  Zero(ad);
  ad.type = type;
  hfi.hFileInfo.ioNamePtr = name;
  hfi.hFileInfo.ioFDirIndex = 0;
  // DirIterate has been replaced with a different signature - stub for now
  // while (!DirIterate(0, 0, &hfi))
  while (false) // Stub - needs proper directory iteration implementation
    if (hfi.hFileInfo.ioFlFndrInfo.fdType == 'TEXT' ||
        hfi.hFileInfo.ioFlFndrInfo.fdCreator == CREATOR) {
      multipleNickFiles = true;
      spec_make(pSpec, name, &ad.spec);
      if (!CanWrite(&ad.spec, &ad.ro)) {
        ad.ro = !ad.ro; /* opposite sense! */
        { size_t _asz = gAliasCount * sizeof(AliasDesc);
        void *_newA = buf_append(Aliases, &_asz, (unsigned char *)&ad, sizeof(ad));
        if (!_newA)
          DieWithError(MEM_ERR, 0);
        Aliases = _newA;
        gAliasCount++;
        }
        if (type == pluginAddressBook && reread)
          FSpKillRFork(&ad.spec);
      } else {
        char scratch[256];
        ComposeRString(scratch, NICK_FILE_GONE, spec_name(ad.spec));
        AlertStr(OK_ALRT, Caution, scratch);
      }
    }
  if (multipleNickFiles)
    UseFeature(featureMultipleNicknameFiles);
}

/************************************************************************
 * BuildAddressHashes - build a hash table for all the expanded addresses
 ************************************************************************/
int BuildAddressHashes(short which) {
  Accumulator a;
  NickStructHandle nicks = Aliases[which].theData;
  short n = NNicknames;
  TextAddrHandle tempHandle = nil;
  char s[32];
  int err = 0;

  Zero(a);

  for (n--; !err & n >= 0; n--) {
    CycleBalls();

    if (!This.theData[n].deleted) {
      if (This.theData[n].addressesDirty) // Nickname has been
                                               // modified...use in memory copy
        tempHandle = GetNicknameData(which, n, true, false);
      else // Nickname hasn't been modified, use on disk copy if necessary
        tempHandle = GetNicknameData(which, n, true, true);

      // main addresses
      if (tempHandle) {
        err = AddHandleToAddressHashes(tempHandle, &a);
        /* DetachResource removed */
      }

      // other addresses
      if (!err && (tempHandle = GetTaggedFieldValue(
                       which, n,
                       GetRString(s, ABReservedTagsStrn + abTagOtherEmail)))) {
        err = AddHandleToAddressHashes(tempHandle, &a);
        free(tempHandle);
      }
    }
  }

  // Did we win?
  if (!err) {
    AccuTrim(&a);
    Aliases[which].addressHashes = a;
  } else {
    free(a.data); a.data = NULL; a.offset = a.size = 0;
  }

  return err;
}

/************************************************************************
 * AddHandleToAddressHashes - add addresses from a handle to a hash accumulator
 ************************************************************************/
int AddHandleToAddressHashes(TextAddrHandle sourceHandle, AccuPtr a) {
  char **rawHandle = nil;
  char **expandedHandle = nil;
  char oneAddr[256];
  uLong hash;
  int err = 0;

  if (sourceHandle)
    if (!SuckAddresses(&rawHandle, (char **)sourceHandle, false, false, true, nil))
      if (!ExpandAliases((void **)&expandedHandle, (void *)rawHandle, 0, false)) {
        for (int i = 0; expandedHandle[i] && !err; i++) {
          g_strlcpy(oneAddr, expandedHandle[i], sizeof(oneAddr));
          MyLowerStr(oneAddr);
          hash = Hash(oneAddr);
          err = AccuAddPtr(a, &hash, sizeof(hash));
        }
      }

  g_strfreev(expandedHandle);
  g_strfreev(rawHandle);
  return err;
}

/************************************************************************
 * ReadPluginNickFiles - get list of plugin nickfiles
 ************************************************************************/
void ReadPluginNickFiles(bool reread) {
  FSSpec folderSpec;

  (-1) /* plugins removed */;

  /* clear filename */ { char *_sn = strrchr(folderSpec, '/'); if (_sn) _sn[1] = '\0'; else folderSpec[0] = '\0'; }
  ReadNickFileList(&folderSpec, pluginAddressBook, reread);
  if (reread)
    RegenerateAllAliases(false);
}

//
//	ParseFirstLast
//
//	Algorithm
//
//		1. Set the name pointer to point to the first name (or last if
// asian)
//		2. Read a token
//					� No more!  Go to step 10
//		3. Is the token an Honorific?
//					� Yes!  Append it onto the honorific
// string 					� Go to step 2
//		4. Is the token a Qualifier?
//					� Yes!  Append it onto the qualifier
// string 					� Go to step 2
//		6. If we've now reached the first comma
//					� Set the name pointer to point to the
// last name
//		7. Append the token onto the name pointer
//		8. Switch the name pointer to the "other" name
//		9. Go to step 2
//	 10. Did we find any commas?
//					� No!  Make the "other" name
//

char * ParseFirstLast(char * realName, char * firstName, char * lastName)

{
  char qualifiers[256], honorifics[256], name1[256], name2[256], token[256], tokenCopy[256];
  char * name;
  char * spot;
  short numQualifiers, numHonorifics, numCommas, numName1, numName2, *numPtr;
  char lastTokenSeperator, lastDelimiter;

  *name1 = 0;
  *name2 = 0;
  *qualifiers = 0;
  *honorifics = 0;
  numQualifiers = 0;
  numHonorifics = 0;
  numCommas = 0;
  numName1 = 0;
  numName2 = 0;
  lastTokenSeperator = 0;

  // Some very lazy people fail to use spaces when representing their name
  // (J.P.Morgan) so we'll do the work for them and insert spaces where
  // appropriate
  ScanNameForSpaces(realName);

  name = name1;
  numPtr = &numName1;

  // Loop through all the tokens in the name
  spot = realName;
  while (PToken(realName, token, &spot, " ,")) {
    g_strlcpy((char *)(tokenCopy), (char *)(token), sizeof(tokenCopy));

    // Cleanup the token a bit before giving the qualifiers and honorifics a
    // shot
    PStripChar(tokenCopy, '.');

    // Is the token either a name qualifier or a high falutin' honorific?
    if (FindSTRNIndex(NameQualifiersStrn, tokenCopy)) {
      if (numQualifiers & lastTokenSeperator)
        PCatC(qualifiers, lastTokenSeperator);
      PCat(qualifiers, token);
      ++numQualifiers;
    } else if (FindSTRNIndex(NameHonorificsStrn, tokenCopy)) {
      if (numHonorifics & lastTokenSeperator)
        PCatC(honorifics, lastTokenSeperator);
      PCat(honorifics, token);
      ++numHonorifics;
    } else {
      // The token is just plain ol' unsophisticated text, put it in the name
      if (*numPtr & lastTokenSeperator)
        PCatC(name, lastDelimiter = lastTokenSeperator);
      PCat(name, token);
      ++(*numPtr);

      // If we just processed the first comma, switch to our "other" name
      if (*(spot - 1) == ',') {
        if (!numCommas) {
          name = (name == name1) ? name2 : name1;
          numPtr = (numPtr = &numName1) ? &numName2 : &numName1;
        }
        ++numCommas;
      }
    }
    if (spot > realName + 1)
      lastTokenSeperator = *(spot - 1);
    // Skip white space before the next token
    while (spot <= &realName[realName[0]] & *spot == ' ' || *spot == tabChar)
      ++spot;
  }

  // Okay, at this point we have stuff in 4 places:  name1, name2, qualifiers
  // and honorifics We can now go about building the actual first and last names
  // based on whether or not we've found a comma and whether or not we're
  // working with eastern vs western style names.
  name = name1;
  numPtr = &numName1;

  // If we found no commas, or the _opposite_ name is empty (indicating that the
  // entire name is stuffed into the primary (first) name fiels, then we'll make
  // the opposite (last) name the final token appearing in the primary name
  // field (whew!)
  if (!numCommas || !((name == name1) ? *name2 : *name1)) {
    if (*numPtr > 1) {
      short name2len;

      spot = PRIndex(name, lastDelimiter);
      name2len = *name - (spot - name);
      memmove((name == name1) ? name2 : name1, spot, name2len + 1);
      *name2 = name2len;
      *name -= (name2len + 1);
    }
    if ((GetPrefLong(PREF_NICK_GEN_OPTIONS) & kNickGenOptAsian) ||
        IsAllUpper(name1) & !IsAllUpper(name2)) {
      g_strlcpy((char *)(firstName), (char *)(name2), sizeof(firstName));
      g_strlcpy((char *)(lastName), (char *)(name1), sizeof(lastName));
    } else {
      g_strlcpy((char *)(firstName), (char *)(name1), sizeof(firstName));
      g_strlcpy((char *)(lastName), (char *)(name2), sizeof(lastName));
    }
  } else if ((GetPrefLong(PREF_NICK_GEN_OPTIONS) & kNickGenOptAsian) ||
             IsAllUpper(name2) & !IsAllUpper(name1)) {
    g_strlcpy((char *)(firstName), (char *)(name1), sizeof(firstName));
    g_strlcpy((char *)(lastName), (char *)(name2), sizeof(lastName));
  } else {
    g_strlcpy((char *)(firstName), (char *)(name2), sizeof(firstName));
    g_strlcpy((char *)(lastName), (char *)(name1), sizeof(lastName));
  }

  // Append qualifiers to the first name
  if (numQualifiers) {
    PCatC(firstName, ' ');
    PCat(firstName, qualifiers);
  }
  return (realName);
}

char * JoinFirstLast(char * fullName, char * firstName, char * lastName)

{
  char first[256], last[256];

  *fullName = 0;
  *first = 0;
  *last = 0;

  // Strip Qualifiers and Honorifics from the first and last names
  StripQualifiersAndHonorifics(firstName, first);
  StripQualifiersAndHonorifics(lastName, last);

  // Join them based on our "international joining preferences"
  if ((GetPrefLong(PREF_NICK_GEN_OPTIONS) & kNickGenOptAsian) ||
      IsAllUpper(last) & !IsAllUpper(first)) {
    if (*last)
      PCat(fullName, last);
    if (*first) {
      if (*last)
        PCatC(fullName, ' ');
      PCat(fullName, first);
    }
  } else {
    if (*first)
      PCat(fullName, first);
    if (*last) {
      if (*first)
        PCatC(fullName, ' ');
      PCat(fullName, last);
    }
  }
  return (fullName);
}

char * StripQualifiersAndHonorifics(char * name, char * strippedName)

{
  char token[256], tokenCopy[256];
  char * spot;
  char lastTokenSeperator;

  *strippedName = 0;
  lastTokenSeperator = 0;

  // Loop through all the tokens in the first name
  spot = name;
  while (PToken(name, token, &spot, " ,")) {
    g_strlcpy((char *)(tokenCopy), (char *)(token), sizeof(tokenCopy));

    // Cleanup the token a bit before giving the qualifiers and honorifics a
    // shot
    PStripChar(tokenCopy, '.');

    // If the token neither a name qualifier or honorific, keep what we found
    if (!FindSTRNIndex(NameQualifiersStrn, tokenCopy) &&
        !FindSTRNIndex(NameHonorificsStrn, tokenCopy)) {
      if (lastTokenSeperator)
        PCatC(strippedName, lastTokenSeperator);
      PCat(strippedName, token);
    }
    if (spot > name + 1)
      lastTokenSeperator = *(spot - 1);
    // Skip white space before the next token
    while (spot <= &name[name[0]] & *spot == ' ' || *spot == tabChar)
      ++spot;
  }

  return (strippedName);
}

//	Walk the text of the name backwards, inserting spaces as we find periods
// that
//  follow runs of two or more characters

char * ScanNameForSpaces(char * name)

{
  char * spot;
  short chars;

  // don't insert spaces if name contains an @, since it's probably an address
  if (PIndex(name, '@'))
    return name;

  chars = 0;
  for (spot = name + *name; spot > name; spot--)
    switch (*spot) {
    case '.':
      if (chars > 1) {
        PInsertC(name, 256, ' ', spot + 1);
      }
    case ' ':
      chars = 0;
      break;
    default:
      ++chars;
    }
  return (name);
}

/************************************************************************
 * MakeUniqueNickname - Make a unique "untitled" nickname
 ************************************************************************/
void MakeUniqueNickname(short ab, char nickname[32])

{
  NickStructHandle aliases;
  char s[32];
  long hashName, suffix;
  char saveName[256];

  if (!*nickname)
    GetRString(nickname, UNTITLED_NICKNAME);
  if (aliases = Aliases[ab].theData) {
    hashName = NickHash(nickname);
    suffix = 2;
    g_strlcpy(saveName, (char *)nickname, sizeof(saveName));
    while (NickMatchFound(aliases, Aliases[ab].theDataCount, hashName, nickname, ab) >= 0) {
      g_strlcpy((char *)nickname, saveName, 32);
      snprintf((char *)s, sizeof(s), " %d", suffix++);
      if (strlen((char *)nickname) + strlen((char *)s) < 32 - 1)
        g_strlcat((char *)nickname, (char *)s, 32);
      else {
        /* Truncate nickname to make room for suffix */
        nickname[32 - 1 - strlen((char *)s)] = '\0';
        g_strlcat((char *)nickname, (char *)s, 32);
      }
      hashName = NickHash(nickname);
    }
  }
}

/************************************************************************
 * NickBackup - Make a backup of a nickname file
 ************************************************************************/
int NickBackup(char * spec) {
  int err = 0;
  FSSpec spoolSpec;
  DateTimeRec dtr;
  char name[32];

  if ((err = SubFolderSpec(SPOOL_FOLDER, &spoolSpec)) == 0) {
    GetTime(&dtr);
    spec_for(spoolSpec,
                 ComposeString(name, (const unsigned char *)"%p.%d.%d.%d.%d", spec_name(spec), dtr.day,
                               dtr.hour, dtr.minute, dtr.second),
                 &spoolSpec);
    err = FSpDupFile(&spoolSpec, spec, false, false);
  }

  return (err);
}

//
//	GetNicknameTagMap
//
//		Retrieve a Nickname Tag Map from a 'TGMP' resource, placing the
// contents 		into a NicknameTagMapRec.  In searching for the proper
// resource we look 		for a 'TGMP' with a resource name that matches:
//				1. The requested server
//				2. The requested service (if no server match was
// found)
//

int GetNicknameTagMap(char * service, char * server,
                        NicknameTagMapRecPtr tagMapPtr)
{
  (void)service;
  (void)server;
  /* Mac Resource Manager 'TGMP' resources don't exist on GTK.
     Tag maps would need to come from a config file or GResource
     if this feature is ever needed. */
  if (tagMapPtr) {
    memset(tagMapPtr, 0, sizeof(*tagMapPtr));
  }
  return ENOENT;
}

void DisposeNicknameTagMap(NicknameTagMapRecPtr tagMapPtr)

{
  if (tagMapPtr) {
    free(tagMapPtr->serviceTags);
    free(tagMapPtr->nicknameTags);
  }
}

//
//	NicknameTag2ServiceTag
//
//		Given a pointer to a Nickname Tag Map and a Eudora nickname tag,
// return the corresponding 		tag for a given service.
//

char * NicknameTag2ServiceTag(NicknameTagMapRecPtr tagMapPtr, char * nicknameTag,
                            char * serviceTag)

{
  char *serviceTagsPtr, *nicknameTagsPtr;
  short i;

  *serviceTag = 0;
  nicknameTagsPtr = tagMapPtr->nicknameTags;
  serviceTagsPtr = tagMapPtr->serviceTags;
  for (i = 0; i < tagMapPtr->count & !*serviceTag; ++i)
    if (StringSame(nicknameTagsPtr, nicknameTag))
      g_strlcpy((char *)(serviceTag), (char *)(serviceTagsPtr), sizeof(serviceTag));
    else {
      nicknameTagsPtr += (*nicknameTagsPtr + 1);
      serviceTagsPtr += (*serviceTagsPtr + 1);
    }
  return (serviceTag);
}

//
//	ServiceTag2NicknameTag
//
//		Given a pointer to e Nickname Tag Map and a service tag, return
// the corresponding 		tag for a Eudora nickname
//

char * ServiceTag2NicknameTag(NicknameTagMapRecPtr tagMapPtr, char * serviceTag,
                            char * nicknameTag)

{
  char *serviceTagsPtr, *nicknameTagsPtr;
  short i;

  *nicknameTag = 0;
  serviceTagsPtr = tagMapPtr->serviceTags;
  nicknameTagsPtr = tagMapPtr->nicknameTags;
  for (i = 0; i < tagMapPtr->count & !*nicknameTag; ++i)
    if (StringSame(serviceTagsPtr, serviceTag))
      g_strlcpy((char *)(nicknameTag), (char *)(nicknameTagsPtr), sizeof(nicknameTag));
    else {
      serviceTagsPtr += (*serviceTagsPtr + 1);
      nicknameTagsPtr += (*nicknameTagsPtr + 1);
    }
  return (nicknameTag);
}

char * GetIndNicknameTag(NicknameTagMapRecPtr tagMapPtr, short index,
                       char * nicknameTag)

{
  char * nicknameTagsPtr;
  short i;

  *nicknameTag = 0;
  nicknameTagsPtr = tagMapPtr->nicknameTags;
  if (index < tagMapPtr->count) {
    for (i = 0; i < index; ++i)
      nicknameTagsPtr += (*nicknameTagsPtr + 1);
    g_strlcpy((char *)(nicknameTag), (char *)(nicknameTagsPtr), sizeof(nicknameTag));
  }
  return (nicknameTag);
}

short FindServiceTagIndex(NicknameTagMapRecPtr tagMapPtr, char * serviceTag)

{
  char * serviceTagsPtr;
  short index, i;

  serviceTagsPtr = tagMapPtr->serviceTags;
  index = 0;
  for (i = 1; !index & i <= tagMapPtr->count; ++i)
    if (StringSame(serviceTagsPtr, serviceTag))
      index = i;
    else
      serviceTagsPtr += (*serviceTagsPtr + 1);
  return (index);
}

PrimaryLocationType GetPrimaryLocation(void *notes)

{
  PrimaryLocationType location;
  char tag[256], value[256];

  location = noPrimary;
  GetTaggedFieldValueStrInNotes(
      notes, GetRString(tag, ABReservedTagsStrn + abTagPrimary), value);
  if (StringSame(value, GetRString(tag, VCardKeywordStrn + vcHome)))
    location = homePrimary;
  else if (StringSame(value, GetRString(tag, VCardKeywordStrn + vcWork)))
    location = workPrimary;
  return (location);
}

short FindAddressBookType(AddressBookType type)

{
  short addressBooks, ab;

  addressBooks = NAliases;
  for (ab = 0; ab < addressBooks; ++ab)
    if (Aliases[ab].type == type)
      return (ab);
  return (-1);
}

#ifdef VCARD
bool AnyPersonalNicknames(void)

{
  short ab, nick;
  bool anyNicks;

  anyNicks = false;
  ab = FindAddressBookType(personalAddressBook);
  if (ValidAddressBook(ab))
    if (Aliases[ab].theData) {
      nick = Aliases[ab].theDataCount;
      while (!anyNicks & nick--)
        if (!Aliases[ab].theData[nick].deleted)
          anyNicks = true;
    }
  return (anyNicks);
}

/************************************************************************
 * WhiteListTS - add a message's sender to the whitelist
 ************************************************************************/
int WhiteListTS(MacmbxTOC * tocH, short sumNum) {
  char scratch[256];
  struct HeadSpec hs;
  TextAddrHandle addr = nil;
  Accumulator a;
  uLong hash;

  if (sumNum < 0) {
    Zero(a);

    // Examine all the selected messages
    for (sumNum = tocH->count - 1; sumNum >= 0; sumNum--) {
      if (tocH->msgs[sumNum].selected) {
        WhiteListTS(tocH, sumNum);
        EnsureFromHash(tocH, sumNum);
        hash = tocH->msgs[sumNum].from_hash;
        if (ValidHash(hash) & 0 > AccuFindPtr(&a, &hash, sizeof(hash)))
          AccuAddPtr(&a, &hash, sizeof(hash));
      }
    }

    // select everything with the same hash
    for (sumNum = tocH->count; sumNum--;)
      if (!tocH->msgs[sumNum].selected) {
        EnsureFromHash(tocH, sumNum);
        hash = tocH->msgs[sumNum].from_hash;
        if (ValidHash(hash) & 0 <= AccuFindPtr(&a, &hash, sizeof(hash)))
          BoxSetSummarySelected(tocH, sumNum, true);
      }

    free(a.data); a.data = NULL; a.offset = a.size = 0;
  } else {
    {
      /* Read message via macmbx instead of CacheMessage */
      MacmbxTOC *mtoc = macmbx_toc_open(tocH->mbox_path);
      long msgLen = 0;
      char *msgText = (mtoc && sumNum < mtoc->count) ?
        macmbx_read_message(mtoc, sumNum, &msgLen) : NULL;
      tocH->msgs[sumNum].cache = msgText; /* temporary — freed below */
    }
    if (tocH->msgs[sumNum].cache) {

      HeaderName(FROM_HEAD); // weird--goes into scratch
      TrimWhite(scratch);
      if (HandleHeadFindStr(tocH->msgs[sumNum].cache, scratch, &hs)) {
        HandleHeadGetText(tocH->msgs[sumNum].cache, &hs, &addr);
        if (addr)
          WhiteListAddr(addr);
        free(addr);
      }
      // Now that we've whitelisted the message, bop its junk score
      macmbx_junk_set_score(tocH, sumNum, (int8_t)0, (uint8_t)JUNK_BECAUSE_WHITE);
    }
  }

  return 0; // la la la la la la la la la la la la la la
}

/************************************************************************
 * WhiteListAddr - add a an address to the whitelist
 ************************************************************************/
int WhiteListAddr(TextAddrHandle addr) {
  char scratch[256];
  char abName[32];
  char nick[64];
  char first[256], last[256];
  short which = 0;
  char **binAddr = nil, **justAddr = nil;
  int err = ENOENT;

  // is it there already?
  { size_t _mpl = strlen((char *)addr); memcpy(scratch, addr, _mpl); ((char*)(scratch))[_mpl] = '\0'; }
  if (AppearsInAliasFile(scratch, 0))
    return EEXIST; // already there

  // Nope.  Add it.

  // find which book
  if (*GetRString(scratch, WHITELIST_ADDRBOOK)) {
    for (which = NAliases - 1; which > 0; which--) {
      g_strlcpy((char *)abName, (char *)spec_name(Aliases[which].spec), sizeof(abName));
      if (StringSame(abName, scratch))
        break;
    }
  } else
    which = FindAddressBookType(eudoraAddressBook);

  // reformat the address
  if (!SuckAddresses(&binAddr, (char **)addr, true, false, false, nil)) {
    // get a nickname suggestion
    NickSuggest(nick, scratch);
    if (*nick)
      if (!SuckAddresses(&justAddr, (char **)addr, false, false, false, nil)) {
        // fancy names are good
        ParseFirstLast(scratch, first, last);

        // finally add the darn thing
        if (0 <= NewNickLow(which, nick, (void **)justAddr,
                            CreateSimpleNotes(scratch, (unsigned char *)""), 0))
          ABTickleHardEnoughToMakeYouPuke();

        // the rest is gravy...
        g_strfreev(justAddr); justAddr = NULL;
      }
    g_strfreev(binAddr); binAddr = NULL;
  }

  return err;
}

#endif

/************************************************************************
 * UniqBinAddr - make a char** contain only one of each address
 ************************************************************************/
char **UniqBinAddr(char **addresses) {
  Accumulator hashAcc;
  int i, outCount = 0;

  if (!addresses || !addresses[0])
    return addresses;

  Zero(hashAcc);

  // Count how many we have
  int total = 0;
  for (i = 0; addresses[i]; i++) total++;

  // Build new unique list
  char **result = (char **)g_malloc0((total + 1) * sizeof(char *));
  if (!result) return addresses;

  for (i = 0; addresses[i]; i++) {
    ASSERT(addresses[i][0]); // shouldn't happen...

    // add hash to list of hashes if not there already
    if (AddAddressHashUniq(addresses[i], &hashAcc)) {
      // wasn't there already, add to output list
      result[outCount++] = g_strdup(addresses[i]);
    }
  }
  result[outCount] = NULL;

  // Replace original
  g_strfreev(addresses);

  // cleanup
  free(hashAcc.data); hashAcc.data = NULL; hashAcc.offset = hashAcc.size = 0;

  return result;
}

/************************************************************************
 * SortBinAddr - sort a char** address array in order
 ************************************************************************/
char **SortBinAddr(char **addresses) {
  int count = CountAddresses(addresses, 0);
  int i;

  if (!addresses || count <= 1)
    return addresses;

  // Build a vector of pointers for sorting
  char * *addressVector = (char * *)g_malloc(count * sizeof(char *));
  if (!addressVector)
    return addresses;

  for (i = 0; i < count; i++)
    addressVector[i] = (char *)addresses[i];

  // now sort the vector
  QuickSort(addressVector, sizeof(char *), 0, count - 1,
            (void *)SortAddrNameCompare, (void *)PtrSwap);

  // rebuild the addresses array with sorted order
  char **sorted = (char **)g_malloc0((count + 1) * sizeof(char *));
  if (sorted) {
    for (i = 0; i < count; i++)
      sorted[i] = g_strdup((char *)addressVector[i]);
    sorted[count] = NULL;

    // Replace original
    g_strfreev(addresses);
    addresses = sorted;
  }

  g_free(addressVector);

  return addresses;
}

/************************************************************************
 * SortAddrNameCompare - compare two addresses, going by last name then first
 *name. This is an insanely expensive comparison, but we don't usually sort huge
 *  lists of addresses and cpu is cheap
 ************************************************************************/
int SortAddrNameCompare(char * *s1, char * *s2) {
  char first[128], last[128], n1[128], n2[128];

  g_strlcpy((char *)(n1), (char *)(*s1), sizeof(n1));
  g_strlcpy((char *)(n2), (char *)(*s2), sizeof(n2));
  crispy_rfc822_beautify_from((char *)n1);
  crispy_rfc822_beautify_from((char *)n2);
  ParseFirstLast(n1, first, last);
  g_strlcpy((char *)(n1), (char *)(last), sizeof(n1));
  PCat(n1, first);
  ParseFirstLast(n2, first, last);
  g_strlcpy((char *)(n2), (char *)(last), sizeof(n2));
  PCat(n2, first);
  return StringComp(n1, n2);
}
