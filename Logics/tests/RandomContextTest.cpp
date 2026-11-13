#include "CKAll.h"
#include "../Behaviors/perlin.h"
#include <cmath>
#include <cstdio>

CKERROR CreateRandomProto(CKBehaviorPrototype **);
int Random(const CKBehaviorContext &);

int main() {
    int failures = 0;
    auto check = [&](bool condition, const char *message) {
        if (!condition) { ++failures; std::fprintf(stderr, "FAIL: %s\n", message); }
    };
    CKStartUp();
    CKContext *first = nullptr, *second = nullptr;
    CKCreateContext(&first, nullptr, 0, 0); CKCreateContext(&second, nullptr, 0, 0);
    if (!first || !second) return 2;
    first->SetRandomSeed(1); second->SetRandomSeed(1);
    const float a = PerlinNoise1D(first, .375f, 2, 2, 3);
    const float b = PerlinNoise1D(second, .375f, 2, 2, 3);
    check(std::isfinite(a) && a == b, "equal seeds produce independent equal noise fields");
    second->SetRandomSeed(1);
    for (int i = 0; i < 256 * 6 + 255; ++i) second->Random();
    check(first->Random() == second->Random(), "first noise call retains the legacy initialization draw count");
    second->SetRandomSeed(1);
    PerlinNoise1D(second, .25f, 2, 2, 4);
    check(second->Random() == 41, "cached noise does not consume more random values");
    // A table belongs to its context; destroying it must not retire another's.
    CKCloseContext(first); first = nullptr;
    check(PerlinNoise1D(second, .375f, 2, 2, 3) == b, "closing another context preserves the noise field");
    CKCreateContext(&first, nullptr, 0, 0);
    first->SetRandomSeed(1);
    check(PerlinNoise1D(first, .375f, 2, 2, 3) == a, "a fresh context rebuilds its own seeded field");

    CKBehaviorPrototype *prototype = nullptr;
    check(CreateRandomProto(&prototype) == CK_OK, "create legacy Random prototype");
    auto *behavior = static_cast<CKBehavior *>(first->CreateObject(CKCID_BEHAVIOR, (CKSTRING)"random"));
    check(behavior && behavior->InitFromPrototype(prototype) == CK_OK, "initialize legacy Random behavior");
    delete prototype;
    float minimum = -2, maximum = 3;
    auto *low = first->CreateCKParameterLocal((CKSTRING)"minimum", CKPGUID_FLOAT);
    auto *high = first->CreateCKParameterLocal((CKSTRING)"maximum", CKPGUID_FLOAT);
    low->SetValue(&minimum); high->SetValue(&maximum);
    behavior->GetInputParameter(0)->SetDirectSource(low);
    behavior->GetInputParameter(1)->SetDirectSource(high);
    CKBehaviorContext call = {};
    call.Context = first; call.Behavior = behavior; call.ParameterManager = first->GetParameterManager();
    first->SetRandomSeed(1); second->SetRandomSeed(1);
    check(Random(call) == CKBR_OK, "execute legacy Random behavior");
    float result = 0;
    behavior->GetOutputParameterValue(0, &result);
    check(std::fabs(result - (-2 + 41 * 5.0f / 32767.0f)) < 1e-6f,
          "Random retains legacy inclusive scaling");
    check(first->Random() == 18467 && second->Random() == 41, "behavior advances only its owning stream");
    CKCloseContext(first); CKCloseContext(second); CKShutdown();
    return failures ? 1 : 0;
}
