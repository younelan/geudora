/* toc.h — TOC constants and function declarations
 * Structs (MacmbxTOC, MacmbxMsgSum) are defined in macmbx.h
 * which is included via mailbox.h.
 */

#ifndef TOC_H
#define TOC_H

#include "mailbox.h"
#include "schizo.h"
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

/* Special TOC access — via macmbx store by type */
#include "gtk_mailbox.h"
static inline MacmbxTOC *_open_special(MacmbxType t) {
  MacmbxStore *s = gtk_mailbox_get_store();
  MacmbxNode *n = s ? macmbx_store_find_special(s, t) : NULL;
  return n ? macmbx_toc_open(n->path) : NULL;
}
#define GetRealInTOC()    _open_special(MACMBX_TYPE_IN)
#define GetRealOutTOC()   _open_special(MACMBX_TYPE_OUT)
#define GetTempInTOC()    _open_special(MACMBX_TYPE_IN_TEMP)
#define GetTempOutTOC()   _open_special(MACMBX_TYPE_OUT_TEMP)
#define GetInTOC()        GetRealInTOC()
#define GetOutTOC()       GetRealOutTOC()
#define GetTrashTOC()     _open_special(MACMBX_TYPE_TRASH)
#define GetJunkTOC()      _open_special(MACMBX_TYPE_JUNK)

/* Constants */
#define kNeverHashed 0
#define kNoMessageId 0
#define kSearchMB 1
#define kDontResort   0
#define kNoSlowResort 1
#define kResortNow    2
#define kResortWhenever 3
#define MBX_IN 1
#define MBX_OUT 2
#define MBX_TRASH 3
#define MBX_JUNK 4
#define MBX_IN_TEMP 11
#define MBX_OUT_TEMP 12

/* Message flags */
#define FLAG_READ 0x0001
#define MSG_FLAG_DELETED 0x0002

/* Message options */
#define OPT_DELETED 0x0001
#define OPT_HTML 0x0002
#define OPT_EMSR_DELETE_REQUESTED 11042
#define OPT_ORPHAN_ATT (1 << 9)
#define OPT_FLOW 0x0004
#define OPT_CHARSET 0x0008
#define OPT_ATT_DEL (1 << 6)
#define OPT_INLINE_SIG 0x0100
#define UseFlowInExcerpt 0

/* Preference bit IDs */
#define TOC_PREF_JUNK_MAILBOX 1
#define TOC_PREF_REPORT 2
#define TOC_PREF_LMOS 3
#define EMSF_JUNK_MAIL_ID 10
#define featureJunk 1

/* Function prototypes — implementations in macmbx or Eudora source */
void InvalSum(MacmbxTOC *tocH, short sumNum);
MacmbxTOC *IsTOCValid(MacmbxTOC *testTOC);
short FindSumByHash(MacmbxTOC *tocH, uint32_t hash);
MacmbxTOC *GetSpecialTOC(short nameId);
MacmbxTOC *GetRealTOC(MacmbxTOC *tocH, short sum, short *realSum);
char *GetMailboxSpec(MacmbxTOC *tocH, short num, char *outSpec);
void BoxSetSummarySelected(MacmbxTOC *tocH, short sum, bool select);
int GetPrefBit(short prefId, int bit);
int GetPrefBitNoDominant(short prefId, int bit);
#undef PrefIsSet
bool PrefIsSet(short prefId);
void UseFeature(short featureId);
void RedateTS(MacmbxTOC *tocH, short sum);
int MoveSelectedMessagesLo(MacmbxTOC *tocH, char *dest, bool a, bool b, bool c, bool d);
int MoveSelectedMessages(MacmbxTOC *tocH, char *dest, bool openIt);
int MoveMessageLo(MacmbxTOC *tocH, int sumNum, char *dest, bool copy, bool toTemp, bool holdOpen);
PersHandle TOCToPers(MacmbxTOC *tocH);
void TOCSetDirty(MacmbxTOC *tocH, bool dirty);
int TOCDates(char *spec, uLong *box, uLong *res, uLong *file);
void CopySum(MacmbxMsgSum *from, MacmbxMsgSum *to, short idx);
short TOCUnreadCount(MacmbxTOC *tocH, bool recentOnly);
bool AmTempToc(MacmbxTOC *tocH);
short FindSumBySerialNum(MacmbxTOC *tocH, long serialNum);

#endif /* TOC_H */
