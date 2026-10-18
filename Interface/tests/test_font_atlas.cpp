#include "CKAll.h"
#include "CKFontManager.h"
#include "RCKTexture.h"
#include "RCKRenderManager.h"
#include "VxFontSearch.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <wchar.h>
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

extern void SetProcessorSpecific_FunctionsPtr();

static void Require(CKBOOL condition, const char *message)
{
    if (!condition) throw message;
}

static unsigned Read32(const CKBYTE *at)
{
    return (unsigned(at[0]) << 24) | (unsigned(at[1]) << 16) | (unsigned(at[2]) << 8) | at[3];
}

static void PrepareFace(CKBOOL italic, CKBOOL regular)
{
    SDL_IOStream *input = SDL_IOFromFile(regular ? TEST_REGULAR_FONT_FILE : TEST_FONT_FILE, "rb");
    Require(input != NULL, "open source font");
    struct Close
    {
        SDL_IOStream *stream;
        ~Close() { SDL_CloseIO(stream); }
    } close = {input};
    Sint64 size = SDL_GetIOSize(input);
    Require(size >= 12 && size < 0x7fffffff, "font fixture size");
    XArray<CKBYTE> bytes;
    bytes.Resize((int)size);
    Require(SDL_ReadIO(input, bytes.Begin(), (size_t)size) == (size_t)size, "read source font");
    if (italic)
    {
        // Mark the fixture's existing outlines as italic to isolate the face's
        // declared style from requested synthesis, without adding another font.
        unsigned count = (unsigned(bytes[4]) << 8) | bytes[5];
        Require(count <= (size - 12) / 16, "font fixture directory");
        bool found = false;
        for (unsigned i = 0; i < count; ++i)
        {
            CKBYTE *record = bytes.Begin() + 12 + i * 16;
            unsigned offset = Read32(record + 8);
            if (!memcmp(record, "head", 4))
            {
                Require(offset <= (unsigned)size - 46, "font head table");
                bytes[offset + 45] |= 2;
                found = true;
            }
            if (!memcmp(record, "OS/2", 4))
            {
                Require(offset <= (unsigned)size - 64, "font OS/2 table");
                bytes[offset + 63] |= 1;
            }
        }
        Require(found, "declared italic face");
    }
    XString directory(SDL_GetBasePath()); directory << "data/fonts";
    Require(SDL_CreateDirectory(directory.CStr()), "font fixture directory");
    directory << "/NotoSans-Bold.ttf";
    SDL_IOStream *output = SDL_IOFromFile(directory.CStr(), "wb");
    Require(output != NULL, "create test face");
    bool written = SDL_WriteIO(output, bytes.Begin(), bytes.Size()) == (size_t)bytes.Size();
    bool closed = SDL_CloseIO(output);
    Require(written && closed, "write test face");
}

struct Runtime
{
    CKContext *context = NULL;
    Runtime()
    {
        SetProcessorSpecific_FunctionsPtr();
        Require(CKStartUp() == CK_OK, "CK startup");
        CKCLASSREGISTERCID(RCKTexture, CKCID_BEOBJECT);
        CKBuildClassHierarchyTable();
        Require(CKCreateContext(&context, NULL, 0, 0) == CK_OK, "CK context");
        new RCKRenderManager(context);
        CKLevel *level = (CKLevel *)context->CreateObject(CKCID_LEVEL, "font-level");
        Require(level != NULL, "font level");
        context->SetCurrentLevel(level);
    }
    ~Runtime()
    {
        if (context) CKCloseContext(context);
        CKShutdown();
    }
};

static void CopyAtlas(CKTexture *texture, XArray<CKDWORD> &pixels, int minimumInk = 1000)
{
    Require(texture != NULL, "font atlas creation");
    VxImageDescEx image;
    Require(texture->GetSystemTextureDesc(image) && image.BitsPerPixel == 32, "atlas format");
    const CKBYTE *surface = texture->LockSurfacePtr();
    Require(surface != NULL, "atlas pixels");
    struct Unlock
    {
        CKTexture *texture;
        ~Unlock() { texture->ReleaseSurfacePtr(); }
    } unlock = {texture};
    pixels.Resize(image.Width * image.Height);
    int ink = 0;
    for (int y = 0; y < image.Height; ++y)
    {
        memcpy(pixels.Begin() + y * image.Width, surface + y * image.BytesPerLine, image.Width * 4);
        for (int x = 0; x < image.Width; ++x)
            if (pixels[y * image.Width + x] >> 24) ++ink;
    }
    Require(ink > minimumInk, "atlas contains real glyph pixels");
}

