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

#ifndef NICKMNG_H
#define NICKMNG_H

/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

#include "mailbox.h"
#include "mydefs.h"
#include "trans.h"

/* StringHandle defined in portable-compat.h as char ** */

// Nickname TOC flags (new for 5.0)
typedef enum {
  nfNone = 0x00000000,
  nfMultipleAddresses = 0x00000001,
  nfAllFlags = 0xFFFFFFFF
} NicknameFlagsType;

typedef enum {
  changeBitUnmodified = 0x0000,
  changeBitModified = 0x0001,
  changeBitDeleted = 0x0002,
  changeBitAdded = 0x0004,
  changeBitArchived = 0x0008,
  changeBitPrivate = 0x0010
} ChangeBitType;

#define AllButPrivate (~changeBitUnmodified & ~changeBitPrivate)

typedef enum {
  nickFieldReplaceExisting, // Replace the value if the field is found -- create
                            // it if not
  nickFieldAppendExisting,  // Append the value if the field is found -- create
                            // it if not
  nickFieldIgnoreExisting,  // Ignore this field if it already exists
} NickFieldSetValueType;

typedef enum { noPrimary, homePrimary, workPrimary } PrimaryLocationType;

/* Structure for a given nickname */
//	ALB 7/15/96, took out handle to nickname and to NickInfoStruct. Added
// nameTOCOffset, 		theViewData, theAddresses, and theNotes
typedef struct {
  long hashName;    /* Hash value on nickname */
  long hashAddress; /* Hash value on address */
                    //	long	createDate;				/* Creation date for
  // this nickname (eventually) */ 	long	modDate;
  ///* Modification date for this nickname (eventually) */ 	long
  /// usageDate;
  ///* Date this nickname was most recently used (eventually) */
  long cacheDate; /* Date this nickname was last cached (which differes from the
                     above) */
  long addressesDirty : 1; /* Have we modified the nickname addresses?  --
                              eventually!!! */
  long notesDirty : 1; /* Have we modified the nickname notes?  -- eventually!!!
                        */
  long pornography : 1; /* Is the photo dirty?  */
  long deleted : 1;     /* Has the nickname been deleted */
  long group : 1;       /* Does the nickname represent a group */
  long unused : 27;     /* Future expansion */
  long addressOffset;   /* Offset in file that address is at */
  long notesOffset;     /* Offset in file that notes is at */
  long nameTOCOffset;   /* Offset to the nickname */
  long valueOffset;     // Offset into notes where we'll find the sort value
  long valueLength;     // Length of the sort value
  void **theAddresses;  /* expansion addresses */
  void **theNotes; /* Notes for nickname; will contain other info such as real
                      name, phone, etc. */
} NickStruct, *NickStructPtr, *NickStructHandle;

typedef enum {
  eudoraAddressBook,
  regularAddressBook,
  pluginAddressBook,
  historyAddressBook,
#ifdef VCARD
  personalAddressBook
#endif
} AddressBookType;

#define IsEudoraAddressBook(aShort)                                            \
  ((*Aliases)[aShort].type == eudoraAddressBook)
#define IsRegularAddressBook(aShort)                                           \
  ((*Aliases)[aShort].type == regularAddressBook)
#define IsPluginAddressBook(aShort)                                            \
  ((*Aliases)[aShort].type == pluginAddressBook)
#define IsHistoryAddressBook(aShort)                                           \
  ((*Aliases)[aShort].type == historyAddressBook)
#define IsPersonalAddressBook(aShort)                                          \
  ((*Aliases)[aShort].type == personalAddressBook)

typedef struct AliasDStruct {
  FSSpec spec;
  NickStructHandle theData;
  void **hNames;    //	ALB 7/16/96, handle to nicknames
  short **sortData; // Contains nickname ID's -- 0 based -- of the sorted data
                    // for this address book
  AddressBookType type;
  bool collapsed;
  bool ro;
  bool dirty;
  bool containsBogusNicks;
  Accumulator addressHashes;
} AliasDesc, *AliasDPtr, *AliasDHandle;

