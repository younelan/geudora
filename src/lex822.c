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

#include "lex822.h"
#include "StringDefs.h"
#include "StringUtil.h"
#include "buildtoc.h"
#include "mime.h"
#include "mydefs.h"
#include "prefdefs.h"
#include "progress.h"
#include "toc.h"
#include "trans.h"
#include "util.h"

/* extern declaration provided in trans.h */
#include "util.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define NO_TABLE 0

#define FILE_NUM 54

#define FILE_NUM 54
/* Copyright (c) 1993 by QUALCOMM Incorporated */
/************************************************************************
 * lexical analysis of RFC 822 headers  (Blech)
 ************************************************************************/

/************************************************************************
 * lexical analysis of RFC 822 headers  (Blech)
 ************************************************************************/

#include "mime.h" // For Dec64

bool Fix1342(unsigned char *chars, long *len);
bool Translate1342(unsigned char *text, unsigned char *charset,
                   unsigned char encoding);

/*
 * this enum names the various character classes
 */
typedef enum {
  ALPHA822,
  DIGIT822,
  CTL822,
  CR822,
  LF822,
  SPACE822,
  HTAB822,
  QUOTE822,
  RPAR822,
  LPAR822,
  AT822,
  COMMA822,
  SEM822,
  COLON822,
  BSLSH822,
  DOT822,
  LSQU822,
  RSQU822,
  CHAR822,
  EQUAL822,
  SLASH822,
  QMARK822,
  CHARTYPE_LIMIT
} Char822Enum;

/*
 * When the analyzer is in CollectLine, this table tells it what state to
 * enter based on character class
 */
unsigned char LineTable[CHARTYPE_LIMIT] = {
    CollectAtom,    CollectAtom,    CollectSpecial, CollectSpecial,
    CollectSpecial, CollectLWSP,    CollectLWSP,    CollectQText,
    CollectComment, CollectSpecial, CollectSpecial, CollectSpecial,
    CollectSpecial, CollectSpecial, CollectSpecial, CollectSpecial,
    CollectDL,      CollectSpecial, CollectAtom,    CollectSpecial,
    CollectSpecial, CollectSpecial};

/*
 * classify characters.
 */
