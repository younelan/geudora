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

/* ==================================================================================

    Eudora Extended Message Services API SDK Macintosh
    This SDK supports EMSAPI version 7
    Copyright 1995-2003 QUALCOMM Inc.
    Send comments and questions to <eudora-emsapi@qualcomm.com>

    Filename: emsapi-mac.h
*/

#ifndef EMS_API_FILE
#define EMS_API_FILE

#ifdef INTERNAL_FOR_QC
/*    Internal Only Note
    ==================
    This version of emsapi-mac.h is used to build both Eudora and QUALCOMM
    internal plug-ins. In addition to controlling what portions of this
    file are used when building, #ifdefs with specific #defines or
    comments also control the creation of an external stripped down
    version of this file by a Perl script.

    The following preprocessor controls are also used by the Perl script
    (#'s dropped to avoid potentially confusing the script):
    * ifdef INTERNAL_FOR_QC
            marks the start of definitions that are internal only -
            i.e. only for use by Eudora and QUALCOMM plugins
    * else // EXTERNAL_FOR_PUBLIC
            closes definitions that are internal only and starts definitions
            that are external only - i.e. for public use
    * endif // INTERNAL_FOR_QC
            closes definitions that are internal only -
            i.e. only for use by Eudora and QUALCOMM plugins
    * endif // EXTERNAL_FOR_PUBLIC
            closes definitions that are external only - i.e. for public use
    * ifdef INTERNAL_FOR_EUDORA
            marks the start of definitions that are internal only for Eudora
            (not for any plugin use, including QUALCOMM's)
    * else // EXTERNAL_FOR_PLUGINS
            closes definitions that are internal only for Eudora and starts
            definitions that are external for plugin use (including QUALCOMM's)
    * endif // INTERNAL_FOR_EUDORA
            closes definitions that are internal only for Eudora
            (not for any plugin use, including QUALCOMM's)
    * endif // EXTERNAL_FOR_PLUGINS
            closes definitions that are external for plugin use (including
   QUALCOMM's)


    Both INTERNAL_FOR_QC and INTERNAL_FOR_EUDORA are #define'd by ems-wglu.h,
    which in turn includes this file. To build a QC internal plug-in #define
    just INTERNAL_FOR_QC.

    One other thing the Perl script looks for:
    * <-SBL-> when used after a recognized #endif the script will skip the
      next line if it's blank

    Note that although the #ifdef above and #else & #endif below don't matter
    for the C building of this file, they are used by the Perl script as
    an easy way to strip out this internal only note leaving only the external
    note.

    End of Internal Only Note. Next section is the external note that will
    remain in the public version of this file.
*/
#else  // EXTERNAL_FOR_PUBLIC
/*    Note: this file is generated automatically by a script and must be
    kept in synch with other translation API definitions, so it should
    probably never be edited manually.
*/
#endif // EXTERNAL_FOR_PUBLIC

/* ===================================================================================
 */

#ifndef __EMS_MAC__
#define __EMS_MAC__

/* Carbon has been removed; rely on `portable-compat.h` for basic types. */

#if PRAGMA_STRUCT_ALIGN
#pragma options align = mac68k
#endif

#ifdef INTERNAL_FOR_EUDORA

#if PRAGMA_IMPORT_SUPPORTED
#pragma import on
#endif

// Define BUILDING_CARBON_PLUGIN to be true for modern builds
#define BUILDING_CARBON_PLUGIN 1