// Structure to represent the contents of a 'TGMP' (Tag Map) resource
typedef struct {
  char service[256];     // Name of the service (Ph, LDAP, etc...)
  char server[256];      // Server (if any)
  short count;        // Number of tags
  void *serviceTags; // Concatenation of char *'s representing tags for this
                      // service and server
  void *nicknameTags; // Concatenation of char *'s representing mapped nickname tags
} NicknameTagMapRec, *NicknameTagMapRecPtr, *NicknameTagMapRecHandle;

#define NAliases (Aliases ? malloc_size(Aliases) / sizeof(AliasDesc) : 0)
#define NNicknames (This.theData ? malloc_size(This.theData) / sizeof(NickStruct) : 0)
#define issep(c) (IsSpace(c) || (c) == ',')

#define NICK_TOC_TYPE 'NToc'
#define NICK_NAMES_TYPE 'NNam'
#define NICK_BASE_RESID 128
#define NICK_RESID_V2 129
#define NICK_RESID_V3 130
#define NICK_RESID_V4 131
#define NICK_RESID_V5 132
#define NICK_RESID 133 // removed opt-space from hashes, too

#define OLD_NICK_TYPE 'NICK'
#define OLD_NICK_RESID1 3001
#define OLD_NICK_RESID2 3002

#define kNickGenOptAsian 1
#define kNickGenOptLastFirst 2

char *AliasExpansion(char *data, long offset);
void *GetTaggedFieldValue(short ab, short nick, char *tag);
void *GetTaggedFieldValueInNotes(void *notes, char *tag);
char *GetTaggedFieldValueStr(short ab, short nick, char *tag,
                                      char *value);
char *GetTaggedFieldValueStrInNotes(void *notes, char *tag,
                                             char *value);
int SetTaggedFieldValue(short ab, short nick, char *tag,
                        char *value, NickFieldSetValueType setValue,
                        short separatorIndex, bool *ignored);
int SetTaggedFieldValueInNotes(void *notes, char *tag, char *value,
                               long length, NickFieldSetValueType setValue,
                               short separatorIndex, bool *ignored);
int SetNicknameChangeBit(void *notes, ChangeBitType changeBits,
                         bool clearFirst);
long GetNicknameChangeBits(void *notes);
bool FindTaggedFieldValueOffsets(short ab, short nick, char *tag,
                                 long *attributeOffset, long *attributeLength,
                                 long *valueOffset, long *valueLength);
int RegenerateAllAliases(bool rebuild);
int BuildAddressHashes(short which);
void *GetNicknameData(short which, short index, bool wantAddresses,
                       bool readFromDisk);
void *GetNicknameName(short which, short index);
char *GetNicknameNamePStr(short which, short index,
                                   char *theName);
void GetNicknameViewData(short which, short index, char *sViewData);
long NickHash(char *newName);
long NickHashString(char *string);
long NickHashHandle(void *h);
long NickHashRawAddresses(void *addresses, bool *group);
long NickGenerateUniqueID(void);
int PrepAllAddressBooksForSync(void);
int PrepAddressBookForSync(short ab);
int PrepNicknameForSync(short ab, short nick, char idTag[256],
                        char changeBitsTag[256]);
int ClearAllAddressBookChangeBits(long mask);
int ClearAddressBookChangeBits(short ab, long mask);
int ClearNicknameChangeBits(short ab, short nick, long mask);

bool IsAnyNickname(char *name);

void CommaList(void *h);
#ifdef NEVER
long CountAliasTotal(NickHandle aliases, long offset);
long CountAliasAlias(NickHandle aliases, long offset);
long CountAliasExpansion(NickHandle aliases, long offset);
#endif

#define CountAliasTotal(a, o)                                                  \
  (CountAliasAlias(a, o) + 1 + CountAliasExpansion(a, o) + 2)
#define CountAliasAlias(a, o) ((unsigned)(*(void ***)(a))[o])
#define ___nba(a, o) ((o) + CountAliasAlias(a, o) + 1)
#define CountAliasExpansion(a, o)                                              \
  (256 * (unsigned)(*(void ***)(a))[___nba(a, o)] +                            \
   (unsigned)(*(void ***)(a))[___nba(a, o) + 1])