unsigned char Class822[256] = {
    /*   0 (.) */ CTL822,   /*   1 (.) */ CTL822,
    /*   2 (.) */ CTL822,
    /*   3 (.) */ CTL822,   /*   4 (.) */ CTL822,
    /*   5 (.) */ CTL822,
    /*   6 (.) */ CTL822,   /*   7 (.) */ CTL822,
    /*   8 (.) */ CTL822,
    /*   9 (.) */ HTAB822,  /*  10 (.) */ LF822,
    /*  11 (.) */ CTL822,
    /*  12 (.) */ CTL822,   /*  13 (.) */ CR822,
    /*  14 (.) */ CTL822,
    /*  15 (.) */ CTL822,   /*  16 (.) */ CTL822,
    /*  17 (.) */ CTL822,
    /*  18 (.) */ CTL822,   /*  19 (.) */ CTL822,
    /*  20 (.) */ CTL822,
    /*  21 (.) */ CTL822,   /*  22 (.) */ CTL822,
    /*  23 (.) */ CTL822,
    /*  24 (.) */ CTL822,   /*  25 (.) */ CTL822,
    /*  26 (.) */ CTL822,
    /*  27 (.) */ CTL822,   /*  28 (.) */ CTL822,
    /*  29 (.) */ CTL822,
    /*  30 (.) */ CTL822,   /*  31 (.) */ CTL822,
    /*  32 ( ) */ SPACE822,
    /*  33 (!) */ CHAR822,  /*  34 (") */ QUOTE822,
    /*  35 (#) */ CHAR822,
    /*  36 ($) */ CHAR822,  /*  37 (%) */ CHAR822,
    /*  38 (&) */ CHAR822,  /*  39 (') */ CHAR822,
    /*  40 (() */ LPAR822,
    /*  41 ()) */ RPAR822,  /*  42 (*) */ CHAR822,
    /*  43 (+) */ CHAR822,
    /*  44 (,) */ COMMA822, /*  45 (-) */ CHAR822,
    /*  46 (.) */ DOT822,
    /*  47 (/) */ SLASH822, /*  48 (0) */ DIGIT822,
    /*  49 (1) */ DIGIT822,
    /*  50 (2) */ DIGIT822, /*  51 (3) */ DIGIT822,
    /*  52 (4) */ DIGIT822,
    /*  53 (5) */ DIGIT822, /*  54 (6) */ DIGIT822,
    /*  55 (7) */ DIGIT822,
    /*  56 (8) */ DIGIT822, /*  57 (9) */ DIGIT822,
    /*  58 (:) */ COLON822,
    /*  59 (;) */ SEM822,   /*  60 (<) */ CHAR822,
    /*  61 (=) */ EQUAL822,
    /*  62 (>) */ CHAR822,  /*  63 (?) */ QMARK822,
    /*  64 (@) */ AT822,
    /*  65 (A) */ ALPHA822, /*  66 (B) */ ALPHA822,
    /*  67 (C) */ ALPHA822,
    /*  68 (D) */ ALPHA822, /*  69 (E) */ ALPHA822,
    /*  70 (F) */ ALPHA822,
    /*  71 (G) */ ALPHA822, /*  72 (H) */ ALPHA822,
    /*  73 (I) */ ALPHA822,
    /*  74 (J) */ ALPHA822, /*  75 (K) */ ALPHA822,
    /*  76 (L) */ ALPHA822,
    /*  77 (M) */ ALPHA822, /*  78 (N) */ ALPHA822,
    /*  79 (O) */ ALPHA822,
    /*  80 (P) */ ALPHA822, /*  81 (Q) */ ALPHA822,
    /*  82 (R) */ ALPHA822,
    /*  83 (S) */ ALPHA822, /*  84 (T) */ ALPHA822,
    /*  85 (U) */ ALPHA822,
    /*  86 (V) */ ALPHA822, /*  87 (W) */ ALPHA822,
    /*  88 (X) */ ALPHA822,
    /*  89 (Y) */ ALPHA822, /*  90 (Z) */ ALPHA822,
    /*  91 ([) */ LSQU822,
    /*  92 (\) */ BSLSH822, /*  93 (]) */ RSQU822,
    /*  94 (^) */ CHAR822,
    /*  95 (_) */ CHAR822,  /*  96 (`) */ CHAR822,
    /*  97 (a) */ ALPHA822,
    /*  98 (b) */ ALPHA822, /*  99 (c) */ ALPHA822,
    /* 100 (d) */ ALPHA822,
    /* 101 (e) */ ALPHA822, /* 102 (f) */ ALPHA822,
    /* 103 (g) */ ALPHA822,
    /* 104 (h) */ ALPHA822, /* 105 (i) */ ALPHA822,
    /* 106 (j) */ ALPHA822,
    /* 107 (k) */ ALPHA822, /* 108 (l) */ ALPHA822,
    /* 109 (m) */ ALPHA822,
    /* 110 (n) */ ALPHA822, /* 111 (o) */ ALPHA822,
    /* 112 (p) */ ALPHA822,
    /* 113 (q) */ ALPHA822, /* 114 (r) */ ALPHA822,
    /* 115 (s) */ ALPHA822,
    /* 116 (t) */ ALPHA822, /* 117 (u) */ ALPHA822,
    /* 118 (v) */ ALPHA822,
    /* 119 (w) */ ALPHA822, /* 120 (x) */ ALPHA822,
    /* 121 (y) */ ALPHA822,
    /* 122 (z) */ ALPHA822, /* 123 ({) */ CHAR822,
    /* 124 (|) */ CHAR822,
    /* 125 (}) */ CHAR822,  /* 126 (~) */ CHAR822,
    /* 127 ( ) */ CHAR822,
    /* 128 () */ CHAR822,   /* 129 () */ CHAR822,
    /* 130 () */ CHAR822,
    /* 131 () */ CHAR822,   /* 132 () */ CHAR822,
    /* 133 () */ CHAR822,
    /* 134 () */ CHAR822,   /* 135 () */ CHAR822,
    /* 136 () */ CHAR822,
    /* 137 () */ CHAR822,   /* 138 () */ CHAR822,
    /* 139 () */ CHAR822,
    /* 140 () */ CHAR822,   /* 141 () */ CHAR822,
    /* 142 () */ CHAR822,
    /* 143 () */ CHAR822,   /* 144 () */ CHAR822,
    /* 145 () */ CHAR822,
    /* 146 () */ CHAR822,   /* 147 () */ CHAR822,
    /* 148 () */ CHAR822,
    /* 149 () */ CHAR822,   /* 150 () */ CHAR822,
    /* 151 () */ CHAR822,
    /* 152 () */ CHAR822,   /* 153 () */ CHAR822,
    /* 154 () */ CHAR822,
    /* 155 () */ CHAR822,   /* 156 () */ CHAR822,
    /* 157 () */ CHAR822,
    /* 158 () */ CHAR822,   /* 159 () */ CHAR822,
    /* 160 () */ CHAR822,
    /* 161 () */ CHAR822,   /* 162 () */ CHAR822,
    /* 163 () */ CHAR822,
    /* 164 () */ CHAR822,   /* 165 () */ CHAR822,
    /* 166 () */ CHAR822,
    /* 167 () */ CHAR822,   /* 168 () */ CHAR822,
    /* 169 () */ CHAR822,
    /* 170 () */ CHAR822,   /* 171 () */ CHAR822,
    /* 172 () */ CHAR822,
    /* 173 () */ CHAR822,   /* 174 () */ CHAR822,
    /* 175 () */ CHAR822,
    /* 176 () */ CHAR822,   /* 177 () */ CHAR822,
    /* 178 () */ CHAR822,
    /* 179 () */ CHAR822,   /* 180 () */ CHAR822,
    /* 181 () */ CHAR822,
    /* 182 () */ CHAR822,   /* 183 () */ CHAR822,
    /* 184 () */ CHAR822,
    /* 185 () */ CHAR822,   /* 186 () */ CHAR822,
    /* 187 () */ CHAR822,
    /* 188 () */ CHAR822,   /* 189 () */ CHAR822,
    /* 190 () */ CHAR822,
    /* 191 () */ CHAR822,   /* 192 () */ CHAR822,
    /* 193 () */ CHAR822,
    /* 194 () */ CHAR822,   /* 195 () */ CHAR822,
    /* 196 () */ CHAR822,
    /* 197 () */ CHAR822,   /* 198 () */ CHAR822,
    /* 199 () */ CHAR822,
    /* 200 () */ CHAR822,   /* 201 () */ CHAR822,
    /* 202 () */ CHAR822,
    /* 203 () */ CHAR822,   /* 204 () */ CHAR822,
    /* 205 () */ CHAR822,
    /* 206 () */ CHAR822,   /* 207 () */ CHAR822,
    /* 208 () */ CHAR822,
    /* 209 () */ CHAR822,   /* 210 () */ CHAR822,
    /* 211 () */ CHAR822,
    /* 212 () */ CHAR822,   /* 213 () */ CHAR822,
    /* 214 () */ CHAR822,
    /* 215 () */ CHAR822,   /* 216 () */ CHAR822,
    /* 217 () */ CHAR822,
    /* 218 () */ CHAR822,   /* 219 () */ CHAR822,
    /* 220 () */ CHAR822,
    /* 221 () */ CHAR822,   /* 222 () */ CHAR822,
    /* 223 () */ CHAR822,
    /* 224 () */ CHAR822,   /* 225 () */ CHAR822,
    /* 226 () */ CHAR822,
    /* 227 () */ CHAR822,   /* 228 () */ CHAR822,
    /* 229 () */ CHAR822,
    /* 230 () */ CHAR822,   /* 231 () */ CHAR822,
    /* 232 () */ CHAR822,
    /* 233 () */ CHAR822,   /* 234 () */ CHAR822,
    /* 235 () */ CHAR822,
    /* 236 () */ CHAR822,   /* 237 () */ CHAR822,
    /* 238 () */ CHAR822,
    /* 239 () */ CHAR822,   /* 240 () */ CHAR822,
    /* 241 () */ CHAR822,
    /* 242 () */ CHAR822,   /* 243 () */ CHAR822,
    /* 244 () */ CHAR822,
    /* 245 () */ CHAR822,   /* 246 () */ CHAR822,
    /* 247 () */ CHAR822,
    /* 248 () */ CHAR822,   /* 249 () */ CHAR822,
    /* 250 () */ CHAR822,
    /* 251 () */ CHAR822,   /* 252 () */ CHAR822,
    /* 253 () */ CHAR822,
    /* 254 () */ CHAR822,   /* 255 () */ CHAR822};

