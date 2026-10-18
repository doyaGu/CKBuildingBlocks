#include "CKAll.h"
#include "CKFontManager.h"
#include "VxFontSearch.h"
#include <stdio.h>
#include <string.h>
#include <locale.h>
#if defined(__linux__)
#include <cstdlib>
#include <new>
#include <dirent.h>
#include <sys/stat.h>

static size_t FailArraySize = 0;

void *operator new[](size_t size)
{
    if (FailArraySize && size == FailArraySize)
    {
        FailArraySize = 0;
        throw std::bad_alloc();
    }
    void *memory = std::malloc(size ? size : 1);
    if (!memory) throw std::bad_alloc();
    return memory;
}

void operator delete[](void *memory) noexcept { std::free(memory); }
void operator delete[](void *memory, size_t) noexcept { std::free(memory); }
#endif

static void Require(CKBOOL condition, const char *message)
{
    if (!condition) throw message;
}

#if defined(__linux__)
static int OpenFileCount()
{
    DIR *directory = opendir("/proc/self/fd");
    Require(directory != NULL, "open file descriptor directory");
    int count = 0;
    for (dirent *entry = readdir(directory); entry; entry = readdir(directory))
        if (entry->d_name[0] >= '0' && entry->d_name[0] <= '9') ++count;
    closedir(directory);
    return count;
}

static void CheckFailedFontRead(CKFontManager *manager, int noto)
{
    VxFontFace face;
    Require(VxFindFontFace(VxGetSystemFonts(), "Noto Sans", 700, FALSE, face), "failure test font");
    struct stat info;
    Require(stat(face.path.CStr(), &info) == 0 && info.st_size > 0, "font file size");
    int before = OpenFileCount();
    FailArraySize = (size_t)info.st_size;
    bool caught = false;
    try { manager->CreateFont("failed-font-read", noto, 700, FALSE, FALSE, 4, -24); }
    catch (const std::bad_alloc &) { caught = true; }
    FailArraySize = 0;
    Require(caught, "font buffer allocation failure was exercised");
    Require(OpenFileCount() == before, "failed font read closes the actual font file");
    Require(!manager->SelectFont("failed-font-read"), "failed font is not published");
    Require(manager->CreateFont("failed-font-read", noto, 700, FALSE, FALSE, 4, -24), "retry after font read failure");
    manager->DeleteFontHandle("failed-font-read");
}
#endif

struct Runtime
{
    CKContext *context = NULL;
    Runtime()
    {
        Require(CKStartUp() == CK_OK, "CK startup");
        Require(CKCreateContext(&context, NULL, 0, 0) == CK_OK, "CK context");
    }
    ~Runtime()
    {
        if (context) CKCloseContext(context);
        CKShutdown();
    }
};

int main()
{
    try
    {
        Require(setlocale(LC_CTYPE, "") != NULL, "native test locale");
        Runtime runtime;
        CKParameterManager *parameters = runtime.context->GetParameterManager();
        Require(parameters->RegisterNewEnum(CKPGUID_FONTNAME, "Font Name", "") == CK_OK, "font enumeration registration");
        CKFontManager *manager = new CKFontManager(runtime.context);
        Require(manager->RegenerateFontEnumeration(), "font enumeration");
        CKEnumStruct *fonts = parameters->GetEnumDescByType(parameters->ParameterGuidToType(CKPGUID_FONTNAME));
        Require(fonts && fonts->NbData > 0, "enumerated font families");
        int noto = -1;
        for (int i = 0; i < fonts->NbData; ++i)
        {
            XString unicode, original;
            Require(VxDecodeAnsiFontName(fonts->Desc[i], unicode), "native font enumeration");
            Require(VxEncodeAnsiFontName(unicode.CStr(), original) && original == fonts->Desc[i], "enumeration byte round trip");
            if (!strcmp(fonts->Desc[i], "Noto Sans")) noto = i;
        }
        Require(noto >= 0, "bundled Noto Sans appears in the real manager");
        const char name[] = "legacy-\xE9\x9C";
        CKObject *object = runtime.context->CreateObject(CKCID_BEOBJECT, (char *)name);
        Require(object != NULL, "legacy object");
        Require(manager->CreateFont((char *)name, noto, 400, FALSE, FALSE, 4, -24), "logical font with raw name bytes");
        Require(manager->SelectFont((char *)name), "select logical font with original bytes");
        Require(!strcmp(object->GetName(), name), "font operations preserve CK object name bytes");
        Require(!strcmp(name, "legacy-\xE9\x9C"), "font input bytes unchanged");
        manager->DeleteFontHandle((char *)name);
        for (int i = 0; i < 10; ++i)
        {
            Require(manager->CreateFont("font-regression", noto, 700, TRUE, TRUE, 4, -24), "create logical font");
            Require(manager->SelectFont("font-regression"), "select logical font");
            FONT_METRIC metrics;
            Require(manager->GetFontMetrics(metrics) && metrics.tmHeight > 0 && metrics.tmWeight == 700, "logical font metrics");
            int widths[2] = {0, 0};
            Require(manager->GetCharWidths('A', 'B', widths) && widths[0] > 0 && widths[1] > 0, "real glyph advances");
            manager->DeleteFontHandle("font-regression");
            Require(!manager->SelectFont("font-regression"), "deleted font is unavailable");
        }
#if defined(__linux__)
        CheckFailedFontRead(manager, noto);
#endif
        return 0;
    }
    catch (const char *failure)
    {
        fprintf(stderr, "%s\n", failure);
        return 1;
    }
}
