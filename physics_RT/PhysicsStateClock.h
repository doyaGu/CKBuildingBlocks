#ifndef PHYSICS_RT_PHYSICSSTATECLOCK_H
#define PHYSICS_RT_PHYSICSSTATECLOCK_H

class IVP_Environment;

class IVP_Environment_External_Clock
{
  public:
    static void Reset(IVP_Environment *environment);
    static short Read(IVP_Environment *environment);
    static void Write(IVP_Environment *environment, short value);
};

#endif // PHYSICS_RT_PHYSICSSTATECLOCK_H
