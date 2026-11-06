#ifndef PHYSICS_RT_RETAILTESTSUPPORT_H
#define PHYSICS_RT_RETAILTESTSUPPORT_H

#define NOMINMAX
#include <windows.h>
#include <float.h>
#include <cstdio>
#include <cstring>

template <class T> static T ParityMaximum(T a, T b) { return a < b ? b : a; }
template <class T> static T ParityMinimum(T a, T b) { return b < a ? b : a; }

extern unsigned char *retail;
extern const char *active_call;
extern int active_frame;

template <class T> static T at(unsigned int rva)
{
    return reinterpret_cast<T>(retail + rva);
}
template <class T> static T read(const void *p, unsigned int offset)
{
    T result;
    std::memcpy(&result, static_cast<const unsigned char *>(p) + offset, sizeof(result));
    return result;
}
template <class T> static void write(void *p, unsigned int offset, T value)
{
    std::memcpy(static_cast<unsigned char *>(p) + offset, &value, sizeof(value));
}

extern "C" void *__cdecl PhysicsRT_DebugKKContactAddress();
extern "C" void *__cdecl PhysicsRT_DebugMinimizeAddress(unsigned int);
extern "C" void *__cdecl PhysicsRT_DebugEventValueAddress(unsigned int);
extern "C" void *__cdecl PhysicsRT_DebugPFValueAddress();
extern "C" void *__cdecl PhysicsRT_DebugPKKKValueAddress();

bool RunCapturedContinuous(const char *directory);

#define RETAIL_SCENARIO(Name, Family, Expression) bool RetailTest_##Name();
#include "retail_scenarios.inc"
#undef RETAIL_SCENARIO

#endif // PHYSICS_RT_RETAILTESTSUPPORT_H
