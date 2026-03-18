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

#include "linkmng.h"
#include "Globals.h"
#include "MyRes.h"
#include "StringDefs.h"
#include "StringUtil.h"
#include "fileutil.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "legacy_shim.h"
#include "lineio.h"
#include "mailbox.h"
#include "mydefs.h"
#include "nickmng.h"
#include "shame.h"
#include "util.h"
#define FILE_NUM 128

/* Forward declarations for functions with no header */
void LinkTickle(void);
int ParseURL(unsigned char *url, unsigned char *proto, unsigned char *host,
             unsigned char *query);
void FixURLString(char *url);
void *DupHandle(void **src);
void LVAdd(ViewListPtr pView, VLNodeInfo *info);
short GetMenuHandle(short menuID);
void GetMenuItemText(short mh, short item, unsigned char *text);
short Names2Icon(unsigned char *name1, unsigned char *name2);
short HiWord(long x);
short LoWord(long x);
int OpenLocalURLPtr(unsigned char *url, long len);
int ParseProtocolFromURLPtr(char *url, long len, unsigned char *proto);
int OpenOtherURLPtr(unsigned char *proto, char *url, long len);
unsigned long SecsToLocalDateTime(unsigned long secs);
bool LinkHasCustomIcons(void);
void QuickSort(void *base, int nmemb, int size,
               int (*compare)(const void *, const void *));
void *GetIconSuite(void **suite, int id, unsigned long svData);
void *DupIconSuite(void *suite, short id);
bool IsAdwareMode(void); /* GTK port: IsAdwareMode/IsAdInPlaylist are
                            Mac-specific ad APIs; skip */
bool IsAdInPlaylist(AdId adId);
#ifndef smSystemScript
#define smSystemScript 0
#endif
#ifndef kCustomIconResource
#define kCustomIconResource (-16455)
#endif
#ifndef svAllAvailableData
#define svAllAvailableData 0
#endif

/* Copyright (c) 1999 by QUALCOMM Incorporated */

/************************************************************************
 *  routines to manage multiple link history lists
 ************************************************************************/

#define LINK_HISTORY_VERSION                                                   \
  4 /* increment this when changing the HistoryStruct */

#define LINK_TOC_TYPE 'LToc'
#define LINK_RESID 130

/* until ListView is taught about items with names > 31 characters */
typedef char URLNameStr[32];

//
// Data structures for link management
//

/* LinkTypeEnum - types of links stored in the link history */
typedef enum {
  ltAd = 0,
  ltHttp,
  ltFtp,
  ltMail,
  ltDirectoryServices,
  ltSetting,
  ltFile,
  LinkTypeLimit
} LinkTypeEnum;

/* LinkLabelEnum - labels for links */
typedef enum {
  llRemindMe = 1,
  llBookmarked,
  llAttempted,
  llNone,
  llNotDisplayed,
  LinkLabelLimit
} LinkLabelEnum;

/* Structure for a given history entry, version 1 */
typedef struct {
  short version;              /* the version of the link history entry */
  long hashName;              /* hash value on history name */
  URLNameStr name;            /* the name of the url */
  unsigned long cacheSeconds; /* date this url was last clicked */
  LinkTypeEnum type;          /* the type of url this is */
  long dirty : 1;             /* has the history entry been modified */
  long deleted : 1;           /* has the history entry been deleted */
  long thumb : 1;             /* does this history entry have a thumbnail */
  long label : 4;             /* label */
  long incompleteAd : 1; /* this entry is an ad that's not ready yo be displayed
                            in the history */
  long remind : 1;       /* remind the user to visit this link */
  long spare : 23;       /* leftovers */
  long urlOffset;        /* offset in file where the real url is at */
  long imageOffset; /* offset in file where the location of the icon is at */
  void *hUrl;       /* void *to URL this entry will link to */
  AdId adId;        /* ID of Ad this entry refers to */
} HistoryStruct, *HistoryStructPtr, *HistoryStructHandle;

/* Structure to keep track of an infividual history file */
typedef struct HistoryDStruct {
  FSSpec spec;                 /* the history file */
  HistoryStructHandle theData; /* the toc */
  bool ro;                     /* read only */
  bool dirty;                  /* is the history file dirty? */
} HistoryDesc, *HistoryDPtr, *HistoryDHandle;

/* Structure just enough info for the history window.  Will sort this puppy. */
typedef struct {
  URLNameStr name;            /* the name of the url */
  unsigned long cacheSeconds; /* date this url was last clicked */
  LinkTypeEnum type;          /* the type of url this is */
  VLNodeID nodeId;     /* id of this node, calculated from which and index */
  LinkLabelEnum label; /* the label of this history entry */
} ShortHistoryStruct, *ShortHistoryStructPtr, *ShortHistoryStructHandle;

/* Structure to maintain a cache of preview icon handles */
typedef struct LHPIconCacheStruct {
  void **theIcon;
  AdId adId;
  struct LHPIconCacheStruct *next;
} LHPIconCacheStruct, *LHPIconCachePtr, *LHPIconCacheHandle;

//
// Globals for link management
//

// Global list of all known history files
HistoryDHandle gHistories = NULL;

/* Global list of loaded preview icons */
LHPIconCacheHandle gPreviewIcons = NULL;

/* Global spec pointing inside the Link History Folder */
FSSpec gLinkHistoryFolder;

// GLobal list of all preview icons

// Types of links we care about if following them fails with some error
LinkTypeEnum gLabelTheseLinks[] = {ltAd, ltHttp, ltFtp, ltMail,
                                   ltDirectoryServices};

//
// Some helpful #defines
//

#ifdef DEBUG
#define COMPACT_THRESHHOLD 2
#else
#define COMPACT_THRESHHOLD 10
#endif

#define AGE_INTERVAL 60 * 60 * 60 // age once an hour

#define MAIN_HISTORY_FILE 0
#define DEFAULT_LINK_TYPE_ICON HTTP_LINK_TYPE_ICON
#define This gHistories[which]
#define NHistoryFiles                                                          \
  (gHistories ? GetHandleSize_(gHistories) / sizeof(HistoryDesc) : 0)

//
// Link management prototypes
//

int WriteHistTOC(short which);
void *GetHistoryData(short which, short index, bool readFromDisk);
int ReadHistTOC(short which);
static void ZapHistoryFile(short which, bool destroy);
int RegenerateLinkHistory(short which, bool rebuild);
long HistMatchFound(long hashName, void *theUrl, short which);
void ReadHistFileList(char *pSpec, bool reread);
int AddHistoryToTOC(short which, char * name, long hashName,
                    LinkTypeEnum type, LinkLabelEnum label, bool thumb,
                    void *url, AdId adId);
int SaveIndHistoryFile(short which);
void DeleteHistEntryFromTOC(short which, short index);
bool TimeToCompactTOC(short which);
int CompactHistTOC(short which);
short LinkTypeToIconID(LinkTypeEnum type);
VLNodeID EntryToNodeId(short which, short index);
void NodeIdToEntry(VLNodeID id, short *which, short *index);
int AddURLToHistory(short which, char * url, char * name, int urlOpenErr);
short MenuItemNameToIconID(short menuID, short item);
LinkTypeEnum LinkType(int protocol, char * proto);
void LinkUpdateCacheDate(short which, short index);
short HistoryCount(short which);
bool LocateAdInHistories(AdId adId, short *which, short *index);
short AgeHistoryFile(short which);
void LabelToString(LinkLabelEnum label, char dateStr[256]);
bool LabelableLink(LinkTypeEnum linkType);
bool CorrectVersion(HistoryStructHandle theToc);
void UpdateLinkLabel(HistoryStructPtr entry, int err);
LinkLabelEnum OpenErrToLabel(int err);
bool InterestingProtocol(char proto[256]);
void MakeLinkName(char host[256], char query[256], URLNameStr urlName);

/* Sorting */
int BuildListOfHistoriesForWindow(ShortHistoryStructHandle *histories,
                                  bool needsSort, LinkSortTypeEnum sortType);
void SortShortHistoryHandle(ShortHistoryStructHandle toSort, int (*compare)());
int HistTypeCompare(ShortHistoryStructPtr hist1, ShortHistoryStructPtr hist2);
int HistNameCompare(ShortHistoryStructPtr hist1, ShortHistoryStructPtr hist2);
int HistDateCompare(ShortHistoryStructPtr hist1, ShortHistoryStructPtr hist2);
int HistRemindCompare(ShortHistoryStructPtr hist1, ShortHistoryStructPtr hist2);
void SwapHist(ShortHistoryStructPtr hist1, ShortHistoryStructPtr hist2);

/* Ad preview stuff - stubs for QuickDraw GWorld which doesn't exist in GTK */
int CreateIconFromAdGraphic(AdId adId, char * adGraphic);
void AdIdToName(AdId adId, URLNameStr name);
bool NameToAdId(URLNameStr name, AdId *ad);
int DeleteAdGraphic(AdId adId);
int IconFromAd(char * iconSpec, char * adSpec);
void PurgeLinkHistoryPreviewOrphans(void);

