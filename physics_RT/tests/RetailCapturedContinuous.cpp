#include "RetailTestSupport.h"

// Replay the observer's recorded scalar inputs against both implementations.
bool RunCapturedContinuous(const char *directory)
{
    const auto query = &PhysicsRT_DebugEventValueAddress;
    const auto pf = &PhysicsRT_DebugPFValueAddress;
    const auto pk = &PhysicsRT_DebugPKKKValueAddress;
    const unsigned rvas[9] = {0x2AB30, 0x2ABC0, 0x2AD90, 0x2AE80, 0x2B530, 0x2B620, 0x2AAE0, 0x2A9C0, 0x2AC40};
    const unsigned sizes[9] = {136, 136, 144, 104, 176, 104, 104, 136, 168};
    unsigned total = 0, differences = 0;
    bool valid = true;
    for (unsigned kind = 0; kind < 9; ++kind)
    {
        char path[MAX_PATH];
        if (kind < 7)
            std::snprintf(path, sizeof(path), "%s/core.bin.event-value-%u.bin", directory, kind);
        else
            std::snprintf(path, sizeof(path), "%s/core.bin.%s-value.bin", directory, kind == 7 ? "pf" : "pk-kk");
        FILE *input = std::fopen(path, "rb");
        if (!input)
            continue;
        unsigned cases = 0, errors = 0;
        while (true)
        {
            unsigned header[3] = {};
            const unsigned header_size = kind < 7 ? 12 : 4;
            const size_t count = std::fread(header, 1, header_size, input);
            if (!count && std::feof(input))
                break;
            alignas(16) unsigned char object[176] = {}, matrix_a[128], matrix_b[128];
            unsigned short control = 0;
            if (count != header_size || (kind < 7 && (header[1] != kind || header[2] != sizes[kind])) ||
                std::fread(object, 1, sizes[kind], input) != sizes[kind] ||
                std::fread(matrix_a, 1, 128, input) != 128 || std::fread(matrix_b, 1, 128, input) != 128 ||
                std::fread(&control, 1, 2, input) != 2)
            {
                valid = false;
                break;
            }
            void *original = retail + rvas[kind];
            void *rebuilt = kind < 7 ? query(kind) : kind == 7 ? pf() : pk();
            unsigned char expected[10], actual[10];
            __asm {
                fldcw control
                lea ecx, object
                lea eax, matrix_b
                push eax
                lea eax, matrix_a
                push eax
                call original
                fstp tbyte ptr expected
                lea ecx, object
                lea eax, matrix_b
                push eax
                lea eax, matrix_a
                push eax
                call rebuilt
                fstp tbyte ptr actual
            }
            ++cases;
            if (std::memcmp(expected, actual, 10) != 0)
            {
                ++errors;
                if (errors <= 8)
                {
                    double e, a;
                    __asm { fld tbyte ptr expected }
                    __asm
                    {
                        fstp e
                    }
                    __asm { fld tbyte ptr actual }
                    __asm {fstp a} std::printf("captured kind%u row%u frame%u CW%04x %.17g / %.17g\n", kind, cases - 1,
                                               header[0], control, e, a);
                }
            }
        }
        std::fclose(input);
        total += cases;
        differences += errors;
        std::printf("captured kind%u mismatches %u / %u\n", kind, errors, cases);
    }
    std::printf("captured continuous mismatches %u / %u\n", differences, total);
    return valid && total > 0 && differences == 0;
}
