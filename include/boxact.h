/* Copyright (c) 2017, Computer History Museum
All rights reserved. (BSD license — see source)
Ported to GTK4/GLib. TOCHandle → MacmbxTOC*, char * → unsigned char* */

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
bool BoxFind(MyWindowPtr win, char *what);
void BoxOpen(MyWindowPtr win);
bool BoxKey(MyWindowPtr win, void *event);
void SelectBoxRange(MacmbxTOC *tocH, int start, int end, bool cmd,
                    int eStart, int eEnd);
void BoxSetSummarySelected(MacmbxTOC *tocH, short sumNum, bool selected);
void BoxCenterSelection(MyWindowPtr win);
void BoxSelectAfter(MyWindowPtr win, short mNum);
int BoxPosition(MyWindowPtr win);
void BoxSelectSame(MacmbxTOC *tocH, short item, short clickedSum);
void MakeMessFileName(MacmbxTOC *tocH, short sumNum, unsigned char *name);
void BoxHelp(MyWindowPtr win, Point mouse);
void BoxDidResize(MyWindowPtr win, Rect *oldContR);
void BoxListFocus(MacmbxTOC *tocH, bool focus);
int BoxGonnaShow(MyWindowPtr win);
void SetPriority(MacmbxTOC *tocH, short sumNum, short priority);
short Item2Status(short item);
short Status2Item(short status);
void InvalTocBox(MacmbxTOC *tocH, short sumNum, short box);
bool RedoTOC(MacmbxTOC *tocH);
void RedoAllTOCs(void);
void MBResort(MacmbxTOC *tocH);
void CheckSortItems(MyWindowPtr win);
void *MenuItem2Handle(short menu, short item);
void ServerMenuChoice(MacmbxTOC *tocH, short sumNum, short item,
                      bool shiftPressed);
void BeenThereDoneThat(MacmbxTOC *tocH, short sumNum);
bool BoxButton(MyWindowPtr win, GtkWidget *widget, GdkEvent *event);
bool BoxScroll(MyWindowPtr win, short h, short v);
bool BoxHasSelection(MyWindowPtr win);
void BoxIdle(MyWindowPtr win);
void BoxInitialSelection(MacmbxTOC *tocH);
int SubjCompare(unsigned char *s1, unsigned char *s2);

#endif /* BOXACT_H */