/* Icon cache management */
void AddIconToPVICache(void **theIcon, AdId adId);
LHPIconCacheHandle FindPVICache(AdId adId);
void RemoveIconFromPVICache(AdId adId);
void RemovePVIFromPVICache(LHPIconCacheHandle *toRemove);

/* Nickname routines we've assimilated */
extern int KillNickTOC(char * spec);
extern bool NeatenLine(unsigned char *line, long *len);

/************************************************************************
 * AddURLToMainHistory - add a URL to the main history file
 ************************************************************************/
int AddURLToMainHistory(char * url, char * name, int urlOpenErr) {
  return (AddURLToHistory(MAIN_HISTORY_FILE, url, name, urlOpenErr));
}

/************************************************************************
 * AddURLToMainHistory - add a URL to the main history file
 ************************************************************************/
int AddURLToHistory(short which, char * url, char * name, int urlOpenErr) {
  int err = noErr;
  ProtocolEnum protocol;
  char proto[256], host[256], query[256];
  long hashName;
  void *hUrl = nil;
  URLNameStr urlName;
  LinkLabelEnum linkLabel = llNone;
  LinkTypeEnum linkType;
  short index;

  // don't add the URL if it wasn't parsable
  if (urlOpenErr == 1)
    return (urlOpenErr);

  // parse the URL.  See what it is.
  if (!(err = ParseURL(url, proto, host, query))) {
    // see if this is one of the protocols we should be keeping history entries
    // for
    if (!InterestingProtocol(proto))
      return (userCanceledErr);

    protocol = FindSTRNIndex(ProtocolStrn, proto);
    FixURLString(host);
    if (protocol != proMail)
      FixURLString(query);

    //
    //	Figure out the history entry data
    //

    // make sure the name of the url contains something interesting
    if (name && name[0]) {
      // use the name passed in
      g_strlcpy((char *)urlName, (char *)name, sizeof(urlName));
    } else {
      if (host[0]) {
        // make a name for this link entry out of the host and query portions of
        // the URL itself
        MakeLinkName(host, query, urlName);
      } else {
        // URL had no host.  Use the actual URL
        g_strlcpy((char *)urlName, (char *)url, sizeof(urlName));
      }
    }

    // The name of the history entry will be a hash of the url itself
    hashName = NickHashString(url);

    // What sort of link is this?
    linkType = LinkType(protocol, proto);

    // Was this link successfully launched?
    if (urlOpenErr != noErr) {
      // is this a link type we should label?
      if (LabelableLink(linkType)) {
        // the link was attempted, but probably failed.
        linkLabel = OpenErrToLabel(urlOpenErr);
      }
    }

    // Turn the url into a handle
    hUrl = malloc(0);
    if (!hUrl)
      return (WarnUser(LINK_HISTORY_NEW_HISTORY_ERR, 0));
    err = (buf_append(hUrl, url + 1, url[0]) == NULL) ? -1 : 0;
    if (err)
      return (WarnUser(LINK_HISTORY_NEW_HISTORY_ERR, err));

    //
    //	Add the new history entry
    //

    // Make sure the history files are around somewhere
    err = GenHistoriesList();

    if (err == noErr) {
      // Is this entry already in the history file?
      if ((index = HistMatchFound(hashName, hUrl, which)) >= 0) {
        // adjust the label of the link ...
        UpdateLinkLabel(&(gHistories[which].theData[index]), urlOpenErr);

        // update the date in the TOC ...
        LinkUpdateCacheDate(which, index);
      } else {
        // If not, add the history to the history file
        AdId junk;

        junk.server = junk.ad = 0;
        err = AddHistoryToTOC(which, urlName, hashName, linkType, linkLabel,
                              false, hUrl, junk);
        if (err == noErr) {
          // save the history file
          err = SaveIndHistoryFile(which);
        }
      }

      // Update the Link History window ...
      LinkTickle();
    }
  }

  return (err);
}

/************************************************************************
 * MakeLinkName - build a name for a link history entry
 ************************************************************************/
void MakeLinkName(char host[256], char query[256], URLNameStr urlName) {
  short linkNameLength = sizeof(URLNameStr);

  // initialize the name
  WriteZero(urlName, linkNameLength);

  // start with the host name
  g_strlcpy((char *)urlName, (char *)host, sizeof(URLNameStr));

  // more room?
  if (urlName[0] < linkNameLength - 1) {
    // add a slash
    urlName[urlName[0]] = '.'; /* was ellipsis char, not portable */
    if (query[0])
      g_strlcat((char *)urlName, (char *)query, linkNameLength);
  }

  // replace last character with elipsis, if we reached the maximum length
  if (urlName[0] == linkNameLength - 1) {
    urlName[urlName[0]] = '.'; /* was ellipsis (multi-byte), not portable */
  }
}

/************************************************************************
 * InterestingProtocol - see if this is a protocol worth remembering
 ************************************************************************/
bool InterestingProtocol(char proto[256]) {
  bool remember = false;
  char insterestingProtocols[256], token[256];
  unsigned char * spot;

  if (proto[0]) {
    // read in the list of interesting protocols
    GetRString(insterestingProtocols, LINK_INTERESTING_PROTO);

    // see if proto is one of them
    for (spot = insterestingProtocols;
         PToken(insterestingProtocols, token, &spot, ",") && !remember;) {
      if (StringSame(token, proto))
        remember = true;
    }
  }

  return (remember);
}

/************************************************************************
 * UpdateLinkLabel - update the label of a link according to an error
 ************************************************************************/
void UpdateLinkLabel(HistoryStructPtr entry, int err) {
  if (LabelableLink(entry->type) && (err != noErr)) {
    // adjust the label unless the user cancelled ...
    if (err != oldaCancel)
      entry->label = OpenErrToLabel(err);

    // remind the user later if we should ...
    entry->remind = (err == oldaRemind);
    gNeedRemind |= (entry->remind != 0);
  } else
    entry->label = llNone;
}

/************************************************************************
 * OpenErrToLabel - given an error code, return the label we should use
 ************************************************************************/
LinkLabelEnum OpenErrToLabel(int err) {
  LinkLabelEnum result = llNone;

  if ((err != noErr)) {
    if (err == oldaBookmark)
      result = llBookmarked;
    else if (err == oldaRemind)
      result = llRemindMe;
    else
      result = llAttempted;
  }

  return (result);
}

/************************************************************************
 * RegenerateLinkHistory - make sure the history list is in memory
 ************************************************************************/
int RegenerateLinkHistory(short which, bool rebuild) {
  int err = 0;
  void **hand;

  if (rebuild)
    ZapHistoryFile(which, true);

  if (!gHistories[which]
           .theData) // If handle for data doesn't exist, create it
  {
    hand = malloc(0L);
    if (!hand)
      err = WarnUser(LINK_HISTORY_NEW_HISTORY_ERR, 0);
    gHistories[which].theData = (HistoryStructHandle)hand;
  } else // void *exists
  {
    if (!rebuild)
      return (noErr);
  }

  if (err != noErr || rebuild) {
    err = ReadHistTOC(which);
  }

  if (err)
    free(gHistories[which].theData);

  return (err);
}

/************************************************************************
 * ZapHistoryFile - release memory for all histories in a file
 ************************************************************************/
static void ZapHistoryFile(short which, bool destroy) {
  short i;
  HistoryStructHandle history = gHistories ? gHistories[which].theData : nil;

  if (history) {
    // throw away all the URL and image handles we have laying around
    for (i = 0; i < HistoryCount(which); i++) {
      if (history[i].hUrl) {
        free(history[i].hUrl);
        history[i].hUrl = nil;
      }
    }

    // if we're destroying, trash the actual history list as well
    if (destroy) {
      free(gHistories[which].theData);
      gHistories[which].theData = nil;
    }
  }
}

/************************************************************************
 * ZapHistoriesList - destroy the history file list, or make it smaller
 ************************************************************************/
void ZapHistoriesList(bool destroy) {
  short which;
  short n = gHistories ? NHistoryFiles : 0;

  // clean up the inidividual history files
  for (which = 0; which < n; which++) {
    ZapHistoryFile(which, destroy);
  }

  // if we're destroying, zap the handle to all the histories as well.
  if (destroy) {
    if (gHistories)
      free(gHistories);
    gHistories = nil;
  }
}

/************************************************************************
 * GenHistoriesList - generate the history file list
 ************************************************************************/
