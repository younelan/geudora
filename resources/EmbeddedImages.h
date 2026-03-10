#ifndef _EMBEDDED_IMAGES_H_
#define _EMBEDDED_IMAGES_H_

#include <stddef.h>

struct EmbeddedImage {
    const char* name;
    const unsigned char* data;
    size_t size;
};

extern const EmbeddedImage g_EmbeddedImages[];
extern const size_t g_EmbeddedImageCount;

#endif