/* BinAddrHandle is now char** (NULL-terminated array). Multiple = has [0] and [1] */
#define ContainsMultipleAddresses(addrs)                                       \
  ((addrs) && (addrs)[0] && (addrs)[1] ? true : false)

bool SaveIndNickFile(short which, bool saveChangeBits);
int URLStringToSpec(StringHandle urlString, char *spec);
short ReplaceNicknameAddresses(short which, char *oldName,
                               TextAddrHandle text);
short ReplaceNicknameNotes(short which, char *oldName,
                           TextAddrHandle text);
static short ReplaceNicknameInfo(short which, char *theName,
                                 TextAddrHandle text, bool fAddresses);
void RemoveNamedNickname(short which, char *name);
short AddNickToTOCfromName(short which, char *name, void *addresses);
short AddNickToTOCfromNotes(short which, char *name, void *notes);
static short AddNickToTOC(short which, char *name, void *hData,
                          bool fFromAddress);
long NickMatchFound(NickStructHandle theNicknames, long hashName,
                    char *theName, short which);
long NickAddressMatchFound(NickStructHandle theNicknames, long hashAddress,
                           char *theAddress, short which);
void MakeMessNick(MyWindowPtr win, short modifiers);
#ifdef VCARD
void MakeCompNick(MyWindowPtr win, char *vcardSpec);
#else
void MakeCompNick(MyWindowPtr win);
#endif
void MakeMboxNick(MyWindowPtr win, short modifiers);
void MakeCboxNick(MyWindowPtr win);
void FlattenListWith(void *h, unsigned char c);
bool SaveAliases(bool saveChangeBits);
int NickUniq(TextAddrHandle addresses, char *sep, bool wantErrors);
#define MAX_NICKNAME 30
void MakeNickFromSelection(MyWindowPtr win);
int GatherCompAddresses(MyWindowPtr win, char *addrList);

int AddTextToNick(short which, char *name, void *text, bool append);
short ChangeNameOfNick(short which, char *oldName,
                       char *newName);
int GatherBoxAddresses(TOCType * tocH, short modifiers, short from, short to,
                       void ***addresses, bool caching);
void ReadNickFileList(char *pSpec, AddressBookType type, bool reread);
void ReadPluginNickFiles(bool reread);
int RegenerateAliases(short which, bool rebuild);
bool ExpandAliasesLow(void **h1, void *h2, int i, bool b1, void *p1, int i2);

void ZapAliasHash(short which);
void ZapAliases(void);
void ZapAliasFile(short which);
void ZapPluginAliases(void);
void SetAliasDirty(short which);

bool MaybeApplySplittingAlgorithm(void *notes);
char *ParseFirstLast(char *realName, char *firstName,
                              char *lastName);
char *JoinFirstLast(char *fullName, char *firstName,
                             char *lastName);
char *ScanNameForSpaces(char *name);
void MakeUniqueNickname(short ab, char nickname[32]);
void SetNickname(short ab, short nick, char *name);

int NickBackup(char * spec);

// Prototypes for using the Nickname Tag Map
int GetNicknameTagMap(char *service, char *server,
                      NicknameTagMapRecPtr tagMapPtr);
void DisposeNicknameTagMap(NicknameTagMapRecPtr tagMapPtr);
char *NicknameTag2ServiceTag(NicknameTagMapRecPtr tagMapPtr,
                                      char *nicknameTag,
                                      char *serviceTag);
char *ServiceTag2NicknameTag(NicknameTagMapRecPtr tagMapPtr,
                                      char *serviceTag,
                                      char *nicknameTag);
char *GetIndNicknameTag(NicknameTagMapRecPtr tagMapPtr, short index,
                                 char *nicknameTag);
short FindServiceTagIndex(NicknameTagMapRecPtr tagMapPtr,
                          char *serviceTag);

PrimaryLocationType GetPrimaryLocation(void *notes);

short FindAddressBookType(AddressBookType type);
int WhiteListAddr(TextAddrHandle addr);
int WhiteListTS(TOCType * tocH, short sumNum);

char **UniqBinAddr(char **addresses);
char **SortBinAddr(char **addresses);

#ifdef VCARD
bool AnyPersonalNicknames(void);
#endif

#endif