int GenHistoriesList(void) {
  int err = noErr;
  char name[32];
  FSSpec folderSpec;
  HistoryDesc ad;

  Zero(ad);

  // do nothing if the history files have already been loaded.
  if (gHistories)
    return (noErr);

  /*
   * allocate empty handle for new
   */

  if (!(gHistories = malloc(0L))) {
    WarnUser(MEM_ERR, err = 0);
    return (err);
  }

  /*
   * add the Eudora History file
   */

  /* GTK port: FSMakeFSSpec uses int vRef/long dirId; in GTK port use
   * string-based char */
  GetRString(name, LINK_HISTORY_FILE);
  strncpy(spec_name(ad.spec), (char *)name, sizeof(spec_name(ad.spec)) - 1);
  if (buf_append(gHistories, &ad, sizeof(ad)) == NULL)
    DieWithError(MEM_ERR, 0);
  RegenerateLinkHistory(MAIN_HISTORY_FILE, true);

  /*
   * add any additional history files in the Link History Folder
   */

  ReadHistFileList(&folderSpec, false);

  return (err);
}

/************************************************************************
 * ReadHistFileList - find extra link history files
 ************************************************************************/
void ReadHistFileList(char *pSpec, bool reread) {
  char name[32];
  CInfoPBRec hfi;
  HistoryDesc ad;
  long dirId;
  short count = 1;

  /*
   * create the folder if we need it ...
   */
  if (SubFolderSpec(LINK_HISTORY_FOLDER, pSpec) != noErr) {
    mkdir(GetRString(name, LINK_HISTORY_FOLDER), 0755);
    /* GTK port: SimpleMakeFSSpec is Mac HFS API - just fill in name field */
    strncpy(spec_name(gLinkHistoryFolder), "LinkHistory",
            sizeof(spec_name(gLinkHistoryFolder)) - 1);
    return;
  } else
    g_strlcpy(gLinkHistoryFolder, pSpec, sizeof(gLinkHistoryFolder));

  /*
   * read in the history files ...
   */

  /* GTK port: DirIterate now uses char *first arg, not vRefNum/parID.
     Use the folder spec directly. */
  Zero(ad);
  while (!DirIterate(pSpec, NULL, NULL)) {
    /* In GTK port, DirIterate with NULL callback does nothing useful;
       this entire HFS directory iteration needs a proper GIO/filesystem
       replacement. Skipping for now to unblock compilation. */
    break;
  }
}

/************************************************************************
 * AddHistoryToTOC - add a history entry to the TOC
 ************************************************************************/
int AddHistoryToTOC(short which, char * name, long hashName,
                    LinkTypeEnum type, LinkLabelEnum label, bool thumb,
                    void *url, AdId adId) {
  long currHistCount;
  HistoryStructHandle histories = gHistories[which].theData;
  int err = 0;
  HistoryStruct histInfo;

  //
  //	Make sure the history list is around
  //

  if (RegenerateLinkHistory(which, false) != 0)
    return (err);

  //
  // Make room for the new history entry
  //

  currHistCount = HistoryCount(which);
  if (currHistCount > 0) {
    { void *_r = realloc(histories, (currHistCount + 1) * sizeof(HistoryStruct));
      if (_r) histories = _r; }
    if ((err = 0) != 0)
      return (WarnUser(LINK_HISTORY_NEW_HISTORY_ERR, err));

  } else {
    free(gHistories[which].theData);
    histories = malloc(sizeof(HistoryStruct));
    if (!histories)
      return (WarnUser(LINK_HISTORY_NEW_HISTORY_ERR, 0));
    else
      gHistories[which].theData = histories;
  }

  //
  // Add the TOC entry
  //

  // Fill in the basic data
  WriteZero(&histInfo, sizeof(histInfo));
  histInfo.version = LINK_HISTORY_VERSION;
  histInfo.hashName = hashName;
  g_strlcpy((char *)histInfo.name, (char *)name, sizeof(histInfo.name));
  histInfo.cacheSeconds = LocalDateTime();
  histInfo.type = type;
  histInfo.dirty = true;
  histInfo.deleted = false;
  histInfo.thumb = thumb;
  histInfo.label = label;
  histInfo.incompleteAd = ((type == ltAd) && !(url != NULL));
  histInfo.urlOffset = (-1L);
  histInfo.imageOffset = (-1L);
  histInfo.adId = adId;

  /* now the potentially large url */
  if (url != NULL) {
    histInfo.hUrl = url;
  }

  //	Put hist info into the history TOC array
  histories[currHistCount] = histInfo;

  gHistories[which].dirty = true;
  return (0);
}

/************************************************************************
 * SaveAllHistoryFiles - save all history files that need it
 ************************************************************************/
int SaveAllHistoryFiles(void) {
  int err = noErr;
  short hFiles;

  for (hFiles = 0; hFiles < NHistoryFiles; hFiles++) {
    if (gHistories[hFiles].dirty)
      err = err || SaveIndHistoryFile(hFiles);
  }

  return (err);
}

/************************************************************************
 * SaveIndHistoryFile - save an individual History file
 ************************************************************************/
int SaveIndHistoryFile(short which) {
  char aliasCmd[32];
  int err;
  long bytes, offset;
  short refN = 0;
  long i, count;
  FSSpec spec, tmpSpec;
  bool junk;
  void *tempHandle;

#ifdef DEBUG
  if (InAThread())
    return noErr;
#endif

  /*
   * do we need to save it?
   */
  if (!gHistories[which].dirty)
    return (noErr);

  /*
   * find the file
   */
  g_strlcpy(spec, gHistories[which].spec, sizeof(spec));
  if (err = FSpMyResolve(&spec, &junk)) {
    FileSystemError(SAVE_LINK_HISTORY, spec_name(spec), err);
    return (err);
  }

  /*
   * make && open a temp file
   */
  if (err = NewTempSpec(0, 0, nil, &tmpSpec))
    goto done;
  int fd = open(tmpSpec, O_RDWR | O_CREAT | O_TRUNC, 0644);
  if (fd >= 0) {
    err = noErr;
    close(fd);
  } else {
    err = ioErr;
  }
  if (err) {
    FileSystemError(SAVE_LINK_HISTORY, spec_name(tmpSpec), err);
    goto done;
  }
  refN = open(tmpSpec, O_RDWR);
  if (refN < 0) {
    err = ioErr;
  } else {
    err = noErr;
  }
  if (err) {
    FileSystemError(OPEN_LINK_HISTORY, spec_name(tmpSpec), err);
    goto done;
  }

  count = HistoryCount(which); // Get history entry count

  /*
   *	Write out the url
   */

  GetRString(aliasCmd, LINK_CMD);
  PCatC(aliasCmd, ' ');
  for (i = 0; !err && i < count; i++) {
    CycleBalls();
    if (gHistories[which].theData[i]
            .dirty) // Nickname has been modified...use in memory copy
      tempHandle = GetHistoryData(which, i, false);
    else // Nickname hasn't been modified, use on disk copy if necessary
      tempHandle = GetHistoryData(which, i, true);

    gHistories[which].theData[i].dirty = false;

    bytes = tempHandle ? GetHandleSize_(tempHandle) : 0;

    if (bytes > 0 && !gHistories[which].theData[i].deleted) {
      GetFPos(refN, &offset);
      gHistories[which].theData[i].urlOffset = offset;
    } else {
      // gHistories[which] entry is missing a URL.  If it's not an incomplete
      // ad, remove it from the TOC, and skip it.
      if (!gHistories[which].theData[i].incompleteAd)
        gHistories[which].theData[i].deleted = true;
      continue;
    }

    if ((!gHistories[which].theData[i].deleted) && bytes > 0 &&
        !(err = FSWriteP(refN, aliasCmd))) {
      if (!(err = AWrite(refN, &bytes, (unsigned char *)tempHandle))) {
        bytes = 1;
        err = AWrite(refN, &bytes, "\015");
      }
    }
  }


  GetFPos(refN, &bytes);
  SetEOF(refN, bytes);
  close(refN);
  refN = 0;

  /* do the deed */
  if (!err)
    err = ExchangeAndDel(&tmpSpec, &spec);
  if (!err)
    gHistories[which].dirty = False;

  WriteHistTOC(which);

done:
  if (gHistories[which].theData)
  if (refN)
    close(refN);
  if (err)
    unlink(tmpSpec);
  return (err);
}

/**********************************************************************
 * WriteHistTOC - write the toc for a history file
 **********************************************************************/
