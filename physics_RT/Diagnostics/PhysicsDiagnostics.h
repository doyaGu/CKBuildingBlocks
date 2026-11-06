#ifndef PHYSICS_RT_PHYSICSDIAGNOSTICS_H
#define PHYSICS_RT_PHYSICSDIAGNOSTICS_H

// Read-only diagnostic transport. Pointers remain opaque to observers.
struct PhysicsRTCoreSnapshot
{
    double position[3], quaternion_last[4], quaternion_next[4], matrix[12];
    float speed[3], angular[3], speed_change[3], angular_change[3], inverse_mass[4];
    unsigned int movement, contacts;
    float spring[2], friction, pressure, gap;
    float normal[3], span[6], contact_cs[3], cross_normal[3];
    double contact_ws[3];
    float contact_inverse_mass, destroyed_energy;
};
typedef int(__cdecl *PhysicsRTReadCore)(void *manager, void *entity, PhysicsRTCoreSnapshot *snapshot,
                                           unsigned int size);

#endif // PHYSICS_RT_PHYSICSDIAGNOSTICS_H