static void CheckNativeAtlas(CKFontManager *manager, int fontIndex, const char *path)
{
    XArray<CKDWORD> pixels;
    CKTexture *texture = manager->CreateTextureFromFont(fontIndex, 4, TRUE, 400, FALSE, TRUE, FALSE, FALSE, -24);
    CopyAtlas(texture, pixels);
    Require(manager->SelectFont(texture->GetName()), "select native atlas");
    size_t size = 0;
    void *bytes = SDL_LoadFile(path, &size);
    Require(bytes != NULL, "native reference face");
    struct FreeBytes { void *value; ~FreeBytes() { SDL_free(value); } } freeBytes = {bytes};
    stbtt_fontinfo font;
    Require(stbtt_InitFont(&font, (const unsigned char *)bytes, 0) != 0, "reference font");
    float scale = stbtt_ScaleForPixelHeight(&font, 24);
    int width = texture->GetWidth(), cell = width / 16;
    for (int c = 0; c < 256; ++c)
    {
        char byte = (char)c;
        wchar_t codepoint = 0;
        mbstate_t state = {};
        size_t length = mbrtowc(&codepoint, &byte, 1, &state);
        bool valid = length != (size_t)-1 && length != (size_t)-2;
        int actualWidth = -1;
        FONT_ABC abc;
        Require(manager->GetCharWidths(c, c, &actualWidth), "native character width");
        Require(manager->GetCharABCWidths(c, c, &abc), "native character ABC");
        if (valid)
        {
            int advance, bearing, x0, y0, x1, y1;
            stbtt_GetCodepointHMetrics(&font, codepoint, &advance, &bearing);
            stbtt_GetCodepointBitmapBox(&font, codepoint, scale, scale, &x0, &y0, &x1, &y1);
            float scaled = bearing * scale;
            int expectedA = (int)(scaled + (scaled >= 0 ? 0.5f : -0.5f));
            int expectedWidth = (int)(advance * scale + 0.5f);
            Require(actualWidth == expectedWidth, "width follows the runtime character set");
            Require(abc.abcA == expectedA && abc.abcB == (CKDWORD)(x1 - x0) &&
                    abc.abcC == expectedWidth - expectedA - (x1 - x0), "ABC follows the runtime character set");
        }
        else
        {
            Require(actualWidth == 0 && abc.abcA == 0 && abc.abcB == 0 && abc.abcC == 0,
                    "incomplete or invalid single-byte slots have no metrics");
            int left = c % 16 * cell, top = c / 16 * cell;
            for (int y = top; y < top + cell; ++y)
                for (int x = left; x < left + cell; ++x)
                    Require((pixels[y * width + x] >> 24) == 0, "invalid slots are not remapped to another glyph");
        }
    }
}

int main(int argc, char **argv)
{
    try
    {
        Require(argc == 1 || (argc == 2 && (!strcmp(argv[1], "--italic") || !strcmp(argv[1], "--clip") || !strcmp(argv[1], "--native"))), "font test mode");
        CKBOOL italic = argc == 2 && !strcmp(argv[1], "--italic");
        CKBOOL clip = argc == 2 && !strcmp(argv[1], "--clip");
        CKBOOL native = argc == 2 && !strcmp(argv[1], "--native");
        Require(setlocale(LC_CTYPE, native ? "" : "C") != NULL, "test locale");
        PrepareFace(italic, clip);
        // Both requests must resolve to the single physical face beside this
        // test executable, including when other Noto fonts exist on the host.
        VxFontFace normal, bold;
        const XClassArray<VxFontFace> &faces = VxGetSystemFonts();
        Require(VxFindFontFace(faces, "Noto Sans", 400, FALSE, normal), "normal request");
        Require(VxFindFontFace(faces, "Noto Sans", italic ? 400 : 700, italic, bold), "styled request");
        Require(normal.path == bold.path && normal.weight == (clip ? 400 : 700) && bold.weight == normal.weight, "same physical face");
        Require(normal.italic == italic && bold.italic == italic, "same declared style");
        Require(normal.path.Find("NotoSans-Bold.ttf") != XString::NOTFOUND, "bundled test face");

        Runtime runtime;
        CKParameterManager *parameters = runtime.context->GetParameterManager();
        Require(parameters->RegisterNewEnum(CKPGUID_FONTNAME, "Font Name", "Noto Sans=0") == CK_OK, "font enumeration");
        CKFontManager *manager = new CKFontManager(runtime.context);
        // Manager initialization enumerates the host's fonts again; the
        // registered seed's index does not identify the test family anymore.
        CKEnumStruct *fonts = parameters->GetEnumDescByType(parameters->ParameterGuidToType(CKPGUID_FONTNAME));
        Require(fonts != NULL, "system font enumeration");
        int fontIndex = -1;
        for (int i = 0; i < fonts->NbData; ++i)
        {
            if (!strcmp(fonts->Desc[i], "Noto Sans"))
            {
                fontIndex = i;
                break;
            }
        }
        Require(fontIndex >= 0, "test family appears in the manager enumeration");
        if (native)
        {
            CheckNativeAtlas(manager, fontIndex, normal.path.CStr());
            return 0;
        }
        if (clip)
        {
            XArray<CKDWORD> pixels;
            CKTexture *texture = manager->CreateTextureFromFont(fontIndex, 1, FALSE, 700, FALSE, FALSE, FALSE, FALSE, -24);
            CopyAtlas(texture, pixels, 100);
            Require(manager->SelectFont(texture->GetName()), "select atlas font");
            FONT_ABC abc;
            Require(manager->GetCharABCWidths('F', 'F', &abc) && abc.abcA > 0 && abc.abcB > 0,
                    "F has its own empty left margin");
            int width = texture->GetWidth(), cell = width / 16;
            int column = ('F' % 16) * cell, row = ('F' / 16) * cell;
            for (int y = row; y < row + cell; ++y)
                Require((pixels[y * width + column] >> 24) == 0,
                        "synthesized E must not spill into the next glyph's cell");
            return 0;
        }
        XArray<CKDWORD> normalPixels, boldPixels;
        CopyAtlas(manager->CreateTextureFromFont(fontIndex, 4, FALSE, 400, FALSE, FALSE, FALSE, FALSE, -24), normalPixels);
        CopyAtlas(manager->CreateTextureFromFont(fontIndex, 4, FALSE, italic ? 400 : 700, italic, FALSE, FALSE, FALSE, -24), boldPixels);
        Require(normalPixels.Size() == boldPixels.Size(), "atlas sizes");
        Require(memcmp(normalPixels.Begin(), boldPixels.Begin(), normalPixels.Size() * sizeof(CKDWORD)) == 0,
                italic ? "a declared italic face must not be sheared a second time" :
                         "a real Bold face must not be emboldened a second time");
        return 0;
    }
    catch (const char *failure)
    {
        fprintf(stderr, "%s\n", failure);
        return 1;
    }
}