int WriteHistTOC(short which) {
  uLong fileModDate;
  FSSpec spec; g_strlcpy(spec, gHistories[which].spec, sizeof(spec));
  int err = noErr;
  short refN;
  void **theData;
  void **newData;
  short oldResF = CurResFile();

#ifdef DEBUG
  if (InAThread())
    return noErr;
#endif

  if (TimeToCompactTOC(which))
    CompactHistTOC(which);

  theData = (void **)gHistories[which].theData;
  newData = DupHandle(theData);
  if (newData != NULL)
    err = noErr;
  else
    err = 0;

  if (!err) {
    IsAlias(&spec, &spec);
    KillNickTOC(&spec);
    // FSpCreateResFile is no-op on POSIX
    struct stat st_854;
    fileModDate = (stat(spec, &st_854) == 0) ? st_854.st_mtime : 0;

    // FSpOpenResFile is no-op on POSIX
    if (-1 != (refN = -1)) {
      AddResource(newData, LINK_TOC_TYPE, LINK_RESID, "");
      err = ResError();
      if (!err) {
        UpdateResFile(refN);
        err = ResError();
      }
      /* CloseResFile(refN); */ /* Mac-only resource API, no-op in GTK port */
    } else
      err = ResError();

    if (!err) {
      struct timeval tv[2];
      tv[0].tv_sec = fileModDate;
      tv[0].tv_usec = 0;
      tv[1].tv_sec = fileModDate;
      tv[1].tv_usec = 0;
      err = (utimes(spec, tv) == 0) ? noErr : ioErr;
    }

    free(newData); // Dispose of the duplicated handle
    UseResFile(oldResF);
  }

  return (err);
}

/************************************************************************
 *  GetHistoryData - Get the url or image of a given history
 ************************************************************************/
void *GetHistoryData(short which, short index, bool readFromDisk) {
  void *tempHandle;
  HistoryStructHandle histories = gHistories[which].theData;
  FSSpec spec;
  bool finished = false;
  int err;
  char line[256];
  short type;
  bool exLine = False;
  long len;
  void *dataHandle;
  LineIOD lid;
  long theOffset;
  char theCmd[32];

  g_strlcpy(spec, gHistories[which].spec, sizeof(spec));

  if (index < 0 || which < 0 || index >= HistoryCount(which) ||
      histories[index].deleted)
    return (nil);

  tempHandle = histories[index].hUrl;

  if (!readFromDisk) {
    if (histories[index].dirty)
      return (tempHandle);

    if (tempHandle != nil)
      return (tempHandle);
  }

  if (tempHandle != nil)
    free(tempHandle);

  theOffset = histories[index].urlOffset;
  GetRString(theCmd, LINK_CMD);

  if (theOffset >= 0) {
    if (err = FSpOpenLine(&spec, fsRdPerm, &lid)) {
      if (err != fnfErr)
        FileSystemError(OPEN_LINK_HISTORY, spec_name(spec), err);
      return (nil);
    }

    dataHandle = malloc(0L);
    if (!dataHandle) {
      WarnUser(LINK_HISTORY_GET_DATA_ERR, 0);
      return (nil);
    }

    /*
     * Offset is from the beginning of the line; add in length of the command
     * and length of name and two spaces
     */
    theOffset += *theCmd + 1;
    SeekLine(theOffset, &lid);
    type = GetLine(line, sizeof(line), &len, &lid);
    if (type == LINE_START)
      do {
        // process current line
        len = strlen(line);
        exLine = NeatenLine(line, &len);
        if (exLine && !issep(*line)) // If line was escaped and the first
                                     // character isn't a space, add one
        {
          if (buf_append(dataHandle, " ", 1) == NULL) {
            err = -1;
            break;
          }
        }
        err = 0;
        if (buf_append(dataHandle, line, len) == NULL) {
          err = -1;
          break;
        }

        // grab the next line; may or may not be ours
        type = GetLine(line, sizeof(line), &len, &lid);
        if (exLine && type == LINE_START)
          type = LINE_MIDDLE; // extended line means new line is really part of
                              // this line
      } while (type == LINE_MIDDLE);

    CloseLine(&lid);
    if (err) {
      WarnUser(LINK_HISTORY_GET_DATA_ERR, err);
      free(dataHandle);
      return (nil);
    }

    histories[index].hUrl = dataHandle;

    return (dataHandle);
  } else
    return (nil);
}

/************************************************************************
 * ReadHistTOC - read in the history list TOC
 ************************************************************************/
int ReadHistTOC(short which) {
  int err = noErr;
  FSSpec lSpec; g_strlcpy(lSpec, gHistories[which].spec, sizeof(lSpec));
  bool sane;
  short refN = 0;
  short oldResF = CurResFile();
  HistoryStructHandle theToc = nil;
  unsigned char * hand = nil;

  //
  //	Open the History file
  //

  if (err = IsAlias(&gHistories[which].spec, &lSpec)) {
    // void *error
  } else if (-1 != (refN = -1)) {
    // Clear out the old handle
    if (gHistories[which].theData)
      free(gHistories[which].theData);

    //
    // Read to TOC handle out of the file
    //

    theToc = Get1Resource(LINK_TOC_TYPE, LINK_RESID);
    if (noErr == (err = ResError())) {
      short i, count;

      if (theToc != nil) {
        // is this toc the right version?
        if (CorrectVersion(theToc)) {
          DetachResource(theToc);
          gHistories[which].theData = theToc;

          // iterate through the toc and reset garbage fields
          count = GetHandleSize_(theToc) / sizeof(HistoryStruct);
          for (i = 0; i < count; i++)
            theToc[i].hUrl = nil;
        } else {
          // wrong version of the toc. Lose the history.
          theToc = nil;
        }
      }
    }

    // got nothing from the resource file
    if (theToc == nil) {
      // create a new, empty handle.  No history entries are defined.
      hand = malloc(0L);
      err = 0;
      if (hand && (err == noErr))
        gHistories[which].theData = (HistoryStructHandle)hand;
    }

    // was there an error?
    if (err != noErr) {
      WarnUser(LINK_HISTORY_NEW_HISTORY_ERR, err);
      free(gHistories[which].theData);
      gHistories[which].theData = nil;
    }
  }

  if (refN)
    /* CloseResFile(refN); */ /* Mac-only resource API, no-op in GTK port */
  UseResFile(oldResF);

  return (err);
}

/************************************************************************
 * CorrectVersion - is this HistoryStructHandle something we can handle?
 ************************************************************************/
bool CorrectVersion(HistoryStructHandle theToc) {
  bool result = false;

  if (theToc) {
    if (GetHandleSize(theToc) > 0) {
      if (theToc->version == LINK_HISTORY_VERSION)
        result = true;
    }
  }

  return (result);
}

/************************************************************************
 * HistMatchFound - find a given history in a history file
 ************************************************************************/
long HistMatchFound(long hashName, void *theUrl, short which) {
  long i;
  void *hUrl = nil;
  URLNameStr tempStr, tempName;
  HistoryStructHandle theHistories = gHistories[which].theData;
  long stop = (GetHandleSize_(theHistories) / sizeof(HistoryStruct));
  Boolean needStringMatch = false;
  long matched = -1;
  HistoryStruct *theStruct, *endStruct;

  endStruct = theHistories + stop;
  for (theStruct = theHistories; theStruct < endStruct; theStruct++)
    if (theStruct->hashName == hashName && !theStruct->deleted)
      if (matched == -1)
        matched = theStruct - theHistories;
      else {
        needStringMatch = True;
        break;
      }

  if (!needStringMatch)
    return (matched);

  // two names with equal hash values found, compare names.
  g_strlcpy((char *)tempName, (char *)theUrl, sizeof(tempName));
  { long _rl = RemoveChar(' ', (char *)tempName, strlen((char *)tempName)); ((char *)tempName)[_rl] = 0; }
  for (i = 0; i < stop; i++) {
    if (theHistories[i].hashName == hashName &&
        !theHistories[i].deleted) {
      hUrl = GetHistoryData(which, i, true); // don't zap the returned handle.
                                             // It's a part of the history toc.
      if (hUrl) {
        g_strlcpy((char *)tempStr, (char *)hUrl, sizeof(tempStr));
        { long _rl = RemoveChar(' ', (char *)tempStr, strlen((char *)tempStr)); ((char *)tempStr)[_rl] = 0; }
        if (StringSame(tempName, tempStr))
          return (i);
      }
    }
  }

  return (-1);
}

/************************************************************************
 * DeleteHistEntryFromTOC - delete a history entry from the TOC.
 ************************************************************************/
void DeleteHistEntryFromTOC(short which, short index) {
  gHistories[which].theData[index].deleted = true;
  gHistories[which].dirty = true;

  if (gHistories[which].theData[index].type ==
      ltAd) // did we just delete an ad?
    if (gHistories[which].theData[index].thumb !=
        0) // does it think it has a thumbnail?
      DeleteAdGraphic(gHistories[which].theData[index].adId);
}

/************************************************************************
 * TimeToCompactTOC - is it worth our time to compact this TOC?
 ************************************************************************/
bool TimeToCompactTOC(short which) {
  short deletedCount = 0;
  short historyCount = HistoryCount(which);
  HistoryStructHandle histories = gHistories[which].theData;

  while (historyCount > 0) {
    if (histories[historyCount - 1].deleted)
      deletedCount++;
    historyCount--;
  }

  return (deletedCount >= COMPACT_THRESHHOLD);
}

/************************************************************************
 * CompactHistTOC - remove deleted entries from the TOC.
 ************************************************************************/