/*
 * some defines to make it easier to access fields
 */
#define State l822p->state
#define Token l822p->token
#define Buffer l822p->buffer
#define Spot l822p->spot
#define End l822p->end
#define InStructure l822p->inStructure
#define UhOh l822p->uhOh
#define Has1522 l822p->has1522
#define ReinitToken l822p->reinitToken

/*
 * for incomplete tokens, we must return a token based on what
 * state the analyzer is in
 */
unsigned char State2Token[] = {
    /* Init822 */ ErrorToken,
    /* CollectLine */ ErrorToken,
    /* CollectLWSP */ LinearWhite,
    /* CollectAtom */ Atom,
    /* CollectComment */ t822Comment,
    /* CollectQText */ QText,
    /* CollectDL */ DomainLit,
    /* CollectSpecial */ Special,
    /* CollectText */ RegText,
    /* ReceiveError */ ErrorToken};

Char822Enum LexFill(TransStream stream, L822SPtr l822p, AccuPtr rawBytes);

/************************************************************************
 * Lex822 - lexical analyzer for RFC 822 header fields
 ************************************************************************/
Token822Enum Lex822(TransStream stream, L822SPtr l822p, AccuPtr fullHeaders) {
  Char822Enum charClass, nextClass;
  Token822Enum returnToken = Continue822;
  State822Enum origState = State;

#define ADD(c) Token[++*Token] = c
#define NEXTCLASS(stream)                                                      \
  (Spot < End - 1 ? Class822[Spot[1]] : LexFill(stream, l822p, fullHeaders))

  /*
   * initialize
   */
  if (State == Init822) {
    State = CollectLine;
    Spot = End = Buffer;
    InStructure = 0;
    UhOh = Has1522 = false;
  }
  ReinitToken = true;

  /*
   * main processing loop
   */
  do {
    /* grab chars if need be */
    if (Spot >= End)
      LexFill(stream, l822p, fullHeaders);

    /* size up the situation */
    charClass = Class822[*Spot];

    /* empty header? */
    if (charClass == CR822 && origState == Init822)
      return (EndOfHeader);

    nextClass = NEXTCLASS(stream);

    /* errors fetching chars? */
    if (State == ReceiveError)
      return (ErrorToken);

    /* early termination? */
    if (UhOh && nextClass == CR822 ||
        origState == Init822 && charClass == DOT822 && nextClass == CR822) {
      /* we did, we did tee a putty tat */
      State = Init822;
      return (EndOfMessage);
    }
    UhOh = charClass == CR822 && nextClass == DOT822;

    /* Do we need to clear the token buffer? */
    if (ReinitToken)
      *Token = InStructure = ReinitToken = 0;

    /* are we going to overrun the token buffer? */
    if (*Token > sizeof(Token) - 3) {
      ReinitToken = 1;
      returnToken = State2Token[State];
      break;
    }

    /*
     * Is this a folded CR?
     */
    if (charClass == CR822 && (nextClass == SPACE822 || nextClass == HTAB822))
      charClass = SPACE822; /* treat it like a space */

    /*
     * ok, process the character
     */
    switch (State) {
    case Init822:
    case ReceiveError:
    case State822Limit:
      break;
    /*********************************/
    case CollectAtom:
      switch (charClass) {
      case ALPHA822:
      case DIGIT822:
      case CHAR822:
      case DOT822:    /* RFC 822 has this as special, MIME does not */
        ADD(*Spot++); /* add char to atom */
        break;

      default:
        returnToken = Atom;
        State = CollectLine;
        break;
      }
      break;

    /*********************************/
    case CollectLWSP:
      switch (charClass) {
      case SPACE822:
      case HTAB822:
        ADD(*Spot++); /* add char to lwsp */
        break;

      default:
        returnToken = LinearWhite;
        State = CollectLine;
        break;
      }
      break;

    /*********************************/
    case CollectComment:
      if (Token[*Token] == '\\')
        ADD(*Spot++);
      else
        switch (charClass) {
        case RPAR822:
          ADD(*Spot++);
          if (!--InStructure) {
            returnToken = t822Comment;
            State = CollectLine;
          }
          break;

        case LPAR822:
          ADD(*Spot++);
          InStructure++;
          break;

        case CR822:
          InStructure = 0;
          returnToken = t822Comment;
          State = CollectLine;
          break;

        default:
          ADD(*Spot++);
          break;
        }
      break;

    /*********************************/
    case CollectQText:
      if (Token[*Token] == '\\')
        ADD(*Spot++);
      else
        switch (charClass) {
        case QUOTE822:
          Spot++; /* skip the quote */
          if (!InStructure) {
            InStructure++; /* we weren't in a quote, but we are now */
            break;
          }
          /* quote has ended; fall-through is deliberate */

        case CR822:
          returnToken = QText;
          State = CollectLine;
          break;

        case SPACE822:
          if (*Spot == '\015') // ignore folded CR in middle of QTEXT
          {
            Spot++;
            break;
          }
          // fall through...
        default:
          ADD(*Spot++);
          break;
        }
      break;

    /*********************************/
    case CollectDL:
      if (Token[*Token] == '\\')
        ADD(*Spot++);
      else
        switch (charClass) {
        case LSQU822:
          Spot++; /* skip the [ */
          break;

        case RSQU822:
          Spot++; /* skip the ], and fall through */

        case CR822:
          returnToken = DomainLit;
          State = CollectLine;
          break;

        default:
          ADD(*Spot++);
          break;
        }
      break;

    /*********************************/
    case CollectSpecial:
      /* are we looking at the end of the world as we know it? */
      if (charClass == CR822 && nextClass == CR822) {
        State = Init822;
        returnToken = EndOfHeader;
      } else if (charClass == QUOTE822)
        State = CollectQText;
      else if (charClass == LPAR822)
        State = CollectComment;
      else {
        ADD(*Spot++);
        returnToken = Special;
        State = CollectLine;
      }
      break;

    /*********************************/
    case CollectText:
      switch (charClass) {
      case CR822:
        returnToken = RegText;
        State = CollectLine;
        break;

      default:
        ADD(*Spot++);
        break;
      }
      break;

    /*********************************/
    case CollectLine:
      State = LineTable[charClass];
      ReinitToken = true;
      break;
    }
  } while (returnToken == Continue822);
  return (returnToken);
}

