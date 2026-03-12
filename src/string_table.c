/* Auto-generated string table from StringDefs.h */
/* Maps Mac STR# resource IDs to their string values */

#include <string.h>
#include <stdint.h>

typedef struct { uint16_t id; const char *str; } StrEntry;

static const StrEntry string_table[] = {
  /* HeaderStrn (1600+n): compose window header labels */
  {1601, "To:"},          /* TO_HEAD */
  {1602, "From:"},        /* FROM_HEAD */
  {1603, "Subject:"},     /* SUBJ_HEAD */
  {1604, "Cc:"},          /* CC_HEAD */
  {1605, "Bcc:"},         /* BCC_HEAD */
  {1606, "Attachments:"}, /* ATTACH_HEAD */
  {4501, ".pdf"}, /* PDF_QUOTE_EXTENSION_UNQUOTE */
  {4502, "Error while filtering for %p:"}, /* ERR_PERS_FILTERING */
  {4503, "Filtering messages for %p."}, /* IMAP_FILTERING_MESSAGES */
  {4504, "200"}, /* OUTGOING_MID_LIST_SIZE */
  {4505, "iChat"}, /* ICHAT_BTN_TEXT */
  {4506, "aim"}, /* AIM_PROTO */
  {4507, "aim:goim?screenname=%p"}, /* AIM_URL_FMT */
  {4508, "FW:"}, /* OUTLOOK_FW_PREFIX */
  {4509, "http,https,ftp"}, /* URL_HOST_CHECK_PROTOS */
  {4510, "18,Emoticons24,24,Emoticons32"}, /* ALTERNATE_EMOTICONS */
  {4511, "0,48000,0"}, /* BADGE_ATTENTION_COLOR */
  {4512, "0,0,48000"}, /* BADGE_NORMAL_COLOR */
  {4513, "65000,65000,65000"}, /* BADGE_TEXT_COLOR */
  {4514, "*"}, /* BADGE_PERS_LIST */
  {4515, "10"}, /* EMO_BATCH_SIZE */
  {4516, ".rtf"}, /* NOT_MOVIE_EXTENSIONS */
  {4517, "http,https,ftp"}, /* SLASHY_SCHEMES */
  {4518, "9"}, /* TB_BUTTON_FONT_SIZE */
  {4519, "You have %d days remaining of your Paid mode preview."}, /* DEMO_DAYS_REMAINING_FMT */
  {4520, "Today is the last day of your Paid mode preview.  Eudora will switch to Sponsored mode tomorrow!"}, /* DEMO_LAST_DAY_FMT */
  {4521, "%p\\n%d days left"}, /* PAYMODE_DEMO_BTN_TITLE_FMT */
  {4522, "Check completed successfully"}, /* CHECK_CONNECTION_SUCCESS */
  {4523, "Host: \"%p\"   Port: %d"}, /* CHECK_CONNECTION_FORMAT */
  {4524, "Eudora Export"}, /* EXPORT_FOLDER_NAME */
  {4525, "Error exporting mail"}, /* EXPORT_MAIL_ERR */
  {4526, "Mail Folder"}, /* EXPORT_MAIL_FOLDER */
  {4527, "Exporting Mail..."}, /* EXPORTING_MAIL */
  {4528, "X-Mozilla-Status: %4x%p"}, /* MOZILLA_FLAGS_FMT */
  {4529, "X-Mozilla-Status2: %8x%p"}, /* MOZILLA_FLAGS2_FMT */
  {4530, "X-Mozilla-Keys:"}, /* MOZILLA_KEYWORD_HDR */
  {5501, "Alternate"}, /* ALT_SIG */
  {5502, "Header:"}, /* HEADER_LABEL */
  {5503, "Match:"}, /* MATCH_LABEL */
  {5504, "Actions:"}, /* ACTION_LABEL */
  {5505, "Make subject:"}, /* MAKE_SUBJECT_LABEL */
  {5506, "An error occurred whilst reading the filtering rules."}, /* READ_FILTERS */
  {5507, "This version of Eudora allows only two terms in rules; do not change the rules, or the extra terms will be removed."}, /* FILTER_OVERTERM */
  {5508, "Unknown keyword in filters.\nDid not understand keyword \"%p.\"  It may have been put there by a different version of Eudora.  The keyword will be removed if you save your Filters.\n\n\n"}, /* FILT_BADKEY_FMT */
  {5509, "Please select a mailbox from the Transfer menu, or command-period to cancel."}, /* FILT_XFER_SEL */
  {5510, "Error saving filters."}, /* SAVE_FILTERS */
  {5511, "&"}, /* SUBJ_REPLACE */
  {5512, "\\n%p Messages have been filtered into the following mailboxes:\\n"}, /* SPEC_INTRO */
  {5513, "Filter Report"}, /* SPEC_TITLE */
  {5514, "%p %d\\n"}, /* SPEC_FMT */
  {5515, "Transfer to:"}, /* FILT_XFER */
  {5516, "Copy to:"}, /* FILT_COPY */
  {5517, "Please choose an application to open \"%p\"."}, /* CHOOSE_APP */
  {5518, "Cannot read the Desktop database."}, /* DT_TROUBLE */
  {5519, "That application doesn't know how to provide word services to Eudora."}, /* NOT_SPELLER */
  {5520, "WORDSERVICES"}, /* WORD_SERVICES */
  {5601, "(One of) the address(es) is (are) too long.  Remember to use COMMAS (\",\") to separate addresses.  Mail cannot be sent with such addresses."}, /* ADDR_TOO_LONG */
  {5602, ""}, /* UNUSED_WAS_ALERT_TIMEOUT */
  {5603, "Nickname:"}, /* ALIAS_A_LABEL */
  {5604, "130"}, /* ALIAS_A_WIDTH */
  {5605, "alias"}, /* ALIAS_CMD */
  {5606, "Address(es):"}, /* ALIAS_E_LABEL */
  {5607, "Eudora Nicknames"}, /* ALIAS_FILE */
  {5608, "70"}, /* ALIAS_ADDR_PC */
  {5609, "The nickname you've typed is too long.\nNicknames in Eudora are limited to %d characters.  The nickname you typed is too long.\n\n\nOK-"}, /* ALIAS_TOO_LONG */
  {5610, "Your address book contains a nickname that refers to itself, directly or indirectly."}, /* ALIA_LOOP */
  {5611, "There is insufficient memory to read the address book."}, /* ALLO_ALIAS */
  {5612, "There is insufficient memory to expand the nicknames."}, /* ALLO_EXPAND */
  {5613, "There is insufficient memory to read mailbox names."}, /* ALLO_MBOX_LIST */
  {5614, "R"}, /* ALREADY_READ */
  {5615, "Geneva"}, /* APPL_FONT */
  {5616, ":%p:%d:%p:"}, /* ATTACH_FMT */
  {5617, "Eudora needs your attention."}, /* ATTENTION */
  {5618, "An address is too long or otherwise malformed.  Mail cannot be sent with such addresses."}, /* BAD_ADDRESS */
  {5619, "Couldn't rename the mailbox; the mail has been saved under a new name."}, /* BAD_COMP_RENAME */
  {5620, "Error involving Domain Name System."}, /* BIND_ERR */
  {5701, "Choose a \"Word Services\" application."}, /* SPELL_PROMPT */
  {5702, "AppleEvent-aware Applications"}, /* SPELL_LABEL */
  {5703, "That service cannot be found and will be removed from the menu."}, /* SPELL_ALIAS_GONE */
  {5704, "X-Stuff:"}, /* X_STUFF */
  {5705, "That stationery is incompatible with this version of Eudora."}, /* INVALID_STATIONERY */
  {5706, "Default"}, /* STATIONERY */
  {5707, "Eudora can only open mailboxes that are inside your Eudora Folder."}, /* ONLY_IN_TREE */
  {5708, "No explanation available"}, /* NO_SETTING_HELP */
  {5709, "%pbegin 644 %p%p"}, /* UUDECODE_FMT */
  {5710, "The original message couldn't be found."}, /* BROKEN_LINK */
  {5711, "X-Link-Box:"}, /* TOC_ALIAS_HEAD */
  {5712, "X-Link-Id:"}, /* XLINK_HEAD */
  {5713, "Return-Receipt-To: %p%p"}, /* RRT_FMT */
  {5714, "Use this menu to change the \"signature\" that will be attached to your mail."}, /* SIG_MENU_HELP */
  {5715, "No signature text will be sent."}, /* NOSIG_HELP */
  {5716, "Your standard signature text will be sent."}, /* SIG_HELP */
  {5717, "A file of your own choosing will be sent."}, /* ARB_SIG_HELP */
  {5718, "Your alternate signature text will be sent."}, /* ALT_SIG_HELP */
  {5719, ""}, /* U_SHOW_ALL_HELP */
  {5720, ""}, /* U_NO_SHOW_ALL_HELP */
  {5801, "(This file must be converted with BinHex"}, /* BINHEX */
  {5802, "There were some extra data in the attachment."}, /* BINHEXEXCESS */
  {5803, "The attachment has been corrupted; an illegal character was found."}, /* BINHEX_BADCHAR */
  {5804, "Couldn't create attachment to decode into."}, /* BINHEX_CREATE */
  {5805, "Receiving file \"%p\"..."}, /* BINHEX_RECV_FMT */
  {5806, "There is insufficient memory to decode the attachment."}, /* BINHEX_MEM */
  {5807, "There was an error opening the attachment."}, /* BINHEX_OPEN */
  {5808, "(This file must be converted with BinHex 4.0)"}, /* BINHEX_OUT */
  {5809, "Sending file \"%p\"..."}, /* BINHEX_PROG_FMT */
  {5810, "Save attachment as:"}, /* BINHEX_PROMPT */
  {5811, "There was an error sending the attachment."}, /* BINHEX_READ */
  {5812, "The attachment is corrupt; it was too short."}, /* BINHEX_SHORT */
  {5813, "There was an error writing the attachment."}, /* BINHEX_WRITE */
  {5814, "The mailbox name:"}, /* BOX_TOO_LONG1 */
  {5815, "is too long; mailbox names must be 27 characters or less."}, /* BOX_TOO_LONG2 */
  {5816, "4096"}, /* BUFFER_SIZE */
  {5817, "Can't queue this message; all messages must have addresses in the to: or bcc: fields, and a valid From address."}, /* CANT_QUEUE */
  {5818, "Check Mail"}, /* CHECK_MAIL */
  {5819, "There was an error closing the mailbox--data may have been lost."}, /* CLOSE_MBOX */
  {5820, "Contacting %p (%i)..."}, /* CNXN_OPENING */
  {5901, "Other...<I"}, /* OTHER_ITEM_TEXT */
  {5902, "Please select a mailbox."}, /* CHOOSE_MBOX */
  {5903, ">"}, /* FLOWED_QUOTE */
  {5904, "--"}, /* SIG_SEPARATOR */
  {5905, "Undo Transfer to %p"}, /* UNDO_XFER */
  {5906, "Redo Transfer to %p"}, /* REDO_XFER */
  {5907, "Couldn't undo the transfer; the transferred messages can't be found."}, /* CANT_XF_UNDO */
  {5908, "Can't find that attachment."}, /* ATTACH_GONE */
  {5909, "Any Header"}, /* FILTER_ANY */
  {5910, "Body"}, /* FILTER_BODY */
  {5911, "external-body"}, /* EXTERNAL_BODY */
  {5912, "mail-server"}, /* MAIL_SERVER */
  {5913, "\\n[The following attachment must be fetched by mail. Command-click the URL below and send the resulting message to get the attachment.]\\n"}, /* EXTERNAL_MAIL */
  {5914, "anon-ftp"}, /* ANON_FTP */
  {5915, "%r %r://%p/%p/%p"}, /* ANARCHIE_FMT */
  {5916, "GET"}, /* ANARCHIE_GET */
  {5917, "TXT"}, /* ANARCHIE_TXT */
  {5918, "ftp"}, /* ANARCHIE_FTP */
  {5919, "AURL"}, /* ANARCHIE_TYPE */
  {5920, "Arch"}, /* ANARCHIE_CREATOR */
  {6001, "34"}, /* COMP_TOP_MARGIN */
  {6002, "Copy failed."}, /* COPY_FAILED */
  {6003, "Couldn't get connection tool."}, /* COULDNT_GET_TOOL */
  {6004, "Couldn't open Mailbox window."}, /* COULDNT_MBOX */
  {6005, "Couldn't apply your changes to the previous address book entry."}, /* COULDNT_MOD_ALIAS */
  {6006, "Couldn't display the settings window."}, /* COULDNT_PREF */
  {6007, "Couldn't save the message."}, /* COULDNT_SAVEAS */
  {6008, "Couldn't create Page Setup information."}, /* COULDNT_SETUP */
  {6009, "Couldn't compact the mailbox."}, /* COULDNT_SQUEEZE */
  {6010, "Couldn't create another window."}, /* COULDNT_WIN */
  {6011, "A checksum error was found in decoding this attachment; use it with caution."}, /* CRC_ERROR */
  {6012, "Couldn't create mail folder."}, /* CREATE_FOLDER */
  {6013, "There was an error creating the settings file."}, /* CREATE_SETTINGS */
  {6014, "Couldn't create your Address Book file."}, /* CREATING_ALIAS */
  {6015, "Couldn't create the mailbox."}, /* CREATING_MAILBOX */
  {6016, "Closing the connection."}, /* CTB_CLOSING */
  {6017, "\\015\\012"}, /* CTB_NEWLINE */
  {6018, "Communications Toolbox error."}, /* CTB_PROBLEM */
  {6019, "Cut failed."}, /* CUT_FAILED */
  {6020, "%r %p"}, /* DATE_HEADER */
  {6101, "\\n[The following attachment must be fetched by ftp.  Command-click the URL below to ask your ftp client to fetch it.]\\n"}, /* EXTERNAL_FTP */
  {6102, "image"}, /* IMAGE */
  {6103, "The EuOM maps are incorrect; cannot send the attachment."}, /* INVALID_MAP */
  {6104, "ascii"}, /* ASCII */
  {6105, "Label:"}, /* FILTER_LABEL_LABEL */
  {6106, "30"}, /* TEXT_DARKER */
  {6107, "0"}, /* AREA_LIGHTER */
  {6108, "^0:^1"}, /* FILTER_NAME */
  {6109, "^1, ^0^3"}, /* ANCIENT_LOCAL_DATE_FMT */
  {6110, "^2, ^0^3"}, /* OLD_LOCAL_DATE_FMT */
  {6111, "^0^3"}, /* LOCAL_DATE_FMT */
  {6112, "-1"}, /* OLD_DATE */
  {6113, "144"}, /* ANCIENT_DATE */
  {6114, "65535"}, /* PASTEL_LIGHT */
  {6115, "20000"}, /* PASTEL_SATUR8 */
  {6116, "Get %p"}, /* FETCH_FN_FMT */
  {6117, "Filtering..."}, /* FILTERING */
  {6118, "Messages remaining to filter:"}, /* LEFT_TO_FILTER */
  {6119, "\\n\\nWARNING: The remainder of this %K message has not been transferred.  Turn on the \"Fetch\" button in the icon bar and check mail again to get the whole thing.\\n"}, /* BIG_MESSAGE_MSG2 */
  {6120, "SPWE"}, /* SPELLSWELL_CREATOR */
  {6201, "%r %r %d%d %d%d:%d%d:%d%d %d\\n"}, /* DATE_STRING_FMT */
  {6202, "Couldn't remove mailbox."}, /* DELETING_BOX */
  {6203, ", \\t"}, /* DELIMITERS */
  {6204, "Finding"}, /* DNR_LOOKUP */
  {6205, "3"}, /* DOUBLE_TOLERANCE */
  {6206, "The \"\\0xE9\" in \"Check for mail every \\0xE9 minute(s)\" must be a number."}, /* EXPL_INTERVAL */
  {6207, "The \"POP Account\" should be an user name, followed by an \"@\", followed by a host name.\\nFOR EXAMPLE, \"joe@ux8.cso.uiuc.edu\"."}, /* EXPL_POP */
  {6208, "The \"SMTP Server\" must be a host name.\\nFOR EXAMPLE, \"ux8.cso.uiuc.edu\"."}, /* EXPL_SMTP */
  {6209, "Eudora cannot continue."}, /* FATAL */
  {6210, "Attachment converted: %p:%p (%p/%p) (%p)\\n"}, /* FILE_FOLDER_FMT */
  {6211, "{%d:%d}"}, /* FILE_LINE_FMT */
  {6212, "Peeking at message %d..."}, /* FIRST_UNREAD */
  {6213, "10"}, /* FLUSH_SECS */
  {6214, "Eudora Folder"}, /* FOLDER_NAME */
  {6215, "\"%p\" is too large or too small."}, /* FONTSIZE_EXPL */
  {6216, "An error occurred."}, /* GENERAL */
  {6217, "Couldn't load menus."}, /* GET_MENU */
  {6218, "Getting ready to make a connection."}, /* HOUSEKEEPING */
  {6219, "3"}, /* ICMP_SECONDS */
  {6220, "In"}, /* IN */
  {6301, "BuyE"}, /* REGISTER_CREATOR */
  {6302, "Register Eudora..."}, /* REGISTER_EUDORA */
  {6303, "Can't find the registration program."}, /* NO_REGISTER */
  {6304, "%c\\t%p\\t%p\\t%p\\t%p\\t%p\\t%d\\t%p\\n"}, /* SUM_COPY_FMT */
  {6305, "Please choose an application to do %p."}, /* CHOOSE_URL_APP */
  {6306, "Can't encrypt a message to \"%p\" without a key.  The message will be saved, but can't be sent unless you turn off encryption."}, /* NO_KEY_FMT */
  {6307, "Can't encrypt or sign a message unless you establish a key for \"%p\"."}, /* NO_ME_KEY */
  {6308, "pgp"}, /* PGP_PROTOCOL */
  {6309, "secring.pgp"}, /* SECRET_KEYRING */
  {6310, ".asc"}, /* ASC_SUFFIX */
  {6311, "mime"}, /* MIME */
  {6312, "x-pgpMimeEncrypted"}, /* MIME_ENC_PGP */
  {6313, "x-pgpMimeClear"}, /* MIME_CLEAR_PGP */
  {6314, "The signature for \"%p\" matches."}, /* PGP_SIG_MATCH */
  {6315, "The signature for \"%r\" does not match.  The message was either corrupted in transit, or possibly forged."}, /* PGP_NO_MATCH */
  {6316, "Couldn't verify the signature."}, /* PGP_CANT_VERIFY */
  {6317, "Unable to decrypt the message."}, /* PGP_CANT_DECRYPT */
  {6318, "Unable to encrypt the message."}, /* PGP_CANT_ENCRYPT */
  {6319, "Unable to attach your signature to the file."}, /* PGP_CANT_SIGN */
  {6320, "PGP gave no explanation"}, /* MR_PGP_BROKEN */
  {6401, "~~~~~~~"}, /* INFINITE_STRING */
  {6402, "Starting the Comm. Toolbox."}, /* INIT_CTB */
  {6403, "Logging into the POP server."}, /* LOGGING_IN */
  {6404, "Invalid POP account or password."}, /* LOGIN_FAILED */
  {6405, "Looking for mail."}, /* LOOK_MAIL */
  {6406, "Unable to find or create your Eudora Folder."}, /* MAIL_FOLDER */
  {6407, "Trying to make a connection."}, /* MAKE_CONNECT */
  {6408, "200000"}, /* FRAGMENT_SIZE */
  {6409, "That mailbox may not be removed."}, /* MAYNT_DELETE_BOX */
  {6410, "That mailbox may not be renamed."}, /* MAYNT_RENAME_BOX */
  {6411, "untitled folder"}, /* UNTITLED_FOLDER */
  {6412, "untitled mailbox"}, /* UNTITLED_MAILBOX */
  {6413, "New"}, /* NEW_BUTTON */
  {6414, "Remove"}, /* REMOVE_BUTTON */
  {6415, ","}, /* LDAP_SEARCH_TOKEN_DELIMS */
  {6416, "note"}, /* NOTE_CMD */
  {6417, "The operation failed; there was not enough memory."}, /* MEM_ERR */
  {6418, "Memory is tight."}, /* MEM_LOW */
  {6419, "Memory sizes: Current %K, Minimum %K."}, /* MEM_PARTITION */
  {6420, "Couldn't create a TextEdit record for this mailbox."}, /* MESS_TE */
  {6501, "The encrypted message isn't in MIME format.  Please use the PGP application directly."}, /* PGP_NOT_MESSAGE */
  {6502, "30"}, /* FILTER_LIST_PERCENT */
  {6503, "%d/%d/%K/%K"}, /* BOX_SIZE_SELECT_FMT */
  {6504, "%d:%r:%p:%p"}, /* TB_FMT */
  {6505, "pgp -kv \\\"%p\\\""}, /* PGP_HAVEKEY_FMT */
  {6506, "Couldn't overwrite decrypted information."}, /* WIPE_ERROR */
  {6507, "Any Recipient"}, /* FILTER_ADDRESSEE */
  {6508, "%q"}, /* RNAME_QUOTE */
  {6509, "%p\"%p\""}, /* KEY_FMT */
  {6510, "\\n\\nWARNING: The remainder of this %K message has not been transferred, because there was not enough disk space.  Make more space and check mail again to get the whole thing."}, /* NOSPACE_SKIP */
  {6511, "ctl+"}, /* CONTROL_PLUS */
  {6512, "opt+"}, /* OPTION_PLUS */
  {6513, "shft+"}, /* SHIFT_PLUS */
  {6514, "cmd+"}, /* COMMAND_PLUS */
  {6515, "Couldn't find stationery file."}, /* READ_STATION */
  {6516, "60"}, /* TB_AUTOHELP_DELAY */
  {6517, "The PGP data in this message is incomplete."}, /* PGP_MISSING */
  {6518, "That message doesn't have a subject.  Send it anyway?"}, /* SUBJECT_WARNING */
  {6519, "That message will be around %d K in size.  Send it anyway?"}, /* SIZE_WARNING */
  {6520, "Show Toolbar"}, /* SHOW_TOOLBAR */
  {6601, "Couldn't move the mailbox."}, /* MOVE_MAILBOX */
  {6602, "Navigate In"}, /* NAVIN */
  {6603, "Navigate Out"}, /* NAVOUT */
  {6604, "\\015\\012"}, /* NEWLINE */
  {6605, "New...<I"}, /* NEW_ITEM_TEXT */
  {6606, "Colons (\":\") are not allowed in mailbox names."}, /* NO_COLONS_HERE */
  {6607, "Couldn't add scroll bars to the window."}, /* NO_CONTROL */
  {6608, "The Communications Toolbox is not installed."}, /* NO_CTB */
  {6609, "Couldn't start the Communications Toolbox Connection Manager."}, /* NO_CTBCM */
  {6610, "Couldn't start the Communications Toolbox Resource Manager."}, /* NO_CTBRM */
  {6611, "Couldn't start the Communications Toolbox Utilities."}, /* NO_CTBU */
  {6612, "No connection tools are available."}, /* NO_CTB_TOOLS */
  {6613, "Couldn't get information on that application."}, /* NO_FINFO */
  {6614, "There is not enough memory to read that message."}, /* NO_MESS_BUF */
  {6615, "Couldn't print the document; there was a problem allocating a GrafPort."}, /* NO_PPORT */
  {6616, "Couldn't find your printer."}, /* NO_PRINTER */
  {6617, "Server not responding."}, /* NO_SMTP_SERVER */
  {6618, "No Subject"}, /* NO_SUBJECT */
  {6619, "No Recipient"}, /* NO_TO */
  {6620, "Eudora requires use of Macintosh System 7.0 or later."}, /* OLD_SYSTEM */
  {6701, "Hide Toolbar"}, /* HIDE_TOOLBAR */
  {6702, ""}, /* FCC_PREFIX */
  {6703, "signed"}, /* MPART_SIGNED */
  {6704, "encrypted"}, /* MPART_ENCRYPTED */
  {6705, "protocol"}, /* PROTOCOL */
  {6706, "micalg"}, /* MICALG */
  {6707, "Transfer"}, /* TRANSFER_MNAME */
  {6708, "Fcc    "}, /* FCC_NAME */
  {6709, "5"}, /* SHORT_OPEN_TIMEOUT */
  {6710, "35"}, /* FILT_LIST_MIN */
  {6711, "60"}, /* FILT_REST_MIN */
  {6712, ".. @:,()[]<>\\\""}, /* NICK_BAD_CHAR */
  {6713, "--_---------\""}, /* NICK_REP_CHAR */
  {6714, "There was an error launching that file."}, /* TB_NO_FILE */
  {6715, "Can only insert plain text files."}, /* TEXT_ONLY */
  {6716, "Attach"}, /* ATTACH */
  {6717, "%r %r%p"}, /* IMPORTANCE_FMT */
  {6718, "Selected"}, /* SELECTED */
  {6719, "(continued)"}, /* CONTINUED */
  {6720, "%pAddress Book Entries from %p"}, /* NICK_HEAD_FMT */
  {6801, "Error opening address book file."}, /* OPEN_ALIAS */
  {6802, "Couldn't open mailbox."}, /* OPEN_MBOX */
  {6803, "Error opening your settings file."}, /* OPEN_SETTINGS */
  {6804, "60"}, /* OPEN_TIMEOUT */
  {6805, "Out"}, /* OUT */
  {6806, "60"}, /* PARTIAL_TICKS */
  {6807, "(continuation #%d)\\n"}, /* PART_FMT */
  {6808, "There was an error during printing.  At least one of your documents did not print."}, /* PART_PRINT_FAIL */
  {6809, "Paste failed."}, /* PASTE_FAILED */
  {6810, "Performance monitoring code failed to initialize."}, /* PERFORMANCE */
  {6811, "The \"PH Server\" must be a host name.\\nFOR EXAMPLE, \"ns.eudora.com\"."}, /* PH_EXPL */
  {6812, "ns.eudora.com"}, /* PH_HOST */
  {6813, "Enter query (%p server is %p):"}, /* PH_LABEL */
  {6814, "105"}, /* PH_PORT */
  {6815, "query"}, /* PH_QUERY */
  {6816, "quit"}, /* PH_QUIT */
  {6817, "------------------------------------------------------------\\n"}, /* PH_SEPARATOR */
  {6818, "Please choose the application you use to open text files."}, /* PICK_CREATOR */
  {6819, "Sending TEXT document \"%p\"..."}, /* PLAIN_PROG_FMT */
  {6820, "110"}, /* POP_PORT */
  {6901, "20"}, /* NICK_PRINT_NICK_PER */
  {6902, "Kids today-they just won't listen to their mothers."}, /* KIDS_TODAY */
  {6903, "Header"}, /* JUST_PLAIN_HEADER */
  {6904, "(%p)"}, /* PAREN_STRING */
  {6905, "the mailbox"}, /* MAILBOX_PRINT */
  {6906, "the message"}, /* MESSAGE_PRINT */
  {6907, "and"}, /* AND_PRINT */
  {6908, "as normal"}, /* NOTIFY_NORM */
  {6909, "in report"}, /* NOTIFY_REPORT */
  {6910, "delete it"}, /* DELETE_PRINT */
  {6911, "fetch it"}, /* FETCH_PRINT */
  {6912, "Couldn't drag."}, /* COULDNT_DRAG */
  {6913, "hesiod"}, /* HESIOD */
  {6914, "pobox"}, /* HESIOD_POBOX */
  {6915, "sloc"}, /* HESIOD_SLOC */
  {6916, "pop"}, /* HESIOD_POP3 */
  {6917, ".AthenaMan 1"}, /* HESIOD_DRIVER */
  {6918, "Hesiod lookup (%p,%p)..."}, /* HESIOD_LOOKUP */
  {6919, "0"}, /* OUT_OUT_STYLE */
  {6920, "2"}, /* IN_OUT_STYLE */
  {7001, "Transferring message %d of %d..."}, /* POP_STATUS_FMT */
  {7002, "Window Position"}, /* POSITION_NAME */
  {7003, "There was an error during printing.  Your document did not print."}, /* PRINT_FAILED */
  {7004, "Courier"}, /* PRINT_FONT */
  {7005, "Times"}, /* PRINT_H_FONT */
  {7006, "18"}, /* PRINT_H_MAR */
  {7007, "12"}, /* PRINT_H_SIZE */
  {7008, "Queue"}, /* QUEUE_BUTTON */
  {7009, ">"}, /* QUOTE_PREFIX */
  {7010, "1024"}, /* RCV_BUFFER_SIZE */
  {7011, "Couldn't read address book."}, /* READ_ALIAS */
  {7012, "Couldn't read mailbox."}, /* READ_MBOX */
  {7013, "That text cannot be modified."}, /* READ_ONLY */
  {7014, "Couldn't read your settings."}, /* READ_SETTINGS */
  {7015, "Couldn't read the table of contents."}, /* READ_TOC */
  {7016, "Received:"}, /* RECEIVED_HEAD */
  {7017, "45"}, /* RECV_TIMEOUT */
  {7018, "(by way of %p)"}, /* REDIST_ANNOTATE */
  {7019, "Couldn't rename mailbox."}, /* RENAMING_BOX */
  {7020, "Re:"}, /* REPLY_INTRO */
  {7101, "0"}, /* IN_IN_STYLE */
  {7102, "6000,6000,48000"}, /* URL_COLOR */
  {7103, "4"}, /* URL_STYLE */
  {7104, "<[{(\"'`\\'\\\""}, /* URL_LEFT */
  {7105, ">]})\"'\\'\\'\\\""}, /* URL_RIGHT */
  {7106, "$-_.+!*'(),%;/?:@&=#~^"}, /* URL_IN_OK */
  {7107, "Empty"}, /* EMPTY_BUTTON */
  {7108, "The trash contains %d message%#.  Do you wish to empty it now?"}, /* EMPTY_TRASH_FMT */
  {7109, "Don't %r"}, /* DONT */
  {7110, "Queue"}, /* QUEUE_BTN */
  {7111, "Transfer"}, /* XFER_BTN */
  {7112, "Delete"}, /* NUKE_BTN */
  {7113, "Trash"}, /* TRASH_BTN */
  {7114, "%r & Don't Warn"}, /* AND_DONT_WARN */
  {7115, "Hesiod lookup (%p,%p) returned error %d"}, /* HESIOD_ERR */
  {7116, "Couldn't find your information with Hesiod."}, /* HESIOD_NOTFOUND */
  {7117, "48000,6000,6000"}, /* REPLY_COLOR */
  {7118, "Tink"}, /* REPLY_SOUND */
  {7119, "%p (was %p)"}, /* STATION_SUBJ_FMT */
  {7120, "Spool Folder"}, /* SPOOL_FOLDER */
  {7201, "Printed for"}, /* RETURN_PRINT_INTRO */
  {7202, "Save as:"}, /* SAVEAS_PROMPT */
  {7203, "Couldn't save your address book."}, /* SAVE_ALIAS */
  {7204, "Couldn't save Page Setup"}, /* SAVE_SETUP */
  {7205, "Couldn't build table of contents."}, /* SAVE_SUM */
  {7206, "secret things"}, /* SECRET */
  {7207, "Send"}, /* SEND_BUTTON */
  {7208, "Eudora Settings"}, /* SETTINGS_FILE */
  {7209, "2"}, /* SHORT_TIMEOUT */
  {7210, "Signature"}, /* SIGNATURE */
  {7211, "25"}, /* SMTP_PORT */
  {7212, "Successfully received %p (%d)"}, /* MSG_GOT */
  {7213, "- RD -QFS-T...X?"}, /* STATE_LABELS */
  {7214, "Status:"}, /* STATUS */
  {7215, "PMF"}, /* STEVE_FOLDER */
  {7216, ""}, /* FWD_INTRO */
  {7217, "eudora-info@qualcomm.com"}, /* S_DORNER */
  {7218, "8"}, /* TAB_DISTANCE */
  {7219, "8192"}, /* TCP_BUFFER_SIZE */
  {7220, ".ipp"}, /* TCP_DRIVER */
  {7301, "Couldn't copy an attachment to add it to the new message.  Don't delete the original attachment until the new message is sent."}, /* COPY_ATTACHMENT */
  {7302, "80"}, /* ENRICHED_MAX_WORD */
  {7303, "70"}, /* ENRICHED_SOFT_LINE */
  {7304, "4"}, /* INDENT_DISTANCE */
  {7305, "Scheduling Compactions..."}, /* COMPACTING */
  {7306, "The translators are not working properly."}, /* LAURENCE */
  {7307, "15"}, /* ALIGN_LIMIT */
  {7308, "Couldn't save style information; trying to save as plain text."}, /* CANT_SAVE_RICH */
  {7309, "Monaco"}, /* FIXED_FONT */
  {7310, "<param>%p</param>"}, /* MIME_RICH_PARAM */
  {7311, "Plain"}, /* PLAIN_TEXT_MITEM */
  {7312, "Completely Plain"}, /* PLAIN_ALL_ITEM */
  {7313, "0,0,0"}, /* TEXT_COLOR */
  {7314, "65535,65535,65535"}, /* BACK_COLOR */
  {7315, "none,1,0,0"}, /* REPLY_INSET */
  {7316, "none,1,0,0"}, /* FORWARD_INSET */
  {7317, ","}, /* COMMA_SPACE */
  {7318, "F%d"}, /* FKEY_FMT */
  {7319, "daemon"}, /* DAEMON_NICKNAME */
  {7320, "At ^3 ^1, ^0 wrote:"}, /* ATTR_PASTE_AS_QUOTE */
  {7401, "TCP/IP Error."}, /* TCP_TROUBLE */
  {7402, ".tmp"}, /* TEMP_SUFFIX */
  {7403, "ttxt"}, /* TEXT_CREATOR */
  {7404, "Couldn't read the document."}, /* TEXT_READ */
  {7405, "That document is too large to load."}, /* TEXT_TOO_BIG */
  {7406, "Couldn't save the document."}, /* TEXT_WRITE */
  {7407, "The text of a message or document cannot be more than 32766 bytes."}, /* TE_TOO_MUCH */
  {7408, ".toc"}, /* TOC_SUFFIX */
  {7409, "You have more than 100 subfolders in your Eudora folder.  Use the Finder to remove some folders."}, /* TOO_MANY_LEVELS */
  {7410, "Recipient names must be less than 62 characters."}, /* TO_TOO_LONG */
  {7411, "\\0xd0>"}, /* TRANSFER_PREFIX */
  {7412, "Trash"}, /* TRASH */
  {7413, "telnet %p %d /stream\\n"}, /* TS_CONNECT_FMT */
  {7414, "Undo"}, /* UNDO */
  {7415, "???@???"}, /* UNKNOWN_SENDER */
  {7416, "Untitled"}, /* UNTITLED */
  {7417, "Opening TCP/IP..."}, /* WHO_AM_I */
  {7418, "This operation can't be undone."}, /* WONT_UNDO */
  {7419, "76"}, /* WRAP_SPOT */
  {7420, "There was an error saving the mail."}, /* WRITE_MBOX */
  {7501, "587"}, /* SUBMISSION_PORT */
  {7502, "Find"}, /* FIND_FIND */
  {7503, "Error %d sending attachment \"%p\"."}, /* ATTACH_MESS_ERR */
  {7504, "Do you want to use the system keychain?\nEudora can store your passwords safely in the system keychain.  You will have to enter your passwords one more time, then never again.\n\nNo\nYes-"}, /* KEYCHAIN_WANNA */
  {7505, "%d/%d"}, /* BOX_SIZE_SELECT_SRCH_FMT */
  {7506, "18"}, /* BOX_SIZE_SELECT_EXTRA */
  {7507, "Whole word"}, /* FIND_WORD */
  {7508, "Backwards"}, /* FIND_BACKWARD */
  {7509, "Match case"}, /* FIND_CASE */
  {7510, "The Ad window can't be positioned properly.\nThe usual positions for the ad window seem to be covered by something.  Please drag the ad window someplace where it will not be covered.  Thanks!\n\n\nYes-"}, /* SORRY_AD_COVERED */
  {7511, "Options:"}, /* FIND_OPTIONS */
  {7512, "Find:"}, /* FIND_FIND_LABEL */
  {7513, "Please choose a mailbox from the Transfer menu."}, /* CHOOSE_MBOX_TRANSFER */
  {7514, "Please choose a mailbox from the Mailbox menu."}, /* CHOOSE_MBOX_MAILBOX */
  {7515, "Search \"%p\""}, /* SEARCH_SOMETHING_FMT */
  {7516, "Translation failed; cannot open this item."}, /* ETL_TRANS_FAILED */
  {7517, "Couldn't create signature."}, /* CREATE_SIG */
  {7518, "Translator initialization failed; consult translator documentation."}, /* ETL_CANT_INIT */
  {7519, "Translator versions don't match; may need to upgrade Eudora or translator."}, /* ETL_BAD_VERSION */
  {7520, "%p: %r %p\\n\\n{%d.%d}"}, /* ETL_ERROR_FMT */
  {7601, "Couldn't save your settings."}, /* WRITE_SETTINGS */
  {7602, "Couldn't write the table of contents."}, /* WRITE_TOC */
  {7603, "\\n%d, {%d:%d}"}, /* WU_FMT */
  {7604, "2048"}, /* VOLUME_MARGIN */
  {7605, "Original Message,begin forwarded text"}, /* CONCON_FORWARD_ON */
  {7606, "54"}, /* PRINT_LEFT_MAR */
  {7607, "36"}, /* PRINT_RIGHT_MAR */
  {7608, "It's a very bad idea to use names beginning with a period."}, /* LEADING_PERIOD */
  {7609, "DialupEudora"}, /* CTB_ME */
  {7610, "[%i]"}, /* TCP_ME */
  {7611, "(|(cn=*^0*))"}, /* LDAP_SEARCH_FILTER */
  {7612, "80"}, /* DEF_MWIDTH */
  {7613, "Attachment corrupt; wrong number of characters on a line."}, /* UU_BAD_LENGTH */
  {7614, "65"}, /* HEX_SIZE_PERCENT */
  {7615, "20000"}, /* HEX_SIZE_THRESH */
  {7616, "40"}, /* BIG_MESSAGE */
  {7617, "\\n\\nWARNING: The remainder of this %K message has not been transferred.  Turn off the \"Skip big messages\" option and check mail again to get the whole thing."}, /* BIG_MESSAGE_MSG */
  {7618, "BREAK [%d ticks]"}, /* BREAKING */
  {7619, "250000"}, /* SPLIT_THRESH */
  {7620, "%d/%K/%K"}, /* BOX_SIZE_FMT */
  {7701, "Could not add translator request."}, /* ETL_CANT_ADD_TRANS */
  {7702, "charset"}, /* MIME_CHARSET */
  {7703, "content-transfer-encoding"}, /* MIME_CTE */
  {7704, "Original-Message-ID: %p%p"}, /* MDN_ORIG_MID */
  {7705, "144"}, /* AD_WINDOW_SIZE_GUESS_X */
  {7706, "Final-Recipient: rfc822; %p%p"}, /* MDN_FINAL_RECIP */
  {7707, "displayed"}, /* MDN_DISPLAYED */
  {7708, "displayed"}, /* MDN_DISPLAYED_LOCAL */
  {7709, "%r/%r"}, /* THING_SLASH_THING */
  {7710, "Error %d with graphic file \"%p\"."}, /* GRAPHIC_FILE_ERR */
  {7711, "5"}, /* CLOSE_ENOUGH_PERCENT */
  {7712, "1"}, /* VERY_SMALL_PERCENT */
  {7713, "15"}, /* MIN_WIN_HI */
  {7714, "15"}, /* NARROW_PERCENT */
  {7715, "Disposition: %r/%r; %r%p"}, /* MDN_DISPOSITION */
  {7716, "report"}, /* MIME_REPORT */
  {7717, "disposition-notification"}, /* MDN_DISPO_NOTIFY */
  {7718, "Your message of %p regarding ``%p''\\015has been %r by %p.\\015\\015"}, /* MDN_DESCRIP */
  {7719, ";report-type=disposition-notification\\015"}, /* MDN_REPORT_PARAM */
  {7720, "Notification for ``%p''"}, /* MDN_SUBJECT */
  {7801, "128"}, /* BOX_SIZE_SIZE */
  {7802, "You may change the subject that appears in the mailbox window and window title by editing this text."}, /* SUB_EDIT_HELP */
  {7803, "This is the text of the message you were sent; you may only change it if you select the pencil icon above."}, /* MESS_HELP */
  {7804, "9"}, /* OLD_BOX_SIZE_FONT_SIZE */
  {7805, ":;,@<>()[]\\\""}, /* ALIAS_VERBOTEN */
  {7806, "The characters \":;@<>()[]\\\"\\',\" are not allowed in nicknames (they can go in the Address(es) section, just not the nickname itself."}, /* WARN_VERBOTEN */
  {7807, "You have new mail."}, /* NEW_MAIL */
  {7808, "(%c%p%p)"}, /* LDAP_TERM_COMBINER */
  {7809, "The attachment is an unknown AppleSingle version:"}, /* UU_BAD_VERSION */
  {7810, "The attachment has an invalid map count:"}, /* UU_INVALID_MAP */
  {7811, "The attachment contained extra information, map:"}, /* UU_SKIP_MAP_INFO */
  {7812, "The decoder has entered an invalid state, the attachment may be damaged."}, /* UU_INVALID_STATE */
  {7813, "Recipient List Suppressed:;"}, /* BCC_ONLY */
  {7814, "Picking up message:"}, /* UUPC_COPY */
  {7815, "UUCP misconfigured; must be !mymac!spoolvol:spooldir:!username!0000"}, /* UUPC_WRONG_SMTP */
  {7816, "D.%p0%p"}, /* UUPC_DMYMAC */
  {7817, ""}, /* FWD_TRAIL */
  {7818, ""}, /* REP_SEND_ATTR */
  {7819, "X.%p0%p"}, /* UUPC_XMYMAC */
  {7820, "10"}, /* WAIT_FOR_START_AE */
  {7901, "The sender has requested notification that you have seen this."}, /* MDN_REQUEST */
  {7902, "Cannot register now; internal error."}, /* CANT_REGISTER */
  {7903, "Cannot translate."}, /* ETL_CANT_TRANSLATE */
  {7904, "One of the requested translators can't be found; the mail will not be sent."}, /* ETL_CANT_FIND_TRANS */
  {7905, ""}, /* QUOTH */
  {7906, ""}, /* UNQUOTH */
  {7907, "Tink"}, /* NOTIFY_SOUND */
  {7908, "Last used %p"}, /* FILT_DATE_LABEL */
  {7909, "Copy Without Styles"}, /* COPY_PLAIN_ITEXT */
  {7910, "Paste Without Styles"}, /* PASTE_PLAIN_ITEXT */
  {7911, "Copy Without Styles & Unwrap"}, /* COPY_UNWRAP_PLAIN_ITEXT */
  {7912, "Paste"}, /* PASTE_ITEXT */
  {7913, "Cut Without Styles"}, /* CUT_PLAIN_ITEXT */
  {7914, "Cut"}, /* CUT_ITEXT */
  {7915, "Cut & Unwrap"}, /* CUT_UNWRAP_ITEXT */
  {7916, "Cut Without Styles & Unwrap"}, /* CUT_UNWRAP_PLAIN_ITEXT */
  {7917, "Couldn't read the signature file for that message."}, /* CANT_READ_SIG */
  {7918, "64"}, /* TB_H_DESK_MARGIN */
  {7919, "48"}, /* TB_V_DESK_MARGIN */
  {7920, "%r: %r/%p; %r=\\\"%p\\\"%p"}, /* MIME_TEXT_SUBTYPE_FMT */
  {8001, "U %p %p\\n"}, /* UUPC_U_CMD */
  {8002, "F %p\\n"}, /* UUPC_F_CMD */
  {8003, "I %p\\n"}, /* UUPC_I_CMD */
  {8004, "C rmail"}, /* UUPC_C_CMD */
  {8005, "remote from %p\\n"}, /* UUPC_REMOTE */
  {8006, "Can only do UUPC send if you do UUPC receive."}, /* UUPC_SECURE */
  {8007, "%d of the %d summar%* in the old table of contents used; %d new summar%* created."}, /* SALV_REPORT */
  {8008, "0"}, /* DESK_LEFT_STRIP */
  {8009, "72"}, /* DESK_RIGHT_STRIP */
  {8010, "0"}, /* DESK_BOTTOM_STRIP */
  {8011, "0"}, /* DESK_TOP_STRIP */
  {8012, ""}, /* XSENDER_FMT */
  {8013, "\\nThe response was too long; the remainder has been lost."}, /* PH_TOO_BIG */
  {8014, "me"}, /* ME */
  {8015, "Reply"}, /* REPLY */
  {8016, "Reply to All"}, /* REPLY_ALL */
  {8017, "End Original Message,end forwarded text"}, /* CONCON_FORWARD_OFF */
  {8018, "Sent: %s"}, /* LOG_SENT */
  {8019, "Rcvd: %s"}, /* LOG_GOT */
  {8020, "dialup"}, /* DIALUP */
  {8101, "%r: %r/%p%p%p"}, /* MIME_TEXTNOTPLAIN */
  {8102, ".,)]"}, /* URL_TRAIL_IGNORE */
  {8103, "alternative"}, /* MIME_ALTERNATIVE */
  {8104, "passwd"}, /* HESIOD_PASSWD */
  {8105, "Eudora does not support enough of the Translation Services API for this."}, /* ETL_IM_STUPID */
  {8106, "30"}, /* PICT_SPOOL_SIZE */
  {8107, "90"}, /* PICT_HIDE_SIZE */
  {8108, "No Stationery"}, /* NO_STATIONERY */
  {8109, "None Installed"}, /* NO_TRANSLATORS */
  {8110, "You may change the contents of this message.  Click the pencil again to save the contents."}, /* MESS_WRITE_HELP */
  {8111, "Click this icon to allow the contents of the message to be edited."}, /* NO_MESS_WRITE_HELP */
  {8112, "<%r://%p/>"}, /* URL_FMT */
  {8113, "This operation would require %d K more memory than is readily available.  Before trying again, please close some windows or quit Eudora and use the Finder's \"Get Info\" command to increase Eudora's memory size."}, /* TOO_MUCH_MEMORY */
  {8114, "Copying"}, /* COPYING */
  {8115, "152"}, /* AD_WINDOW_SIZE_GUESS_Y */
  {8116, "Eudora couldn't find any files to import.\nWould you like to try again using using a specific importer?\n\nNo-\nYes-"}, /* IMPORT_MESSAGE_TRY_AGAIN */
  {8117, "Another program is using your settings file.  Might you be running another copy of Eudora?"}, /* SETTINGS_BUSY */
  {8118, "30"}, /* PH_BG_IDLE */
  {8119, "300"}, /* PH_FG_IDLE */
  {8120, "Insert System Configuration"}, /* INSERT_CONFIGURATION */
  {8201, "Invalid date format."}, /* DATE_ERROR */
  {8202, "Dates in the past are not allowed."}, /* THE_PAST */
  {8203, "12"}, /* NEVER_WARN */
  {8204, "20"}, /* BIG_MESSAGE_FRAGMENT */
  {8205, "At ^3 ^1, ^0 wrote:"}, /* ATTRIBUTION */
  {8206, "Notes:"}, /* ALIAS_N_LABEL */
  {8207, "0"}, /* LOCAL_PORT */
  {8208, "That nickname already exists.\n\"%p\" already appears in the \"%p\" address book.  Nicknames within the same address book must be unique.\n\n\nOK-"}, /* NICK_IN_USE */
  {8209, "Eudora Log"}, /* LOG_NAME */
  {8210, "Old Log"}, /* OLD_LOG */
  {8211, "Succeeded."}, /* LOG_SUCCEEDED */
  {8212, "Failed (%d)."}, /* LOG_FAILED */
  {8213, "Sending %p."}, /* SENDING */
  {8214, "Dismissed with %d."}, /* ALERT_DISMISSED_ITEM */
  {8215, "Couldn't make address book entry.No address found."}, /* NO_ADDRESSES */
  {8216, "106"}, /* PW_PORT */
  {8217, "enter the new"}, /* NEW */
  {8218, "Couldn't change your password."}, /* PW_ERROR */
  {8219, "Those two passwords didn't match."}, /* PW_MISMATCH */
  {8220, "verify the new"}, /* VERIFY_NEW */
  {8301, "3"}, /* PH_LIVE_MIN_CHARS */
  {8302, "60"}, /* PH_LIVE_MIN_TICKS */
  {8303, "return"}, /* PH_RETURN_KEYWORD */
  {8304, "Eudora Stuff"}, /* STUFF_FOLDER */
  {8305, "Couldn't find Eudora's text editor component."}, /* NO_PETE */
  {8306, "The text editor component you have installed is the wrong version."}, /* BAD_PETE */
  {8307, "pp%^1"}, /* ETL_ICON_HELP_FMT */
  {8308, "%p%p"}, /* ATTR_TIME_FMT */
  {8309, "New... and Other... from subfolders cannot be added to the toolbar."}, /* CANT_ADD_NESTED */
  {8310, "Click the \"Notify Sender\" button to send a receipt to the sender telling him you received the message.  Hold down the option key and click to get rid of the request without sending the notice."}, /* MESS_NOTIFY_HELP */
  {8311, "Filter \"%p\" matches \"%p\""}, /* FILT_LOG_FMT */
  {8312, "Eudora requires the \"Component Manager\"; this is on your original disks, and is also part of QuickTime and Macintosh Easy Open."}, /* NEED_COMPONENT_MGR */
  {8313, "Any Addressee"}, /* FILTER_ADDRESSEE_OLD */
  {8314, "x-mac-text"}, /* MIME_MACTEXT */
  {8315, "Reading filters..."}, /* READING_FILTERS */
  {8316, "%p%p"}, /* SPECIAL_KEY_FMT */
  {8317, "New Message With"}, /* NEW_MESSAGE_WITH */
  {8318, "This file provides help texts to Eudora.Put it in the same folder as the Eudora application.  The help will appear in the Help menu or in the menu with the help icon on the rightish side of your menu bar."}, /* OPEN_HELP_ERR */
  {8319, "This file is a resource plug-in for Eudora.  Put it in your Eudora Folder or the system Preferences folder or the same folder as the Eudora application."}, /* OPEN_PLUG_ERR */
  {8320, "About Message Plug-ins..."}, /* ABOUT_TRANSLATORS */
  {8401, "enter the"}, /* ENTER */
  {8402, "80"}, /* WRAP_THRESH */
  {8403, "79"}, /* FINGER_PORT */
  {8404, "%r %d %r"}, /* PRIORITY_FMT */
  {8405, "Use this menu to set the message's priority (priorities are informational only)."}, /* PRIOR_MENU_HELP */
  {8406, "25"}, /* COMPACT_WASTE_PER */
  {8407, "1"}, /* COMPACT_FREE_PER */
  {8408, "finger"}, /* FINGER */
  {8409, "Default"}, /* DEFAULT */
  {8410, ">"}, /* FWD_QUOTE */
  {8411, "^1 <^0>"}, /* ADD_REALNAME */
  {8412, "%p %d (%d)"}, /* START_POP_LOG */
  {8413, "%p %d"}, /* START_SEND_LOG */
  {8414, "It looks like a previous compaction was attempted and failed.  Get help."}, /* SQUISH_LEFTOVERS */
  {8415, "16"}, /* WDS_LIMIT */
  {8416, "Couldn't open a connection to ^0, trying ^1...\\n"}, /* PH_FAIL */
  {8417, "%p seems willing to talk to us.\\n"}, /* PH_SUCCEED */
  {8418, "No one"}, /* NOONE */
  {8419, "You haven't sent all those messages; %r them anyway?\\n(They won't ever be sent.)"}, /* UNSENT_WARNING */
  {8420, "trash"}, /* TRASH_VERB */
  {8501, "bulk"}, /* BULK */
  {8502, "Cannot read main address book."}, /* NO_MAIN_NICK */
  {8503, "The window has internal errors and will be closed."}, /* DOC_DAMAGED_ERR */
  {8504, "Eudora Folder"}, /* USA_EUDORA_FOLDER */
  {8505, "Eudora Nicknames"}, /* USA_EUDORA_NICKNAMES */
  {8506, "In"}, /* USA_IN */
  {8507, "Out"}, /* USA_OUT */
  {8508, "Trash"}, /* USA_TRASH */
  {8509, "Attachments Folder"}, /* USA_ATTACHMENTS_FOLDER */
  {8510, "Signature Folder"}, /* USA_SIG_FOLDER */
  {8511, "Stationery Folder"}, /* USA_STATION_FOLDER */
  {8512, "Spool Folder"}, /* USA_SPOOL_FOLDER */
  {8513, "Nicknames Folder"}, /* USA_NICKNAME_FOLDER */
  {8514, "Standard"}, /* USA_SIGNATURE */
  {8515, "Alternate"}, /* USA_ALTERNATE */
  {8516, "Eudora Settings"}, /* USA_SETTINGS */
  {8517, "Default"}, /* USA_DEFAULT */
  {8518, "No Quick Recipients"}, /* NO_RECIPS */
  {8519, "<l%p@%p>"}, /* LIGHT_MSGID_FMT */
  {8601, "You haven't read all those messages; %r them anyway?"}, /* UNREAD_WARNING */
  {8602, "transfer"}, /* XFER_VERB */
  {8603, "Temporary file found. It has been put in the mailbox menus, but there may be a problem."}, /* TEMP_WARNING */
  {8604, "^0 ^1^2"}, /* DATE_SUM_FMT */
  {8605, "Waiting..."}, /* WAITING */
  {8606, "Expecting: \"%p\""}, /* LOG_EXPECT */
  {8607, "Found expected string."}, /* LOG_FOUND */
  {8608, "Didn't find expected string."}, /* LOG_NOTFOUND */
  {8609, "Your navigation script requires a dialup username.  Use the \"Personal Information\" section of the \"Settings...\" dialog to set one."}, /* NO_AUXUSR */
  {8610, "^1, ^2, ^3"}, /* MESS_TITLE_PLUG */
  {8611, "The attachment is not in proper BinHex format."}, /* BAD_HEXBIN_FORMAT */
  {8612, "Couldn't initialize AppleEvents."}, /* INSTALL_AE */
  {8613, ""}, /* PH_RETURN */
  {8614, "7"}, /* SCROLL_THROTTLE */
  {8615, "Please set the location and timezone of your Macintosh with the \"Map\" and/or \"Date & Time\" control panels.  Otherwise Eudora can't handle Date: headers properly."}, /* USE_MAP */
  {8616, "32"}, /* MIME_ATTR_MAX */
  {8617, "128"}, /* MIME_VAL_MAX */
  {8618, "1.0"}, /* MIME_VERSION */
  {8619, "multipart"}, /* MIME_MULTIPART */
  {8620, "text"}, /* MIME_TEXT */
  {8801, "message"}, /* MIME_MESSAGE */
  {8802, "base64"}, /* MIME_BASE64 */
  {8803, "application"}, /* MIME_APPLICATION */
  {8804, "applefile"}, /* MIME_APPLEFILE */
  {8805, "appledouble"}, /* MIME_APPLEDOUBLE */
  {8806, "digest"}, /* MIME_DIGEST */
  {8807, "mac-binhex-40"}, /* MIME_BINHEX */
  {8808, "iso-8859-1"}, /* MIME_ISO_LATIN1 */
  {8809, "partial"}, /* MIME_PARTIAL */
  {8810, "One or more attachments were corrupt."}, /* BAD_HEX_MSG */
  {8811, "%d encoding error%# were found."}, /* BAD_ENC_MSG */
  {8812, "name"}, /* NAME */
  {8813, "---Temp---"}, /* SINGLE_TEMP */
  {8814, "quoted-printable"}, /* MIME_QP */
  {8815, "%r: %r%p"}, /* MIME_V_FMT */
  {8816, "%r: %r/%r; %r=\\\"%p\\\"%p"}, /* MIME_MP_FMT */
  {8817, ".ps"}, /* PS_SUFFIX */
  {8818, "============_%d==_%p%r"}, /* MIME_BOUND1_FMT */
  {8819, "mixed"}, /* MIME_MIXED */
  {8820, "macintosh"}, /* MIME_MAC */
  {8901, "50"}, /* ALIAS_NICK_LIST_PRCT */
  {8902, "Personal Nicknames"}, /* USA_PERSONAL_ALIAS_FILE */
  {8903, "Recipient List"}, /* ALIAS_ON_RECIPIENT_LIST */
  {8904, "untitled"}, /* ALIAS_UNTITLED_NICK */
  {8905, "Your registration has been placed in your Out mailbox, and will be sent next time you send queued messages."}, /* REGISTRATION_QUEUED */
  {8906, "Your registration has been sent.  You should receive a reply shortly."}, /* REGISTRATION_SENT */
  {8907, "There is insufficient memory to create a new nickname. The nicknames window is going to be closed."}, /* ALIAS_NEW_NICK_ERR_QUIT */
  {8908, "There is insufficient memory to create a new nickname."}, /* ALIAS_NEW_NICK_ERR */
  {8909, "There is insufficient memory to modify the current nickname."}, /* ALIAS_REPLACE_NICK_ERR */
  {8910, "There is insufficient memory to manipulate current nickname."}, /* ALIAS_GET_NICK_DATA_ERR */
  {8911, "There is insufficient memory to sort the nickname list."}, /* ALIAS_SORT_ERR */
  {8912, "There is insufficient memory to create a new nickname file."}, /* ALIAS_NEW_NICK_FILE_ERR */
  {8913, "The address book file \\\"Eudora Nicknames\\\" cannot be removed."}, /* ALIAS_REMOVE_DEFAULT_NICK_FILE */
  {8914, ""}, /* UNUSED_SITE_TO_VISIT */
  {8915, "(PAY Only)"}, /* PRO_ONLY_FEATURE */
  {8916, "In"}, /* FILE_ALIAS_IN */
  {8917, "Out"}, /* FILE_ALIAS_OUT */
  {8918, "Eudora Folder"}, /* FILE_ALIAS_EUDORA_FOLDER */
  {8919, "Trash"}, /* FILE_ALIAS_TRASH */
  {8920, "Standard"}, /* FILE_ALIAS_STANDARD */
  {9001, "; charset=\\\"%p\\\""}, /* MIME_CSET */
  {9002, "%r: %r/%r%p%p"}, /* MIME_TEXTPLAIN */
  {9003, "990"}, /* MAX_SMTP_LINE */
  {9004, "%r: %p/%p; %r=\\\"%p\\\"%p"}, /* MIME_CT_PFMT */
  {9005, "%r: %p%p"}, /* MIME_P_FMT */
  {9006, "octet-stream"}, /* MIME_OCTET_STREAM */
  {9007, "binary"}, /* MIME_BINARY */
  {9008, "Some attachments could not be found and were not included in the new message."}, /* ATTACH_REMOVED */
  {9009, "param"}, /* MIMERICH_COMMENT */
  {9010, "nofill"}, /* MIMERICH_NOFILL */
  {9011, "-WorkGroup"}, /* GROUP_DONT_HIDE */
  {9012, "enriched"}, /* MIME_RICHTEXT */
  {9013, "Couldn't send message; %pserver says \"%s\"."}, /* BAD_XMIT_ERR_TEXT */
  {9014, "plain"}, /* MIME_PLAIN */
  {9015, ""}, /* LDAP_CN_SEARCH_FILTER */
  {9016, "<%r>"}, /* MIME_RICH_ON */
  {9017, "</%r>"}, /* MIME_RICH_OFF */
  {9018, "us-ascii"}, /* MIME_USASCII */
  {9019, "Eudora Filters"}, /* FILTERS_NAME */
  {9020, "../\\\"\\\\"}, /* OTHER_FN_BAD */
  {9101, "Alternate"}, /* FILE_ALIAS_ALTERNATE */
  {9102, "Eudora Nicknames"}, /* FILE_ALIAS_NICKNAMES */
  {9103, "Your Eudora Filters file appears to have been created with with Eudora in Paid or Sponsored Mode.  These filter actions are not available in Light Mode so your filters may not be fully functional."}, /* PRO_FILT_WARNING */
  {9104, "12"}, /* PREVIEW_HEADER_HIDE */
  {9105, "Photo Folder"}, /* USA_PHOTO_FOLDER */
  {9106, "Error %d saving information for personality \"%p\""}, /* PERS_SAVE_ERR */
  {9107, "There is not enough memory available to use Quickdraw GX.  Plain old boring printing features will be enabled instead."}, /* GX_MEMORY_INIT_ERR */
  {9108, "Could not initialize Quickdraw GX.  Plain old boring printing features will be enabled instead."}, /* GX_INIT_ERR */
  {9109, "A QuickDraw GX memory related error has occurred.  Try giving Eudora more memory."}, /* GX_MEMORY_ERR */
  {9110, "Could not update the page setup record for GX.  Your printer may be confused."}, /* GX_UPDATE_ERR */
  {9111, "selected messages"}, /* GX_PRINT_SELECTION */
  {9112, "Cannot have more than 99 personalities."}, /* YOU_ARE_PSYCHO */
  {9113, "Dominant"}, /* DOMINANT */
  {9114, "You already have a personality called that."}, /* USED_PERSONALITY */
  {9115, "Queue For Delivery"}, /* QUEUE_M_ITEM */
  {9116, "Send Immediately"}, /* SEND_M_ITEM */
  {9117, "5"}, /* PERS_CHECK_SLOP */
  {9118, "An error occurred while initializing Open Transport.  MacTCP will be used instead."}, /* OT_INIT_ERR */
  {9119, "html"}, /* HTML_SUFFIX */
  {9120, "http"}, /* HTTP */
  {9201, "==='="}, /* OTHER_FN_REP */
  {9202, "..:/"}, /* MAC_FN_BAD */
  {9203, "==-/"}, /* MAC_FN_REP */
  {9204, "No connections can be made, because you have set the connection method to \"Offline.\""}, /* OFFLINE */
  {9205, "Signature Folder"}, /* SIG_FOLDER */
  {9206, "Eudora cannot move mailboxes from one volume to another.  Please use the Finder."}, /* CANT_VOL_MOVE */
  {9207, "Nicknames Folder"}, /* NICK_FOLDER */
  {9208, "PostScript"}, /* POSTSCRIPT */
  {9209, "%!"}, /* PS_MAGIC */
  {9210, "Queue Message"}, /* OLD_QUEUE_M_ITEM */
  {9211, "Send Message Now"}, /* OLD_SEND_M_ITEM */
  {9212, "There is insufficient disk space."}, /* NOT_ENOUGH_ROOM */
  {9213, ", \\t\\r\\n()[]:<"}, /* RFC1342_DELIMS */
  {9214, "=?%p?Q?%p?="}, /* RFC1342_FMT */
  {9215, "Having trouble building AppleEvents."}, /* AE_TROUBLE */
  {9216, "Couldn't start that application."}, /* COULDNT_LAUNCH */
  {9217, "mac-binhex40"}, /* MIME_BINHEX2 */
  {9218, "%%%p"}, /* DOUBLE_RFORK_FMT */
  {9219, "rfc822"}, /* MIME_RFC822 */
  {9220, "%p CMSetConfig"}, /* CTB_SETCONFIG_FMT */
  {9301, "Couldn't send the html to your browser."}, /* SAVING_HTML */
  {9302, "Eudora Items"}, /* ITEMS_FOLDER */
  {9303, "Plugins"}, /* PLUGINS_FOLDER */
  {9304, "An Open Transport related error has occurred."}, /* OT_UNKNOWN_ERR */
  {9305, "600"}, /* AE_SHORT_TIMEOUT_TICKS */
  {9306, "Checking mail for %p..."}, /* PERS_CHECKING_MAIL */
  {9307, "Sending mail for %p..."}, /* PERS_SENDING_MAIL */
  {9308, "4"}, /* TOOLBAR_EXTRA_PIXELS */
  {9309, "4"}, /* TOOLBAR_EXTRA_COUNT */
  {9310, "-1"}, /* TOOLBAR_SEP_PIXELS */
  {9311, "26"}, /* TXT_FMT_BAR_HEIGHT */
  {9312, "3"}, /* TXT_FMT_BAR_FONT_FAM_ID */
  {9313, "9"}, /* TXT_FMT_BAR_FONT_SIZE */
  {9314, ""}, /* TXT_FMT_BAR_POPUP_CHECK_MARK */
  {9315, "35"}, /* TXT_FMT_BAR_FONT_POPUP_MAX_WIDTH */
  {9316, "35"}, /* TXT_FMT_BAR_SIZE_POPUP_MAX_WIDTH */
  {9317, "40"}, /* TXT_FMT_BAR_COLOR_POPUP_MAX_WIDTH */
  {9318, "Translating..."}, /* TRANSLATING */
  {9319, "Really delete personality \"%p\"?  The deletion cannot be undone."}, /* PERS_DELETE */
  {9320, "An error occurred while initializing Open Transport Internet Services."}, /* OT_INET_SVCS_ERR */
  {9401, "CMSetConfig"}, /* SETCONFIG */
  {9402, "Content-Disposition"}, /* CONTENT_DISPOSITION */
  {9403, "attachment"}, /* ATTACHMENT */
  {9404, "%r: %r; %r=\\\"%p\\\"%p"}, /* MIME_CD_FMT */
  {9405, "header-set"}, /* MIME_HEADERSET */
  {9406, "appledouble"}, /* MIME_DOUBLE_SENDSUB */
  {9407, "This beta version of Eudora has expired.\nPlease visit our website to get a newer beta or the final version.\n\nQuit-\nVisit & Quit-"}, /* BETA_EXPIRED */
  {9408, "Selection is not a properly formatted mailbox."}, /* NOT_MAILBOX */
  {9409, "; %r=\\\"%p\\\"%p"}, /* MIME_CT_ANNOTATE */
  {9410, "Memory error; couldn't add the message summary to the table of contents."}, /* SAVE_SUM_ERR */
  {9411, "16000"}, /* TEXT_QP_TASTE */
  {9412, "2"}, /* SCROLL_ARROW_THROTTLE */
  {9413, "60"}, /* IDLE_SECS */
  {9414, "10"}, /* MAX_QUOTE */
  {9415, "18000"}, /* AE_TIMEOUT_TICKS */
  {9416, "x-uuencode"}, /* X_UUENCODE */
  {9417, "; %r=%p%p"}, /* NQ_ANNOTATE */
  {9418, "8bit"}, /* MIME_8BIT */
  {9419, "<v%p@%p>"}, /* MSGID_FMT */
  {9420, "Couldn't save information for Leave Mail On Server.  Some messages may be fetched again."}, /* SAVE_POPD */
  {9501, "There was not enough memory to start a new connection."}, /* OT_CON_MEM_ERR */
  {9502, "Establishing PPP connection ..."}, /* OT_PPP_CONNECT */
  {9503, "An error occurred while establishing the connection."}, /* OT_DIALUP_CONNECT_ERR */
  {9504, "Closing current connection ..."}, /* OT_PPP_DISCONNECT */
  {9505, "body"}, /* URL_BODY */
  {9506, "Unable to determine the current state of the PPP connection."}, /* OT_PPP_STATE_ERR */
  {9507, "Connecting ..."}, /* OT_PPP_CONNECT_MESSAGE */
  {9508, "Unable to open your TCP/IP preferences file."}, /* OT_TCPIP_PREF_ERR */
  {9509, ""}, /* PH_NEWLINE */
  {9510, ""}, /* UNUSEND_WAS_DOC_DAMAGED_FMT */
  {9511, ""}, /* PH_SERVER_SERVER */
  {9512, "Couldn't setup the script manager."}, /* INTERNATIONAL_FAILURE */
  {9513, "Redialing ..."}, /* OT_PPP_REDIALING */
  {9514, "en"}, /* LOCALIZED_VERSION_LANG */
  {9515, "Couldn't create your Personal Nicknames file."}, /* CREATING_PERSONAL_ALIAS */
  {9516, "Couldn't create stationery file."}, /* CREATE_STA */
  {9517, "There was an error saving the stationery."}, /* WRITE_STA */
  {9518, "The Thread Manager is not installed in this version of your OS. For faster performance, you may want to upgrade."}, /* THREAD_MANAGER_WARNING */
  {9519, "Eudora is busy sending some of these messages. You cannot modify them until they have been sent."}, /* SENDING_WARNING */
  {9520, "In.temp"}, /* IN_TEMP */
  {9601, "1"}, /* POP_SCAN_INTERVAL */
  {9602, "4"}, /* UNREAD_STYLE */
  {9603, "5"}, /* UNREAD_LIMIT */
  {9604, "response"}, /* ANSWER_RESPONSE */
  {9605, "54"}, /* SETTINGS_CELL_SIZE */
  {9606, "Will cancel on: \"%p\""}, /* LOG_HANGUP */
  {9607, "Found cancel string."}, /* LOG_HANGUP_FOUND */
  {9608, "(Unverified)"}, /* UNVERIFIED */
  {9609, "\\n\\nMust be between %d and %d."}, /* PREFLIMIT_EXPLAIN */
  {9610, "Invalid setting:\\n"}, /* PREFERROR_INTRO */
  {9611, ".Kerberos"}, /* KERBEROS_DRIVER */
  {9612, "Cannot communicate with Kerberos."}, /* NO_KERBEROS */
  {9613, "kpop"}, /* KERBEROS_POP_SERVICE */
  {9614, "^0.^1@^2"}, /* KERBEROS_SERVICE_FMT */
  {9615, "justjunk"}, /* KERBEROS_VERSION */
  {9616, "1400"}, /* KERBEROS_BSIZE */
  {9617, "0"}, /* KERBEROS_CHECKSUM */
  {9618, "Getting ticket for \"%p\"..."}, /* KERBEROS_TICK_FMT */
  {9619, "."}, /* KERBEROS_ESCAPES */
  {9620, "meaningless"}, /* KERBEROS_FAKE_PASS */
  {9701, "Out.temp"}, /* OUT_TEMP */
  {9702, "%r, %d %r %d %d%d:%d%d:%d%d %c%d%d%d%d"}, /* R822_DATE_FMT */
  {9703, "Really rename this personality? If you rename, mail currently stored with this personality will belong to the dominant personality."}, /* PERS_RENAME */
  {9704, "Mail Folder"}, /* MAIL_FOLDER_NAME */
  {9705, "An error occurred moving one of your mailboxes to the mail subfolder.  You may need to move this one with the Finder."}, /* MOVING_MAILBOXES */
  {9706, "Only folders on the same volume as your attachments folder may be used in a filter action."}, /* SAME_VOLUME_DUMMY */
  {9707, "30"}, /* OLD_FILTER */
  {9708, "3"}, /* NICK_POPUP_FONT_FAM_ID */
  {9709, "9"}, /* NICK_POPUP_FONT_SIZE */
  {9710, "IDDB"}, /* IDDB_FILE */
  {9711, "MailDB"}, /* MAILDB_FILE */
  {9712, "TOC"}, /* TOC_FILE */
  {9713, "Info"}, /* INFO_FILE */
  {9714, "10"}, /* SHORT_MODAL_IDLE_SECS */
  {9715, "Messages to deliver:"}, /* TP_LEFT_TO_DELIVER */
  {9717, "Messages remaining to move:"}, /* LEFT_TO_MOVE */
  {9718, "An error occurred while opening the Make Filter dialog."}, /* MAKEFILTER_ERR */
  {9719, "An OT Library is missing."}, /* OT_MISSING_LIBRARY */
  {9720, "Related"}, /* MIME_RELATED */
  {9801, "10"}, /* OLD_SETTINGS_FONT_SIZE */
  {9802, "Checking Mail..."}, /* CHECKING_MAIL */
  {9803, "Sending Mail..."}, /* SENDING_MAIL */
  {9804, "Moving Messages..."}, /* MOVING_MESSAGES */
  {9805, "Finding..."}, /* FINDING */
  {9806, "Messages remaining to transfer:"}, /* LEFT_TO_TRANSFER */
  {9807, "Messages remaining to process:"}, /* LEFT_TO_PROCESS */
  {9808, "Changing Password..."}, /* CHANGING_PW */
  {9809, "Attachments Folder"}, /* ATTACH_FOLDER */
  {9810, "Preparing to transfer..."}, /* PREPARING_CONNECTION */
  {9811, "Cleaning up..."}, /* CLEANUP_CONNECTION */
  {9812, "This message will be downloaded in full next time you check mail.  Click here to prevent the download."}, /* MESS_FETCH_HELP */
  {9813, "The bulk of this message was skipped.  Click here to have it downloaded on the next mail check."}, /* MESS_FETCH_HELP2 */
  {9814, "This message will be deleted from the POP server the next time you check mail.  Click here to leave it on the server."}, /* MESS_DELETE_HELP */
  {9815, "This message is on your POP server.  Click here to have it deleted the next time you check mail."}, /* MESS_DELETE_HELP2 */
  {9816, "This message is still on your POP server.  The icons in this box let you delete it (or fetch the rest, if part was skipped)."}, /* MESS_SERVER_HELP */
  {9817, "Use the tow truck to drag the message to a different mailbox window."}, /* MESS_DRAG_HELP */
  {9818, "Progress"}, /* PROGRESS */
  {9819, "Your attachments folder cannot be found.\nEudora will use \"Attachments Folder\" in your Eudora folder for now.  Should Eudora use that permanently?\nChoose New...\nNo-\nYes-"}, /* NO_ATTACH_FOLDER */
  {9820, ""}, /* U_AN_APPLICATION */
  {9901, "Fwd:"}, /* FWD_PREFIX */
  {9902, "manual-action"}, /* MDN_MAN_ACTION */
  {9903, "automatic-action"}, /* MDN_AUTO_ACTION */
  {9904, "MDN-sent-manually"}, /* MDN_MAN_SENT */
  {9905, "MDN-sent-automatically"}, /* MDN_AUTO_SENT */
  {9906, "%r: <%p>%p"}, /* CID_SEND_FMT */
  {9907, "The default folder you specified for new mailboxes no longer exists.  Eudora will use the Mail Folder instead."}, /* MF_DEFAULT_FOLDER_NOT_FOUND */
  {9908, "There was an error saving this filter."}, /* MF_SAVE_FILTER_ERROR */
  {9909, "PersonalitiesmPrs"}, /* PERSONALITIES_SETTING */
  {9910, "Eudora encountered some messages that it had problems sending. They have been transferred to your Out box."}, /* UNSYNCH_OUT_TEMP */
  {9911, "Eudora can't detect any common elements to make a filter with.  Try adjusting your selection, or use the filters window directly."}, /* MF_NO_COMMON_ERROR */
  {9912, "An error occurred while scanning the messages."}, /* MF_SCAN_ERROR */
  {9913, "An error occurred with the Make Filter window."}, /* MF_MISC_ERR */
  {9914, "30"}, /* BG_YIELD_INTERVAL */
  {9915, "60"}, /* FG_YIELD_INTERVAL */
  {9916, "%r: :%p:%p:%x:%x:%x:%x\\n"}, /* RELATED_FMT */
  {9917, "<!x-stuff-for-pete base=\\\"%p\\\" src=\\\"%p\\\" id=\\\"%d\\\" charset=\\\"%p\\\">"}, /* MHTML_INFO_TAG */
  {9918, "Parts Folder"}, /* PARTS_FOLDER */
  {9919, "1,1,1"}, /* NICK_COLOR */
  {9920, "1"}, /* NICK_STYLE */
  {10001, "15"}, /* MIN_COLOR_LIGHT */
  {10002, "85"}, /* MAX_COLOR_LIGHT */
  {10003, "110"}, /* KERB_POP_PORT */
  {10004, "100"}, /* LOG_ROLLOVER */
  {10005, "----"}, /* DEFAULT_CREATOR */
  {10006, "????"}, /* DEFAULT_TYPE */
  {10007, "^0 ^1^3"}, /* FIXED_DATE_FMT */
  {10008, "All headers and other sludge are displayed in this message."}, /* SHOW_ALL_HELP */
  {10009, "Less important headers and formatting commands are hidden."}, /* NO_SHOW_ALL_HELP */
  {10010, "Purchasing Information..."}, /* PURCHASE_EUDORA */
  {10011, "http://www.eudora.com/offer/?Mac.%p"}, /* PURCH_URL */
  {10012, ".pre"}, /* PREFILTER_SUFFIX */
  {10013, ".pst"}, /* POSTFILTER_SUFFIX */
  {10014, "flowed"}, /* FORMAT_FLOWED */
  {10015, "This version of Eudora expires on %p.\nPlease visit our website to get a newer beta or the final version.\n\nLater-\nVisit Site-"}, /* WILL_EXPIRE */
  {10016, "There were errors reading the address book \"%p\".  Nicknames from that file will not be used.  Restart Eudora to try again."}, /* NICK_FILE_GONE */
  {10017, "%p: %r"}, /* TP_SINGLE_FMT */
  {10018, "%p: %r & %r"}, /* TP_DUAL_FMT */
  {10019, "Newsgroups:"}, /* NEWSGROUPS */
  {10020, "5"}, /* AE_INITIAL_TIMEOUT_SECS */
  {10101, "An error occurred while initializing nickname utilities."}, /* NICK_WATCHER_LOAD_ERR */
  {10102, "Auto-configuring..."}, /* ACAPPING */
  {10103, "vcf"}, /* VCARD_FILE_EXTENSION */
  {10104, "Automatic configuration failed.  You will have to configure Eudora manually."}, /* ACAP_FAILED */
  {10105, "ACAP Search..."}, /* ACAP_SERVER_SEARCH */
  {10106, "20"}, /* MAX_MULTI_DEPTH */
  {10107, "Fetch Settings Now"}, /* LOAD_SETTINGS_NOW */
  {10108, "389"}, /* LDAP_PORT_REALLY */
  {10109, "674"}, /* ACAP_PORT */
  {10110, ""}, /* DUMMY_POP_AUTHENTICATE */
  {10111, ""}, /* DUMMY_SEND_FORMAT */
  {10112, "Can't find Appearance Manager. Please install the Appearance Manager extensions."}, /* NO_APPEARANCE */
  {10113, "30"}, /* DRAG_EXPAND_TICKS */
  {10114, "%r %p"}, /* AUTHPLAIN_OUTER_FMT */
  {10115, "Plugin Filters"}, /* PLUGIN_FILTERS */
  {10116, "Plugin Nicknames"}, /* PLUGIN_NICKNAMES */
  {10117, "body=%r"}, /* BODY_EQUALS */
  {10118, "<<None Chosen>>"}, /* MF_NONE_CHOSEN */
  {10119, "%p (%d);"}, /* FSE_NAME_ERR */
  {10120, "Last Check: %p"}, /* TP_LAST_CHECK */
  {10201, "Your request has been placed in your Out mailbox, and will be sent next time you send queued messages."}, /* MAIL_QUEUED */
  {10202, "Your request has been sent.  You should receive a reply shortly."}, /* MAIL_SENT */
  {10203, "Your settings file appears to be corrupt.  ResEdit may be able to repair it."}, /* SETTINGS_BAD */
  {10204, "============"}, /* MIME_BOUND2 */
  {10205, "<%p>"}, /* NORMAL_MFROM */
  {10206, "<>"}, /* EMPTY_MFROM */
  {10207, "3,4,5,77,218"}, /* SACRED_PREFS */
  {10208, "Really reset your settings?\nSave all your work first, Eudora will quit after the reset and not save anything.\n\nCancel-\nReset-"}, /* RESET_PREFS_WARN */
  {10209, "5"}, /* CTB_RETRY */
  {10210, "STR# id %d was corrupt and had to be repaired.  Should be ok now."}, /* FIXED_STRN */
  {10211, "Colons are no longer allowed in nicknames.  Eudora changed your colons to plusses (+)."}, /* FIXED_COLON */
  {10212, "+"}, /* PLUS */
  {10213, "Couldn't create temporary file."}, /* NO_TEMP_FILE */
  {10214, "inline"}, /* INLINE */
  {10215, "The resource fork of this file is corrupt."}, /* CORRUPT_RFORK */
  {10216, "Stationery Folder"}, /* STATION_FOLDER */
  {10217, "Some of those messages are queued to be sent; %r them anyway?\\n(They won't ever be sent.)"}, /* QUEUED_WARNING */
  {10218, "900"}, /* MAX_MESSAGE_SIZE */
  {10219, "There was an error during editing."}, /* PETE_ERR */
  {10220, "Couldn't figure out that URL."}, /* URL_PARSE */
  {10301, "Next Check: %p"}, /* TP_NEXT_CHECK */
  {10302, "Checking now..."}, /* TP_CHECKING_NOW */
  {10303, "Never"}, /* TP_NEVER */
  {10304, "Not scheduled"}, /* TP_NOT_SCHEDULED */
  {10305, "Incoming mail waiting to be delivered at idle time."}, /* TP_WAITING_TO_DELIVER */
  {10306, "Click button to deliver immediately."}, /* TP_CLICK_TO_DELIVER */
  {10307, "Eudora could not check mail in the background. Try closing some windows or switch background threading off."}, /* THREAD_CANT_CHECK */
  {10308, "Eudora could not send mail in the background. Try closing some windows or switch background threading off."}, /* THREAD_CANT_SEND */
  {10309, "Processing outgoing messages..."}, /* PROCESSING_OUT */
  {10310, "Eudora may not be able to send all your queued messages."}, /* THREAD_CANT_SEND_ALL */
  {10311, "\\n{%d:%d}"}, /* WU_NOERR_FMT */
  {10312, "ldap.four11.com"}, /* DEFAULT_LDAP_HOST */
  {10313, "LDAP"}, /* LDAP_NAME */
  {10314, "Could not fetch this message because Eudora is low on stack memory.  Please try again."}, /* THREAD_LOW_STACK */
  {10315, "Could not fetch this message because Eudora is low on stack memory."}, /* LOW_STACK */
  {10316, "Filter Messages"}, /* FILTER_ITEXT */
  {10317, "Filter Waiting Messages"}, /* FILTER_WAITING_ITEXT */
  {10318, "Revert To Default Tabs"}, /* REVERT_TO_DEFAULT_TABS */
  {10319, "Tabs"}, /* TABS_ITEXT */
  {10320, "With Bullets"}, /* ADD_BULLETS */
  {10401, "URL"}, /* URL */
  {10402, "MPGP"}, /* MAC_PGP_CREATOR */
  {10403, "^0@^1"}, /* PH_ALIAS_FMT */
  {10404, "^0"}, /* PH_BOX_FMT */
  {10405, "1800"}, /* TOC_SMALLDIRTY */
  {10406, "8"}, /* PETE_NIBBLE */
  {10407, "10"}, /* OLD_CONFIG_FONT_SIZE */
  {10408, "150"}, /* STRING_CACHE */
  {10409, "%p"}, /* P_FMT */
  {10410, "%r"}, /* R_FMT */
  {10411, "delete"}, /* NUKE_VERB */
  {10412, "Check Mail Specially..."}, /* CHECK_SPECIAL_ITEXT */
  {10413, "Send Messages Specially..."}, /* SEND_SPECIAL_ITEXT */
  {10414, "Send Queued Messages"}, /* SEND_ITEXT */
  {10415, "Save"}, /* SAVE_ITEXT */
  {10416, "Save All"}, /* SAVE_ALL_ITEXT */
  {10417, "Close"}, /* CLOSE_ITEXT */
  {10418, "Close All"}, /* CLOSE_ALL_ITEXT */
  {10419, "Print..."}, /* PRINT_ITEXT */
  {10420, "Print Selection..."}, /* PRINT_SEL_ITEXT */
  {10501, "80"}, /* DEF_FIXED_MWIDTH */
  {10502, "Eudora LDAP Library"}, /* LDAP_SHARED_LIB_TRUE_NAME */
  {10503, "An error occurred while initializing LDAP."}, /* LDAP_INIT_ERR */
  {10504, "Error:\\n\\nEudora cannot do LDAP searches because the Eudora LDAP Library is not properly installed."}, /* LDAP_NOT_LOADED_MSG */
  {10505, "Error:\\n\\nCannot open session with LDAP server\\n"}, /* LDAP_OPEN_ERR_MSG */
  {10506, "Number of matches:"}, /* LDAP_RESULT_COUNT */
  {10507, "An error occurred:\\n"}, /* LDAP_RESULT_ERR */
  {10508, "------------------------------------------------------------"}, /* LDAP_SEPARATOR */
  {10509, "18"}, /* MIN_LDAP_LABEL_LEN */
  {10510, ""}, /* DEF_FIXED_SIZE */
  {10511, "Message is currently being displayed in a fixed-width font; click to revert to normal font"}, /* MESS_FIXED_HELP */
  {10512, "To display message in a fixed-width font, click this"}, /* NO_MESS_FIXED_HELP */
  {10513, "Error remembering what mail has been fetched.  Try again later."}, /* BUILD_POPD */
  {10514, "Preview"}, /* PREVIEW */
  {10515, "html public \\\"-//W3C//DTD W3 HTML//EN\\\""}, /* HTML_DOCTYPE */
  {10516, "text/css"}, /* HTML_STYLE_TYPE */
  {10517, "5"}, /* TOLERABLE_CTLCHARS_PPT */
  {10518, "Error while checking mail for %p:"}, /* ERR_PERS_CHECKING_MAIL */
  {10519, "Error while sending mail for %p:"}, /* ERR_PERS_SENDING_MAIL */
  {10520, "120"}, /* THREAD_RECV_TIMEOUT */
  {10601, "Copy"}, /* COPY_ITEXT */
  {10602, "Copy & Unwrap"}, /* COPY_UNWRAP_ITEXT */
  {10603, "Wrap Selection"}, /* WRAP_ITEXT */
  {10604, "Unwrap Selection"}, /* UNWRAP_ITEXT */
  {10605, "Finish Address Book Entry"}, /* FINISH_ITEXT */
  {10606, "Finish & Expand Address Book Entry"}, /* FINISH_EXP_ITEXT */
  {10607, "Insert Recipient"}, /* INSERT_ITEXT */
  {10608, "Insert & Expand Recipient"}, /* INSERT_EXP_ITEXT */
  {10609, "Sort"}, /* SORT_ITEXT */
  {10610, "Sort Descending"}, /* SORT_DESCEND_ITEXT */
  {10611, "Quoting Selection"}, /* REP_SELECTION */
  {10612, "To All"}, /* REP_ALL */
  {10613, "Without Quoting"}, /* REP_NOSEL */
  {10614, "With"}, /* REP_WITH */
  {10615, "Redirect"}, /* REDIRECT */
  {10616, "To"}, /* TO_ITEXT */
  {10617, "Turbo"}, /* TURBO */
  {10618, "Without Delete"}, /* TB_REDIR_NO_DEL */
  {10619, "Change Queueing..."}, /* CHANGE_QUEUEING */
  {10620, "Make Address Book Entry..."}, /* MAKE_NICK_ITEXT */
  {10701, "*"}, /* SASL_CANCEL */
  {10702, "Error while performing unknown task for %p:"}, /* ERR_PERS_UNKNOWN_TASK */
  {10703, "Can't send to \"%p\"; SMTP server says \"%p\"."}, /* BAD_ADDRESS_ERR_TEXT */
  {10704, "Checking"}, /* TP_CHECKING */
  {10705, "Sending"}, /* TP_SENDING */
  {10706, "Offline"}, /* TP_OFFLINE */
  {10707, "An unknown error occurred while transferring this message."}, /* UNKNOWN_ERR_TEXT */
  {10708, "Change Password..."}, /* CHANGE_PW */
  {10709, "Forget Password"}, /* FORGET_PW */
  {10710, "Change Password for Selected Personalities..."}, /* CHANGE_PERS_PW */
  {10711, "Forget Password for Selected Personalities"}, /* FORGET_PERS_PW */
  {10712, "%p.%d.%d"}, /* CID_ONLY_FMT */
  {10713, "6000,6000,48000"}, /* LINK_COLOR */
  {10714, "4"}, /* LINK_STYLE */
  {10715, "Are you sure that is an URL?\nThe text you have entered doesn't look like proper URL syntax.\n\nCancel-\nLink As Is-"}, /* NOT_URL */
  {10716, "1024"}, /* OPEN_AT_END_THRESH */
  {10717, "Error:\\n\\nEudora cannot do LDAP searches because an error occurred\\n"}, /* LDAP_NOT_SUPPORTED_MSG */
  {10718, "Ph"}, /* PH_NAME */
  {10719, "ns"}, /* PH_HOST_COMMON_PREFIX */
  {10720, "ldap"}, /* LDAP_HOST_COMMON_PREFIX */
  {10801, "Make Address Book Entry From Selection..."}, /* MAKE_SEL_NICK_ITEXT */
  {10802, "Delete"}, /* DELETE_ITEXT */
  {10803, "Nuke"}, /* NUKE_ITEXT */
  {10901, "%p%pMessage and/or attachments are probably corrupt."}, /* BAD_HEXBIN_ERR_TEXT */
  {10902, "42000,42000,42000"}, /* SEP_LINE_DARK_COLOR */
  {10903, "65535,65535,65535"}, /* SEP_LINE_LIGHT_COLOR */
  {10904, "50000"}, /* LIGHT_COLOR */
  {10905, "an image"}, /* SOME_PART */
  {10906, "Please choose a folder from the Mailbox menu."}, /* CHOOSE_MBOX_FOLDER */
  {10907, "This Folder    "}, /* CHOOSE_MBOX_THIS_FOLDER */
  {10908, "Waiting for PPP connection ..."}, /* OT_PPP_WAIT */
  {10909, "Enter query (server is %p):"}, /* PH_LABEL_NO_TYPE */
  {10910, "iso-2022-jp"}, /* ISO_2022_JP */
  {10911, "Address book names must be no more than 31 characters long."}, /* NICKFILE_TOO_LONG */
  {10912, "Address book names must not contain colons."}, /* NICKFILE_BADCHAR */
  {10913, "You already have an address book called that."}, /* NICKFILE_DUPLICATE */
  {10914, "Insufficient memory to display graphic."}, /* GRAPHIC_MEM */
  {10915, "Moving Messages to In Box..."}, /* MOVING_MESSAGES_TO_IN */
  {10916, "base"}, /* LDAP_SCOPE_BASE_STR */
  {10917, "one"}, /* LDAP_SCOPE_ONELEVEL_STR */
  {10918, "sub"}, /* LDAP_SCOPE_SUBTREE_STR */
  {10919, "body"}, /* MAILTO_BODY */
  {10920, "blockquote, dl, ul, ol, li { padding-top: 0 ; padding-bottom: 0 }"}, /* HTML_NOSPACESTYLE */
  {11101, "4"}, /* VICOM_FACTOR */
  {11102, "????"}, /* LDAP_SHARED_LIB_ARCH_TYPE */
  {11103, "Error:\\n\\nEudora cannot do LDAP searches because LDAP is not enabled in this version of Eudora."}, /* LDAP_NOT_ENABLED_MSG */
  {11104, "Error:\\n\\nAn error occurred while loading the Eudora LDAP library\\n"}, /* LDAP_LIB_OPEN_ERR_MSG */
  {11105, "Error:\\n\\nEudora cannot do LDAP searches because the version of the CFM-68K Runtime Enabler installed is too old.  You must install version 4.0 or later of the CFM-68K Runtime Enabler, or be running Mac OS 8.0 or later."}, /* LDAP_NEEDS_NEWER_CFM68K_MSG */
  {11106, "Eudora Settings location problem.\nYou cannot use \"%p\" for an Eudora Folder.  Please move the settings file into its own folder and try again.\n\n\nCancel-"}, /* NO_EF_DESKTOP */
  {11107, "Are you sure that's an Eudora Folder?\nThe folder \"%p\" doesn't seem to be a proper folder for mail.  Really convert \"%p\" for use as an Eudora Folder?\n\nCancel-\nConvert"}, /* EF_WARNING */
  {11108, "One or more unknown errors have occurred. Eudora could not report this due to a memory error."}, /* TP_UNKNOWN_ERR_TEXT */
  {11109, "Unknown personality"}, /* TP_UNKNOWN_PERS */
  {11110, ""}, /* JUST_FOR_KIRAN */
  {11111, "Error:\\n\\nEudora cannot do LDAP searches because the currently installed Eudora LDAP Library is incompatible with this version of Eudora."}, /* LDAP_LIB_VERS_BAD_MSG */
  {11112, "Error:\\n\\nThere is not enough memory to load the Eudora LDAP library.  Try increasing Eudora's memory partition."}, /* LDAP_LIB_OPEN_MEM_ERR_MSG */
  {11113, "Error:\\n\\nThere is not enough memory to open a session with the LDAP server."}, /* LDAP_OPEN_MEM_ERR_MSG */
  {11114, "ns.eudora.com"}, /* MASTER_SERVER_SERVER */
  {11115, "Error:\\n\\nEudora cannot do LDAP searches because the CFM-68K Runtime Enabler is not installed."}, /* LDAP_NEEDS_CFM68K_MSG */
  {11116, "Error:\\n\\nUnable to connect to the specified Directory Services server."}, /* DIR_SVC_SERVER_RESOLVE_ERR_MSG */
  {11117, "Error:\\n\\nEudora cannot do an LDAP lookup because a Base Object has not been specified for the search.  You can specify a Base Object as part of the LDAP server's URL."}, /* LDAP_SEARCH_REQUIRES_BASE_OBJECT */
  {11118, "LDAP lookup canceled by user."}, /* LDAP_SEARCH_USER_CANCELED_MSG */
  {11119, "%p %p"}, /* NICK_VIEW_ANNOTATE */
  {11120, "WIDTH"}, /* LZ_WIDTH */
  {11301, "HEIGHT"}, /* LZ_HEIGHT */
  {11302, "true"}, /* LZ_TRUE */
  {11303, "false"}, /* LZ_FALSE */
  {11304, ".html-.htm"}, /* HTMLISH_SUFFICES */
  {11305, "url"}, /* LZ_URL */
  {11306, "Upgrade Validation Number"}, /* UVN */
  {11307, "2398377600"}, /* TOO_EARLY_FILE */
  {11308, "Cache Folder"}, /* CACHE_FOLDER */
  {11309, "..@:,()[]<>\\\""}, /* NICK_STORED_BAD_CHAR */
  {11310, "-----------\""}, /* NICK_STORED_REP_CHAR */
  {11311, "Delivery Folder"}, /* DELIVERY_FOLDER */
  {11312, "Delivery Folder"}, /* USA_DELIVERY_FOLDER */
  {11313, "3"}, /* DELIVERY_BATCH */
  {11314, "60"}, /* FILTER_HOG_TICKS */
  {11315, ""}, /* COMP_EXTRA_LINES */
  {11316, "iso-8859-1"}, /* UNSPECIFIED_CHARSET */
  {11317, "You may need to close some windows, move messages out of your In, Out, and Trash mailboxes, or increase Eudora's memory size."}, /* MEM_EXPLANATION */
  {11318, "330"}, /* PROG_CONT_WIDTH */
  {11319, "143"}, /* IMAP_PORT */
  {11320, "IMAP Folder"}, /* IMAP_MAIL_FOLDER_NAME */
  {11501, "24000,24000,24000"}, /* QUOTE_COLOR */
  {11502, "This Mailbox<I"}, /* IMAP_THIS_MAILBOX_NAME */
  {11503, ""}, /* IMAP_MAILBOX_LOCATION_PREFIX */
  {11504, "20"}, /* RECENT_DIR_LIMIT */
  {11505, "Configured server:\\n"}, /* CONFIG_DIR_SERVER */
  {11506, "Servers used recently:\\n"}, /* RECENT_DIR_SERVERS */
  {11507, "\\t<%p>\\n"}, /* RECENT_DIR_FMT */
  {11508, "12"}, /* PREVIEW_HI */
  {11509, "3"}, /* PREVIEW_USELESS */
  {11510, "5"}, /* PREVIEW_READ_SECS */
  {11511, "%p Window Tab\\n\\nClick on this tab to switch to the %p window. Click and drag the tab to reorder the tabs, to drag the tab to another tool window, or to create a new tool window."}, /* WAZOO_TAB_HELP */
  {11512, "(text=plain)"}, /* MANGLE_ARGS */
  {11513, "15"}, /* PREVIEW_STILL_TICKS */
  {11514, "IMAPSpool%x"}, /* IMAP_SPOOL_FMT */
  {11515, "(This message can't be fetched from the remote server because you're offline.)"}, /* IMAP_GETTING_MESSAGE_OFFLINE */
  {11516, "IMAP Attachments"}, /* IMAP_ATTACH_FOLDER */
  {11517, "^0.^3@^2"}, /* IMAP_KERBEROS_SERVICE_FMT */
  {11518, "$Name$"}, /* NAME_VAR */
  {11519, "$Email$"}, /* ADDR_VAR */
  {11520, "0"}, /* AUTOSAVE_INTERVAL */
  {11701, "56540,21600,44444"}, /* SPELL_COLOR */
  {11702, "4"}, /* SPELL_FACE */
  {11703, "Spelling Dictionaries"}, /* SPELL_DICTS */
  {11704, "0"}, /* SPELL_MEM_LIMIT */
  {11705, "1"}, /* SPELL_SUGGEST_DEPTH */
  {11706, "Your message has misspellings.\nYou can send it anyway, or cancel and correct the misspellings.\n\nCancel-\nSend Anyway-"}, /* SPELL_WARNING */
  {11707, "User Dictionary"}, /* SPELL_UDICT */
  {11708, "User Anti-Dictionary"}, /* SPELL_UADICT */
  {11709, "#LID 1033 1 3\\nEudora\\nQUALCOMM\\ne-mail\\n"}, /* SPELL_UDICT_CONTENTS */
  {11710, "#LID 1033 3 0\\n"}, /* SPELL_UADICT_CONTENTS */
  {11711, "Keystroke is already in use.\nThe keystroke you've chosen is already in use for \"%p\".  Do you really want to reassign it?\n\nCancel-\nReassign-"}, /* CMDKEY_CONFLICT */
  {11712, "50"}, /* QUOTE_BLEND */
  {11713, "Search"}, /* SEARCH_BUTTON */
  {11714, "Searching:"}, /* SEARCH_SEARCHING */
  {11715, "More"}, /* MORE_CHOICES */
  {11716, "Fewer"}, /* FEWER_CHOICES */
  {11717, "Error in regular expression"}, /* REG_EXP_ERR */
  {11718, "too many ()"}, /* TOO_MANY_PAREN_ERR */
  {11719, "unmatched ()"}, /* UNMATCHED_PAREN_ERR */
  {11720, "*+ operand could be empty"}, /* EMPTY_OPERAND_ERR */
  {11901, "nested *?+"}, /* NESTED_OPERAND_ERR */
  {11902, "invalid [] range"}, /* INVALID_BRACKET_RANGE_ERR */
  {11903, "unmatched []"}, /* UNMATCHED_BRACKET_ERR */
  {11904, "?+* follows nothing"}, /* REPEAT_NOTHING_ERR */
  {11905, "trailing \\\\"}, /* TRAILING_SLASH_ERR */
  {11906, "$User$"}, /* USER_VAR */
  {11907, "Resynchronizing mailbox %p."}, /* IMAP_RESYNC_MAILBOX */
  {11908, "(This message will be fetched from the remote server.)"}, /* IMAP_GETTING_MESSAGE */
  {11909, "Stop"}, /* STOP_BUTTON */
  {11910, "%p.%p%p"}, /* IMAP_TEMP_MAILBOX_FORMAT */
  {11911, "Search results"}, /* SEARCH_RESULTS */
  {11912, "6"}, /* SEARCH_HITS_SIZE */
  {11913, "Fetching message from %p."}, /* IMAP_FETCHING_MESSAGE */
  {11914, "Error while resyncing mailbox for %p:"}, /* ERR_PERS_RESYNCING */
  {11915, "Error while fetching message for %p:"}, /* ERR_PERS_FETCHING */
  {11916, "20"}, /* FILEID_PAUSE_LEN */
  {11917, "2064"}, /* FILEID_AFFECTED_SYSVERSION */
  {11918, "Eudora couldn't find any files to import.\nWould you like to manually select an account to import?\n\nNo-\nYes-"}, /* IMPORT_MESSAGE_BY_HAND */
  {11919, "Deleting message(s) from %p."}, /* IMAP_DELETING_MESSAGES */
  {11920, "Marking message %d of %d."}, /* IMAP_MARKING_MESSAGES */
  {12101, "Removing deleted messages from %p."}, /* IMAP_EXPUNGE_MAILBOX */
  {12102, "Error while deleting message for %p:"}, /* ERR_PERS_DELETING */
  {12103, "Error while transferring message for %p:"}, /* ERR_PERS_TRANSFERRING */
  {12104, "Remove Deleted Messages"}, /* IMAP_REMOVE_DELETED_MESSAGES_ITEXT */
  {12105, "Error while expunging mailbox for %p:"}, /* ERR_PERS_EXPUNGING */
  {12106, "Transferring IMAP message(s)."}, /* IMAP_TRANSFER_MESSAGES */
  {12107, "Copying IMAP message(s)."}, /* IMAP_COPY_MESSAGES */
  {12108, "Undelete"}, /* UNDELETE_ITEXT */
  {12109, "Error while undeleting messages for %p:"}, /* ERR_PERS_UNDELETING */
  {12110, "Undeleting message(s) in %p."}, /* IMAP_UNDELETING_MESSAGES */
  {12111, "Eudora is my friend."}, /* SPEAK_SAMPLE_TEXT */
  {12112, "From"}, /* SPEAK_SENDER_PREFIX */
  {12113, "dot"}, /* SPEAK_DOT */
  {12114, "the sender's name"}, /* SPEAK_SENDERS_NAME */
  {12115, "the subject line"}, /* SPEAK_SUBJECT */
  {12116, "nothing"}, /* SPEAK_NOTHING */
  {12117, "using"}, /* SPEAK_USING */
  {12118, "the default voice"}, /* SPEAK_DEFAULT_VOICE */
  {12119, "Really delete personality \"%p\"?  The deletion cannot be undone.\nDoing so will cause the local IMAP mail cache to be deleted.\n\nCancel-\nOK-"}, /* IMAP_DELETE_CACHE */
  {12120, "Really change personality \"%p\" to POP?\nDoing so will cause the local IMAP mail cache to be deleted.\n\nCancel-\nConvert to POP-"}, /* IMAP_TO_POP */
  {12301, "Inbox"}, /* IMAP_INBOX_NAME */
  {12302, "16384"}, /* IMAP_TRANSFER_BUFFER_SIZE */
  {12303, "30"}, /* SPELL_HOLD_OPEN_SECS */
  {12304, "60"}, /* LONG_MODAL_IDLE_SECS */
  {12305, "Waiting to decode attachment ..."}, /* IMAP_WAITING_FOR_DECODER */
  {12306, "Could not %p."}, /* IMAP_ERR_OPERATION_FMT */
  {12307, "Please choose a mailbox for deleted messages."}, /* CHOOSE_IMAP_TRASH_MAILBOX */
  {12308, "Really use \"%p\" as your trash mailbox?\n\"Empty Trash\" will remove all messages from it.\n\nCancel-\nOK-"}, /* IMAP_TRASH_SELECT */
  {12309, "Empty All Trash Mailboxes"}, /* IMAP_EMPTY_REMOTE_TRASH */
  {12310, "Empty Local Trash Mailbox"}, /* IMAP_EMPTY_LOCAL_TRASH */
  {12311, "Empty Trash"}, /* IMAP_EMPTY_TRASH */
  {12312, "Permanently remove the nickname file \"%p\"?\nThis operation cannot be undone.\n\nCancel-\nRemove It-"}, /* ALIAS_REMOVE_FILE_ALERT */
  {12313, "180"}, /* MIN_MB_SORT_TICKS */
  {12314, "Unable to rename"}, /* CANT_RENAME_ERR */
  {12315, "filename"}, /* MIME_CONTENT_DISP_FILENAME */
  {12316, "x-mac-type"}, /* MIME_MAC_TYPE */
  {12317, "x-mac-creator"}, /* MIME_MAC_CREATOR */
  {12318, "No plug-ins with settings"}, /* NO_TRANSLATORS_WITH_SETTINGS */
  {12319, "Attach Document"}, /* ATTACH_DOCUMENT_NAV_TITLE */
  {12320, "Attach"}, /* ATTACH_DOCUMENT_ACTION_BUTTON */
  {12501, "Select the file you wish to attach to the message and click \"Attach\""}, /* ATTACH_DOCUMENT_INSTRUCTIONS */
  {12502, "Insert Graphic"}, /* INSERT_DOCUMENT_NAV_TITLE */
  {12503, "Insert"}, /* INSERT_DOCUMENT_ACTION_BUTTON */
  {12504, "Select the graphic you wish to insert into the message and click \"Insert\""}, /* INSERT_DOCUMENT_INSTRUCTIONS */
  {12505, "Please select a folder in the list, then click \"Choose\" to use that folder."}, /* NAV_CHOOSE_FOLDER_MESSAGE */
  {12506, "Please locate the Eudora Settings file you wish to use, then click \"Choose\"."}, /* NAV_CHOOSE_SETTINGS */
  {12507, "Choose a Mailbox"}, /* CHOOSE_MAILBOX_NAV_TITLE */
  {12508, "Choose a Word Service"}, /* CHOOSE_WORD_SERVICE_NAV_TITLE */
  {12509, "Content-"}, /* MIME_CONTENT_PREFIX */
  {12510, "Fetching attachment:"}, /* IMAP_FETCH_ATTACHMENT */
  {12511, "Fetching %p from the server."}, /* IMAP_FETCH_ATTACHMENT_NAME */
  {12512, "Searching mailbox %p."}, /* IMAP_SEARCHING_MAILBOX */
  {12513, "Could not load the Navigation Services Library.  Try closing some windows or allocating more memory to Eudora."}, /* NAV_COULD_NOT_LOAD_ERR */
  {12514, "There was a problem using Navigation Services.  You may need to close some windows or allocate more memory to Eudora."}, /* NAV_GENERAL_ERR */
  {12515, "Error while fetching attachment for %p:"}, /* ERR_PERS_ATTACHMENT_FETCH */
  {12516, "Error while searching mailbox for %p:"}, /* ERR_PERS_SEARCH */
  {12517, "Decoding attachment ..."}, /* IMAP_DECODING_ATTACHMENT */
  {12518, "Doing search for %p."}, /* IMAP_SEARCHING_PERS */
  {12519, ""}, /* COMP_OUT_INTRO */
  {12520, ""}, /* COMP_IN_INTRO */
  {12701, "For IMAP messages, the match term(s) you have specified will search only the local message cache. Messages whose headers have not been downloaded will not be searched.\nYou can cancel or search local cache only for IMAP messages.\n\nCancel-\nSearch-"}, /* IMAP_SEARCH_LOCAL_WARN */
  {12702, "imap_stub"}, /* IMAP_STUB_ENCODING */
  {12703, "Searching on server..."}, /* SEARCHING_SERVER */
  {12704, "["}, /* SQUARE_LEFT */
  {12705, "]"}, /* SQUARE_RIGHT */
  {12706, ""}, /* MAIL2NEWS */
  {12707, "80"}, /* FLOW_WRAP_THRESH */
  {12708, "70"}, /* FLOW_WRAP_SPOT */
  {12709, "60"}, /* IMAP_MAIN_CON_TIMEOUT */
  {12710, "5"}, /* IMAP_MAX_CONNECTIONS */
  {12711, "Waiting for free connection to the server ..."}, /* IMAP_WAITING_FOR_CONNECTION */
  {12712, "Search Folder"}, /* SEARCH_FOLDER */
  {12713, "80"}, /* MAX_FIND_SELECTION */
  {12714, "Logging into the IMAP server."}, /* IMAP_LOGGING_IN */
  {12715, "IMAP Attachment Stubs"}, /* IMAP_SAFE_ATTACH_FOLDER */
  {12716, "Search"}, /* SEARCH_SEARCH */
  {12717, "RegCode"}, /* REG_CODE_FILE */
  {12718, "jump.cgi"}, /* URL_JUMP_COMMAND */
  {12719, "5"}, /* TYPING_THRESH */
  {12720, "http://jump.eudora.com"}, /* REG_SITE */
  {12901, "30"}, /* TYPING_RECENTLY_THRESH */
  {12902, "Eudora Business Cards"}, /* VCARD_FOLDER */
  {12903, "Eudora Business Cards"}, /* USA_VCARD_FOLDER */
  {12904, "Misplaced Items"}, /* USA_MISPLACED_FOLDER */
  {12905, "MacOS"}, /* REG_PLATFORM */
  {12906, "Eudora"}, /* REG_PRODUCT */
  {12907, "%d.%d.%d.%d"}, /* REG_VERSION_FORMAT */
  {12908, "Misplaced Items"}, /* MISPLACED_FOLDER */
  {12909, ""}, /* SASL_DONT */
  {12910, "Click to download images."}, /* GET_GRAPHICS_HELP */
  {12911, "Click to download images. Disabled because images have already been downloaded."}, /* NO_GET_GRAPHICS_HELP */
  {12912, "Next message."}, /* SPEAK_NEXT_MESSAGE */
  {12913, "Quote"}, /* SPEAK_QUOTE */
  {12914, "Unquote"}, /* SPEAK_UNQUOTE */
  {12915, "[[slnc %r]]"}, /* SPEAK_SILENCE_COMMAND */
  {12916, "1000"}, /* SPEAK_MESSAGE_PAUSE_DURATION */
  {12917, "[[pbas +4]]"}, /* SPEAK_QUOTE_MODIFY_VOICE_COMMAND */
  {12918, "[[pbas -4]]"}, /* SPEAK_UNQUOTE_MODIFY_VOICE_COMMAND */
  {12919, "Replying to: %p\n\n\n\nOK-"}, /* PASSIVE_REPLY_TO_ASTR */
  {12920, "To"}, /* SPEAK_TO_PREFIX */
  {13101, "with"}, /* SPEAK_WITH */
  {13102, "0"}, /* OT_PPP_RACE_HACK */
  {13103, "Now pausing %p seconds for ARA 3.x bug ..."}, /* OT_PPP_SMART_ASS */
  {13104, "400"}, /* SPEAK_QUOTE_PAUSE_DURATION */
  {13105, "Pronunciation Dictionary"}, /* SPEAK_DICTIONARY */
  {13106, "Your Speech Dictionary file appears to be damaged."}, /* SPEAK_DICTIONARY_BAD */
  {13107, "blockquote, dl, ul, ol, li { margin-top: 0 ; margin-bottom: 0 }"}, /* HTML_NOSPACESTYLE1 */
  {13108, "blockquote, dl, ul, ol, li { padding-top: 0 ; padding-bottom: 0 }"}, /* HTML_NOSPACESTYLE2 */
  {13109, "Number of messages / Combined size of all messages / Space wasted in the local cache.\\n\\nClick to remove deleted messages from this mailbox."}, /* IMAP_COMPACT_HELP */
  {13110, "Error while resyncing IMAP mailboxes:"}, /* ERR_MULT_RESYNCING */
  {13111, "Resync current mailbox ..."}, /* IMAP_CHECK_SPECIAL_ITEXT */
  {13112, "%p%p  Try holding down the shift key and choosing Message->Change->Server Options->Redownload Entire Message to refetch this entire message."}, /* IMAP_BAD_HEXBIN_ERR_TEXT */
  {13113, "Merge the nicknames from \"%p\" into \"%p\"?  The nickname file \"%p\" will be permanently removed.\nThis operation cannot be undone.\n\nCancel-\nOK-"}, /* ALIAS_MERGE_AND_REMOVE_FILE_ALERT */
  {13114, "100"}, /* NICK_CACHE_SIZE */
  {13115, "History List"}, /* CACHE_ALIAS_FILE */
  {13116, "History List"}, /* USA_CACHE_ALIAS_FILE */
  {13117, "Speak"}, /* SPEAK_ITEXT */
  {13118, "Speak Selection"}, /* SPEAK_SEL_ITEXT */
  {13119, "IMAP ALERT message for %p:"}, /* ERR_IMAP_ALERT */
  {13120, "GIFf,PNGf,JPEG"}, /* EXPORT_PICT_LIST */
  {13301, "Empty Trash on Current Server"}, /* IMAP_EMPTY_CURRENT_TRASH */
  {13302, "Empty Trash for Selected IMAP Personalities"}, /* IMAP_EMPTY_PERS_TRASH */
  {13303, "This will permanently remove messages from the trash mailboxes for the selected accounts.  Do you really wish to do this?"}, /* IMAP_EMPTY_TRASH_WARN */
  {13304, "UsageStats"}, /* AUDIT_FILE */
  {13305, "%d%d%d%d%d%d%d%d%d%d %d %d"}, /* AUDIT_INTRO_FORMAT */
  {13306, "Ad Folder"}, /* AD_FOLDER_NAME */
  {13307, "Payment & Registration..."}, /* PAY_AND_REGISTER */
  {13308, "This file describes how you use Eudora; we might ask you for it someday to help us understand our users better.  It will n e v e r contain any of your email or personal information, and it will n e v e r be sent anywhere without your permission.\\n\\n"}, /* AUDIT_RELAX */
  {13309, "Sponsored Mode\\n(free, with ads)"}, /* ADWARE_VERSION_BUTTON_TITLE */
  {13310, "Paid Mode\\n(costs money, no ads)"}, /* PAY_VERSION_BUTTON_TITLE */
  {13311, "Light Mode\\n(free, fewer features)"}, /* FREE_VERSION_BUTTON_TITLE */
  {13312, "Register with Us"}, /* REGISTER_BUTTON_TITLE */
  {13313, "Profile"}, /* CUSTOMIZE_ADS_BUTTON_TITLE */
  {13314, "Find the Latest Update to Eudora"}, /* UPDATES_BUTTON_TITLE */
  {13315, "Change your Code"}, /* CHANGE_REGISTRATION_BUTTON_TITLE */
  {13316, "<no registration name>"}, /* NO_REG_NAME_TEXT */
  {13317, "<no registration code>"}, /* NO_REG_CODE_TEXT */
  {13318, "QuickTime 3.0 or better is not installed.\nQuickTime is necessary to display ads.  Would you like instructions about how to get QuickTime 3.0?  Alternately, you may switch to the free reduced-featured version of Eudora.\n\nReduce Features\nGet QuickTime-"}, /* CANT_AD */
  {13319, "Link History"}, /* LINK_HISTORY_FILE */
  {13320, "Link History Folder"}, /* LINK_HISTORY_FOLDER */
  {13501, "Couldn't create your Link History file."}, /* CREATING_LINK_HISTORY */
  {13502, "There is insufficient memory to create a new history entry."}, /* LINK_HISTORY_NEW_HISTORY_ERR */
  {13503, "Error opening the link history file."}, /* OPEN_LINK_HISTORY */
  {13504, "Couldn't save the link history file."}, /* SAVE_LINK_HISTORY */
  {13505, "There is insufficient memory to manipulate current nickname."}, /* LINK_HISTORY_GET_DATA_ERR */
  {13506, "link"}, /* LINK_CMD */
  {13507, "14"}, /* LINK_AGE */
  {13508, "<%r %p>"}, /* MIME_FLOWED_ON */
  {13509, "4000"}, /* AUDIT_SEND_THRESH */
  {13510, "eudusage@eudora.com"}, /* AUDIT_SEND_ADDR */
  {13511, "Please send this message after you've reviewed it and found it non-threatening (there is a legend at the bottom of the file explaining what the items mean).  Thank you for your help.\\n\\n-- Steve Dorner & the rest of the Mac Eudora development team\\n\\n"}, /* AUDIT_PLEASE_SEND */
  {13512, "7"}, /* AUDIT_NUKE_DAYS */
  {13513, "<p%p@%p>"}, /* PAY_MSGID_FMT */
  {13514, "<a%p@%p>"}, /* ADWARE_MSGID_FMT */
  {13515, "<f%p@%p>"}, /* FREEWARE_MSGID_FMT */
  {13516, "0"}, /* LINK_HISTORY_UI_FLAGS */
  {13517, "Eudora usage statistics"}, /* AUDIT_SUBJECT */
  {13518, "Eudora Updates"}, /* UPDATE_FILE */
  {13519, "http://jump.eudora.com"}, /* UPDATE_SITE */
  {13520, "Mailbox \"%p\" is full.\nYou cannot have more than 32,000 messages in a mailbox.\n\n\nOK-"}, /* TOO_MANY_MESSAGES */
  {13701, "Checking for Eudora updates..."}, /* UPDATE_WINDOW_PROGRESS */
  {13702, "Distributor ID"}, /* DIST_ID */
  {13703, "-- \\n"}, /* SIG_INTRO */
  {13704, "%p (Error code: %d)"}, /* HTTP_ERR_FORMAT */
  {13705, "That registration code is either not valid for this version or is not in the correct format.\nPlease verify you have entered your name and code correctly. For further help, click the ? button below.\n\n\nOK-"}, /* REG_INVALID */
  {13706, "iso-8859-15"}, /* MIME_ISO_LATIN15 */
  {13707, "windows-1252"}, /* MIME_WIN_1252 */
  {13708, "Demog.txt"}, /* PROFILE_FILE_NAME */
  {13709, "http://jump.eudora.com"}, /* TECH_SUPPORT_SITE */
  {13710, "0"}, /* PREF_OFFLINE_LINK_ACTION */
  {13711, "2"}, /* OFFLINE_LINK_OPEN_TIMEOUT */
  {13712, "300"}, /* OFFLINE_LINK_NAG_TIME */
  {13713, ""}, /* DEBUG_PLAYLIST_URL */
  {13714, "\\\\Draft"}, /* IMAP_SENT_FLAG */
  {13715, "The attached registration file has been discarded"}, /* STOLEN_REG_FILE */
  {13716, "Switch permanently to sponsored mode?\nIn order to receive a refund for your Eudora purchase, you should contact a Eudora representative.  If you have not done so, press Cancel now.\n\nCancel\nPermanently Switch"}, /* SWITCH_TO_SPONSORED */
  {13717, "Performing queued commands for %p"}, /* IMAP_QUEUED_COMMANDS */
  {13718, "http,https"}, /* LINK_INTERESTING_PROTO */
  {13719, "iso-2022"}, /* ISO_2022 */
  {13720, "1800"}, /* TCP_PREF_REUSE_INTERVAL */
  {13901, "60"}, /* IMAP_SECONDARY_CON_TIMEOUT */
  {13902, "%d of the %d summar%* in the old table of contents used; %d message(s) will be refetched."}, /* IMAP_SALV_REPORT */
  {13903, "Unable to create ad window."}, /* INIT_ADWIN_ERR */
  {13904, "This message had characters which were illegal for its character set encoding."}, /* BAD_CHARSET_ERR */
  {13905, "The update server does not appear to be responding at this time.  Please try again later."}, /* UPDATE_CHECK_ERR */
  {13906, "The registration information you have received from our web site is below. If you wish to register using this information, click OK. Otherwise, click Cancel and you may register later."}, /* REG_FROM_FILE_DESC */
  {13907, "To complete your registration, please enter the name you registered under and your registration code below."}, /* REG_FROM_BUTTON_DESC */
  {13908, "Eudora seems to be confused about the current operating mode.\nSince we can't figure out if you prefer to run in Sponsored, Paid or Light mode, we'll default to Sponsored. You'll be able to choose the right operating mode from Payment & Registration.\n\n\nOK-"}, /* STATE_INVALID_AT_STARTUP */
  {13909, "Your paid registration information is invalid.\nFor now, Eudora will start in Sponsored mode.  You'll need to revalidate your registration information once Eudora has finished launching.\n\n\nOK-"}, /* REG_INVALID_AT_STARTUP */
  {13910, "Thank you for your registration!"}, /* REG_THANK_YOU_PROMPT */
  {13911, "Your registration information is invalid"}, /* REG_INVALID_PROMPT */
  {13912, "Double check the information below, or click on \"No Code\" to go to Eudora's web site for more information."}, /* REG_FROM_INVALID_CODE */
  {13913, "<html><body><img src=\\\"x-eudora-pictres:%d\\\"></body></html>"}, /* PICT_HELP_HTML */
  {13914, "x-eudora"}, /* X_EUDORA */
  {13915, "Registered To: %p, %p %r (%p)"}, /* ABOUT_REG_W_OLD */
  {13916, "Registered To: %p, %p %r"}, /* ABOUT_REG */
  {13917, "%r\\000%p\\000%p"}, /* AUTHPLAIN_FMT */
  {13918, ""}, /* ANAL_WHITE */
  {13919, "Authentication is required.\nThe SMTP server for %p wants you to authorize, but you have forbidden it, so the send will probably fail.  Do you want to allow authorization?\nTry Anyway\nCancel-\nAllow-"}, /* RECONSIDER_AUTH */
  {13920, "Refreshing IMAP Cache"}, /* IMAP_CACHE_MESSAGE */
  {14101, "Fetching list of mailboxes."}, /* IMAP_MAILBOX_LIST_FETCH_GENERAL */
  {14102, "Creating mailbox %p."}, /* IMAP_CACHE_CREATE */
  {14103, "Updating cache mailboxes ..."}, /* IMAP_CACHE_CREATE_GENERAL */
  {14104, "Date,Subject,From,To,X-Priority,Content-Type"}, /* IMAP_SHORT_HEADER_FIELDS */
  {14105, "Insufficient ad facetime."}, /* FACETIME_ERR */
  {14106, "Updating your profile\nYou will now be taken to the Eudora web site to update your profile information.\n\nCancel-\nContinue-"}, /* PRE_PROFILE_UPDATE_NOTE */
  {14107, "aeiouyh"}, /* FROG_CHARS */
  {14108, "Preparing local messages."}, /* IMAP_PREPARE_MESSAGES */
  {14109, "Encoding message %d of %d..."}, /* IMAP_PREPARE_FMT */
  {14110, "Update your Registration"}, /* UPDATE_REGISTER_BUTTON_TITLE */
  {14111, "Enter your Code"}, /* ENTER_REGISTRATION_BUTTON_TITLE */
  {14112, "58981,6553,6553"}, /* STAT_CURRENT_COLOR */
  {14113, "13107,13107,65535"}, /* STAT_PREVIOUS_COLOR */
  {14114, "65535,65535,32767"}, /* STAT_AVERAGE_COLOR */
  {14115, "2"}, /* STAT_CURRENT_TYPE */
  {14116, "1"}, /* STAT_PREVIOUS_TYPE */
  {14117, "1"}, /* STAT_AVERAGE_TYPE */
  {14118, "1800"}, /* MAX_ANAL_IDLE */
  {14119, "Flame Dictionary"}, /* FLAME_DICTIONARY */
  {14120, "8192"}, /* MAX_ANAL_BITE */
  {14301, "Your message may cause offense.\nYour message to %p regarding \"%p\" %r\n\nCancel-\nSend Anyway-"}, /* ANAL_WARNING */
  {14302, "3"}, /* ANAL_WARNING_LEVEL */
  {14303, "1 12 2 3 4 5 6 7 8 9 11 10"}, /* TOC_INVERSION_MATRIX */
  {14304, ""}, /* PW_CHANGE_SERVER */
  {14305, "This message %r."}, /* ANAL_COMP_HELP */
  {14306, "Importing mail"}, /* IMPORT_PROGRESS_TITLE */
  {14307, "Importing mail from %p"}, /* IMPORT_MESSAGE_PROGRESS_SUBTITLE */
  {14308, "Would you like to import settings and mail from another email application?\n\n\nNo-\nYes-"}, /* IMPORT_ON_STARTUP */
  {14309, "untitled address book"}, /* UNTITLED_ADDRESS_BOOK */
  {14310, "untitled nickname"}, /* UNTITLED_NICKNAME */
  {14311, "Personal Nicknames"}, /* PERSONAL_ALIAS_FILE */
  {14312, "Eudora has finished importing your data.\nYou may import other accounts or use the Import Mail command to do so later.\n\n\nOK-"}, /* IMPORT_COMPLETE */
  {14313, "Eudora Statistics.xml"}, /* STATISTICS_FILE */
  {14314, "Eudora Statistics.xml"}, /* USA_STATISTICS_FILE */
  {14315, "Scripts"}, /* SCRIPTS_FOLDER */
  {14316, "Number of messages selected / Total number of messages / Combined size of all messages / Space wasted.\\n\\nClick to remove deleted messages from this mailbox."}, /* IMAP_COMPACT_SHOW_NUM_HELP */
  {14317, "Downgrading your Eudora to Light Mode\nBecause you have not profiled yourself, you may no longer use Eudora in Sponsored mode.  You will now be placed in Light Mode until you fill out a profile.\nProfile\n\nOK-"}, /* PROFILE_FAILURE */
  {14318, "Waiting to Profile\nWhen you have finished filling out your profile on our web site, click OK.  Eudora will then contact our ad server to verify your profile.\n\nCancel-\nOK-"}, /* PROFILING_NOW */
  {14319, "Eudora has finished importing your data.\nYou will be unable to use the accounts or signatures you just imported in Light mode.  Would you like to switch to Sponsored mode now, or ignore the data you just imported?\n\nIgnore-\nSwitch-"}, /* LIGHT_IMPORT_COMPLETE */
  {14320, "That field can't be edited in this early beta version of Eudora."}, /* BETA_INACTIVE_FIELD */
  {14501, "Update your Profile"}, /* UPDATE_PROFILE_BUTTON_TITLE */
  {14502, "View By:"}, /* VIEW_BY_LABEL */
  {14503, "The selected address book could not be removed."}, /* ALIAS_REMOVE_NICK_FILE_ERR */
  {14504, "..."}, /* NICKLIST_MISSING_FIELD_VALUE */
  {14505, "{}"}, /* NICKLIST_PAREN */
  {14506, "A problem was encountered when attempting to display the nickname.  Eudora is probably running low on memory."}, /* ALIAS_DISPLAY_ERR */
  {14507, "This setting cannot be changed with an x-eudora-setting URL."}, /* FORBIDDEN_SETTING */
  {14508, "500"}, /* IMAP_SEARCH_CHUNK_SIZE */
  {14509, "Unspecified"}, /* SOME_BOZO */
  {14510, "Franklin Antonio"}, /* FRANKLIN */
  {14511, "X-Eudora-Plugin-Info"}, /* PLUGIN_INFO */
  {14512, "[]"}, /* SUBJ_TRIM_STR */
  {14513, "Reset Statistics"}, /* RESET_STATS_NOW */
  {14514, "Nickname1,1,4,1"}, /* NICK_PRINT_MARG1 */
  {14515, "Nickname2,2,4,1"}, /* NICK_PRINT_MARG2 */
  {14516, "Nickname3,3,6,1"}, /* NICK_PRINT_MARG3 */
  {14517, "That name is already in use."}, /* NAME_IN_USE */
  {14518, "Eudora co-branding spot.\\n\\n\"%p\"\\n\\nClick to view more information in web browser"}, /* SPONSOR_AD_HELP */
  {14519, "Create"}, /* CREATE */
  {14520, "A mailbox or mail folder at this level will not display in the Mailbox and Transfer menus due to limitations in MacOS.  Create it anyway?"}, /* MAILBOX_LEVEL_WARNING */
  {14701, "ihate thebox"}, /* THE_DAVE_AND_CHUCK_LOVE_CONNECTION */
  {14702, "An error occurred trying to find the latest version."}, /* UPDATE_FAILED */
  {14703, "Thank you for choosing to continue your support of Eudora.\nThis version of Eudora will remain in Paid Mode for about an hour or so, but will then switch to Sponsored Mode until your purchase has been completed.\n\n\nOK-"}, /* GRACE_PERIOD_PAY_NOW_ALRT */
  {14704, "Other versions of Eudora are available.\nWe'll take you to our web site to show you what's currently available, and where you can pay for this update.  This version will switch to Sponsored Mode in about an hour.\n\n\nOK-"}, /* GRACE_PERIOD_SHOW_VERSIONS_ALRT */
  {14705, "You should choose Rebuild.\nThe most likely reason for the toc being out of date is that your machine crashed during a mail transfer, and you might lose mail if you don't hit \"Rebuild\".\nCancel-\nUse Old\nRebuild-"}, /* REBUILD_TOC_ALRT_2 */
  {14706, "Please choose Rebuild.\n\"Use Old\" may cause you to lose mail.\nCancel-\nUse Old\nRebuild-"}, /* REBUILD_TOC_ALRT_3 */
  {14707, "An error occurred while importing your data.\nSome of your data may not have been imported.\n\n\nOK-"}, /* IMPORT_INCOMPLETE */
  {14708, "Registered To: %p, %p %r (%d)"}, /* ABOUT_REG_WITH_MONTH */
  {14709, "2"}, /* ANAL_DELAY_LEVEL */
  {14710, "10"}, /* ANAL_DELAY_MINUTES */
  {14711, "1"}, /* MOOD_H_FACE */
  {14712, "65535,0,0"}, /* MOOD_H_COLOR */
  {14713, "1"}, /* MOOD_FACE */
  {14714, "65535,0,0"}, /* MOOD_COLOR */
  {14715, "Choose a Photo"}, /* CHOOSE_PICTURE_TITLE */
  {14716, "Locate the photo to be displayed in your Address Book."}, /* CHOOSE_NICK_PICTURE_PROMPT */
  {14717, "Photo Album"}, /* PHOTO_FOLDER */
  {14718, "Select"}, /* SELECT */
  {14719, "Problem while saving nickname photo.\nEudora was not able to save the photo for %p because of an error (%d).\n\n\nOK-"}, /* NICK_PHOTO_COULD_NOT_SAVE */
  {14720, "untitled.csv"}, /* UNTITLED_CSV */
  {14901, "Save Address Book"}, /* NICK_SAVE_AS_TITLE */
  {14902, "There was an error while exporting nicknames.  Use the export file with extreme caution."}, /* NICK_EXPORT_FAIL */
  {14903, ","}, /* NICK_EXPORT_COMMA */
  {14904, "\\015"}, /* NICK_EXPORT_EOL */
  {14905, "\\004"}, /* NICK_COMMA_REPLACE_CHAR */
  {14906, "\\003"}, /* NICK_CR_REPLACE_CHAR */
  {14907, "Save As..."}, /* SAVE_AS_ITEXT */
  {14908, "Save Selection As..."}, /* SAVE_AS_SEL_ITEXT */
  {14909, "Exporting Nicknames..."}, /* EXPORTING_NICKNAMES */
  {14910, "Now exporting:"}, /* EXPORTING_SUBTITLE */
  {14911, "That nickname already exists.\nA nickname called \"%p\" already appears in another address book.  Duplicate nicknames can cause some confusion when expanding addresses.  Do you want to give this nickname a unique name?\n\nNo-\nYes-"}, /* DUP_NICKNAME_WARNING */
  {14912, "Can't find the Network Setup Extension.\nThis extension is required for Eudora's Internet Dialup features to work properly.  Would you like me to enable it for you?\n\nNo-\nYes-"}, /* NO_NS_LIB_WARNING */
  {14913, "The Network Setup Extensionhas been enabled.\nYou may need to restart your computer for the Internet Dialup features to work properly.\n\n\nOK-"}, /* NS_LIB_ENABLED */
  {14914, "Could not enable the Network Setup Extension.\nYou will have to use the Extensions Manager to enable the extension yourself, then restart your computer before the Internet Dialup features will work properly.\n\n\nOK-"}, /* NS_LIB_NOT_ENABLED */
  {14915, "60"}, /* NICK_BUTTON_DRAG_TICKS */
  {14916, "No User"}, /* PALM_SYNC_USERNAME */
  {14917, "\\n"}, /* URL_STRIP_CHARS */
  {14918, "\\n"}, /* NICK_CONCAT_NEWLINE */
  {14919, "-"}, /* NICK_CONCAT_SEPARATOR */
  {14920, "Do not include this nickname when syncing"}, /* ALIAS_DO_NOT_SYNC */
  {32401, "Script execution error.\nError in script \"%p\":\\n\\n%p"}, /* SCRIPT_EXECUTION_ERR */
  {32402, "995"}, /* POP_SSL_PORT */
  {32403, "465"}, /* SMTP_SSL_PORT */
  {32404, "993"}, /* IMAP_SSL_PORT */
  {32405, "636"}, /* LDAP_SSL_PORT */
  {32406, "SSL negotiation failed."}, /* SSL_ERR_STRING */
  {32407, "Error connecting to the Ph server.\\n%d; %p\\n"}, /* PH_CONNECT_ERROR */
  {32408, "Settings Prefill"}, /* SETTINGS_PREFILL_FILE */
  {32409, "Settings Prefill.processed"}, /* SETTINGS_PREFILL_PROCESSED */
  {32410, "150"}, /* STAT_GRAPH_HEIGHT */
  {32411, "350"}, /* STAT_GRAPH_WIDTH */
  {32412, "85"}, /* STAT_LEGEND_WIDTH */
  {32413, "Eudora SSL Certificates"}, /* CERT_FOLDER */
  {32414, "1N"}, /* COMMAND_KEY_CHECKMAIL_REPLACEMENT */
  {32415, "1A"}, /* COMMAND_KEY_ATTACH_REPLACEMENT */
  {32416, "3"}, /* SCROLL_WHEEL_LINES */
  {32417, ".aiff-.aif"}, /* SOUND_SUFFICES */
  {32418, "Couldn't find your printer.  Please use Print Center to set up your printer."}, /* NO_OSX_PRINTER */
  {32419, "Documents"}, /* USA_DOCUMENTS */
  {32420, "[%i]"}, /* NAT_FMT */
  {32421, "imap"}, /* KERBEROS_IMAP_SERVICE */
  {32422, "pop"}, /* K5_POP_SERVICE */
  {32423, "smtp"}, /* K5_SMTP_SERVICE */
  {32424, "^0@^1"}, /* K5_SERVICE_FMT */
  {32425, "qt"}, /* ERROR_KEYWORD */
  {32426, "explanation"}, /* EXPLANATION_KEYWORD */
  {32427, "GSSAPI: \"%p\""}, /* GSSAPI_LOG_FMT */
  {32428, "Any Address Book"}, /* ANY_ALIAS_FILE */
  {32429, "for personality \"%p\""}, /* FOR_PERSONALITY */
  {32430, "You don't have a personality named \"%p\"."}, /* NO_SUCH_PERSONALITY */
  {32431, "You connected to a server which returned this SSL certificate that is not in your keychain. Would you like to add it to your keychain?"}, /* SSL_CERT_PROMPT */
  {32432, "Personality names cannot be empty."}, /* PERS_MUST_HAVE_NAME */
  {32433, "Temporary error receiving mail; couldn't find %r (%d).  Mail may be delayed, but will show up eventually."}, /* THREAD_SUBFOLDER_ERR */
  {32434, "Temporary error receiving mail; couldn't create %p (%d).  Mail may be delayed, but will show up eventually."}, /* THREAD_DELIVER_CREATE_ERR */
  {32435, "Temporary error receiving mail; couldn't exchange %p and %p (%d).  Mail may be delayed, but will eventually be delivered."}, /* THREAD_DELIVER_EXCHANGE_ERR */
  {32436, "Hostname:"}, /* SSL_CERTDLG_HOSTNAMAE */
  {32437, "Expires:"}, /* SSL_CERTDLG_EXPIRES */
  {32438, "Fingerprints"}, /* SSL_CERTDLG_FINGERPRINTS */
  {32439, "SHA1:"}, /* SSL_CERTDLG_SHA1 */
  {32440, "MD5:"}, /* SSL_CERTDLG_MD5 */
  {32441, "Type:"}, /* SSL_CERTDLG_TYPE */
  {32442, "Serial #:"}, /* SSL_CERTDLG_SERIAL */
  {32443, "Not Valid Before:"}, /* SSL_CERTDLG_BEFORE */
  {32444, "Not Valid After:"}, /* SSL_CERTDLG_AFTER */
  {32445, "Issuer"}, /* SSL_CERTDLG_ISSUER */
  {32446, "Subject"}, /* SSL_CERTDLG_SUBJECT */
  {32447, "Country:"}, /* SSL_CERTDLG_COUNTRY */
  {32448, "State:"}, /* SSL_CERTDLG_STATE */
  {32449, "Locality:"}, /* SSL_CERTDLG_LOCALITY */
  {32450, "Organization:"}, /* SSL_CERTDLG_ORGANIZATION */
  {32451, "Organization Unit:"}, /* SSL_CERTDLG_ORGUNIT */
  {32452, "Common Name:"}, /* SSL_CERTDLG_CNAME */
  {32453, "Email:"}, /* SSL_CERTDLG_EMAIL */
  {32454, "Quit"}, /* FILE_QUIT_ITEM_STR */
  {32455, "About Eudora"}, /* APPLE_ABOUT_EUDORA_ITEM_STR */
  {32456, "769"}, /* SSL_VERSION_STD_PORT */
  {32457, "0"}, /* SSL_VERSION_ALT_PORT */
  {32458, "Temporary error receiving mail; will not filter right now (%d).  Mail may be delayed, but will show up eventually."}, /* THREAD_PUNT_FILTER_ERR */
  {32459, "Username:"}, /* USERNAME_PROMPT */
  {32460, "Adjusting to broken IMail..."}, /* IMAIL_DOES_PLAIN_WRONG */
  {32461, "The message you are sending is missing at least one of its images.  It will be sent without the missing image(s)."}, /* AT_LEAST_ONE_GRAPHIC_MISSING */
  {32462, ""}, /* PPP_REACHABLE_HOST */
  {32463, "PlugIns"}, /* PACKAGE_PLUGINS_FOLDER */
  {32464, "MacOS"}, /* PACKAGE_MACOS_FOLDER */
  {32465, "[auth"}, /* POP3_AUTH_RESP_CODE */
  {32466, "Use \"%p\" as your Trash mailbox for your %p personality?\n\"Empty Trash\" will remove all messages from it.\n\nCancel-\nOK-"}, /* IMAP_TRASH_REUSE */
  {32467, "MsoNormal,margin-bottom:.0"}, /* WARNING_SIGNS_YOU_MIGHT_HAVE_OUTLOOK */
  {32468, "Junk"}, /* JUNK */
  {32469, "Junk"}, /* FILE_ALIAS_JUNK */
  {32470, "50"}, /* JUNK_MAILBOX_THRESHHOLD */
  {32471, "30"}, /* JUNK_MAILBOX_EMPTY_DAYS */
  {32472, "Trash"}, /* JUNK_MAILBOX_EMPTY_DEST */
  {32473, "0"}, /* JUNK_MAILBOX_EMPTY_THRESH */
  {32474, "Trim your %r mailbox now?\n%d messages in your %r mailbox are at least %r days old, and due for deletion.  Delete them?\nDon't Warn\nNo-\nTrim-"}, /* JUNK_EMPTY_WARNING */
  {32475, "My Junk"}, /* JUNK_PREEXISTING_RENAME_NAME */
  {32476, "You have a mailbox called \"%r\".\nEudora's junk mail feature uses a mailbox named \"%r\".  Would you like Eudora to use it, rename it, or disable the junk mailbox feature?\nDisable-\nRename\nUse It-"}, /* JUNK_PREEXISTING_WARNING */
  {32477, "100"}, /* JUNK_XFER_SCORE */
  {32478, "100"}, /* JUNK_JUNK_SCORE */
  {32479, "25"}, /* JUNK_MIN_REASONABLE */
  {32480, "Junk threshold too low\nWe recommend you not set the %r mailbox threshold to less than %r, or legitimate mail may be kept in the %r mailbox.  Set it to %r?\nUse Value\nCancel-\nOK-"}, /* JUNK_UNREASONABLE_WARNING */
  {32481, "Error trimming %r.\nThe old mail in your %r mailbox can't be trimmed, because the mailbox \"%p\" can't be found.\n\n\nCancel-"}, /* JUNK_CANT_ARCHIVE */
  {32482, "The following document was sent as an embedded object but not referenced by the email above:\\n"}, /* UNREFERENCED_PART */
  {32483, "32"}, /* WAZOO_TOPMARGIN */
  {32484, "21"}, /* WAZOO_TABHEIGHT */
  {32485, "RE:,FW:"}, /* OUTLOOK_CRAP_FIND */
  {32486, "Re:,Fwd:"}, /* OUTLOOK_CRAP_FIX */
  {32487, "Flushed: %s"}, /* LOG_FLUSHED */
  {32488, "1"}, /* FLUSH_TIMEOUT */
  {32489, "7"}, /* JUNK_TRIM_SOON */
  {32490, "1"}, /* JUNK_TRIM_INTERVAL */
  {32491, ""}, /* WHITELIST_ADDRBOOK */
  {32492, "Unable to initialize proxy list."}, /* PROXY_INIT_FAILED */
  {32493, "60"}, /* LONGTERM_IDLE */
  {32494, "5000"}, /* GRAPHICS_CACHE_MAX */
  {32495, "Failed to initialize the Content Concentrator."}, /* COULDNT_INIT_CONCON */
  {32496, "Compact"}, /* CONCON_PREVIEW_PROFILE */
  {32497, "..."}, /* CONCON_ELLIPSIS_TRAIL */
  {32498, "...snip..."}, /* CONCON_ELLIPSIS_CENTER */
  {32499, "..."}, /* CONCON_ELLIPSIS_LEAD */
  {32601, "Terse"}, /* CONCON_MULTI_PREVIEW_PROFILE */
  {32602, "20"}, /* CONCON_MULTI_MAX */
  {32603, "Original Message"}, /* CONCON_QUOTE_ON */
  {32604, "End Original"}, /* CONCON_QUOTE_OFF */
  {32605, "ConConInit: %p"}, /* CONCON_DEBUG_FMT */
  {32606, "%p/%p"}, /* CHARSET_FLUX_FMT */
  {32607, "None"}, /* CONCON_MESSAGE_PROFILE */
  {32608, "50"}, /* ENCODED_FLOWED_WRAP_SPOT */
  {32609, "You have a mail folder called \"%r\".\nEudora's junk mail feature uses a mailbox named \"%r\".  Your folder will be renamed.\n\n\nOK-"}, /* JUNK_PREEXISTING_FOLDER_WARNING */
  {32610, "What should cmd-J do?\nYou might use Eudora 6's Junk and Not Junk commands often.  You can use cmd-J and opt-cmd-J for them, or use cmd-J for manual message filtering. Which do you prefer?\n\nFilter-\nJunk-"}, /* USE_CMD_J_FOR_JUNK */
  {32611, "Trimming \"%r\" mailbox..."}, /* TRIMMING_JUNK */
  {32612, "5"}, /* PREVIEW_SINGLE_DELAY */
  {32613, "30"}, /* PREVIEW_MULTI_DELAY */
  {32614, ""}, /* SMALL_SYS_FONT_NAME */
  {32615, ""}, /* SMALL_SYS_FONT_SIZE */
  {32616, "10"}, /* WIN_GEN_WARNING_THRESH */
  {32617, "50"}, /* MULT_RESPOND_WARNING_THRESH */
  {32618, "Really open that many windows?\nYou're about to open %d windows.  Is this what you intend?\n\nCancel-\nOK-"}, /* WIN_GEN_WARNING */
  {32619, "Really respond to all those messages?\nYou're about to respond to %d messages all at once. Is this what you intend?\n\nCancel-\nOK-"}, /* MULT_RESPOND_WARNING */
  {32620, "%p, etc."}, /* AMBIG_SUBJ_FMT */
  {32621, "Terse"}, /* CONCON_MULTI_REPLY_PROFILE */
  {32622, "Use \"%p\" as your Junk mailbox for your %p personality?\nMessages may be removed from it automatically in the future.\n\nCancel-\nOK-"}, /* IMAP_JUNK_SELECT */
  {32623, "Please choose a Junk mailbox."}, /* CHOOSE_IMAP_JUNK_MAILBOX */
  {32624, "Eudora"}, /* EUDORA_EUDORA */
  {32625, "100"}, /* IMAP_DEFAULT_JUNK_SCORE */
  {32626, "Transfer to %p"}, /* TRANSFER_CONTEXT_FMT */
  {32627, "Could not remove; one or more items are locked."}, /* ITS_LOCKED_DUMMY */
  {32628, "%p (%p)"}, /* IMAP_MAILBOXTITLE_FMT */
  {32629, "15"}, /* IMAP_FLAGGED_LABEL */
  {32630, "Cannot trim \"%r\" mailbox to itself!\nPlease select another mailbox, or turn off junk trimming. How about \"%r\"?\n\nCancel-\nUse Trash-"}, /* JUNK_JUNK_IS_BAD_TRIM_DEST */
  {32631, "SpamWatch has been disabled.\nYour IMAP server does not support the UIDPLUS extension.  SpamWatch has been disabled for your %p personality until your server is upgraded to send COPYUID responses.\n\n\nOK-"}, /* JUNK_PREFNOIMAP_WARNING */
  {32632, "SpamWatch has been re-enabled.\nYour IMAP server now appears to support the required server extensions.  SpamWatch has been turned back on for your %p personality.\n\n\nOK-"}, /* JUNK_PREFIMAPAVAIL_WARNING */
  {32633, "The document you selected is an alias, but the original is no longer available."}, /* ALIAS_TO_NOWHERE */
  {32634, "X-Folder"}, /* X_FOLDER_ITEMS */
  {32635, "x-folder"}, /* MIME_X_FOLDER */
  {32636, "Report a Bug"}, /* HELP_BUG_MTEXT */
  {32637, "Make a Suggestion"}, /* HELP_SUGGEST_MTEXT */
  {32638, "%c\\t%p\\t%p\\t%p\\t%p\\t%p\\t%d\\t%p\\t%p\\n"}, /* SUM_SEARCH_COPY_FMT */
  {32639, "Do you want to set Eudora to be your default mail handler?\nEudora can handle 'mailto:' links on web pages and other sources. We'll only ask just once. \n\nNo\nYes-"}, /* DEFAULT_MAILER_Q */
  {32640, "44000"}, /* SEP_LINE_GREY */
  {32641, "Search %p for \"%p\""}, /* SEARCH_FOR_FMT */
  {32642, "Web"}, /* SEARCH_FOR_WEB */
  {32643, "http://jump.eudora.com"}, /* SEARCH_SITE */
  {32644, "Search %r"}, /* SEARCH_NOTHING_FMT */
  {32645, "This will search the web.\nNote: when you click this toolbar button with some text selected, Eudora will search the web for that text.\nDon't Warn\nCancel-\nOK-"}, /* SEARCH_TEXT_WARNING */
  {32646, "http://www.eudora.com/buying"}, /* BUYING_BY_HTTP */
  {32647, "mailto:buying@eudora.com"}, /* BUYING_BY_MAIL */
  {32648, "%p (%r %d)"}, /* REGCODE_PLUS_MONTH */
  {32649, "Checking %p for new messages."}, /* IMAP_POLLING_MAILBOXES */
  {32650, "Looking for new messages in %p."}, /* IMAP_POLL_MAILBOX */
  {32651, "Single Message"}, /* CONCON_PROF_SINGLE */
  {32652, "Multiple Messages"}, /* CONCON_PROF_MULTI */
  {32653, "Default (%p)"}, /* CONCON_PROF_DEFAULT */
  {32654, "96"}, /* CONCON_PREV_PROF_WIDTH */
  {32655, "None"}, /* CONCON_NONE */
  {32656, "OS X Address Book"}, /* OSXAB_FILE */
  {32657, "1"}, /* SPIN_LENGTH */
  {32658, "20"}, /* MAX_CONTEXT_FILE_CHOICES */
  {32659, "15000,15000,15000"}, /* FAKE_CONTENT_COLOR */
  {32660, "Eudora"}, /* SEARCH_FOR_EMAIL */
  {32661, "10"}, /* RECENT_SEARCH_LIMIT */
  {32662, "20"}, /* TB_FKEY_LABEL_WIDE */
  {32663, "30"}, /* GROUP_SUBJ_MAX_TIME */
  {32664, "%p"}, /* JUNK_NICK_FMT */
  {32665, "%p"}, /* HIST_NICK_FMT */
  {32666, "panel"}, /* SETTINGS_PANEL_KEYWORD */
  {32667, "10"}, /* LOCKED_FILE_PERSISTENCE */
  {32668, "60"}, /* POP_BEFORE_SMTP_AUTH_AGE_LIMIT */
  {32669, "digest"}, /* CONCON_DIGEST_ITEMS */
  {32670, "--"}, /* ATTCONV_BAD_CHARS */
  {32671, ""}, /* ATTCONV_REP_CHARS */
  {32672, "Missing Content Concentrator Profile\nYour settings indicate use of the \"%r\" profile for this operation, but that profile doesn't exist.\n\n\nOK-"}, /* CONCON_PROFILE_MISSING */
  {32673, "10"}, /* SPEECH_VOLUME */
  {32674, "#&$*- Hidden"}, /* IMAP_HIDDEN_TOC_NAME */
  {32675, "Undo Delete from %p"}, /* IMAP_UNDO_DELETE */
  {32676, ".com.,.org.,.net.,.edu."}, /* NAUGHTY_URL_TLDS */
  {32677, "Warning: The URL you are about to visit may be deceptive.  Visit it anyway?\n%p\n\nCancel-\nVisit"}, /* NAUGHTY_URL_ALERT */
  {32678, "Emoticons"}, /* EMOTICON_FOLDER */
  {32679, "Couldn't find a Trash mailbox for %p.\nWould you like to disable the IMAP Trash mailbox?\n\nOK-\nCancel-"}, /* IMAP_MISSING_TRASH */
  {32680, "Couldn't find a Junk mailbox for %p.\nWould you like to disable SpamWatch for this IMAP personality?\n\nOK-\nCancel-"}, /* IMAP_MISSING_JUNK */
  {32681, "unknown"}, /* UNKNOWN_CHARSET_NAME */
  {32682, "20"}, /* IMAP_AUTOEXPUNGE_THRESHOLD */
  {32683, "Should Eudora do automatic EXPUNGE commands on your IMAP mailboxes?\nIf you don't know what an EXPUNGE command is, just click \"Yes\".  For more information, click the \"?\" button.\n\nNo-\nYes-"}, /* IMAP_AUTOEXPUNGE_WARNING */
  {32684, "%p*%d"}, /* RFC2184FMT */
  {32685, "64"}, /* MIN_LEFT_APPEND */
  {32686, "..."}, /* ELIDE_2184_STRING */
  {32687, "%p  (%p)"}, /* EMO_MENU_FMT */
  {32688, "au,br,tw,hk,uk"}, /* COUNTRY_DOMAINS */
  {32689, "5"}, /* SEARCH_DF_AGE */
  {32690, "4"}, /* SEARCH_DF_AGE_REL */
  {32691, "3"}, /* SEARCH_DF_AGE_SPFY */
  {32692, "0"}, /* SEARCH_DF_ATT */
  {32693, "3"}, /* SEARCH_DF_ATT_REL */
  {32694, "Discarding: \"%p\""}, /* DISCARD_LOG_FMT */
  {32695, "Rebuilding table of contents for \"%p\""}, /* REBUILDING_TOC */
  {32696, "www."}, /* URL_NOTNAUGHTY_PREFIXES */
  {32697, "com.com"}, /* URL_NAUGHTY_EXCEPTIONS */
  {32698, "4"}, /* HTML_MIN_IMAGE_SIZE */
  {32699, "1x1"}, /* HTML_BAD_IMAGE_DIMENSIONS */
};

#define STRING_TABLE_SIZE 1689

const char *string_table_lookup(uint16_t id) {
  int lo = 0, hi = STRING_TABLE_SIZE - 1;
  while (lo <= hi) {
    int mid = (lo + hi) / 2;
    if (string_table[mid].id == id) return string_table[mid].str;
    else if (string_table[mid].id < id) lo = mid + 1;
    else hi = mid - 1;
  }
  return NULL;
}

/* GetIndString replacement.
 * Original Mac encoding: strListID = 100*(id/100), index = id%100.
 * We reconstruct the flat ID and look it up. Result is Pascal string. */
void GetIndString_impl(unsigned char *dest, short strListID, short index) {
  if (!dest) return;
  dest[0] = 0;
  uint16_t id = (uint16_t)(strListID + index);
  const char *s = string_table_lookup(id);
  if (s) {
    size_t len = strlen(s);
    if (len > 255) len = 255;
    dest[0] = (unsigned char)len;
    memcpy(dest + 1, s, len);
    dest[len + 1] = '\0';
  }
}