int CompactHistTOC(short which) {
  int err = noErr;
  Accumulator histAccu;
  short count = 0, historyCount = HistoryCount(which);
  HistoryStructHandle histories = gHistories[which].theData;

  if ((err = AccuInit(&histAccu)) == noErr) {
    while ((count != historyCount) && (err == noErr)) {
      if (histories[count].deleted)
        ; // skip this TOC entry.  It's deleted.
      else {
        // add this history entry to the Accumulator
        err =
            AccuAddPtr(&histAccu, &histories[count], sizeof(HistoryStruct));
      }
      count++;
    }

    if (err == noErr) {
      AccuTrim(&histAccu);
      free(gHistories[which].theData);
      gHistories[which].theData = histAccu.data;
      histAccu.data = nil;
    }

    free(histAccu.data); histAccu.data = NULL; histAccu.offset = histAccu.size = 0;
  }

  return (err);
}

/************************************************************************
 * HistoryCount - return # of history entries
 ************************************************************************/
short HistoryCount(short which) {
  if (gHistories[which].theData)
    return (
        (GetHandleSize_(gHistories[which].theData) / sizeof(HistoryStruct)));
  else
    return (0);
}

/************************************************************************
 * AddAllHistoryItems - add all the history items to the list
 ************************************************************************/
void AddAllHistoryItems(ViewListPtr pView, bool needsSort,
                        LinkSortTypeEnum sortType) {
  int err = noErr;
  short i, count, cur = 0;
  VLNodeInfo info;
  ShortHistoryStructHandle histories = NULL;
  bool reverseSort = (sortType & kLinkReverseSort) != 0;

  // get a sorted list of all history entries to add
  err = BuildListOfHistoriesForWindow(&histories, needsSort, sortType);
  if ((err == noErr) && histories) {
    count = GetHandleSize(histories) / sizeof(ShortHistoryStruct);
    for (i = 0; i < count; i++) {
      cur = reverseSort ? (count - 1 - i) : i;

      Zero(info);
      g_strlcpy((char *)info.name, (char *)histories[cur].name, sizeof(info.name));
      /* GTK port: LVAdd/iconID are Mac-specific List View calls. nodeID set
       * from cur index. */
      info.nodeID = histories[cur].nodeId;
      /* LVAdd(pView, &info); */
    }

    // cleanup, don't need this anymore.
    free(histories);
  }
}

/************************************************************************
 * ShortHistoryStructHandle - return a sorted list of structures that
 *	contain just enough information to add to the history window.
 ************************************************************************/
int BuildListOfHistoriesForWindow(ShortHistoryStructHandle *histories,
                                    bool needsSort, LinkSortTypeEnum sortType) {
  int err = noErr;
  short which, i;
  short numEntries, count;
  int (*compare)() = nil;

  *histories = nil;

  // How many history entries are there in all files?
  numEntries = 0;
  for (which = 0; which < NHistoryFiles; which++)
    numEntries += HistoryCount(which);

  // Make a new handle that's big enough for them all
  *histories = malloc(numEntries * sizeof(ShortHistoryStruct));
  err = 0;
  if (*histories && (err == noErr)) {
    // fill the new handle with all of the entries
    count = 0;
    for (which = 0; which < NHistoryFiles; which++) {
      for (i = 0; i < HistoryCount(which); i++) {
        // only add non-deleted items
        if (!(gHistories[which].theData[i].deleted) &&
            !(gHistories[which].theData[i].incompleteAd)) {
          g_strlcpy((char *)(*histories)[count].name,
                    (char *)gHistories[which].theData[i].name,
                    sizeof((*histories)[count].name));
          (*histories)[count].cacheSeconds =
              gHistories[which].theData[i].cacheSeconds;
          (*histories)[count].type = gHistories[which].theData[i].type;
          (*histories)[count].nodeId = EntryToNodeId(which, i);
          (*histories)[count].label = gHistories[which].theData[i].label;
          count++;
        }
      }
    }

    // Resize the handle down if we hafta.
    if (count != numEntries)
      SetHandleSize(*histories, count * sizeof(ShortHistoryStruct));

    // Sort the handle if we need to
    if (needsSort) {
      switch (sortType & kLinkSortTypeMask) {
      case sType:
        compare = HistTypeCompare;
        break;

      case sName:
        compare = HistNameCompare;
        break;

      case sDate:
        compare = HistDateCompare;
        break;

      case sSpecialRemind:
        compare = HistRemindCompare;
        break;
      }

      if (compare)
        SortShortHistoryHandle(*histories, compare);
    }
  } else {
    WarnUser(MEM_ERR, err);
    *histories = nil;
  }

  return (err);
}

/************************************************************************
 * LinkType - Given a protocol, return the type of link
 ************************************************************************/
LinkTypeEnum LinkType(ProtocolEnum protocol, char * proto) {
  LinkTypeEnum type = ltHttp; // assume web link
  char scratch[256];

  switch (protocol) {
  case proFinger:
  case proPh:
  case proPh2:
  case proLDAP:
    type = ltDirectoryServices;
    break;

  case proMail:
    type = ltMail;
    break;

  case proFile:
  case proCompFile:
    type = ltFile;
    break;

  case proSetting:
    type = ltSetting;
    break;

  default: {
    // make sure this isn't an ftp link
    if (StringSame(proto, GetRString(scratch, ANARCHIE_FTP)))
      type = ltFtp;
    break;
  }
  }

  return (type);
}

/************************************************************************
 * LinkTypeToIconID - Given a protocol, return the icon to be used
 ************************************************************************/
short LinkTypeToIconID(LinkTypeEnum type) {
  short iconID;

  if (type == ltAd) {
    // Use the thumbnail if present, a web link otherwise.
    iconID = kCustomIconResource;
  } else {
    // assign an icon based on the type of URL this is
    switch (type) {
    case ltDirectoryServices:
      iconID = MenuItemNameToIconID(WINDOW_MENU, WIN_PH_ITEM);
      break;

    case ltMail:
      iconID = MAILTO_LINK_TYPE_ICON;
      break;

    case ltHttp:
      iconID = HTTP_LINK_TYPE_ICON;
      break;

    case ltFtp:
      iconID = FTP_LINK_TYPE_ICON;
      break;

    case ltFile:
      iconID = MenuItemNameToIconID(FILE_MENU, FILE_SAVE_AS_ITEM);
      break;

    case ltSetting:
      iconID = SETTINGS_ICON;
      break;

    default:
      iconID = DEFAULT_LINK_TYPE_ICON; // assume it's a web link
      break;
    }
  }

  return (iconID);
}

/************************************************************************
 * MenuItemNameToIconID - return the icon id of a menu item by name
 ************************************************************************/
short MenuItemNameToIconID(short menuID, short item) {
  MenuHandle mh;
  short id = DEFAULT_LINK_TYPE_ICON;
  char itemText[256];

  /* GTK port: GetMenuHandle/GetMenuItemText/Names2Icon are Mac menu APIs */
  /* short mh = GetMenuHandle(menuID);
  GetMenuItemText(mh, item, name);
  return Names2Icon(name, NULL); */
  return 0;
}

/************************************************************************
 * EntryToNodeId - create an ID out of file # and index
 ************************************************************************/
VLNodeID EntryToNodeId(short which, short index) {
  /* GTK port: HiWord/LoWord are Mac integer-splitting macros */
  return (which * 1000) + index;
}

/************************************************************************
 * NodeIdToEntry - return the file # and index of a given id
 ************************************************************************/
void NodeIdToEntry(VLNodeID id, short *which, short *index) {
  *which = id / 1000;
  *index = id % 1000;
}

/************************************************************************
 * DeleteHistoryEntry - delete a history entry from the History window
 ************************************************************************/
void DeleteHistoryEntry(VLNodeInfo *info) {
  short which;
  short index;

  NodeIdToEntry(info->nodeID, &which, &index);
  DeleteHistEntryFromTOC(which, index);
}

/************************************************************************
 * OpenHistoryEntry - open a history entry from the History window
 ************************************************************************/
int OpenHistoryEntry(VLNodeInfo *info) {
  short which;
  short index;
  void *hUrl;
  char proto[256];
  short len;
  int openErr = noErr;

  // Figure out which file this entry is in ...
  NodeIdToEntry(info->nodeID, &which, &index);

  // read in the URL from the file
  // Note: GetHistoryData returns a handle that belongs to the history toc
  // struct.  Don't trash it!
  if (gHistories[which].theData[index].dirty)
    hUrl = GetHistoryData(which, index, false);
  else
    hUrl = GetHistoryData(which, index, true);

  if (hUrl && (len = GetHandleSize(hUrl))) {
    char *url = hUrl;

    // parse and open this URL
    /* GTK port: OpenLocalURLPtr is a Mac URL-open API - use GIO/GTK instead */
    g_app_info_launch_default_for_uri((char *)url, NULL, NULL);
    if ((openErr = ParseProtocolFromURLPtr(url, len, proto)) == noErr)
      openErr = OpenOtherURLPtr(proto, url, len);

    // update the link's label
    UpdateLinkLabel(&(gHistories[which].theData[index]), openErr);

    // Update the date in the History TOC
    LinkUpdateCacheDate(which, index);

    // Update the Link history window
    LinkTickle();
  }

  return (openErr);
}