/************************************************************************
 * LexFill - grab some chars for the lexical analyzer
 *  returns the character class of the first of the new chars
 *  if no chars, returns CR (since this will eventually terminate the analyzer)
 *    and sets state to ReceiveError.
 ************************************************************************/
Char822Enum LexFill(TransStream stream, L822SPtr l822p, AccuPtr rawBytes) {
  unsigned char *readHere;
  long size;

  if (State == ReceiveError)
    return (CR822); /* once an error, always an error */

  /*
   * if we have a leftover char, move it to front of buffer and put new
   * bytes after it.  Else, just read into entire buffer
   */
  if (Spot < End) {
    *Buffer = *Spot;
    Spot = Buffer;
    readHere = Spot + 1;
  } else
    readHere = Spot = Buffer;

  /*
   * receive chars
   */
  size = sizeof(Buffer) - (Spot - Buffer) - 2;
  if (RecvLine(stream, readHere, &size) || !size) {
    State = ReceiveError; /* Abort.  Things are bad. */
    *readHere = '\015';   /* back out of current token. */
    size = 0;             /* ignore anything we might have gotten */
  } else if (IsFromLine(readHere)) {
    BMD(readHere, readHere + 1, ++size);
    *readHere = '>'; /* escape envelope */
  }
#ifdef BITNET_MATTERS_ANYMORE
  /*
   * ok, this is a bit non-standard--treat a line with a single space
   * as though it were a blank line.
   */
  else if (size == 2 && readHere[0] == ' ' && readHere[1] == '\015') {
    *readHere = '\015';
    size--;
  }
#endif

  /*
   * save the raw bytes now
   */
  if (AccuAddPtr(rawBytes, readHere, size)) {
    *readHere = '\015';
    State = ReceiveError;
    size = 0;
  }

#ifndef SAVE_MIME
  /*
   * This isn't strictly kosher, but we're going to do it here anyway,
   * because I can't imagine how to do this the "right" way, which is
   * to integrate it with the lexing & grammar.  So shoot me.
   */
  Has1522 = Fix1342(readHere, &size) || Has1522;
#endif

  ByteProgress(NULL, -size, 0);

  End = readHere + size;
  return (Class822[*readHere]);
}

