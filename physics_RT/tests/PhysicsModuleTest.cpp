#include <windows.h>
#include <cstdio>
#include "PhysicsState.h"

int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    HMODULE module = LoadLibraryA(argv[1]);
    if (!module)
    {
        std::fprintf(stderr, "LoadLibrary failed: %lu\n", GetLastError());
        return 1;
    }
    typedef int(__cdecl * Count)();
    typedef void *(__cdecl * Info)(int);
    Count count = reinterpret_cast<Count>(GetProcAddress(module, "CKGetPluginInfoCount"));
    Info info = reinterpret_cast<Info>(GetProcAddress(module, "CKGetPluginInfo"));
    PhysicsRTGetStateApi query =
        reinterpret_cast<PhysicsRTGetStateApi>(GetProcAddress(module, "PhysicsRT_GetStateApi"));
    const PhysicsRTStateApi *api = query ? query(1) : NULL;
    const bool diagnostics = argv[2][0] == '1';
    const unsigned char *image = reinterpret_cast<const unsigned char *>(module);
    const IMAGE_DOS_HEADER *dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(image);
    const IMAGE_NT_HEADERS *nt = reinterpret_cast<const IMAGE_NT_HEADERS *>(image + dos->e_lfanew);
    const DWORD exportRva = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    const IMAGE_EXPORT_DIRECTORY *exports = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY *>(image + exportRva);
    const bool ok = count && count() == 2 && info && info(0) && info(1) && !info(-1) && !info(2) &&
                    GetProcAddress(module, "RegisterBehaviorDeclarations") &&
                    (diagnostics || exports->NumberOfNames == 4) && api && api->size == sizeof(*api) &&
                    api->version == 1 && !query(0) && !query(2) && api->read_velocity && api->reset_time &&
                    (GetProcAddress(module, "PhysicsRT_ReadCore") != NULL) == diagnostics &&
                    (GetProcAddress(module, "PhysicsRT_DebugForceLayout") != NULL) == diagnostics &&
                    (GetProcAddress(module, "PhysicsRT_DebugMinimizeAddress") != NULL) == diagnostics;
    FreeLibrary(module);
    if (!ok)
        std::fprintf(stderr, "physics module exports or versioned state interface failed\n");
    return ok ? 0 : 1;
}