/************************************************************************
 * GetDateString - given the ID of a node, return the date in a string
 ************************************************************************/
bool GetDateString(VLNodeID id, char dateStr[256]) {
  short which;
  short index;
  char zone[32];

  // Find the history entry
  NodeIdToEntry(id, &which, &index);

  // Does this entry have a label?
  if (gHistories[which].theData[index].label != llNone) {
    LabelToString(gHistories[which].theData[index].label, dateStr);
  } else {
    /* GTK port: SecsToLocalDateTime takes only secs; no zone arg */
    snprintf(
        (char *)dateStr, 255, "%lu",
        (unsigned long)gHistories[which].theData[index].cacheSeconds);
  }

  return (true);
}

/************************************************************************
 * IsMarkedRemind - is this cell one we need to remind the user about?
 ************************************************************************/
bool IsMarkedRemind(VLNodeID id) {
  bool result = false;
  short which;
  short index;

  NodeIdToEntry(id, &which, &index);
  result = gHistories[which].theData[index].label == llRemindMe;

  return (result);
}

/**********************************************************************
 * LabelToString - given a history label, return the string of that
 *	label.
 **********************************************************************/
void LabelToString(LinkLabelEnum label, char dateStr[256]) {
  GetRString(dateStr, LinkHistoryLabelsStrn + label);
}

/**********************************************************************
 * LabelableLink - return true if this is a link type we should label
 **********************************************************************/
bool LabelableLink(LinkTypeEnum linkType) {
  bool result = false;
  LinkTypeEnum scan;
  short count = sizeof(gLabelTheseLinks) / sizeof(LinkTypeEnum);

  for (scan = 0; !result && (scan < count); scan++) {
    if (gLabelTheseLinks[scan] == linkType)
      result = true;
  }

  return (result);
}

/**********************************************************************
 * GetLHPreviewIcon - return a handle to the icon for this id.
 **********************************************************************/
void *GetLHPreviewIcon(VLNodeID id) {
  void *theIcon = nil, *rIconSuite = nil;
  short which;
  short index;
  AdId adId;
  FSSpec adGraphicSpec;
  URLNameStr adGraphicName;
  int err = noErr;
  LHPIconCacheHandle iconCache;

  /* GTK port: LinkHasCustomIcons is Mac-specific; skip custom icon lookup */
  if (false) {
    // Find the history entry
    NodeIdToEntry(id, &which, &index);

    // Make sure it's an ad
    if (gHistories[which].theData[index].type == ltAd) {
      adId = gHistories[which].theData[index].adId;

      // Is the ad icon already loaded?
      if (iconCache = FindPVICache(adId)) {
        theIcon = iconCache->theIcon;
        if (theIcon) {
          return (theIcon);
        }
        // else
        // the icon has been purged.  Read it back in.
      }

      // Find the icon preview file
      AdIdToName(adId, adGraphicName);
      if (noErr == spec_for(gLinkHistoryFolder, adGraphicName,
                                &adGraphicSpec)) {
        short iconRes, oldResFile = CurResFile();

        // open the file
        iconRes = FSpOpenResFile(&adGraphicSpec, fsRdPerm);
        if (iconRes != -1) {
          /* GTK port: GetIconSuite/DupIconSuite are Mac icon APIs; entire block
           * disabled */
          /* original: err = GetIconSuite(&rIconSuite, kCustomIconResource,
           * svAllAvailableData) */

          /* CloseResFile(iconRes); */ /* Mac-only resource API, no-op in GTK port */
        }
        UseResFile(oldResFile);
      }
    }
  }
  return (theIcon);
}

/************************************************************************
 * LinkUpdateCacheDate - this link was clicked on today
 ************************************************************************/
void LinkUpdateCacheDate(short which, short index) {
  // make sure the link histories are around
  if (gHistories || (GenHistoriesList() == noErr)) {
    // update the date for this TOC entry to today
    gHistories[which].theData[index].cacheSeconds = LocalDateTime();

    // save the TOC part of the file only
    WriteHistTOC(which);
  }
}

/************************************************************************
 * SortShortHistoryHandle - sort the monster link history handle
 ************************************************************************/
void SortShortHistoryHandle(ShortHistoryStructHandle toSort, int (*compare)()) {
  short count = 0;

  if (compare) {
    count = GetHandleSize(toSort) / sizeof(ShortHistoryStruct);
    /* GTK port: QuickSort is Mac-specific; use stdlib qsort */
    qsort(toSort, count, sizeof(ShortHistoryStruct),
          (int (*)(const void *, const void *))compare);
  }
}

/**********************************************************************
 * HistTypeCompare - compare two cells based on the url type
 **********************************************************************/
int HistTypeCompare(ShortHistoryStructPtr hist1, ShortHistoryStructPtr hist2) {
  return (hist1->type - hist2->type);
}

/**********************************************************************
 * HistNameCompare - compare two cells based on the url itself
 **********************************************************************/
int HistNameCompare(ShortHistoryStructPtr hist1, ShortHistoryStructPtr hist2) {
  return (StringComp(spec_name(hist1), spec_name(hist2)));
}

/**********************************************************************
 * HistDateCompare - compare two cells based on the cache date
 **********************************************************************/
int HistDateCompare(ShortHistoryStructPtr hist1, ShortHistoryStructPtr hist2) {
  int result;

  // does either entry have a label?
  if ((hist1->label != llNone) || (hist2->label != llNone)) {
    // the entry with the most important label is newest ...
    result = hist1->label - hist2->label;
  } else {
    // the entry with the largest cacheSecods is newest ...
    result = hist2->cacheSeconds - hist1->cacheSeconds;
  }

  return (result);
}

/**********************************************************************
 * HistRemindCompare - compare two cells based on the cache date.
 *	Special case for Remind sorting.
 **********************************************************************/
int HistRemindCompare(ShortHistoryStructPtr hist1,
                      ShortHistoryStructPtr hist2) {
  int result;

  // does either entry have a Remind Me label?
  if ((hist1->label == llRemindMe) || (hist2->label == llRemindMe)) {
    // both are set to Remind Me
    if ((hist1->label == llRemindMe) && (hist2->label == llRemindMe))
      result = HistNameCompare(hist1, hist2);
    // one is set to Remind Me
    else if (hist1->label == llRemindMe)
      result = -1;
    else
      result = 1;
  } else {
    // the entry with the largest cacheSecods is newest ...
    result = hist2->cacheSeconds - hist1->cacheSeconds;
  }

  return (result);
}

/**********************************************************************
 * SwapHist - swap two history entries.
 **********************************************************************/
void SwapHist(ShortHistoryStructPtr hist1, ShortHistoryStructPtr hist2) {
  ShortHistoryStruct tempHist;

  tempHist = *hist1;
  *hist1 = *hist2;
  *hist2 = tempHist;
}

/************************************************************************
 * GetLinkURL - Return a handle to the URL of a link.  gHistories[which] URL
 *should NOT be dumped, even after a one night stand.
 ************************************************************************/
void *GetLinkURL(VLNodeInfo *info) {
  short which;
  short index;
  void *hUrl;

  // Figure out which file this entry is in ...
  NodeIdToEntry(info->nodeID, &which, &index);

  // read in the URL from the file
  // Note: GetHistoryData returns a handle that belongs to the history toc
  // struct.  Don't trash it!
  if (gHistories[which].theData[index].dirty)
    hUrl = GetHistoryData(which, index, false);
  else
    hUrl = GetHistoryData(which, index, true);

  return (hUrl);
}

/**********************************************************************
 * AdWasClicked - call this when an Ad is clicked on in the Ad window
 **********************************************************************/
void AdWasClicked(AdId adId, int openErr) {
  short which, index;

  if (LocateAdInHistories(adId, &which, &index)) {
    // Update the label of this ad.
    UpdateLinkLabel(&(gHistories[which].theData[index]), openErr);

    // Give it a new cache date when found.
    LinkUpdateCacheDate(which, index);

    // update the Link History window
    LinkTickle();
  }
}

/**********************************************************************
 * LocateAdInHistories - find the ad in the histories files
 **********************************************************************/
