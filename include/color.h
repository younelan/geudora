/* Copyright (c) 2017, Computer History Museum 
All rights reserved. 
Redistribution and use in source and binary forms, with or without modification, are permitted (subject to 
the limitations in the disclaimer below) provided that the following conditions are met: 
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer. 
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following 
   disclaimer in the documentation and/or other materials provided with the distribution. 
 * Neither the name of Computer History Museum nor the names of its contributors may be used to endorse or promote products 
   derived from this software without specific prior written permission. 
NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE 
COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH 
DAMAGE. */

#ifndef COLOR_H
#define COLOR_H
#include "mywindow.h"
#include "emsapi-mac.h"

typedef enum {
	e3DNone,
	e3DSlight,
	e3DOverBearing
} D3EffectEnum;

enum {
	wHiliteColorLight = 5,
	wHiliteColorDark,
	wTitleBarLight,
	wTitleBarDark,
	wDialogLight,
	wDialogDark,
	wTingeLight,
	wTingeDark
};

void SetMenuColor(EUDORA_MenuHandle menu, short item, EUDORA_RGBColor *color);

EUDORA_RGBColor *GetLabelColor(short index, EUDORA_RGBColor *color);
int MyGetLabel(short labelNumber, EUDORA_RGBColor *color, char * labelString);

void Color3DRect(EUDORA_Rect *r, EUDORA_RGBColor *color, D3EffectEnum howMuch, bool raised);
void TwoToneFrame(EUDORA_Rect *r, EUDORA_RGBColor *topLeft, EUDORA_RGBColor *botRight);
#define BlackWhite(c) (Black(c)||White(c))
bool Black(EUDORA_RGBColor *color);
bool White(EUDORA_RGBColor *color);
EUDORA_RGBColor *LightenColor(EUDORA_RGBColor *color, short percent);
EUDORA_RGBColor *DarkenColor(EUDORA_RGBColor *color, short percent);
EUDORA_RGBColor *LimitColorRange(EUDORA_RGBColor *color);
EUDORA_RGBColor *PastelColor(EUDORA_RGBColor *color);
EUDORA_RGBColor *SetRGBGrey(EUDORA_RGBColor *color, short greyValue);
void SetForeGrey(short greyValue);
void SetBGGrey(short greyValue);
EUDORA_RGBColor *SetRGBGrey(EUDORA_RGBColor *color, short greyValue);
short LightestGrey(EUDORA_Rect *r);
int ColorParam(EUDORA_RGBColor *color, char * text);
bool ColorIsLight(EUDORA_RGBColor *color);

bool ColCtlPicker(EUDORA_ControlHandle cntl);
void ColCtlSet(EUDORA_ControlHandle cntl, EUDORA_RGBColor *color);
EUDORA_RGBColor *ColCtlGet(EUDORA_ControlHandle cntl, EUDORA_RGBColor *color);
void DrawDivider(EUDORA_Rect *r, bool raised);
void WinGreyBG(MyWindowPtr win);

void Frame3DOrNot(EUDORA_Rect *r, EUDORA_RGBColor *baseColor, bool erase);

#define k8Grey1			61166
#define	k8Grey2			56797
#define	k8Grey3			52428
#define	k8Grey4			48059
#define	k8Grey5			43690
#define	k8Grey6			34952
#define	k8Grey7			30583
#define	k8Grey8			21845
#define	k8Grey9			17476
#define	k8Grey10			8738
#define	k8Grey11			4369

#define	k4Grey1			49152
#define	k4Grey2			32768
#define	k4Grey3			8192

#endif
