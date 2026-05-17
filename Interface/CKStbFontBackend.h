#ifndef CK_STB_FONT_BACKEND_H
#define CK_STB_FONT_BACKEND_H

#include "VxMath.h"

struct CKStbSystemFontFace
{
    XString family;
    XString style;
    XString path;
    int fontIndex;
    int weight;
    XBOOL italic;
};

void CKStbDefaultFontDirectories(XClassArray<XString> &directories);
void CKStbEnumerateFontDirectories(const XClassArray<XString> &directories,
                                   XClassArray<CKStbSystemFontFace> &faces);
void CKStbEnumerateSystemFonts(XClassArray<CKStbSystemFontFace> &faces);
XBOOL CKStbFindBestFontFace(const XClassArray<CKStbSystemFontFace> &faces,
                             const char *family,
                             int weight,
                             XBOOL italic,
                             CKStbSystemFontFace &result);

#endif
