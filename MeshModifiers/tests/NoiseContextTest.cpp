#include "CKAll.h"
#include <cmath>
#include <cstdio>

float noise3(CKContext *, const VxVector &);

int main() {
    int failures = 0;
    auto check = [&](bool condition, const char *message) {
        if (!condition) { ++failures; std::fprintf(stderr, "FAIL: %s\n", message); }
    };
    CKStartUp();
    CKContext *first = nullptr, *second = nullptr;
    CKCreateContext(&first, nullptr, 0, 0); CKCreateContext(&second, nullptr, 0, 0);
    if (!first || !second) return 2;
    first->SetRandomSeed(10); second->SetRandomSeed(10);
    const VxVector point(.125f, .75f, 1.25f);
    const float a = noise3(first, point), b = noise3(second, point);
    check(std::isfinite(a) && a == b, "equal seeds build equal independent mesh noise tables");
    second->SetRandomSeed(10);
    for (int i = 0; i < 256 * 6 + 255; ++i) second->Random();
    check(first->Random() == second->Random(), "noise retains legacy initialization draws");
    second->SetRandomSeed(1);
    noise3(second, VxVector(.25f, .5f, .75f));
    check(second->Random() == 41, "reusing a table does not consume its context's stream");
    CKCloseContext(first); first = nullptr;
    check(noise3(second, point) == b, "closing another context leaves mesh noise intact");
    CKCreateContext(&first, nullptr, 0, 0); first->SetRandomSeed(10);
    check(noise3(first, point) == a, "context recreation rebuilds its seeded field");
    CKCloseContext(first); CKCloseContext(second); CKShutdown();
    return failures ? 1 : 0;
}
