#include "CKStbFontBackend.h"

#include <stdio.h>
#include <stdlib.h>

static void Require(XBOOL condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

int main()
{
    XClassArray<XString> dirs;
    dirs.PushBack(XString(TEST_FONT_DIR));

    XClassArray<CKStbSystemFontFace> faces;
    CKStbEnumerateFontDirectories(dirs, faces);
    Require(faces.Size() > 0, "expected vendored font directory to enumerate at least one font");

    CKStbSystemFontFace regular;
    Require(CKStbFindBestFontFace(faces, "Roboto", 400, FALSE, regular), "expected Roboto regular to be found by family");
    Require(regular.family == "Roboto", "expected Roboto family name to come from the font name table");
    Require(regular.path.Find("roboto-regular.ttf") != XString::NOTFOUND, "expected regular weight to select roboto-regular.ttf");

    CKStbSystemFontFace bold;
    Require(CKStbFindBestFontFace(faces, "Roboto", 700, FALSE, bold), "expected Roboto bold to be found by family");
    Require(bold.path.Find("roboto-bold.ttf") != XString::NOTFOUND, "expected bold weight to select roboto-bold.ttf");

    CKStbSystemFontFace fallback;
    Require(CKStbFindBestFontFace(faces, "Arial", 400, FALSE, fallback), "expected Arial alias to find a sans-serif fallback");

    return 0;
}
