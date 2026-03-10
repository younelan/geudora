/* Copyright (c) 2017, Computer History Museum
   All rights reserved.
   Redistribution and use in source and binary forms, with or without
   modification, are permitted (subject to the limitations in the disclaimer
   below) provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
    * Neither the name of Computer History Museum nor the names of its
   contributors may be used to endorse or promote products derived from this
   software without specific prior written permission. NO EXPRESS OR IMPLIED
   LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE. THIS
   SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#include "lineio.h"
#include "StringDefs.h"
#include "fileutil.h"
#include <string.h>
#define FILE_NUM 20
/* Copyright (c) 1990-1992 by the University of Illinois Board of Trustees */

// #pragma segment FileUtil

#define LIKE_BUFFER 8192

#define Buffer lip->buffer
#define BufferSize lip->bufferSize
#define BFilled lip->bFilled
#define FSpot lip->fSpot
#define BSpot lip->bSpot
#define LastSpot lip->lastSpot
#define Eof lip->eof
#define fd lip->fd

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MemError() 0
#define eofErr (-39)
#define CycleBalls()

/* Removed macro wrappers for Handles, using standard C pointers now */

short OpenLine(short vRef, long dirId, unsigned char *name, short perm,
               LineIOP lip) {
  /* Bridge legacy call to portable OpenLineDirect.
     We assume 'name' holds a usable path or filename.
     vRef and dirId are ignored as they are Mac legacy. */
  if (name == NULL)
    return -1;
  return OpenLineDirect((const char *)name, perm, lip);
}

short OpenLineDirect(const char *path, short perm, LineIOP lip) {
  int err = 0;
  int localFd;
  struct stat st;

  Zero(*lip);

  int flags = O_RDONLY;
  if (perm == fsWrPerm)
    flags = O_WRONLY;
  else if (perm == fsRdWrPerm)
    flags = O_RDWR;

  if ((localFd = open(path, flags)) < 0) {
    err = errno;
    goto failure;
  }
  fd = localFd;

  /*
   * allocate a buffer
   */
  if (fstat(fd, &st) < 0) {
    err = errno;
    goto failure;
  }
  BufferSize = st.st_size;
  BufferSize += 2;
  if (BufferSize > LIKE_BUFFER)
    BufferSize = LIKE_BUFFER;

  /* Direct malloc instead of NewHandle */
  if ((Buffer = malloc(BufferSize)) == NULL) {
    err = ENOMEM;
    goto failure;
  }

  /*
   * fill the first buffer
   */
  /* Read directly into buffer, no handle dereferencing */
  BFilled = read(fd, Buffer, BufferSize - 1);
  if (BFilled < 0) {
    err = errno;
    goto failure;
  }
  BSpot = LastSpot = FSpot = 0;
  Buffer[BFilled] = '\015'; /* a marker, to expedite searches */
  return (noErr);

failure:
  CloseLine(lip);
  return (err);
}

/************************************************************************
 * SeekLine - seek the line routines to a given spot
 ************************************************************************/
int SeekLine(long spot, LineIOP lip) {
  int err = 0;

  if (lseek(fd, spot, SEEK_SET) < 0) {
    err = errno;
    goto failure;
  }

  /*
   * fill the first buffer
   */
  BFilled = read(fd, Buffer, BufferSize - 1);
  if (BFilled < 0) {
    err = errno;
    goto failure;
  }
  Eof = 0;
  BSpot = 0;
  LastSpot = FSpot = spot;
  Buffer[BFilled] = '\015'; /* a marker, to expedite searches */
  return (noErr);

failure:
  CloseLine(lip);
  return (err);
}

/**********************************************************************
 * NLGetLine - get a line, possiby preceeded by a linefeed
 **********************************************************************/
int NLGetLine(unsigned char *line, int size, long *len, LineIOP lip) {
  short l = GetLine(line, size, len, lip);

  if (l == LINE_START && *len && *line == '\012') {
    memmove(line, line + 1, --*len); /* Replaces BMD(line + 1, line, --*len) */
    if (!*len)
      l = 0;
  }
  return (l);
}

/**********************************************************************
 * GetLine - read a line of a given size.  returns 0 for eof, negative
 * for file manager errors, LINE_START if returning the beginning of
 * a line, LINE_MIDDLE if a partial line is being returned.
 **********************************************************************/
int GetLine(unsigned char *line, int size, long *len, LineIOP lip) {
  register unsigned char *bp;
  register unsigned char *cp = line;
  int where;
  int err;

  if (!BFilled)
    return (0); /* we have no chars */
  size--;       // make sure we don't overrun buffer
  CycleBalls();

  /* Buffer is now a char pointer, no dereferencing needed */
  bp = (unsigned char *)Buffer + BSpot;
  where = (bp == (unsigned char *)Buffer || bp[-1] == '\015') ? LINE_START
                                                              : LINE_MIDDLE;
  LastSpot = FSpot + BSpot; /* remember where this line begins */
  for (;;) {
    while (*bp != '\015' && --size > 0) {
      *cp = *bp++;
      if (!*cp)
        *cp = ' ';
      cp++;
    }
    BSpot = bp - (unsigned char *)Buffer;
    if (BSpot == BFilled) {
      FSpot += BFilled;
      BFilled = read(fd, Buffer, BufferSize - 1);
      Eof = !BFilled;
      if (BFilled < 0) {
        err = errno;
        FileSystemError(READ_MBOX, "", err);
        return (err);
      }
      if (BFilled == 0) {
        *cp = 0;
        if (len)
          *len = cp - line;
        return (where);
      }
      Buffer[BFilled] = '\015'; /* a marker, to expedite searches */
      BSpot = 0;
      bp = (unsigned char *)Buffer;
    } else {
      if (size > 0) {
        *cp++ = '\015';
        BSpot++;
      }
      *cp = 0;
      if (len)
        *len = cp - line;
      return (where);
    }
  }
}

/**********************************************************************
 * CloseLine - shut up shop.	Calling it on closed file does no harm.
 **********************************************************************/
void CloseLine(LineIOP lip) {
  if (lip) {
    if (fd > 0) {
      close(fd);
      fd = 0;
    }
    if (Buffer != NULL) {
      free(Buffer); /* Standard C free */
      Buffer = NULL;
    }
    BFilled = 0;
    Zero(*lip);
  }
}

long TellLine(LineIOP lip) { return (LastSpot); }