/************************************************************************
 * Fix1342 - translate RFC 1342 stuff
 ************************************************************************/
bool Fix1342(unsigned char *chars, long *len) {
  unsigned char *equal, *lastEqual;
  unsigned char *q[4];
  unsigned char **thisQ;
  unsigned char *end = chars + *len;
  unsigned char *spot = chars;
  unsigned char text[256];
  unsigned char charset[32];
  char encoding;
  unsigned char *nextEqual;
  bool found = false;

  while (spot < end) {
    /*
     * scan for first =
     */
    while (end - spot > 8 && *spot != '=')
      spot++;
    if (end - spot <= 8)
      return (found); /* done */
    equal = spot;

    /*
     * is next char a ?
     */
    if (spot[1] != '?') {
      spot++;
      continue;
    }
    q[0] = spot + 1;

    /*
     * find the remaining three ?'s
     */
    for (thisQ = q + 1; thisQ < q + 4; thisQ++) {
      for (thisQ[0] = thisQ[-1] + 1; **thisQ != '?'; ++*thisQ)
        if (end - *thisQ < 2)
          return (found);
    }

    /*
     * is the character after the last ? an =?
     */
    if (q[3][1] != '=') {
      spot += 2;
      continue;
    }
    lastEqual = q[3] + 1;

    /*
     * is the encoding method a single char?
     */
    if (q[2] - q[1] != 2) {
      spot += 2;
      continue;
    }

    /*
     * They're here.
     */
    found = true;
    encoding = q[1][1];
    MakePStr(charset, q[0] + 1, q[1] - q[0] - 1);
    MakePStr(text, q[2] + 1, q[3] - q[2] - 1);
    if (!PrefIsSet(PREF_ALWAYS_CHARSET) &&
        Translate1342(text, charset, encoding)) {
      /* move new chars into place */
      BMD(text + 1, equal, *text);
      /* check for stuff we should or should not remove */
      // This newline stripping code just wasn't working correctly; pulled for
      // 601. if (lastEqual[1]=='\015') lastEqual++; /* toast a trailing return
      // */ else
      { /* if the next thing is an encoded word, toast intervening */
        for (nextEqual = lastEqual + 1; nextEqual < end; nextEqual++)
          if (nextEqual[0] == '=' && nextEqual[1] == '?') {
            lastEqual = nextEqual - 1;
            break;
          } else if (*nextEqual != ' ' && *nextEqual != '\t')
            break;
        // This newline stripping code just wasn't working correctly; pulled for
        // 601. else if (*nextEqual!=' ' && *nextEqual!='\t' &&
        // *nextEqual!='\015') break;
      }
      /* move chars from after encoded text to after decoded text */
      BMD(lastEqual + 1, equal + *text, end - lastEqual - 1);
      /* adjust end to account for deleted chars */
      end -= lastEqual - equal + 1 - *text;
      *len = end - chars;
      /* adjust lastEqual to point to last decoded char */
      lastEqual = equal + *text - 1;
#ifdef NEVER
      if (*lastEqual == ' ' || *lastEqual == '\012')
        lastEqual++;
#endif
    }
    spot = lastEqual + 1;
  }
  return (found);
}

