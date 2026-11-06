#define PHYSICS_RT_STATE_BUILD
#include "PhysicsState.h"
#include "PhysicsStateClock.h"
#include "CKIpionManager.h"

#include "ivp_core.hxx"
#include "ivp_environment.hxx"
#include "ivp_time.hxx"

extern int ivp_srand_read();

void IVP_Environment_External_Clock::Reset(IVP_Environment *environment)
{
    environment->get_time_manager()->base_time = IVP_Time(0.0);
    environment->current_time = IVP_Time(0.0);
    environment->time_of_last_psi = IVP_Time(0.0);
    environment->time_of_next_psi = IVP_Time(1.0 / 66.0);
}

short IVP_Environment_External_Clock::Read(IVP_Environment *environment)
{
    return environment->next_movement_check;
}
void IVP_Environment_External_Clock::Write(IVP_Environment *environment, short value)
{
    environment->next_movement_check = value;
}

static void PHYSICS_RT_CALL ResetTime(void *opaque, float delta_ms)
{
    CKIpionManager *manager = static_cast<CKIpionManager *>(opaque);
    IVP_Environment_External_Clock::Reset(manager->GetEnvironment());
    // Reset the smoothed delta directly; its setter also derives another field.
    manager->m_DeltaTime = delta_ms;
}

static void PHYSICS_RT_CALL ReadBaseline(void *opaque, float *factor, short *movement_check, int *seed)
{
    CKIpionManager *manager = static_cast<CKIpionManager *>(opaque);
    *factor = manager->m_PhysicsTimeFactor;
    *movement_check = IVP_Environment_External_Clock::Read(manager->GetEnvironment());
    *seed = ivp_srand_read();
}

static void PHYSICS_RT_CALL RestoreBaseline(void *opaque, float factor, short movement_check, int seed)
{
    CKIpionManager *manager = static_cast<CKIpionManager *>(opaque);
    manager->m_PhysicsTimeFactor = factor;
    IVP_Environment_External_Clock::Write(manager->GetEnvironment(), movement_check);
    ivp_srand(seed);
}

static void PHYSICS_RT_CALL SetTimeFactor(void *opaque, float factor)
{
    static_cast<CKIpionManager *>(opaque)->m_PhysicsTimeFactor = factor;
}

static int PHYSICS_RT_CALL ReadVelocity(void *opaque, void *entity, float *linear, float *angular)
{
    PhysicsObject *object = static_cast<CKIpionManager *>(opaque)->GetPhysicsObject(static_cast<CK3dEntity *>(entity));
    if (!object || !object->m_RealObject)
        return 0;

    IVP_Core *core = object->m_RealObject->get_core();
    IVP_U_Float_Point velocity, rotation;
    if (linear)
    {
        velocity.add(&core->speed, &core->speed_change);
        for (int i = 0; i < 3; ++i)
            linear[i] = velocity.k[i];
    }
    if (angular)
    {
        rotation.add(&core->rot_speed, &core->rot_speed_change);
        for (int i = 0; i < 3; ++i)
            angular[i] = rotation.k[i];
    }
    return 1;
}

static int PHYSICS_RT_CALL IsSleeping(void *opaque, void *entity)
{
    PhysicsObject *object = static_cast<CKIpionManager *>(opaque)->GetPhysicsObject(static_cast<CK3dEntity *>(entity));
    return object && object->m_RealObject && object->m_RealObject->get_core()->movement_state >= IVP_MT_CALM;
}

extern "C" PHYSICS_RT_STATE_API const PhysicsRTStateApi *PHYSICS_RT_CALL PhysicsRT_GetStateApi(unsigned int version)
{
    static const PhysicsRTStateApi api = {sizeof(PhysicsRTStateApi),
                                          1,
                                          ResetTime,
                                          ReadBaseline,
                                          RestoreBaseline,
                                          SetTimeFactor,
                                          ReadVelocity,
                                          IsSleeping,
                                          ivp_srand,
                                          ivp_srand_read,
                                          ivp_rand};
    return version == api.version ? &api : NULL;
}
