#ifndef _RESOURCE_MAP_H_
#define _RESOURCE_MAP_H_

#include <wx/bitmap.h>
#include <map>
#include <string>

void InitResources();
wxBitmap GetBitmap(const std::string& id);

#endif
