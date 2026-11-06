#ifndef PHYSICS_RT_PHYSICSDIAGNOSTICACCESS_H
#define PHYSICS_RT_PHYSICSDIAGNOSTICACCESS_H

#include "PhysicsDiagnostics.h"
#include <cstring>

#define PHYSICS_RT_DIAGNOSTIC extern "C" __declspec(dllexport)

static_assert(sizeof(PhysicsRTCoreSnapshot) == 368, "Unexpected core snapshot layout");

template <class Method> static void *PhysicsDiagnosticAddress(Method method)
{
    static_assert(sizeof(method) == sizeof(void *), "A direct member-function address is required");
    void *address = NULL;
    std::memcpy(&address, &method, sizeof(address));
    return address;
}

#endif // PHYSICS_RT_PHYSICSDIAGNOSTICACCESS_H
