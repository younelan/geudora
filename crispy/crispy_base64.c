/* base64.c — Base64 encode/decode for SASL authentication
 * Part of maillib: standalone, no external dependencies.
 */

#include "crispy_smtp.h" /* for crispy_base64_encode/decode declarations */
#include <stdlib.h>
#include <string.h>

static const char b64_table[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const unsigned char b64_decode_table[256] = {
  ['A']=0,  ['B']=1,  ['C']=2,  ['D']=3,  ['E']=4,  ['F']=5,  ['G']=6,
  ['H']=7,  ['I']=8,  ['J']=9,  ['K']=10, ['L']=11, ['M']=12, ['N']=13,
  ['O']=14, ['P']=15, ['Q']=16, ['R']=17, ['S']=18, ['T']=19, ['U']=20,
  ['V']=21, ['W']=22, ['X']=23, ['Y']=24, ['Z']=25,
  ['a']=26, ['b']=27, ['c']=28, ['d']=29, ['e']=30, ['f']=31, ['g']=32,
  ['h']=33, ['i']=34, ['j']=35, ['k']=36, ['l']=37, ['m']=38, ['n']=39,
  ['o']=40, ['p']=41, ['q']=42, ['r']=43, ['s']=44, ['t']=45, ['u']=46,
  ['v']=47, ['w']=48, ['x']=49, ['y']=50, ['z']=51,
  ['0']=52, ['1']=53, ['2']=54, ['3']=55, ['4']=56, ['5']=57, ['6']=58,
  ['7']=59, ['8']=60, ['9']=61, ['+']=62, ['/']=63,
};

char *crispy_base64_encode(const char *in, long inLen, long *outLen) {
  long olen = 4 * ((inLen + 2) / 3);
  char *out = (char *)malloc(olen + 1);
  if (!out) return NULL;

  const unsigned char *src = (const unsigned char *)in;
  char *dst = out;

  for (long i = 0; i < inLen; i += 3) {
    unsigned int a = src[i];
    unsigned int b = (i + 1 < inLen) ? src[i + 1] : 0;
    unsigned int c = (i + 2 < inLen) ? src[i + 2] : 0;
    unsigned int triple = (a << 16) | (b << 8) | c;

    *dst++ = b64_table[(triple >> 18) & 0x3f];
    *dst++ = b64_table[(triple >> 12) & 0x3f];
    *dst++ = (i + 1 < inLen) ? b64_table[(triple >> 6) & 0x3f] : '=';
    *dst++ = (i + 2 < inLen) ? b64_table[triple & 0x3f] : '=';
  }

  *dst = '\0';
  if (outLen) *outLen = (long)(dst - out);
  return out;
}

char *crispy_base64_decode(const char *in, long inLen, long *outLen) {
  if (inLen < 0) inLen = (long)strlen(in);

  /* Skip whitespace, count valid chars */
  long validLen = 0;
  for (long i = 0; i < inLen; i++) {
    unsigned char c = (unsigned char)in[i];
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') || c == '+' || c == '/' || c == '=')
      validLen++;
  }

  long olen = 3 * (validLen / 4);
  char *out = (char *)malloc(olen + 1);
  if (!out) return NULL;

  unsigned char *dst = (unsigned char *)out;
  unsigned int buf = 0;
  int bits = 0;
  int pad = 0;

  for (long i = 0; i < inLen; i++) {
    unsigned char c = (unsigned char)in[i];
    if (c == '=') { pad++; buf <<= 6; bits += 6; }
    else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
             (c >= '0' && c <= '9') || c == '+' || c == '/') {
      buf = (buf << 6) | b64_decode_table[c];
      bits += 6;
    } else continue;

    if (bits >= 8) {
      bits -= 8;
      *dst++ = (unsigned char)((buf >> bits) & 0xFF);
    }
  }

  /* Remove padding bytes */
  long written = (long)(dst - (unsigned char *)out);
  written -= pad;
  if (written < 0) written = 0;

  out[written] = '\0';
  if (outLen) *outLen = written;
  return out;
}