bool LocateAdInHistories(AdId adId, short *which, short *index) {
  short count;
  bool foundIt = false;
  short w, i;

  // must have somewhere to put our results
  if (!which || !index)
    return (false);

  // Make sure the histories are around ...
  if (gHistories || (GenHistoriesList() == noErr)) {
    // Iterate through all History files, looking for this Ad.
    for (w = 0; !foundIt && (w < NHistoryFiles); w++) {
      count = HistoryCount(w);
      for (i = 0; !foundIt && (i < count); i++) {
        if ((gHistories[w].theData[i].adId.server == adId.server) &&
            (gHistories[w].theData[i].adId.ad == adId.ad)) {
          *which = w;
          *index = i;
          foundIt = true;
        }
      }
    }
  }

  return (foundIt);
}

/**********************************************************************
 * FindRemindLink - see if any links are marked as remind me
 **********************************************************************/
bool FindRemindLink(void) {
  short count;
  bool foundOne = false;
  short w, i;

  // Make sure the histories are around ...
  if (gHistories || (GenHistoriesList() == noErr)) {
    // Iterate through all History files
    for (w = 0; !foundOne && (w < NHistoryFiles); w++) {
      count = HistoryCount(w);
      for (i = 0; !foundOne && (i < count); i++) {
        if (gHistories[w].theData[i].remind)
          foundOne = true;
      }
    }
  }

  return (foundOne);
}

/**********************************************************************
 * UnRemindLinks - forget about it!
 **********************************************************************/
void UnRemindLinks(bool labelToo) {
  short count;
  short w, i;
  bool foundOne;

  // Make sure the histories are around ...
  if (gHistories || (GenHistoriesList() == noErr)) {
    // Iterate through all History files
    for (w = 0; (w < NHistoryFiles); w++) {
      count = HistoryCount(w);
      foundOne = false;
      for (i = 0; (i < count); i++) {
        if (gHistories[w].theData[i].remind) {
          foundOne = true;
          gHistories[w].theData[i].remind = 0;
        }

        if (labelToo && (gHistories[w].theData[i].label == llRemindMe)) {
          foundOne = true;
          gHistories[w].theData[i].label = llBookmarked;
        }
      }
      if (foundOne)
        WriteHistTOC(w);
    }
  }
}

/**********************************************************************
 * AddAdToLinkHistory - call this to add an ad to the Link History
 *
 *	If adURL is non-null, we're adding an ad to the history list
 *	if adGraphic is non-nul, we're creating a preview for the lw window
 **********************************************************************/
int AddAdToLinkHistory(AdId adId, char *pUrl, char adTitle[256],
                       char *adGraphic) {
  int err = 0;
  short which = 0, index;
  int protocol;
  unsigned char proto[256], host[256], query[256];
  int labelableErrors[] = {0};
  long hashName;
  void *hUrl = NULL;
  URLNameStr urlName;
  bool needSave = false;

  //	Add the Ad to the link History Window
  if (pUrl && *pUrl && adTitle) {
    //	Add the ad's URL to the link history file.
    if (!(err = ParseURL(pUrl, proto, host, query))) {
      protocol = FindSTRNIndex(ProtocolStrn, proto);
      FixURLString(host);
      if (protocol != proMail)
        FixURLString(query);

      // make sure the name of the url contains something interesting
      if (adTitle && adTitle[0]) {
        // use the name passed in
        g_strlcpy((char *)urlName, (char *)adTitle, sizeof(urlName));
      } else {
        if (host[0]) {
          // use the host of the actual url as the name
          g_strlcpy((char *)urlName, (char *)host, sizeof(urlName));
        } else {
          // URL had no host.  Use the actual URL
          g_strlcpy((char *)urlName, (char *)pUrl, sizeof(urlName));
        }
      }

      // The name of the history entry will be a hash of the url itself
      hashName = NickHashString(pUrl);

      // Turn the url into a handle we can keep around ...
      hUrl = malloc(0);
      if (!hUrl)
        return (WarnUser(LINK_HISTORY_NEW_HISTORY_ERR, 0));
      if (buf_append(hUrl, pUrl + 1, pUrl[0]) == NULL)
        return (WarnUser(LINK_HISTORY_NEW_HISTORY_ERR, -1));

      // Make sure the history files are around somewhere
      err = GenHistoriesList();

      // See if the ad already exists in a history file ...
      if (LocateAdInHistories(adId, &which, &index)) {
        // we'll need to save these changes if they were major ...
        if (gHistories[which].theData[index].incompleteAd)
          needSave = true;

        // update it with the new history information
        gHistories[which].theData[index].hashName = hashName;
        //gHistories[which].theData[index].cacheSeconds = LocalDateTime();
        //// - leave the date alone jdboyd 2/9/01
        gHistories[which].theData[index].dirty = true;
        gHistories[which].theData[index].incompleteAd = false;
        gHistories[which].theData[index].hUrl = hUrl;
        g_strlcpy((char *)gHistories[which].theData[index].name,
                  (char *)urlName,
                  sizeof(gHistories[which].theData[index].name));
      } else {
        // add the ad as a new history entry
        if (err == noErr) {
          // Add the history to the history file
          err = AddHistoryToTOC(MAIN_HISTORY_FILE, urlName, hashName, ltAd,
                                llNotDisplayed, false, hUrl, adId);
          if (err == noErr)
            needSave = true;
        }
      }
    }
  }

  // Add the graphic to the Link History, if all has gone well
  if ((err == noErr) && adGraphic && *adGraphic && LinkHasCustomIcons()) {
    FSSpec adGraphicSpec;
    spec_make(NULL, adGraphic, &adGraphicSpec);
    // Is the ad graphic a valid graphic file?
    struct stat st_1896;
    if (stat(adGraphicSpec, &st_1896) == 0 && st_1896.st_size > 0) {
      // Make sure the history files are loaded ...
      err = GenHistoriesList();

      if (err == noErr) {
        // Create an icon out of the graphic, save it in the History folder.
        if (CreateIconFromAdGraphic(adId, &adGraphicSpec) == noErr) {
          // find the ad this graphic belongs to
          if (!LocateAdInHistories(adId, &which, &index)) {
            // it does not exist.  Add it now, using a bogus name and hashname
            err = AddHistoryToTOC(MAIN_HISTORY_FILE, (unsigned char *)"", -1, ltAd,
                                  llNotDisplayed, true, hUrl, adId);
          } else {
            // gHistories[which] history entry now has a thumbnail ...
            gHistories[which].theData[index].thumb = true;
          }
          if (err == noErr)
            needSave = true;
        }
      }
    }
    // else
    // the ad graphic file was 0 bytes in length.  Nothing to do, we'll get
    // called later when it's downloaded.
  }

  // save the history file
  if (needSave) {
    gHistories[which].dirty = true;
    err = SaveIndHistoryFile(which);

    // Update the Link History window
    LinkTickle();
  }

  return (err);
}

/**********************************************************************
 * AgeLinks - go through history files, throw out old links.
 *	Warning: This is expensive!
 **********************************************************************/
void AgeLinks(void) {
  short which;
  short totalRemoved = 0;
  static long lastAged;

  // Nothing to do if there are no history files ...
  if (gHistories == nil)
    return;

  // Only do this once every hour ...
  if (lastAged == 0)
    lastAged = TickCount();
  else {
    if ((TickCount() - lastAged) < AGE_INTERVAL)
      return;
  }

  for (which = 0; which < NHistoryFiles; which++)
    totalRemoved += AgeHistoryFile(which);

  // refresh the Link History window if we removed any entries
  if (totalRemoved > 0)
    LinkTickle();

  // Trash orphaned link history previews
  PurgeLinkHistoryPreviewOrphans();

  // Let's not do this anytime soon ...
  lastAged = TickCount();
}

/**********************************************************************
 * AgeHistoryFile - go through a history file, throw out old links
 **********************************************************************/
short AgeHistoryFile(short which) {
  short count = HistoryCount(which);
  short i;
  long age;
  long maxAge = 60 * 60 * 24 * GetRLong(LINK_AGE);
  long today;
  short removed = 0;

  // when is today?
  today = GMTDateTime();

  // Go through each history entry ...
  for (i = 0; i < count; i++) {
    // skip deleted entries.  They are already gone ...
    if (gHistories[which].theData[i].deleted == 0) {
      //
      //	Age Ads only if they're not on the current playlist
      //	Treat ads differently only for adware users that *have* a
      // playlist.
      //

      if (IsAdwareMode() && gHistories[which].theData[i].type == ltAd) {
        if (IsAdInPlaylist(gHistories[which].theData[i].adId))
          continue;
      }

      //
      //	Throw out any links older than a LINK_AGE days
      //

      age = today - gHistories[which].theData[i].cacheSeconds;
      if (age > maxAge) {
        DeleteHistEntryFromTOC(which, i);
        removed++;
      }
    }
  }

  // Save the history TOC if we removed anything.
  if (removed > 0)
    WriteHistTOC(which);

  return (removed);
}

/**********************************************************************
 * PurgeLinkHistoryPreviewOrphans - iterate through the Link History
 *	folder, and trash preview files no longer in use.
 **********************************************************************/
