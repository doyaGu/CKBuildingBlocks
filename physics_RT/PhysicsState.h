#ifndef PHYSICS_RT_PHYSICSSTATE_H
#define PHYSICS_RT_PHYSICSSTATE_H

#if defined(_WIN32)
#define PHYSICS_RT_CALL __cdecl
#else
#define PHYSICS_RT_CALL
#endif

#if defined(_WIN32) && !defined(CK_LIB)
#if defined(PHYSICS_RT_STATE_BUILD)
#define PHYSICS_RT_STATE_API __declspec(dllexport)
#else
#define PHYSICS_RT_STATE_API __declspec(dllimport)
#endif
#elif defined(__GNUC__) && !defined(CK_LIB)
#define PHYSICS_RT_STATE_API __attribute__((visibility("default")))
#else
#define PHYSICS_RT_STATE_API
#endif

// Versioned physics state access. Manager and entity pointers are opaque handles.
struct PhysicsRTStateApi
{
    unsigned int size;
    unsigned int version;
    void(PHYSICS_RT_CALL *reset_time)(void *manager, float delta_ms);
    void(PHYSICS_RT_CALL *read_baseline)(void *manager, float *factor, short *movement_check, int *seed);
    void(PHYSICS_RT_CALL *restore_baseline)(void *manager, float factor, short movement_check, int seed);
    void(PHYSICS_RT_CALL *set_time_factor)(void *manager, float factor);
    int(PHYSICS_RT_CALL *read_velocity)(void *manager, void *entity, float *linear, float *angular);
    int(PHYSICS_RT_CALL *is_sleeping)(void *manager, void *entity);
    void(PHYSICS_RT_CALL *set_seed)(int seed);
    int(PHYSICS_RT_CALL *read_seed)();
    float(PHYSICS_RT_CALL *random)();
};

typedef const PhysicsRTStateApi *(PHYSICS_RT_CALL *PhysicsRTGetStateApi)(unsigned int version);

extern "C" PHYSICS_RT_STATE_API const PhysicsRTStateApi *PHYSICS_RT_CALL PhysicsRT_GetStateApi(unsigned int version);

#endif // PHYSICS_RT_PHYSICSSTATE_H
