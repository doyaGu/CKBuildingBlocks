#include "RetailTestSupport.h"
#include <wincrypt.h>

unsigned char *retail;
const char *active_call = "startup";
int active_frame = -1;
static LONG WINAPI crash_report(EXCEPTION_POINTERS *exception)
{
    std::fprintf(
        stderr, "exception 0x%lX at %p (retail RVA 0x%X), %s frame %d\n", exception->ExceptionRecord->ExceptionCode,
        exception->ExceptionRecord->ExceptionAddress,
        static_cast<unsigned int>(static_cast<unsigned char *>(exception->ExceptionRecord->ExceptionAddress) - retail),
        active_call, active_frame);
    return EXCEPTION_EXECUTE_HANDLER;
}

static bool verify_image(const char *path)
{
    HCRYPTPROV provider = 0;
    HCRYPTHASH hash = 0;
    if (!CryptAcquireContextA(&provider, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
        return false;
    if (!CryptCreateHash(provider, CALG_SHA_256, 0, 0, &hash))
    {
        CryptReleaseContext(provider, 0);
        return false;
    }
    FILE *file = std::fopen(path, "rb");
    bool ok = file != NULL;
    unsigned char buffer[8192];
    if (file)
    {
        size_t count;
        while ((count = std::fread(buffer, 1, sizeof(buffer), file)) != 0)
            if (!CryptHashData(hash, buffer, static_cast<DWORD>(count), 0))
                ok = false;
        ok = ok && !std::ferror(file);
        std::fclose(file);
    }
    unsigned char digest[32];
    DWORD length = 32;
    ok = ok && CryptGetHashParam(hash, HP_HASHVAL, digest, &length, 0) != FALSE;
    char text[65];
    if (ok)
    {
        for (int i = 0; i < 32; ++i)
            std::sprintf(text + i * 2, "%02X", digest[i]);
    }
    CryptDestroyHash(hash);
    CryptReleaseContext(provider, 0);
    return ok && std::strcmp(text, "E72E4AFCFA5C33A7D3D27776137F8C997B3C52D89D8A8A4745F1CA21E45893EC") == 0;
}
struct RetailScenario
{
    const char *name;
    bool (*run)();
};

static const RetailScenario retail_scenarios[] = {
#define RETAIL_SCENARIO(Name, Family, Expression) {#Name, RetailTest_##Name},
#include "retail_scenarios.inc"
#undef RETAIL_SCENARIO
};

int main(int argc, char **argv)
{
    const bool captured = argc == 5 && std::strcmp(argv[3], "capture-continuous") == 0;
    if (argc != 4 && !captured)
    {
        std::fprintf(stderr, "usage: physics_rt_parity retail.dll runtime-bin scenario [capture-directory]\n");
        return 2;
    }
    const RetailScenario *scenario = NULL;
    for (unsigned int i = 0; i < sizeof(retail_scenarios) / sizeof(retail_scenarios[0]); ++i)
    {
        if (std::strcmp(argv[3], retail_scenarios[i].name) == 0)
        {
            scenario = retail_scenarios + i;
            break;
        }
    }
    if (!scenario && !captured)
    {
        std::fprintf(stderr, "unknown scenario: %s\n", argv[3]);
        return 2;
    }
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(crash_report);
    std::setvbuf(stdout, NULL, _IONBF, 0);
    SetDllDirectoryA(argv[2]);
    if (!verify_image(argv[1]))
    {
        std::fprintf(stderr, "unsupported retail image (SHA256 mismatch)\n");
        return 2;
    }
    retail = reinterpret_cast<unsigned char *>(LoadLibraryA(argv[1]));
    if (!retail)
    {
        std::fprintf(stderr, "LoadLibrary failed: %lu\n", GetLastError());
        return 2;
    }
    // Refuse to execute arbitrary addresses in a different image.
    if (read<unsigned char>(retail, 0x138B0) != 0xB8 || read<unsigned char>(retail, 0x15EF0) != 0x56)
    {
        std::fprintf(stderr, "unsupported retail image\n");
        return 2;
    }
    unsigned int ignored;
    _controlfp_s(&ignored, _PC_24, _MCW_PC);
    const bool ok = captured ? RunCapturedContinuous(argv[4]) : scenario->run();
    return ok ? 0 : 1;
}