/************************************************************************
 * Translate1342 - translate a string according to the 1342 rules
 *  returns whether or not the string was successfully translated
 ************************************************************************/
bool Translate1342(unsigned char *text, unsigned char *charset,
                   unsigned char encoding) {
  short tableId = FindMIMECharset(charset);

  /*
   * do we have the charset?
   */
  if (tableId == NO_TABLE && !EqualStrRes(charset, MIME_USASCII) &&
      !EqualStrRes(charset, MIME_MAC))
    return (false);

  /*
   * first, we undo the encoding
   */
  switch (encoding) {
  case 'Q':
  case 'q':
    PseudoQP(text);
    break;
  case 'B':
  case 'b':
    if (DecodeB64String(text))
      return (false); /* decode errors abort */
    break;
  default:
    return (false);
    break;
  }

  /*
   * now, we transliterate
   */
  if (tableId != NO_TABLE)
    TransLitRes(text + 1, *text, tableId);

  //
  // some bad people put commas in here -- protect ourselves
  if (PIndex((PStr)text, ',') && *text < 253 && text[1] != '"') {
    PInsertC((PStr)text, 256, '"', text + 1);
    PCatC((PStr)text, (unsigned char)'"');
  }

  /*
   * there.  that didn't hurt.  all that much.  at least not that you can see.
   */
  return (true);
}

