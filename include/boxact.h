/* Copyright (c) 2017, Computer History Museum
All rights reserved. (BSD license — see source)
Ported to GTK4/GLib. TOCHandle → TOCType*, PStr → unsigned char* */

#ifndef BOXACT_H
#define BOXACT_H

#include "mailbox.h"
#include "message.h"
#include <stdbool.h>

#define kBoxSizeRefCon 'boxs'
#define kDrawerSwitch  'dwrs'
#define kConConProfRefCon 'ccpp'

void BoxUpdate(MyWindowPtr win);
void BoxClick(MyWindowPtr win, void *event);
void BoxActivate(MyWindowPtr win);
bool BoxMenu(MyWindowPtr win, int menu, int item, short modifiers);
bool BoxClose(MyWindowPtr win);
bool BoxFind(MyWindowPtr win, unsigned char *what);
void BoxOpen(MyWindowPtr win);
bool BoxKey(MyWindowPtr win, void *event);
void SelectBoxRange(TOCType *tocH, int start, int end, bool cmd,
                    int eStart, int eEnd);
void BoxSetSummarySelected(TOCType *tocH, short sumNum, bool selected);
void BoxCenterSelection(MyWindowPtr win);
void BoxSelectAfter(MyWindowPtr win, short mNum);
int BoxPosition(MyWindowPtr win);
void BoxSelectSame(TOCType *tocH, short item, short clickedSum);
void MakeMessFileName(TOCType *tocH, short sumNum, unsigned char *name);
void BoxHelp(MyWindowPtr win, Point mouse);
void BoxDidResize(MyWindowPtr win, Rect *oldContR);
void BoxListFocus(TOCType *tocH, bool focus);
int BoxGonnaShow(MyWindowPtr win);
void SetPriority(TOCType *tocH, short sumNum, short priority);
short Item2Status(short item);
short Status2Item(short status);
void InvalTocBox(TOCType *tocH, short sumNum, short box);
bool RedoTOC(TOCType *tocH);
void RedoAllTOCs(void);
void MBResort(TOCType *tocH);
void CheckSortItems(MyWindowPtr win);
Handle MenuItem2Handle(short menu, short item);
void ServerMenuChoice(TOCType *tocH, short sumNum, short item,
                      bool shiftPressed);
void BeenThereDoneThat(TOCType *tocH, short sumNum);
bool BoxButton(MyWindowPtr win, GtkWidget *widget, GdkEvent *event);
bool BoxScroll(MyWindowPtr win, short h, short v);
bool BoxHasSelection(MyWindowPtr win);
void BoxIdle(MyWindowPtr win);
void BoxInitialSelection(TOCType *tocH);
int SubjCompare(unsigned char *s1, unsigned char *s2);

#endif /* BOXACT_H */