#else // EXTERNAL_FOR_PLUGINS
#ifdef __cplusplus
extern "C" {
#endif
#endif // EXTERNAL_FOR_PLUGINS <-SBL->

#ifndef BUILDING_CARBON_PLUGIN
#if TARGET_API_MAC_CARBON
#define BUILDING_CARBON_PLUGIN 1
#endif
#endif

/* ===== CONSTANTS AND RETURN VALUES
 * ================================================= */

typedef long EUDORA_ComponentResult;

/* ----- Translator return codes --- store as a long
 * --------------------------------- */
#define EMSR_OK (0L)            /* The translation operation succeeded */
#define EMSR_UNKNOWN_FAIL (1L)  /* Failed for unspecified reason */
#define EMSR_CANT_TRANS (2L)    /* Don't know how to translate this */
#define EMSR_INVALID_TRANS (3L) /* The translator ID given was invalid */
#define EMSR_NO_ENTRY (4L)      /* The value requested doesn't exist */
#define EMSR_NO_INPUT_FILE (5L) /* Couldn't find input file */
#define EMSR_CANT_CREATE (6L)   /* Couldn't create the output file */
#define EMSR_TRANS_FAILED (7L)  /* The translation failed. */
#define EMSR_INVALID (8L)       /* Invalid argument(s) given */
#define EMSR_NOT_NOW (9L) /* Translation can be done not in current context */
#define EMSR_NOW (10L) /* Indicates translation can be performed right away */
#define EMSR_ABORTED (11L)        /* Translation was aborted by user */
#define EMSR_DATA_UNCHANGED (12L) /* Trans OK, data was not changed */
#ifdef INTERNAL_FOR_QC
#define EMSR_NOT_INTERESTED                                                    \
  (13L)                           /* V5! Peanut Not now, but not in DISPLAY?   \
                                   */
#define EMSR_USER_CANCELLED (14L) /* V5! User cancelled */
#define EMSR_DELETE_MESSAGE                                                    \
  (15L) /* V6! Translation complete, delete parent message */
#endif  //    INTERNAL_FOR_QC
#define EMSR_NOT_IN_THIS_MODE                                                  \
  (16L) /* V6+ Can't perform this operation in this mode (free, ad, paid) */

/* ----- Translator types --- store as a long
 * ---------------------------------------- */
#define EMST_NO_TYPE (-1L)
#define EMST_LANGUAGE (0x10L)
#define EMST_TEXT_FORMAT (0x20L)
#define EMST_GRAPHIC_FORMAT (0x30L)
#define EMST_COMPRESSION (0x40L)
#define EMST_COALESCED (0x50L)
#define EMST_SIGNATURE (0x60L)
#define EMST_PREPROCESS (0x70L)
#define EMST_CERT_MANAGEMENT (0x80L)
#ifdef INTERNAL_FOR_QC
#define EMST_IMPORTER (0x90L)
#endif //    INTERNAL_FOR_QC

/* ----- Translator info flags and contexts --- store as a long
 * ---------------------- */
/* Used both as bit flags and as constants */
#define EMSF_ON_ARRIVAL (0x0001L) /* Call on message arrival */
#define EMSF_ON_DISPLAY (0x0002L) /* Call when user views message */
#define EMSF_ON_REQUEST (0x0004L) /* Call when selected from menu */
#define EMSF_Q4_COMPLETION                                                     \
  (0x0008L) /* Queue and call on complete composition of a message */
#define EMSF_Q4_TRANSMISSION                                                   \
  (0x0010L) /* Queue and call on transmission of a message */
#ifndef EMSF_JUNK_MAIL
#define EMSF_JUNK_MAIL                                                         \
  (0x0020L) /* Call for scoring or marking a message as junk or not (NEW in    \
               Eudora 6.0/EMSAPI 7.0) */
#endif
#define EMSF_WHOLE_MESSAGE                                                     \
  (0x0200L) /* Works on the whole message even if it has sub-parts. (e.g.      \
               signature) */
#define EMSF_REQUIRES_MIME                                                     \
  (0x0400L) /* Items presented for translation should be MIME entities with    \
               canonical end of line representation, proper transfer encoding  \
               and headers */
#define EMSF_GENERATES_MIME (0x0800L) /* Data produced will be MIME format */
#define EMSF_ALL_HEADERS                                                       \
  (0x1000L) /* All headers in & out of trans when MIME format is used */
#define EMSF_BASIC_HEADERS                                                     \
  (0x2000L) /* Just the basic to, from, subject, cc, bcc headers */
#define EMSF_DEFAULT_Q_ON                                                      \
  (0x4000L) /* Causes queued translation to be on for a new message by default \
             */
#define EMSF_TOOLBAR_PRESENCE (0x8000L) /* Appear on the Toolbar */
#define EMSF_ALL_TEXT (0x10000L)        /* ON_REQUEST WANTS WHOLE MESSAGE */
#define EMSF_PREFER_PLAIN                                                      \
  (0x100000L) /* EMSF_JUNK_MAIL prefers plain text (NEW in                     \
                 Eudora 6.0/EMSAPI 7.0) - more efficient when off */
//
#ifdef INTERNAL_FOR_QC
#define EMSF_DONTSAVE                                                          \
  (0x80000000L) /* Mark messages as unchanged so user is not prompted for      \
                   save. Add to any return code */
#endif          // INTERNAL_FOR_QC
/* all other flag bits in the long are RESERVED and may not be used */

/* ----- The version of the API defined by this include file
 * ------------------------- */
#define EMS_VERSION (7) /* Used in plugin init */
#ifdef INTERNAL_FOR_QC
#define EMS_NEW_MAIL_CONFIG_VERSION                                            \
  (7) /* Used to determine whether the new or old mail config struct should be \
         used */
#define EMS_MINOR_VERSION                                                      \
  (0)  /* Used to provide the minor EMSAPI version in emsMailConfig */
#endif // INTERNAL_FOR_QC
#define EMS_COMPONENT 'EuTL' /* Macintosh component type */
#define EMS_COMPONENT_OSX                                                      \
  'EcTL'                   /* Macintosh component type for Carbon OSX builds */
#define EMS_PB_VERSION (3) /* Minimum version that uses parameter blocks */

/* ----- Translator and translator type specific return codes
 * ------------------------ */
#define EMSC_SIGOK (1L)      /* A signature verification succeeded */
#define EMSC_SIGBAD (2L)     /* A signature verification failed */
#define EMSC_SIGUNKNOWN (3L) /* Result of verification unknown */

/* ----- NEW in Eudora 6.0/EMSAPI 7.0 - IDLE Events
 * --------------------------------- */
/* Note that it is possible to have both EMSIDLE_UI_ALLOWED and EMSIDLE_QUICK
 * set, in that case the plug-in should only do a UI operation if it's
 * absolutely essential (i.e., errors only). If EMSIDLE_UI_ALLOWED is NOT set,
 * then using the progress
 */

/* ----- NEW in Eudora 6.0/EMSAPI 7.0 - Values for ems_idle flags
 * -------------------- */
#define EMSFIDLE_UI_ALLOWED (0x0001L) /* Interactions with user are allowed */
#define EMSFIDLE_QUICK                                                         \
  (0x0002L) /* Now is NOT the time to do something lengthy */
#define EMSFIDLE_OFFLINE (0x0004L)      /* Eudora is in "offline" mode */
#define EMSFIDLE_PRE_SEND (0x0008L)     /* Eudora is about to send mail */
#define EMSFIDLE_TRANSFERRING (0x0010L) /* Currently transferring mail */

/* ----- NEW in Eudora 6.0/EMSAPI 7.0 - Values for emsGetMailBox flags
 * --------------- */
#define EMSFGETMBOX_ALLOW_NEW (0x0001L) /* Allow creation of new mailboxes */
#define EMSFGETMBOX_ALLOW_OTHER                                                \
  (0x0002L) /* Allow selection of "other" mailboxes */
#define EMSFGETMBOX_DISALLOW_NON_LOCAL                                         \
  (0x0004L) /* Disallow selection of non-local (e.g. IMAP) mailboxes */

/* ----- NEW in Eudora 6.0/EMSAPI 7.0 - Values for emsJunkInfo context flags
 * --------- */
#define EMSFJUNK_SCORE_ON_ARRIVAL                                              \
  (0x0001L)                        /* Score incoming message as it arrives */
#define EMSFJUNK_RESCORE (0X0002L) /* Rescore message */
#define EMSFJUNK_MARK_IS_JUNK (0x0004L) /* User is marking message as JUNK */
#define EMSFJUNK_MARK_NOT_JUNK                                                 \
  (0x0008L) /* User is marking message as NOT junk */
#define EMSFJUNK_USER_INITIATED                                                \
  (0x0010L) /* User (not filter or any other automatic mechanism) initiated    \
               action */

#ifdef INTERNAL_FOR_QC // start MODELESS EMSAPI
#define EMS_PW_WINDOWKIND (99)

// Constants (and bitfields) for special menu commands a plugin can access
// File Menu
#define EMS_MENU_FILE_CLOSE (0x0010000L)
#define EMS_MENU_FILE_SAVE (0x0020000L)
// Edit Menu
#define EMS_MENU_EDIT_UNDO (0x0100000L)
#define EMS_MENU_EDIT_CUT (0x0200000L)
#define EMS_MENU_EDIT_COPY (0x0400000L)
#define EMS_MENU_EDIT_PASTE (0x0800000L)
#define EMS_MENU_EDIT_CLEAR (0x1000000L)
#define EMS_MENU_EDIT_SELECTALL (0x2000000L)
#endif // INTERNAL_FOR_QC - end MODELESS EMSAPI

/* ========== FORWARD DECLARATIONS
 * ======================================================== */
#ifndef EMSAPI_FORWARD_DECLARATIONS
#define EMSAPI_FORWARD_DECLARATIONS
struct emsProgressDataS;
struct emsGetDirectoryDataS;
struct emsRegenerateDataS;
struct emsGetMailBoxDataS;
struct emsGetPersonalityDataS;
struct emsGetPersonalityInfoDataS;
struct emsSetMailBoxTagDataS;
struct emsCreateMailBoxDataS;
struct emsPlugwindowDataS;
struct emsGDeviceRgnDataS;
struct emsMakeSigDataS;
struct emsMakeAddressBookDataS;
struct emsMakeABEntryDataS;
struct emsMakeMailboxDataS;
struct emsMakeOutMessDataS;
struct emsMakeMessageDataS;
struct emsImportMboxDataS;
struct emsIsInAddressBookDataS;
#endif /* EMSAPI_FORWARD_DECLARATIONS */

/* ===== NEW in Eudora 6.0/EMSAPI 7.0 = CALL BACK FUNCTIONS FROM EUDORA ACROSS
 * THE API === */

typedef short (*emsPrivateFunction)();

/* The progress function  */
typedef short (*emsProgress)(struct emsProgressDataS *progData);
#define CallEMSProgressProc(userRoutine, data) (*(userRoutine))((data))

/* Displays dialog with list of mailboxes and returns mailbox user chooses */
typedef void (*emsGetMailBox)(struct emsGetMailBoxDataS *getMailBoxData);
#define CallEMSGetMailProc(userRoutine, data) (*(userRoutine))((data))

#ifdef INTERNAL_FOR_QC
typedef void (*emsSetMailBoxTag)(
    struct emsSetMailBoxTagDataS *setMailBoxTagData);
#define CallEMSSetMailBoxTagProc(userRoutine, data) (*(userRoutine))((data))

typedef void (*emsGetMailBoxTag)(
    struct emsSetMailBoxTagDataS *setMailBoxTagData);
#define CallEMSGetMailBoxTagProc(userRoutine, data) (*(userRoutine))((data))
#endif // INTERNAL_FOR_QC

/* Displays dialog with list of personalities and returns personality user
 * chooses */
typedef short (*emsGetPersonality)(
    struct emsGetPersonalityDataS *getPersonalityData);
#define CallEMSGetPersonalityProc(userRoutine, data) (*(userRoutine))((data))

/* Retrieves email address for specified personality */
typedef short (*emsGetPersonalityInfo)(
    struct emsGetPersonalityInfoDataS *getPersonalityData);
#define CallEMSGetPersonalityInfoProc(userRoutine, data)                       \
  (*(userRoutine))((data))

/* Check to see if the address is in any Eudora address book */
typedef short (*emsIsInAddressBook)(struct emsIsInAddressBookDataS *abData);
#define CallEMSIsInAddressBookProc(userRoutine, data) (*(userRoutine))((data))

/* Causes Eudora to reload either plugin filters or plugin nicknames */
typedef short (*emsRegenerate)(struct emsRegenerateDataS *RegenerateData);
#define CallEMSRegenerateProc(userRoutine, data) (*(userRoutine))((data))

/* Retrieves location of specified Eudora folder */
typedef short (*emsGetDirectory)(struct emsGetDirectoryDataS *GetDirectoryData);
#define CallEMSGetDirectoryProc(userRoutine, data) (*(userRoutine))((data))

/* Tells Eudora to update its Windows - call if necessary during plugin update
 * event processing */
typedef void (*emsUpdateWindows)(void);
#define CallEMSUpdateWindowsProc(userRoutine) (*(userRoutine))()

/* Creates mailbox */
typedef short (*emsCreateMailBox)(
    struct emsCreateMailBoxDataS *createMailboxData);
#define CallEMSCreateMailBoxProc(userRoutine, data) (*(userRoutine))((data))

#ifdef INTERNAL_FOR_QC // start MODELESS EMSAPI
typedef short (*emsPlugwindowRegister)(
    struct emsPlugwindowDataS *PlugwindowRegisterData);
#define CallEMSPlugwindowRegisterProc(userRoutine, data)                       \
  (*(userRoutine))((data))

typedef void (*emsPlugwindowUnRegister)(
    struct emsPlugwindowDataS *PlugwindowUnRegisterData);
#define CallEMSPlugwindowUnRegisterProc(userRoutine, data)                     \
  (*(userRoutine))((data))

typedef short (*emsGDeviceRgn)(struct emsGDeviceRgnDataS *GDeviceRgnData);
#define CallEMSGDeviceRgnProc(userRoutine, data) (*(userRoutine))((data))

typedef void *(*emsFrontWindow)(void);
#define CallEMSFrontWindowProc(userRoutine) (*(userRoutine))()

typedef void (*emsSelectWindow)(void *selectWindowData);
#define CallEMSSelectWindowProc(userRoutine, data) (*(userRoutine))(data)
#endif // INTERNAL_FOR_QC - end MODELESS EMSAPI

#ifdef INTERNAL_FOR_QC // start IMPORTERS
typedef short (*emsMakeSig)(struct emsMakeSigDataS *makeSigData);
#define CallEMSMakeSigProc(userRoutine, data) (*(userRoutine))((data))

typedef short (*emsMakeAddressBook)(
    struct emsMakeAddressBookDataS *makeAddressBookData);
#define CallEMSMakeAddressBookProc(userRoutine, data) (*(userRoutine))((data))

typedef short (*emsMakeABEntry)(struct emsMakeABEntryDataS *makeABEntryData);
#define CallEMSMakeABEntryProc(userRoutine, data) (*(userRoutine))((data))

typedef short (*emsMakeMailbox)(struct emsMakeMailboxDataS *makeMailboxData);
#define CallEMSMakeMailboxProc(userRoutine, data) (*(userRoutine))((data))

typedef short (*emsMakeOutMess)(struct emsMakeOutMessDataS *makeOutMessData);
#define CallEMSMakeOutMessProc(userRoutine, data) (*(userRoutine))((data))

typedef short (*emsMakeMessage)(struct emsMakeMessageDataS *MakeMessageData);
#define CallEMSMakeMessageProc(userRoutine, data) (*(userRoutine))((data))

// typedef struct emsImportMboxDataS  *emsImportMboxDataP, **emsImportMboxDataH;

typedef short (*emsImportMbox)(struct emsImportMboxDataS *ImportMBoxData);
#define CallEMSImportMboxProc(userRoutine, data) (*(userRoutine))((data))
#endif // INTERNAL_FOR_QC - end IMPORTERS <-SBL->

/* ===== DATA STRUCTURES
 * ============================================================= */
/* All strings on the Mac are PASCAL strings. Most strings are allocated as
   void **s. A few strings may appear in the structure themselves (e.g. char
   [256])
 */

/* ----- Progress Data
 * --------------------------------------------------------------- */
#ifndef EMSAPI_FORWARD_TYPEDEFS
typedef struct emsProgressDataS *emsProgressDataP;
#endif
typedef struct emsProgressDataS {
  long size;              /* Size of this data structure */
  long value;             /* Range of Progress, percent complete */
  unsigned char *message; /* Progress Message */
} emsProgressData;

/* ----- NEW in Eudora 6.0/EMSAPI 7.0 - GetMailBoxFunction Data
 * ---------------------- */
#ifndef EMSAPI_FORWARD_TYPEDEFS
typedef struct emsGetMailBoxDataS *emsGetMailBoxDataP;
#endif
typedef struct emsGetMailBoxDataS {
  long size;             /* IN: Size of this data structure */
  long flags;            /* IN: see flags above */
  unsigned char *prompt; /* IN: Prompt for user */
  void **mailbox;        /* OUT: the chosen mailbox */
} emsGetMailBoxData;

/* ----- User Address
 * ---------------------------------------------------------------- */
typedef struct emsAddressS *emsAddressP, *emsAddressH;
typedef struct emsAddressS {
  long size;                /* Size of this data structure */
  unsigned char **address;  /* Actual email address */
  unsigned char **realname; /* Real name portion of email address */
  emsAddressH next;         /* Linked list of addresses */
} emsAddress;

/* ----- NEW in Eudora 6.0/EMSAPI 7.0 - GetPersonalityData
 * --------------------------- */
#ifndef EMSAPI_FORWARD_TYPEDEFS
typedef struct emsGetPersonalityDataS *emsGetPersonalityDataP;
#endif
typedef struct emsGetPersonalityDataS {
  long size; /* IN: Size of this data structure */
  unsigned char
      *prompt;      /* IN: Prompt for user, Set to NULL for standard prompt */
  bool defaultPers; /* IN: set to true for default personality, otherwise
                       selection dialog comes up */
  short persCount;  /* OUT: Number of personalities */
  emsAddress personality; /* OUT: the chosen personality */
  char **personalityName; /* OUT: text name of chosen personality */
} emsGetPersonalityData;

/* ----- NEW in Eudora 6.0/EMSAPI 7.0 - GetPersonalityInfo
 * --------------------------- */
#ifndef EMSAPI_FORWARD_TYPEDEFS
typedef struct emsGetPersonalityInfoDataS *emsGetPersonalityInfoDataP;
#endif
typedef struct emsGetPersonalityInfoDataS {
  long size;              /* IN: Size of this data structure */
  char **personalityName; /* IN: name of Personality */
  emsAddress personality; /* OUT: the chosen personality */
} emsGetPersonalityInfoData;

#ifdef INTERNAL_FOR_QC
/* ----- NEW - SetMailBoxTag Data
 * ---------------------------------------------------- */
typedef struct emsSetMailBoxTagDataS *emsSetMailBoxTagDataP;
typedef struct emsSetMailBoxTagDataS {
  long size;      /* IN: Size of this data structure */
  void **mailbox; /* IN: the selected mailbox */
  long key;       /* IN: the attribute key (usually the plug-in's ID */
  long value;     /* IN: the attribute value, zero to clear the attribute */
  long oldvalue;  /* OUT: the attribute's old value, zero by default */
  long oldkey;    /* OUT: the attribute's old key */
} emsSetMailBoxTagData;
#endif // INTERNAL_FOR_QC <-SBL->

/* ----- NEW in Eudora 6.0/EMSAPI 7.0 - CreateMailBox Data
 * --------------------------- */
#ifndef EMSAPI_FORWARD_TYPEDEFS
typedef struct emsCreateMailBoxDataS *emsCreateMailBoxDataP;
#endif
typedef struct emsCreateMailBoxDataS {
  long size;           /* IN: Size of this data structure */
  void **parentFolder; /* IN: relative path off of EMS_MailDir to the parent
                            folder, or NULL for default location */
  unsigned char *name; /* IN: name for the new mailbox or folder */
  bool
      createFolder; /* IN: set to 1 to create a folder, 0 to create a mailbox */
  void **mailboxOrFolder; /* OUT: the created mailbox or folder */
} emsCreateMailBoxData;

/* ----- NEW in Eudora 6.0/EMSAPI 7.0 - Address status
 * ------------------------------- */
typedef enum {
  emsAddressNotChecked,
  emsAddressNotInAB,
  emsAddressIsInAB
} AddressInABStatus;

/* ----- NEW in Eudora 6.0/EMSAPI 7.0 - InInAddressBook Data
 * ------------------------- */
#ifndef EMSAPI_FORWARD_TYPEDEFS
typedef struct emsIsInAddressBookDataS *emsIsInAddressBookDataP;
#endif
typedef struct emsIsInAddressBookDataS {
  long size;           /* IN: Size of this data structure */
  emsAddressP address; /* IN: Address to check */
  AddressInABStatus
      addressStatus; /* OUT: Indicates whether or not the address is known */
} emsIsInAddressBookData;

/* ----- NEW in Eudora 6.0/EMSAPI 7.0 - Type of Updated Files
 * ------------------------ */
typedef enum { emsRegenerateFilters, emsRegenerateNicknames } RegenerateType;

/* ----- NEW in Eudora 6.0/EMSAPI 7.0 - Regenerate Data
 * ------------------------------ */
#ifndef EMSAPI_FORWARD_TYPEDEFS
typedef struct emsRegenerateDataS *emsRegenerateDataP;
#endif
typedef struct emsRegenerateDataS {
  long size;            /* IN: Size of this data structure */
  RegenerateType which; /* IN: What should Eudora regenerate? */
} emsRegenerateData;

/* ----- GetDirectory
 * ---------------------------------------------------------------- */
typedef enum {
  EMS_EudoraDir,          /* Eudora folder, contains all otherfolders */
  EMS_AttachmentsDir,     /* Attachments folder */
  EMS_PluginFiltersDir,   /* Filters folder for plug-ins */
  EMS_PluginNicknamesDir, /* Nicknames folder for plug-ins */
  EMS_ConfigDir,          /* Folder for plug-ins' preferences */
  EMS_MailDir,            /* Folder containing mailboxes */
  EMS_NicknamesDir,       /* Eudora nicknames folder */
  EMS_SignaturesDir,      /* Signatures folder */
  EMS_SpoolDir,           /* Spool folder */
  EMS_StationeryDir       /* Stationery folder */
} DirectoryEnum;

#ifndef EMSAPI_FORWARD_TYPEDEFS
typedef struct emsGetDirectoryDataS *emsGetDirectoryDataP;
#endif
typedef struct emsGetDirectoryDataS {
  long size;           /* IN: Size of this data structure */
  DirectoryEnum which; /* IN: Which directory? */
  char directory[256]; /* OUT: Path for directory (replaced FSSpec) */
} emsGetDirectoryData;

/* ----- MIME Params
 * ----------------------------------------------------------------- */
typedef struct emsMIMEparam *emsMIMEparamP, *emsMIMEparamH;
typedef struct emsMIMEparam {
  long size;
  char name[64]; /* Mime parameter name (e.g. charset) */
  void **value;  /* param value (e.g. us-ascii) */
  struct emsMIMEparam **next;
} emsMIMEparam;

/* ----- MIME Info
 * ------------------------------------------------------------------- */
typedef struct emsMIMEtype *emsMIMEtypeP, *emsMIMEtypeH;
typedef struct emsMIMEtype {
  long size;
  char mimeVersion[64]; /* The MIME-Version header */
  char mimeType[64];    /* Top-level MIME type */
  char subType[64];     /* MIME sub-type */
  emsMIMEparamH params; /* void ** to first parameter in MIME parameter list */
  char contentDisp[64]; /* Content-Disposition */
  emsMIMEparamH contentParams; /* void ** to first parameter in linked list of
                                  parameters */
} emsMIMEtype;

/* ----- Header Data
 * ----------------------------------------------------------------- */
typedef struct emsHeaderData {
  long size;                  /* Size of this data structure */
  struct emsAddressS ***to;   /* To Header */
  struct emsAddressS ***from; /* From Header */
  char **subject;             /* Subject Header */
  struct emsAddressS ***cc;   /* cc Header */
  struct emsAddressS ***bcc;  /* bcc Header */
  void **rawHeaders;          /* The 822 headers */
} emsHeaderData;

/* ----- NEW in Eudora 6.0/EMSAPI 7.0 - structure to hold the callback functions
 * ----- */
/* Each callback is called with a pointer to a parameter block */
typedef struct emsCallBackS {
  emsGetMailBox EMSGetMailBoxFunction;
#ifdef INTERNAL_FOR_QC
  emsSetMailBoxTag EMSSetMailBoxTagFunction;
#else  // EXTERNAL_FOR_PUBLIC
  emsPrivateFunction EMSPrivateFunctionDontCall1;
#endif // EXTERNAL_FOR_PUBLIC
  emsGetPersonality EMSGetPersonalityFunction;
  emsProgress EMSProgressFunction;
  emsRegenerate EMSRegenerateFunction;
  emsGetDirectory EMSGetDirectoryFunction;
  emsUpdateWindows EMSUpdateWindowsFunction;
#ifdef INTERNAL_FOR_QC
  emsGetMailBoxTag EMSGetMailBoxTagFunction;
#else  // EXTERNAL_FOR_PUBLIC
  emsPrivateFunction EMSPrivateFunctionDontCall2;
#endif // EXTERNAL_FOR_PUBLIC
  emsGetPersonalityInfo EMSGetPersonalityInfoFunction;
#ifdef INTERNAL_FOR_QC
  // start MODELESS EMSAPI
  emsPlugwindowRegister EMSPlugwindowRegisterFunction;
  emsPlugwindowUnRegister EMSPlugwindowUnRegisterFunction;
  emsGDeviceRgn EMSGDeviceRgnFunction;
  emsFrontWindow EMSFrontWindowFunction;
  emsSelectWindow EMSSelectWindowFunction;
  // end MODELESS EMSAPI
  // start IMPORTERS
  emsMakeSig EMSMakeSigFunction;
  emsMakeAddressBook EMSMakeAddressBookFunction;
  emsMakeABEntry EMSMakeABEntryFunction;
  emsMakeMailbox EMSMakeMailboxFunction;
  emsMakeOutMess EMSMakeOutMessFunction;
  emsMakeMessage EMSMakeMessageFunction;
  // end IMPORTERS
  emsCreateMailBox EMSCreateMailBoxFunction;
#else  // EXTERNAL_FOR_PUBLIC
  emsPrivateFunction EMSPrivateFunctionDontCall3;
  emsPrivateFunction EMSPrivateFunctionDontCall4;
  emsPrivateFunction EMSPrivateFunctionDontCall5;
  emsPrivateFunction EMSPrivateFunctionDontCall6;
  emsPrivateFunction EMSPrivateFunctionDontCall7;
  emsPrivateFunction EMSPrivateFunctionDontCall8;
  emsPrivateFunction EMSPrivateFunctionDontCall9;
  emsPrivateFunction EMSPrivateFunctionDontCall10;
  emsPrivateFunction EMSPrivateFunctionDontCall11;
  emsPrivateFunction EMSPrivateFunctionDontCall12;
  emsPrivateFunction EMSPrivateFunctionDontCall13;
  emsPrivateFunction EMSPrivateFunctionDontCall14;
#endif // EXTERNAL_FOR_PUBLIC
  emsIsInAddressBook EMSIsInAddressBook;
} emsCallBack, *emsCallBacksP;

/* ----- How Eudora is configured
 * ---------------------------------------------------- */
#ifdef INTERNAL_FOR_QC
// This only exists to service old versions of ESP and importers (until they can
// be updated)
typedef struct emsPre7MailConfigS {
  long size;                     /* Size of this data structure */
  char configDir[256];           /* Path for config file */
  struct emsAddressS userAddr;   /* Current users full name and address */
  struct emsCallBackS callBacks; /* callback structure */
} emsPre7MailConfig;
#endif // INTERNAL_FOR_QC
typedef struct emsMailConfigS *emsMailConfigP;
typedef struct emsMailConfigS {
  long size;        /* Size of this data structure */
  FSSpec configDir; /* Optional directory for config file */
  emsAddress
      userAddr; /* Current users full name and address from Eudora config */
  emsCallBacksP
      callBacks; /* NEW in Eudora 6.0/EMSAPI 7.0: callback structure */
  short eudAPIMinorVersion; /* NEW in Eudora 6.0/EMSAPI 7.0: Minor API version -
                               used when calls are being added during beta or
                               when an EMSAPI change is done for a minor version
                               release */
} emsMailConfig;

/* ----- Plugin Info
 * ----------------------------------------------------------------- */
typedef struct emsPluginInfoS *emsPluginInfoP;
typedef struct emsPluginInfoS {
  long size;         /* Size of this data structure */
  long id;           /* Place to return unique plugin id */
  long numTrans;     /* Place to return num of translators */
  long numAttachers; /* Place to return num of attach hooks */
  long numSpecials;  /* Place to return num of special hooks */
  char **desc;       /* Return for string description of plugin */
  void **icon;       /* Return for plugin icon data */
#ifdef INTERNAL_FOR_QC
  long mem_rqmnt;      /* V4! Return Memory Required to run this */
  long numMBoxContext; /* V4! Peanut Place to return num of mailbox context
                          hooks */
  /* These are shown when the mailbox has a non-zer attribute with the plugin-s
   * ID as key */
#else
  long reserved1; /* Reserved for internal Eudora use */
  long reserved2; /* Reserved for internal Eudora use */
#endif               // INTERNAL_FOR_QC
  long idleTimeFreq; /* NEW in Eudora 6.0/EMSAPI 7.0: Return 0 for no idle time,
                        otherwise initial idle frequency in milliseconds */
#ifdef INTERNAL_FOR_QC
  long numImporters; /* V6! Place to return num of importer hooks */
#else
  long reserved3; /* Reserved for internal Eudora use */
#endif // INTERNAL_FOR_QC
} emsPluginInfo;

/* ----- Translator Info
 * ------------------------------------------------------------- */
typedef struct emsTranslator *emsTranslatorP;
typedef struct emsTranslator {
  long size;           /* Size of this data structure */
  long id;             /* ID of translator to get info for */
  long type;           /* translator type, e.g., EMST_xxx */
  unsigned long flags; /* translator flags */
  char **desc;         /* translator string description */
  void **icon;         /* Return for plugin icon data */
  char **properties;   /* Properties for queued translations */
} emsTranslator;

/* ----- Menu Item Info
 * -------------------------------------------------------------- */
typedef struct emsMenuS *emsMenuP;
typedef struct emsMenuS {
  long size;   /* Size of this data structure */
  long id;     /* ID of translator to get info for */
  char **desc; /* translator string description */
  void **icon; /* Icon suite */
  long flags;  /* any special flags (e.g. EMSF_TOOLBAR_PRESENCE) */
} emsMenu;

/* ----- Translation Data
 * ------------------------------------------------------------ */
typedef struct emsDataFileS *emsDataFileP, *emsDataFileH;
typedef struct emsDataFileS {
  long size; /* Size of this data structure */
  long context;
  emsMIMEtypeH mimeInfo; /* MIME type of data to check */
  emsHeaderDataP
      header;     /* EMSF_BASIC_HEADERS & EMSF_ALL_HEADERS determine contents */
  char file[256]; /* Input file path */
} emsDataFile;

/* ----- Resulting Status Data
 * ------------------------------------------------------- */
typedef struct emsResultStatus *emsResultStatusP;
typedef struct emsResultStatus {
  long size;    /* Size of this data structure */
  char **desc;  /* Returned string for display with the result */
  char **error; /* Place to return string with error message */
  long code;    /* Return for translator-specific result code */
} emsResultStatus;

/* ----- Idle Data
 * ------------------------------------------------------------------- */
typedef struct emsIdleDataS *emsIdleDataP;
typedef struct emsIdleDataS {
  long flags;         /* IN: Idle Flags */
  long *idleTimeFreq; /* IN/OUT: current->new request idle */
  emsProgress
      progress; /* IN: Callback function to report progress/check for abort */
} emsIdleData;

/* ----- NEW in Eudora 6.0/EMSAPI 7.0 - Junk Info
 * ------------------------------------ */
typedef struct emsJunkInfo *emsJunkInfoP;
typedef struct emsJunkInfo {
  long size;     /* Size of this data structure */
  long context;  /* Junk context flags see EMSJUNK #defines */
  long pluginID; /* For ems_user_mark_junk ID of plugin that assigned previous
                    junk score, or 0 if user marked  */
} emsJunkInfo;

/* ----- NEW in Eudora 6.0/EMSAPI 7.0 - Message Information for Junk Scoring
 * --------- */
typedef struct emsMessageInfo *emsMessageInfoP;
typedef struct emsMessageInfo {
  long size;               /* Size of this data structure */
  long context;            /* Reserved for future use */
  unsigned long messageID; /* Uniquely identifies message for months - generally
                              ok to persist */
  AddressInABStatus
      fromAddressStatus; /* Indicates whether from address is known */
  emsHeaderDataP
      header; /* EMSF_BASIC_HEADERS & EMSF_ALL_HEADERS determine contents */
  emsMIMEtypeP textType; /* MIME type of text to check */
  void **text; /* void ** to text of message if provided in format indicated by
                 textType or NULL - handle size determines string length */
} emsMessageInfo;

/* ----- NEW in Eudora 6.0/EMSAPI 7.0 - Junk Score
 * ----------------------------------- */
typedef struct emsJunkScore *emsJunkScoreP;
typedef struct emsJunkScore {
  long size;  /* Size of this data structure */
  char score; /* Junk score between 0 and 100 with a special value of -1.
                 -1 => white list - absolutely NOT junk - use sparingly or not
                 at all; 0 => NOT junk; 100 => absolutely junk */
} emsJunkScore;
//
#ifdef INTERNAL_FOR_QC
/* ----- NEW - MailBoxContextFolder Data --------------------- */
typedef struct emsMailBoxContextFolderDataS *emsMailBoxContextFolderDataP;
typedef struct emsMailBoxContextFolderDataS {
  long size;           /* IN: Size of this data structure */
  long value;          /* IN: the attribute value for the mailbox in question */
  FSSpec pluginFolder; /* OUT: Folder that the plugin associates with the given
                          mailbox tags */
} emsMailBoxContextFolderData;
#endif // INTERNAL_FOR_QC

/* ----- NEW - Type of Mode Event Notifications --------------- */
typedef enum { EMS_GetDowngradeInfo, EMS_ModeChanged } EMSModeEventEnum;
typedef enum {
  EMS_ModeFree,
  EMS_ModeSponsored,
  EMS_ModePaid,
  EMS_ModeCustom = 250
} EMSModeTypeEnum;

/* ----- NEW - EudoraModeNotification Data --------------------- */
typedef struct emsEudoraModeNotificationDataS *emsEudoraModeNotificationDataP;
typedef struct emsEudoraModeNotificationDataS {
  long size; /* IN: Size of this data structure */
  EMSModeEventEnum
      modeEvent; /* IN: Notification of change or request for downgrade text */
  EMSModeTypeEnum isFullFeatureSet; /* IN: The current mode that we are in */
  EMSModeTypeEnum
      needsFullFeatureSet;    /* OUT: What mode the plugin needs to run? */
  bool downgradeWasBeingUsed; /* OUT: Return whether or not plugin was being
                                 used */
  short productCode;          /* IN: App Product code */
  unsigned long modeFlags;    /* OUT: What modes we will function in */
} emsEudoraModeNotificationData;

#ifdef INTERNAL_FOR_QC
// start MODELESS EMSAPI
typedef struct emsPlugwindowDataS {
  long size;              /* IN: Size of this data structure */
  void *nativeWindow;     /* reference to GtkWidget * */
  long plugwindowID;      /* local id assigned by plug-in */
  unsigned long menuMask; /* Mask for menu items to be used */
  long top;               /* For saving size and position */
  long left;              /* not used by plug-in except for reopen() */
  long bottom;
  long right;
  long plugin_refcon; /* for plug-in's use, NEVER used by Eudora */
  long pluginID;      /* id of the plug-in */
  bool isDialogLayer; /* Should Eudora keep this window in the dialog layer? */
} emsPlugwindowData, *emsPlugwindowDataP;

typedef struct emsPlugwindowEventS {
  long size;         /* IN: Size of this data structure */
  void *nativeEvent; /* reference to EventRecord on Mac */
} emsPlugwindowEvent, *emsPlugwindowEventP;

typedef struct emsPlugwindowMenuDataS {
  long size;              /* IN: Size of this data structure */
  unsigned long menuMask; /* Mask for menu item */
} emsMenuData, *emsPlugwindowMenuDataP;

typedef struct emsPlugwindowDragDataS {
  long size;                   /* IN: Size of this data structure */
  DragTrackingMessage message; /* message of 0xfff means "drop" */
  void * drag;          /* drag data */
} emsPlugwindowDragData, *emsPlugwindowDragDataP;

typedef struct emsGDeviceRgnDataS {
  long size;             /* IN: Size of this data structure */
  short gdIndex;         /* IN: index of device you want to know about */
  Rgnvoid **usableRgn;   /* usable region of the screen */
  Rect largestRectangle; /* largest rect available on the screen */
} emsGDeviceRgnData, *emsGDeviceRgnDataP;
// end MODELESS EMSAPI

// start IMPORTERS
/* ----- Importer Info -------------------------------- */
typedef struct emsImporterS *emsImporterP, *emsImporterH;
typedef struct emsImporterS {
  long size;            /* Size of this data structure */
  long id;              /* ID of translator to get info for */
  long type;            /* translator type, e.g., EMST_xxx */
  unsigned long flags;  /* translator flags */
  unsigned char **desc; /* translator string description */
  void **icon;          /* translator icon data */
} emsImporter;

typedef enum {
  EMS_IMPORT_Name_Query,
  EMS_IMPORT_Query,
  EMS_IMPORT_Settings,
  EMS_IMPORT_Signatures,
  EMS_IMPORT_Addresses,
  EMS_IMPORT_Mail
} ImportOperationEnum;

/* ----- Importer Data ------------------------------- */
typedef struct emsImporterDataS *emsImporterDataP, *emsImporterDataH;
typedef struct emsImporterDataS {
  long size;                /* Size of this data structure */
  ImportOperationEnum what; /* import operation selector */
  void *params;             /* parameters for the operation */
  void *results;            /* results of the opertaion */
} emsImporterData;

/* ----- Account Data ------------------------------ */
typedef struct ImportAccountInfoS *ImportAccountInfoP, *ImportAccountInfoH;
typedef struct ImportAccountInfoS {
  long size;         /* IN: Size of this data structure */
  FSSpec importSpec; /* spec pointing to folder or file to import */
  char accountName[64]; /* name of account */
  char appName[64];     /* name of Application */
  void **icon;       /* Application icon data */
  long id;           /* The importer that will actually do the work */
} ImportAccountInfoS;

/* ----- Import Settings Data --------------------- */
typedef struct ImportPersDataS *ImportPersDataP, *ImportPersDataH;
typedef struct ImportPersDataS {
  long size;            /* IN: Size of this data structure */
  long makeDominant;    /* Is this the main account? */
  char accountName[256];   /* The name of the personality to be created */
  char returnAddress[256]; /* return address */
  char realName[256];      /* real name */
  char userName[256];      /* username */
  char mailServer[256];    /* mail server */
  char smtpServer[256];    /* smtp server */
  bool isIMAP;          /* is this an IMAP account? */
} ImportPersData;

/* ----- Import Signature Data --------------------- */
typedef struct ImportSignaturesDataS ImportSignaturesDataS,
    *ImportSignaturesDataP, **ImportSignaturesDataH;
typedef struct ImportSignaturesDataS {
  long size;            /* IN: Size of this data structure */
  FSSpec importSpec;    /* Spec pointing to the folder or file to import */
  emsMakeSig makeSig;   /* Callback to actually create the signature file */
  emsProgress progress; /* Callback for progress */
} ImportSignaturesData;

/* ----- Make Sig callback data ------------------- */
typedef struct emsMakeSigDataS {
  long size;           /* Size of this data structure */
  unsigned char *name; /* the name of the signature file */
  void **sigText;      /* The text of the signature */
} emsMakeSigData;

/* ----- Import Contact Data ---------------------- */
typedef struct ImportAddressDataS ImportAddressDataS, *ImportAddressDataP,
    **ImportAddressDataH;
typedef struct ImportAddressDataS {
  long size;         /* IN: Size of this data structure */
  FSSpec importSpec; /* Spec pointing to the folder or file to import */
  char accountName[64]; /* name of the account */
  emsMakeAddressBook
      makeAddressBook;        /* Callback to create an address book file */
  emsMakeABEntry makeABEntry; /* Callback to create an address book entry */
  emsProgress progress;       /* Callback for progress */
} ImportAddressData;

/* ----- Make Address Book data ------------------- */
typedef struct emsMakeAddressBookDataS {
  long size;           /* Size of this data structure */
  unsigned char *name; /* the name of the address book file */
} emsMakeAddressBookData;

/* ----- Make Address Book Entry data ------------- */
typedef struct emsMakeABEntryDataS {
  long size;               /* Size of this data structure */
  short which;             /* where to put the address book entry */
  bool isGroup;            /* is this a group? */
  unsigned char *nickName; /* The name of the address book entry */
  void **addresses;        /* the addresses */
  void **notes;            /* the notes */
} emsMakeABEntryData;

/* ----- Import Messages and Mailbox Data -------- */
typedef struct ImportMailDataS ImportMailDataS, *ImportMailDataP,
    **ImportMailDataH;
typedef struct ImportMailDataS {
  long size;         /* IN: Size of this data structure */
  FSSpec importSpec; /* Spec pointing to the folder or file to import */
  char accountName[64]; /* name of the account */
  emsMakeMailbox makeMailbox; /* Callback to create a mailbox */
  emsMakeOutMess makeOutMess; /* Callback to create an outgoing message */
  emsMakeMessage
      makeMessage;      /* Callback to turn some MIME into a Eudora message */
  emsProgress progress; /* Callback for progress */
  emsImportMbox importMBox; /* Callback to run UUCP data */
} ImportMailData;

/* ----- Make Mailbox Callback Data -------------- */
typedef enum {
  EMS_IMPORT_MAILBOX_Start,
  EMS_IMPORT_MAILBOX_Create_Mailbox,
  EMS_IMPORT_MAILBOX_Flush_Mailbox,
  EMS_IMPORT_MAILBOX_Done
} ImportMailboxOperationEnum;

typedef struct emsMakeMailboxDataS {
  long size;                          /* Size of this data structure */
  ImportMailboxOperationEnum command; /* IN: what to do */
  FSSpec boxSpec;                     /* IN/OUT: the mailbox to create */
  bool isFolder;                      /* IN: is this mailbox a folder? */
  bool noSelect;                      /* IN: is this mailbox a simple folder? */
} emsMakeMailboxData;

/* ----- Make OutMess Calback Data -------------- */
typedef struct emsMakeOutMessDataS {
  long size;                  /* Size of this data structure */
  unsigned char *fromAddress; /* from address */
  void **toAddresses;         /* addresses in the To: field */
  void **ccAddresses;         /* addresses in the Cc: field */
  void **bccAddresses;        /* addresses in the Bcc: field */
  unsigned char *subject;     /* 255 characters of the subject */
  void **text;                /* the text of the message */
  short numAttachments;       /* the total number of attachments */
  void **attachSpecs;         /* the attachments */
  long inBoxNumber;           /* the mailbox the message is in */
  FSSpec boxSpec;             /* FSSpec of the mailbox */
  bool isSent;                /* Is this message a sent message? */
  long date;                  /* the date this message was sent */
} emsMakeOutMessData;

/* ----- Make MessFromMime Callback Data -------- */
typedef struct emsMakeMessageDataS {
  long size;          /* Size of this data structure */
  short ref;          /* where the data is coming from */
  long offset;        /* offset into the file where the message can be found */
  long len;           /* length of the message to be read */
  short state;        /* the message state the plugin believes to see */
  FSSpec boxSpec;     /* where the message should end up */
  void **attachments; /* the attachments to be stuck into this message */
} emsMakeMessageData;

/* ----- Import MBox Callback Data -------- */
typedef struct emsImportMboxDataS {
  long size;         /* Size of this data structure */
  FSSpec sourceMBox; /* The mbox file to read from */
  FSSpec boxSpec;    /* where the messages should end up */
} emsImportMboxData;

// end IMPORTERS

#endif // INTERNAL_FOR_QC

#ifdef INTERNAL_FOR_EUDORA
/* ========== ENTRY POINT ENUM CONSTANTS ==================================*/

enum {
  kems_plugin_versionRtn = 256,
  kems_plugin_initRtn = 257,
  kems_translator_infoRtn = 258,
  kems_can_translateRtn = 259,
  kems_translate_fileRtn = 260,
  kems_plugin_finishRtn = 261,
  kems_plugin_configRtn = 262,
  kems_queued_propertiesRtn = 263,
  kems_attacher_infoRtn = 264,
  kems_attacher_hookRtn = 265,
  kems_special_infoRtn = 266,
  kems_special_hookRtn = 267,
  kems_idleRtn = 268,
  kems_mbox_context_infoRtn = 269,
  kems_mbox_context_hookRtn = 270,
  // start MODELESS EMSAPI
  kems_plugwindow_closeRtn = 271,
  kems_plugwindow_reopenRtn = 272,
  kems_plugwindow_eventRtn = 273,
  kems_plugwindow_menu_enableRtn = 274,
  kems_plugwindow_menuRtn = 275,
  kems_plugwindow_dragRtn = 276,
  // end MODELESS EMSAPI
  // start Importer EMSAPI
  kems_importer_infoRtn = 277,
  kems_importer_hookRtn = 278,
  // end Importer EMSAPI
  kems_eudora_mode_notificationRtn = 279,
  kems_mbox_context_folderRtn = 280,
  kems_score_junkRtn = 281,
  kems_user_mark_junkRtn = 282
};
#endif // INTERNAL_FOR_EUDORA

/* ===== FUNCTION PROTOTYPES
 * ========================================================= */

#ifdef INTERNAL_FOR_QC
// Marshal parameters for component calls
#define EMSGCPCALL(x)                                                          \
  (ems_GCP.what = kems_##x##Rtn,                                               \
   ems_GCP.paramSize = (sizeof(ems_##x##Params) - sizeof(ComponentInstance)),  \
   ems_GCP.flags = 0, Callems_ComponentFunction(ems_GCP))
#endif // INTERNAL_FOR_QC

/* ----- Get the API Version number this plugin implements
 * --------------------------- */
EUDORA_ComponentResult ems_plugin_version(
    void **globals,   /* Out: Return for allocated instance structure */
    short *apiVersion /* Out: Plugin Version */
);
//
#ifdef INTERNAL_FOR_EUDORA
#if TARGET_RT_MAC_CFM
typedef struct ems_plugin_versionParams {
  short *apiVersion;
  void **globals;
} ems_plugin_versionParams;

#define ems_plugin_version(p1, p2)                                             \
  (ems_GCP.params.ems_plugin_version.globals = p1,                             \
   ems_GCP.params.ems_plugin_version.apiVersion = p2,                          \
   EMSGCPCALL(plugin_version))
#endif
#endif // INTERNAL_FOR_EUDORA <-SBL->

/* ----- Initialize plugin and get its basic info
 * ------------------------------------ */
EUDORA_ComponentResult ems_plugin_init(
    void **globals,      /* Out: Return for allocated instance structure */
    short eudAPIVersion, /* In: The API version eudora is using */
    emsMailConfigP mailConfig, /* In: Eudoras mail configuration */
    emsPluginInfoP pluginInfo  /* Out: Return Plugin Information */
);

typedef struct ems_plugin_initParams {
  emsPluginInfoP pluginInfo;
  emsMailConfigP mailConfig;
  short eudAPIVersion;
  void **globals;
} ems_plugin_initParams;
//
#ifdef INTERNAL_FOR_EUDORA
#if TARGET_RT_MAC_CFM
#define ems_plugin_init(p1, p2, p3, p4)                                        \
  (ems_GCP.params.ems_plugin_init.globals = p1,                                \
   ems_GCP.params.ems_plugin_init.eudAPIVersion = p2,                          \
   ems_GCP.params.ems_plugin_init.mailConfig = p3,                             \
   ems_GCP.params.ems_plugin_init.pluginInfo = p4, EMSGCPCALL(plugin_init))
#endif
#endif // INTERNAL_FOR_EUDORA <-SBL->

/* ----- Get details about a translator in a plugin
 * ---------------------------------- */
EUDORA_ComponentResult ems_translator_info(
    void **globals,          /* Out: Return for allocated instance structure */
    emsTranslatorP transInfo /* In/Out: Return Translator Information */
);
//
#ifdef INTERNAL_FOR_EUDORA
#if TARGET_RT_MAC_CFM
typedef struct ems_translator_infoParams {
  emsTranslatorP transInfo;
  void **globals;
} ems_translator_infoParams;

#define ems_translator_info(p1, p2)                                            \
  (ems_GCP.params.ems_translator_info.globals = p1,                            \
   ems_GCP.params.ems_translator_info.transInfo = p2,                          \
   EMSGCPCALL(translator_info))
#endif
#endif // INTERNAL_FOR_EUDORA <-SBL->

/* ----- Check and see if a translation can be performed
 * ----------------------------- */
EUDORA_ComponentResult ems_can_translate(
    void **globals,           /* Out: Return for allocated instance structure */
    emsTranslatorP trans,     /* In: Translator Info */
    emsDataFileP inTransData, /* In: What to translate */
    emsResultStatusP transStatus /* Out: Translations Status information */
);
//
#ifdef INTERNAL_FOR_EUDORA
#if TARGET_RT_MAC_CFM
typedef struct ems_can_translateParams {
  emsResultStatusP transStatus;
  emsDataFileP inTransData;
  emsTranslatorP trans;
  void **globals;
} ems_can_translateParams;

#define ems_can_translate(p1, p2, p3, p4)                                      \
  (ems_GCP.params.ems_can_translate.globals = p1,                              \
   ems_GCP.params.ems_can_translate.trans = p2,                                \
   ems_GCP.params.ems_can_translate.inTransData = p3,                          \
   ems_GCP.params.ems_can_translate.transStatus = p4,                          \
   EMSGCPCALL(can_translate))
#endif
#endif // INTERNAL_FOR_EUDORA <-SBL->

/* ----- Actually perform a translation on a file
 * ------------------------------------ */
EUDORA_ComponentResult ems_translate_file(
    void **globals,       /* Out: Return for allocated instance structure */
    emsTranslatorP trans, /* In: Translator Info */
    emsDataFileP inFile,  /* In: What to translate */
    emsProgress progress, /* Func to report progress/check for abort */
    emsDataFileP outFile, /* Out: Result of the translation */
    emsResultStatusP transStatus /* Out: Translations Status information */
);
//
#ifdef INTERNAL_FOR_EUDORA
#if TARGET_RT_MAC_CFM
typedef struct ems_translate_fileParams {
  emsResultStatusP transStatus;
  emsDataFileP outFile;
  emsProgress progress;
  emsDataFileP inFile;
  emsTranslatorP trans;
  void **globals;
} ems_translate_fileParams;

#define ems_translate_file(p1, p2, p3, p4, p5, p6)                             \
  (ems_GCP.params.ems_translate_file.globals = p1,                             \
   ems_GCP.params.ems_translate_file.trans = p2,                               \
   ems_GCP.params.ems_translate_file.inFile = p3,                              \
   ems_GCP.params.ems_translate_file.progress = p4,                            \
   ems_GCP.params.ems_translate_file.outFile = p5,                             \
   ems_GCP.params.ems_translate_file.transStatus = p6,                         \
   EMSGCPCALL(translate_file))
#endif
#endif // INTERNAL_FOR_EUDORA <-SBL->

/* ----- End use of a plugin and clean up
 * -------------------------------------------- */
EUDORA_ComponentResult ems_plugin_finish(
    void **globals /* Out: Return for allocated instance structure */
);
//
#ifdef INTERNAL_FOR_EUDORA
#if TARGET_RT_MAC_CFM
typedef struct ems_plugin_finishParams {
  void **globals;
} ems_plugin_finishParams;

#define ems_plugin_finish(p1)                                                  \
  (ems_GCP.params.ems_plugin_finish.globals = p1, EMSGCPCALL(plugin_finish))
#endif
#endif // INTERNAL_FOR_EUDORA <-SBL->

/* ----- Call the plug-ins configuration Interface
 * ----------------------------------- */
EUDORA_ComponentResult ems_plugin_config(
    void **globals,           /* Out: Return for allocated instance structure */
    emsMailConfigP mailConfig /* In: Eudora mail info */
);
//
#ifdef INTERNAL_FOR_EUDORA
#if TARGET_RT_MAC_CFM
typedef struct ems_plugin_configParams {
  emsMailConfigP mailConfig;
  void **globals;
} ems_plugin_configParams;

#define ems_plugin_config(p1, p2)                                              \
  (ems_GCP.params.ems_plugin_config.globals = p1,                              \
   ems_GCP.params.ems_plugin_config.mailConfig = p2,                           \
   EMSGCPCALL(plugin_config))
#endif
#endif // INTERNAL_FOR_EUDORA <-SBL->

/* ----- Manage properties for queued translations
 * ----------------------------------- */
EUDORA_ComponentResult ems_queued_properties(
    void **globals,       /* Out: Return for allocated instance structure */
    emsTranslatorP trans, /* In/Out: The translator */
    long *selected        /* In/Out: State of this translator */
);
//
#ifdef INTERNAL_FOR_EUDORA
#if TARGET_RT_MAC_CFM
typedef struct ems_queued_propertiesParams {
  long *selected;
  emsTranslatorP trans;
  void **globals;
} ems_queued_propertiesParams;

#define ems_queued_properties(p1, p2, p3)                                      \
  (ems_GCP.params.ems_queued_properties.globals = p1,                          \
   ems_GCP.params.ems_queued_properties.trans = p2,                            \
   ems_GCP.params.ems_queued_properties.selected = p3,                         \
   EMSGCPCALL(queued_properties))
#endif
#endif // INTERNAL_FOR_EUDORA <-SBL->

/* ----- Info about menu hook to attach/insert composed object
 * ----------------------- */
EUDORA_ComponentResult ems_attacher_info(
    void **globals,     /* Out: Return for allocated instance structure */
    emsMenuP attachMenu /* Out: The menu */
);
//
#ifdef INTERNAL_FOR_EUDORA
#if TARGET_RT_MAC_CFM
typedef struct ems_attacher_infoParams {
  emsMenuP attachMenu;
  void **globals;
} ems_attacher_infoParams;

#define ems_attacher_info(p1, p2)                                              \
  (ems_GCP.params.ems_attacher_info.globals = p1,                              \
   ems_GCP.params.ems_attacher_info.attachMenu = p2,                           \
   EMSGCPCALL(attacher_info))
#endif
#endif // INTERNAL_FOR_EUDORA <-SBL->

/* ----- Call an attacher hook to compose some special object
 * ------------------------ */
EUDORA_ComponentResult ems_attacher_hook(
    void **globals,           /* Out: Return for allocated instance structure */
    emsMenuP attachMenu,      /* In: The menu */
    char *attachDir,        /* In: Location to put attachments */
    long *numAttach,          /* Out: Number of files attached */
    emsDataFileH *attachFiles /* Out: Name of files written */
);
//
#ifdef INTERNAL_FOR_EUDORA
#if TARGET_RT_MAC_CFM
typedef struct ems_attacher_hookParams {
  emsDataFileH *attachFiles;
  long *numAttach;
  char *attachDir;
  emsMenuP attachMenu;
  void **globals;
} ems_attacher_hookParams;

#define ems_attacher_hook(p1, p2, p3, p4, p5)                                  \
  (ems_GCP.params.ems_attacher_hook.globals = p1,                              \
   ems_GCP.params.ems_attacher_hook.attachMenu = p2,                           \
   ems_GCP.params.ems_attacher_hook.attachDir = p3,                            \
   ems_GCP.params.ems_attacher_hook.numAttach = p4,                            \
   ems_GCP.params.ems_attacher_hook.attachFiles = p5,                          \
   EMSGCPCALL(attacher_hook))
#endif
#endif // INTERNAL_FOR_EUDORA <-SBL->

/* ----- Info about special menu items hooks
 * ----------------------------------------- */
EUDORA_ComponentResult ems_special_info(
    void **globals,      /* Out: Return for allocated instance structure */
    emsMenuP specialMenu /* Out: The menu */
);
//
#ifdef INTERNAL_FOR_EUDORA
#if TARGET_RT_MAC_CFM
typedef struct ems_special_infoParams {
  emsMenuP specialMenu;
  void **globals;
} ems_special_infoParams;

#define ems_special_info(p1, p2)                                               \
  (ems_GCP.params.ems_special_info.globals = p1,                               \
   ems_GCP.params.ems_special_info.specialMenu = p2, EMSGCPCALL(special_info))
#endif
#endif // INTERNAL_FOR_EUDORA <-SBL->

/* ----- Call a special menu item hook
 * ----------------------------------------------- */
EUDORA_ComponentResult ems_special_hook(
    void **globals,      /* Out: Return for allocated instance structure */
    emsMenuP specialMenu /* In: The menu */
);
//
#ifdef INTERNAL_FOR_EUDORA
#if TARGET_RT_MAC_CFM
typedef struct ems_special_hookParams {
  emsMenuP specialMenu;
  void **globals;
} ems_special_hookParams;

#define ems_special_hook(p1, p2)                                               \
  (ems_GCP.params.ems_special_hook.globals = p1,                               \
   ems_GCP.params.ems_special_hook.specialMenu = p2, EMSGCPCALL(special_hook))
#endif
#endif // INTERNAL_FOR_EUDORA <-SBL->

/* -----   Get some idle time
 * -------------------------------------------------------- */
EUDORA_ComponentResult
ems_idle(void **globals, /* Out: Return for allocated instance structure */
         emsIdleDataP idleData /* In:  data */
);
//
#ifdef INTERNAL_FOR_EUDORA
#if TARGET_RT_MAC_CFM
typedef struct ems_idleParams {
  emsIdleDataP idleData;
  void **globals;
} ems_idleParams;

#define ems_idle(p1, p2)                                                       \
  (ems_GCP.params.ems_idle.globals = p1,                                       \
   ems_GCP.params.ems_idle.idleData = p2, EMSGCPCALL(idle))
#endif
#endif // INTERNAL_FOR_EUDORA <-SBL->

/* ----- NEW in Eudora 6.0/EMSAPI 7.0 - called to score junk
 * ------------------------- */
EUDORA_ComponentResult
ems_score_junk(void **globals, /* In: Allocated instance structure returned in
                                 ems_plugin_init */
               emsTranslatorP trans,    /* In: Translator Info */
               emsJunkInfoP junkInfo,   /* In: Junk information */
               emsMessageInfoP message, /* In: Message to score */
               emsJunkScoreP junkScore, /* Out: Junk score */
               emsResultStatusP status  /* Out: Status information */
);
//
#ifdef INTERNAL_FOR_EUDORA
#if TARGET_RT_MAC_CFM
typedef struct ems_score_junkParams {
  emsResultStatusP status;
  emsJunkScoreP junkScore;
  emsMessageInfoP message;
  emsJunkInfoP junkInfo;
  emsTranslatorP trans;
  void **globals;
} ems_score_junkParams;

#define ems_score_junk(p1, p2, p3, p4, p5, p6)                                 \
  (ems_GCP.params.ems_score_junk.globals = p1,                                 \
   ems_GCP.params.ems_score_junk.trans = p2,                                   \
   ems_GCP.params.ems_score_junk.junkInfo = p3,                                \
   ems_GCP.params.ems_score_junk.message = p4,                                 \
   ems_GCP.params.ems_score_junk.junkScore = p5,                               \
   ems_GCP.params.ems_score_junk.status = p6, EMSGCPCALL(score_junk))
#endif
#endif // INTERNAL_FOR_EUDORA <-SBL->

/* ----- NEW in Eudora 6.0/EMSAPI 7.0 - called to mark as junk or not junk
 * ----------- */
EUDORA_ComponentResult
ems_user_mark_junk(void **globals, /* In: Allocated instance structure returned
                                     in ems_plugin_init */
                   emsTranslatorP trans,    /* In: Translator Info */
                   emsJunkInfoP junkInfo,   /* In: Junk information */
                   emsMessageInfoP message, /* In: Message that is either being
                                               marked as Junk or Not Junk */
                   emsJunkScoreP junkScore, /* In: Previous junk score */
                   emsResultStatusP status  /* Out: Status information */
);
//
#ifdef INTERNAL_FOR_EUDORA
#if TARGET_RT_MAC_CFM
typedef struct ems_user_mark_junkParams {
  emsResultStatusP status;
  emsJunkScoreP junkScore;
  emsMessageInfoP message;
  emsJunkInfoP junkInfo;
  emsTranslatorP trans;
  void **globals;
} ems_user_mark_junkParams;

#define ems_user_mark_junk(p1, p2, p3, p4, p5, p6)                             \
  (ems_GCP.params.ems_user_mark_junk.globals = p1,                             \
   ems_GCP.params.ems_user_mark_junk.trans = p2,                               \
   ems_GCP.params.ems_user_mark_junk.junkInfo = p3,                            \
   ems_GCP.params.ems_user_mark_junk.message = p4,                             \
   ems_GCP.params.ems_user_mark_junk.junkScore = p5,                           \
   ems_GCP.params.ems_user_mark_junk.status = p6, EMSGCPCALL(user_mark_junk))
#endif
#endif // INTERNAL_FOR_EUDORA <-SBL->

EUDORA_ComponentResult ems_eudora_mode_notification(
    void **globals, /* In: Allocated instance structure */
    emsEudoraModeNotificationDataP
        eudoraModeNotificationData /* In/Out: Mode notification information */
);

#ifdef INTERNAL_FOR_EUDORA
typedef struct ems_eudora_mode_notificationParams {
  emsEudoraModeNotificationDataP eudoraModeNotificationData;
  void **globals;
} ems_eudora_mode_notificationParams;

#if TARGET_RT_MAC_CFM
#define ems_eudora_mode_notification(p1, p2)                                   \
  (ems_GCP.params.ems_eudora_mode_notification.globals = p1,                   \
   ems_GCP.params.ems_eudora_mode_notification.eudoraModeNotificationData =    \
       p2,                                                                     \
   EMSGCPCALL(eudora_mode_notification))
#endif
#endif // INTERNAL_FOR_EUDORA

#ifdef INTERNAL_FOR_QC

/* -----  Get context menu for a given mailbox
 * -------------------------------------- */
EUDORA_ComponentResult ems_mbox_context_info(
    void **globals, emsMenuP mbox_context_Menu /* Out: The menu */
);

#ifdef INTERNAL_FOR_EUDORA
typedef struct ems_mbox_context_infoParams {
  emsMenuP mbox_context_Menu;
  void **globals;
} ems_mbox_context_infoParams;

#if TARGET_RT_MAC_CFM
#define ems_mbox_context_info(p1, p2)                                          \
  (ems_GCP.params.ems_mbox_context_info.globals = p1,                          \
   ems_GCP.params.ems_mbox_context_info.mbox_context_Menu = p2,                \
   EMSGCPCALL(mbox_context_info))
#endif
#endif // INTERNAL_FOR_EUDORA

/* ----- void ** context menu hit for mailbox
 * --------------------------------------- */
EUDORA_ComponentResult
ems_mbox_context_hook(void **globals,
                      emsMBoxP mailbox,          /* Out: The Chosen Mailbox */
                      emsMenuP mbox_context_Menu /* In: The menu */
);

#ifdef INTERNAL_FOR_EUDORA
typedef struct ems_mbox_context_hookParams {
  emsMenuP mbox_context_Menu;
  emsMBoxP mailbox;
  void **globals;
} ems_mbox_context_hookParams;

#if TARGET_RT_MAC_CFM
#define ems_mbox_context_hook(p1, p2, p3)                                      \
  (ems_GCP.params.ems_mbox_context_hook.globals = p1,                          \
   ems_GCP.params.ems_mbox_context_hook.mailbox = p2,                          \
   ems_GCP.params.ems_mbox_context_hook.mbox_context_Menu = p3,                \
   EMSGCPCALL(mbox_context_hook))
#endif
#endif // INTERNAL_FOR_EUDORA

EUDORA_ComponentResult ems_mbox_context_folder(
    void **globals, /* In: Allocated instance structure */
    emsMailBoxContextFolderDataP
        mailBoxContextFolderData /* In/Out: Mode notification information */
);

#ifdef INTERNAL_FOR_EUDORA
typedef struct ems_mbox_context_folderParams {
  emsMailBoxContextFolderDataP mailBoxContextFolderData;
  void **globals;
} ems_mbox_context_folderParams;

#if TARGET_RT_MAC_CFM
#define ems_mbox_context_folder(p1, p2)                                        \
  (ems_GCP.params.ems_mbox_context_folder.globals = p1,                        \
   ems_GCP.params.ems_mbox_context_folder.mailBoxContextFolderData = p2,       \
   EMSGCPCALL(mbox_context_folder))
#endif
#endif // INTERNAL_FOR_EUDORA

// start MODELESS EMSAPI
EUDORA_ComponentResult ems_plugwindow_close(
    void **globals, emsPlugwindowDataP data /* In: window to close */
);

#ifdef INTERNAL_FOR_EUDORA
typedef struct ems_plugwindow_closeParams {
  emsPlugwindowDataP data;
  void **globals;
} ems_plugwindow_closeParams;

#if TARGET_RT_MAC_CFM
#define ems_plugwindow_close(p1, p2)                                           \
  (ems_GCP.params.ems_plugwindow_close.globals = p1,                           \
   ems_GCP.params.ems_plugwindow_close.data = p2,                              \
   EMSGCPCALL(plugwindow_close))
#endif
#endif // INTERNAL_FOR_EUDORA

void ems_plugwindow_reopen(
    void **globals, emsPlugwindowDataP data /* In: window to close */
);

#ifdef INTERNAL_FOR_EUDORA
typedef struct ems_plugwindow_reopenParams {
  emsPlugwindowDataP data;
  void **globals;
} ems_plugwindow_reopenParams;

#if TARGET_RT_MAC_CFM
#define ems_plugwindow_reopen(p1, p2)                                          \
  (ems_GCP.params.ems_plugwindow_reopen.globals = p1,                          \
   ems_GCP.params.ems_plugwindow_reopen.data = p2,                             \
   EMSGCPCALL(plugwindow_reopen))
#endif
#endif // INTERNAL_FOR_EUDORA

EUDORA_ComponentResult ems_plugwindow_event(void **globals,
                                            emsPlugwindowDataP data,
                                            emsPlugwindowEventP event);

#ifdef INTERNAL_FOR_EUDORA
typedef struct ems_plugwindow_eventParams {
  emsPlugwindowEventP event;
  emsPlugwindowDataP data;
  void **globals;
} ems_plugwindow_eventParams;

#if TARGET_RT_MAC_CFM
#define ems_plugwindow_event(p1, p2, p3)                                       \
  (ems_GCP.params.ems_plugwindow_event.globals = p1,                           \
   ems_GCP.params.ems_plugwindow_event.data = p2,                              \
   ems_GCP.params.ems_plugwindow_event.event = p3,                             \
   EMSGCPCALL(plugwindow_event))
#endif
#endif // INTERNAL_FOR_EUDORA

void ems_plugwindow_menu_enable(void **globals, emsPlugwindowDataP data,
                                       emsPlugwindowMenuDataP menuData);

#ifdef INTERNAL_FOR_EUDORA
typedef struct ems_plugwindow_menu_enableParams {
  emsPlugwindowMenuDataP menuData;
  emsPlugwindowDataP data;
  void **globals;
} ems_plugwindow_menu_enableParams;

#if TARGET_RT_MAC_CFM
#define ems_plugwindow_menu_enable(p1, p2, p3)                                 \
  (ems_GCP.params.ems_plugwindow_menu_enable.globals = p1,                     \
   ems_GCP.params.ems_plugwindow_menu_enable.data = p2,                        \
   ems_GCP.params.ems_plugwindow_menu_enable.menuData = p3,                    \
   EMSGCPCALL(plugwindow_menu_enable))
#endif
#endif // INTERNAL_FOR_EUDORA

EUDORA_ComponentResult ems_plugwindow_menu(void **globals,
                                           emsPlugwindowDataP data,
                                           emsPlugwindowMenuDataP menuData);

#ifdef INTERNAL_FOR_EUDORA
typedef struct ems_plugwindow_menuParams {
  emsPlugwindowMenuDataP menuData;
  emsPlugwindowDataP data;
  void **globals;
} ems_plugwindow_menuParams;

#if TARGET_RT_MAC_CFM
#define ems_plugwindow_menu(p1, p2, p3)                                        \
  (ems_GCP.params.ems_plugwindow_menu.globals = p1,                            \
   ems_GCP.params.ems_plugwindow_menu.data = p2,                               \
   ems_GCP.params.ems_plugwindow_menu.menuData = p3,                           \
   EMSGCPCALL(plugwindow_menu))
#endif
#endif // INTERNAL_FOR_EUDORA

EUDORA_ComponentResult ems_plugwindow_drag(void **globals,
                                           emsPlugwindowDataP data,
                                           emsPlugwindowDragDataP dragData);

#ifdef INTERNAL_FOR_EUDORA
typedef struct ems_plugwindow_dragParams {
  emsPlugwindowDragDataP dragData;
  emsPlugwindowDataP data;
  void **globals;
} ems_plugwindow_dragParams;

#if TARGET_RT_MAC_CFM
#define ems_plugwindow_drag(p1, p2, p3)                                        \
  (ems_GCP.params.ems_plugwindow_drag.globals = p1,                            \
   ems_GCP.params.ems_plugwindow_drag.data = p2,                               \
   ems_GCP.params.ems_plugwindow_drag.dragData = p3,                           \
   EMSGCPCALL(plugwindow_drag))
#endif
#endif // INTERNAL_FOR_EUDORA
// end MODELESS EMSAPI

/* ---------- Get details about an importer in a plugin ------------------ */
EUDORA_ComponentResult ems_importer_info(
    void **globals,           /* Out: Return for allocated instance structure */
    emsImporterP importerInfo /* In/Out: Return Translator Information */
);

#ifdef INTERNAL_FOR_EUDORA
typedef struct ems_importer_infoParams {
  emsImporterP importerInfo;
  void **globals;
} ems_importer_infoParams;

#if TARGET_RT_MAC_CFM
#define ems_importer_info(p1, p2)                                              \
  (ems_GCP.params.ems_importer_info.globals = p1,                              \
   ems_GCP.params.ems_importer_info.importerInfo = p2,                         \
   EMSGCPCALL(importer_info))
#endif
#endif // INTERNAL_FOR_EUDORA

/* ----- Call an importer hook ------------------------------------- */
EUDORA_ComponentResult ems_importer_hook(
    void **globals, /* Out: Return for allocated instance structure */
    emsImporterDataP importerData /* In/Out: Return Translator Information */
);

#ifdef INTERNAL_FOR_EUDORA
typedef struct ems_importer_hookParams {
  emsImporterDataP importerData;
  void **globals;
} ems_importer_hookParams;

#if TARGET_RT_MAC_CFM
#define ems_importer_hook(p1, p2)                                              \
  (ems_GCP.params.ems_importer_hook.globals = p1,                              \
   ems_GCP.params.ems_importer_hook.importerData = p2,                         \
   EMSGCPCALL(importer_hook))
#endif
#endif // INTERNAL_FOR_EUDORA

#endif // INTERNAL_FOR_QC

#ifdef INTERNAL_FOR_EUDORA
/* ========== UNION DEFINITION PPC only ====================================*/
#if TARGET_RT_MAC_CFM
typedef union ems_ComponentParamUnion {
  ems_plugin_versionParams ems_plugin_version;
  ems_plugin_initParams ems_plugin_init;
  ems_translator_infoParams ems_translator_info;
  ems_can_translateParams ems_can_translate;
  ems_translate_fileParams ems_translate_file;
  ems_plugin_finishParams ems_plugin_finish;
  ems_plugin_configParams ems_plugin_config;
  ems_queued_propertiesParams ems_queued_properties;
  ems_attacher_infoParams ems_attacher_info;
  ems_attacher_hookParams ems_attacher_hook;
  ems_special_infoParams ems_special_info;
  ems_special_hookParams ems_special_hook;
  ems_idleParams ems_idle;
  ems_score_junkParams ems_score_junk;
  ems_user_mark_junkParams ems_user_mark_junk;
  ems_mbox_context_infoParams ems_mbox_context_info;
  ems_mbox_context_hookParams ems_mbox_context_hook;
  ems_eudora_mode_notificationParams ems_eudora_mode_notification;
  ems_mbox_context_folderParams ems_mbox_context_folder;
  // start MODELESS EMSAPI
  ems_plugwindow_closeParams ems_plugwindow_close;
  ems_plugwindow_reopenParams ems_plugwindow_reopen;
  ems_plugwindow_eventParams ems_plugwindow_event;
  ems_plugwindow_menu_enableParams ems_plugwindow_menu_enable;
  ems_plugwindow_menuParams ems_plugwindow_menu;
  ems_plugwindow_dragParams ems_plugwindow_drag;
  // end MODELESS EMSAPI
  ems_importer_infoParams ems_importer_info;
  ems_importer_hookParams ems_importer_hook;
} ems_ComponentParamUnion;

typedef struct ems_ComponentParameters {
  unsigned char flags;     /* Flags - set to zero */
  unsigned char paramSize; /* Size of the params struct */
  short what;              /* The component request selector */
  ems_ComponentParamUnion params;
} ems_ComponentParameters;

/* This global takes the routine parameters for each component function call */
extern ems_ComponentParameters ems_GCP;

/* Routine to put a copy of the component params on the stack and call
   CallComponentUPP.  This function is implemented in emsapi-gum.c */
EUDORA_ComponentResult
Callems_ComponentFunction(ems_ComponentParameters params);
#endif

#if PRAGMA_IMPORT_SUPPORTED
#pragma import off
#endif

#else // EXTERNAL_FOR_PLUGINS
#ifdef __cplusplus
}
#endif
#endif // EXTERNAL_FOR_PLUGINS <-SBL->

#if PRAGMA_STRUCT_ALIGN
#pragma options align = reset
#endif

#endif /* __EMS_MAC__ */

#endif /* EMS API FILE */