/************************************************************************
 * PseudoQP - quoted-printable, kind of
 ************************************************************************/
void PseudoQP(unsigned char *text) {
  unsigned char *spot = text;
  unsigned char *end = text + strlen((const char *)text);
  unsigned char *copySpot = spot;

  while (spot < end) {
    switch (*spot) {
    case '_':
      *copySpot++ = ' ';
      break;
    default:
      *copySpot++ = *spot;
      break;
    case '=':
      if (end - spot < 3)
        *copySpot++ = *spot;
      else {
        Hex2Bytes(spot + 1, 2, copySpot++);
        spot += 2;
      }
      break;
    }
    spot++;
  }
  *copySpot = '\0';
}

/************************************************************************
 * DecodeB64String - decode a base64 string
 ************************************************************************/
short DecodeB64String(unsigned char *s) {
  Dec64 d64;
  unsigned char sPrime[256];
  long len;
  long result;

  Zero(d64);
  result = Decode64(s, strlen((const char *)s), sPrime, &len, &d64, true);
  if ((d64.decoderState + d64.padCount) % 4)
    result++;

  if (!result) {
    sPrime[len] = '\0';
    strcpy((char *)s, (char *)sPrime);
  }
  return (result);
}

/**********************************************************************
 * Quote822 - quote a string, 822-style, if need be
 **********************************************************************/
unsigned char *Quote822(unsigned char *into, unsigned char *from, bool space) {
  unsigned char *spot, *end;
  unsigned char *intoSpot, *intoEnd;
  Char822Enum class;

  end = from + strlen((const char *)from);
  for (spot = from; spot < end; spot++) {
    class = Class822[*spot];
    switch (class) {
    case ALPHA822:
    case DIGIT822:
    case SLASH822:
    case EQUAL822:
    case QMARK822:
      break;
    case CHAR822:
      if (*spot == '<' || *spot == '>')
        goto out;
      break;
    case SPACE822:
      if (space)
        break;
      /* else fall */
    default:
      goto out;
    }
  }

out:
  if (spot < end) {
    /* gotta quote */
    intoSpot = into;
    intoEnd = intoSpot + 253;
    *intoSpot++ = '"';
    for (spot = from; spot < end && intoSpot < intoEnd; spot++) {
      if (*spot == '"')
        *intoSpot++ = '\\';
      *intoSpot++ = *spot;
    }
    *intoSpot++ = '"';
    *intoSpot = '\0';
  } else
    strcpy((char *)into, (const char *)from);

  return (into);
}