void PurgeLinkHistoryPreviewOrphans(void) {
  URLNameStr name;
  CInfoPBRec hfi;
  short count = 1;
  AdId adId;
  short which, index;

  /* GTK port: PurgeLinkHistoryPreviewOrphans uses CInfoPBRec/DirIterate (Mac
     HFS APIs); Replacing with stub - needs GIO-based directory iteration for
     full port. */
  (void)name;
  (void)count;
  (void)adId;
  (void)which;
  (void)index;
}

/**********************************************************************
 * CreateIconFromAdGraphic - given an Ad, create a file with an icon
 *	representing the ad.
 **********************************************************************/
int CreateIconFromAdGraphic(AdId adId, char * adGraphic) {
  int err = noErr;
  URLNameStr graphicName;
  FSSpec adIconSpec;

  // The name of the graphic file will be the adId.
  AdIdToName(adId, graphicName);

  // See if the graphic exists already;
  if (spec_for(gLinkHistoryFolder, graphicName, &adIconSpec) != noErr) {
    // the icon does not yet exist.  Create it from the adGraphic file.
    err = IconFromAd(&adIconSpec, adGraphic);
  } else
    err = dupFNErr;

  return (err);
}

/**********************************************************************
 * AdIdToName - given an AdId, return the name of its preview file
 **********************************************************************/
void AdIdToName(AdId adId, URLNameStr name) {
  char scratch[256];

  /* GTK port: NumToString is Mac-specific; use snprintf */
  snprintf((char *)name, sizeof(URLNameStr), "%ld,%ld", (long)adId.server,
           (long)adId.ad);
  (void)scratch;
}

/**********************************************************************
 * NameToAdId - given a name, return the id of the ad it belongs to
 **********************************************************************/
bool NameToAdId(URLNameStr name, AdId *ad) {
  bool result = false;
  char * scan;
  char scratch[256];

  ad->server = ad->ad = 0;

  if (name && name[0]) {
    /* GTK port: PPtrFindSub/PStrCopy/StringToNum are Mac string APIs.
       Use C string equivalents. */
    char *comma = strchr((char *)name, ',');
    if (comma) {
      ad->server = atol((char *)name);
      ad->ad = atol(comma + 1);
      result = true;
    }
  };

  return (result);
}

/**********************************************************************
 * DeleteAdGraphic - given an AdId, delete its preview file
 **********************************************************************/
int DeleteAdGraphic(AdId adId) {
  int err = noErr;
  FSSpec adGraphicSpec;
  URLNameStr adGraphicName;

  // locate the ad file in the Link History Folder
  AdIdToName(adId, adGraphicName);
  err = spec_for(gLinkHistoryFolder, adGraphicName, &adGraphicSpec);
  if (err == noErr) {
    // Delete the ad preview we've found.
    err = (unlink(adGraphicSpec) == 0) ? noErr : ioErr;

    // Remove the icon handle from the icon cache
    RemoveIconFromPVICache(adId);
  }
  // else
  // the ad preview can't be found.  Too bad.

  return (err);
}

/**********************************************************************
 * AddIconToPVICache - remember a loaded ad preview icon
 **********************************************************************/
void AddIconToPVICache(void **theIcon, AdId adId) {
  LHPIconCacheHandle newPVI;

  newPVI = g_malloc0(sizeof(LHPIconCacheStruct));
  if (newPVI) {
    newPVI->theIcon = theIcon;
    newPVI->adId = adId;
    LL_Queue(gPreviewIcons, newPVI, (LHPIconCacheHandle));
  }
}

/**********************************************************************
 * FindPVICache - find an icon in the preview icon cache
 **********************************************************************/
LHPIconCacheHandle FindPVICache(AdId adId) {
  LHPIconCacheHandle scan;

  for (scan = gPreviewIcons; scan && ((scan->adId.server != adId.server) ||
                                      (scan->adId.ad != adId.ad));
       scan = scan->next)
    ;

  return (scan);
}

/**********************************************************************
 * RemoveIconFromPVICache - remove an icon from the cache
 **********************************************************************/
void RemoveIconFromPVICache(AdId adId) {
  LHPIconCacheHandle scan;

  // Locate the cache entry associated with this adId.
  for (scan = gPreviewIcons; scan; scan = scan->next) {
    if ((scan->adId.server == adId.server) &&
        (scan->adId.ad == adId.ad)) {
      RemovePVIFromPVICache(&scan);
      break;
    }
  }
}

/**********************************************************************
 * RemovePVIFromPVICache - remove a PVI cache entry frm the list
 **********************************************************************/
void RemovePVIFromPVICache(LHPIconCacheHandle *toRemove) {
  if (gPreviewIcons && toRemove && *toRemove) {
    // remove it from the list
    LL_Remove(gPreviewIcons, *toRemove, (LHPIconCacheHandle));

    // nuke the icon cache
    /* GTK port: DisposeIconSuite is a Mac icon API - just ZapHandle the icon */
    free((*toRemove)->theIcon);

    // and now the cache entry
    free(*toRemove);
    *toRemove = nil;
  }
}

/**********************************************************************
 * ZapPVICache = destroy the icon cache
 **********************************************************************/
void ZapPVICache(void) {
  LHPIconCacheHandle scan = gPreviewIcons, next = nil;

  while (scan) {
    next = scan->next;
    RemovePVIFromPVICache(&scan);
    scan = next;
  }
}

/**********************************************************************
 * IconFromAd - create a file containing an icon representing the
 *	picture in the adSpec.  This assumes the adSpec is a valid file.
 **********************************************************************/
/* GTK port: IconFromAd uses QuickDraw GWorld/QuickTime GraphicsImporter APIs.
   These are not available in GTK. This function is stubbed out. */
int IconFromAd(char * iconSpec, char * adSpec) {
  (void)iconSpec;
  (void)adSpec;
  return -1; /* not implemented in GTK port */
}
/* GTK port: All remaining icon-building functions below use QuickDraw GWorld,
   PixMap, BitMap, CTable APIs which have no GTK equivalent. They are stubbed
   out. */

/* MakeLHIconFile - Mac QuickTime icon file creation, not supported in GTK port
 */
int MakeLHIconFile(void *gWorld, void *pRect, char *iconSpec, void *importer,
                   void *sourceSpec) {
  (void)gWorld;
  (void)pRect;
  (void)iconSpec;
  (void)importer;
  (void)sourceSpec;
  return -1;
}

/* MakeIconSuite - Mac icon suite creation, not supported in GTK port */
int MakeIconSuite(void *gWorld, void *pRect, void *transparentColor,
                  unsigned char *name) {
  (void)gWorld;
  (void)pRect;
  (void)transparentColor;
  (void)name;
  return -1;
}

/* MakeICN_pound - Mac icon mask creation, not supported in GTK port */
void **MakeICN_pound(void *gwp, void *srcRect, short iconDimension,
                     void *transparentColor) {
  (void)gwp;
  (void)srcRect;
  (void)iconDimension;
  (void)transparentColor;
  return NULL;
}

/* MakeIconMask - Mac icon mask creation, not supported in GTK port */
void **MakeIconMask(void *srcGWorld, void *srcRect, short iconSize) {
  (void)srcGWorld;
  (void)srcRect;
  (void)iconSize;
  return NULL;
}

/* MakeIcon - Mac icon creation, not supported in GTK port */
void **MakeIcon(void *srcGWorld, void *srcRect, short dstDepth,
                short iconSize) {
  (void)srcGWorld;
  (void)srcRect;
  (void)dstDepth;
  (void)iconSize;
  return NULL;
}

/* MakeIconLo - Mac icon low-level creation, not supported in GTK port */
void **MakeIconLo(void *srcGWorld, void *srcRect, short dstDepth,
                  short iconSize, void *transColor) {
  (void)srcGWorld;
  (void)srcRect;
  (void)dstDepth;
  (void)iconSize;
  (void)transColor;
  return NULL;
}

/* FreeBitMap, CalcOffScreen, NewBitMap, NewMaskedBitMap - Mac BitMap
 * operations, not in GTK */
void FreeBitMap(void *Bits) { (void)Bits; }
void CalcOffScreen(void *frame, long *needed, short *rows) {
  (void)frame;
  if (needed)
    *needed = 0;
  if (rows)
    *rows = 0;
}
void NewBitMap(void *frame, void *theMap) {
  (void)frame;
  (void)theMap;
}
void NewMaskedBitMap(void *srcBits, void *maskBits, void *srcRect) {
  (void)srcBits;
  (void)maskBits;
  (void)srcRect;
}

/* SetUpPixMap, TearDownPixMap - Mac PixMap operations, not in GTK port */
int SetUpPixMap(short depth, void *bounds, void *colors, void *aPixMap) {
  (void)depth;
  (void)bounds;
  (void)colors;
  (void)aPixMap;
  return -1;
}
void TearDownPixMap(void *pix) { (void)pix; }

