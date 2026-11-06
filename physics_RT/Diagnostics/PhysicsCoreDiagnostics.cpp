#include "PhysicsDiagnosticAccess.h"
#include "CKIpionManager.h"
#include "PhysicsStateClock.h"

#include "ivp_physics.hxx"
#include "ivp_core.hxx"
#include "ivp_environment.hxx"
#include "ivp_time.hxx"
#include "ivp_friction.hxx"
#include "ivp_calc_next_psi_solver.hxx"
#include "ivp_sim_unit.hxx"

extern int ivp_srand_read();

// clang-format off
extern "C" void physics_rt_debug_force_entry();
#pragma comment(linker, "/alternatename:_physics_rt_debug_force_entry=?do_simulation_controller@PhysicsControllerForce@@UAEXPAVIVP_Event_Sim@@PAV?$IVP_U_Vector@VIVP_Core@@@@@Z")
// clang-format on

static int ReadCoreSnapshot(IVP_Core *core, PhysicsRTCoreSnapshot *snapshot, unsigned int size, bool contacts = true)
{
    if (!snapshot || !core || size != sizeof(*snapshot))
        return 0;
    std::memset(snapshot, 0, sizeof(*snapshot));
    for (int i = 0; i < 3; ++i)
    {
        snapshot->position[i] = core->pos_world_f_core_last_psi.k[i];
        snapshot->speed[i] = core->speed.k[i];
        snapshot->angular[i] = core->rot_speed.k[i];
        snapshot->speed_change[i] = core->speed_change.k[i];
        snapshot->angular_change[i] = core->rot_speed_change.k[i];
    }
    std::memcpy(snapshot->quaternion_last, &core->q_world_f_core_last_psi, 32);
    std::memcpy(snapshot->quaternion_next, &core->q_world_f_core_next_psi, 32);
    const IVP_U_Matrix *matrix = core->get_m_world_f_core_PSI();
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            snapshot->matrix[r * 3 + c] = matrix->get_elem(r, c);
    for (int i = 0; i < 3; ++i)
        snapshot->matrix[9 + i] = matrix->vv.k[i];
    std::memcpy(snapshot->inverse_mass, core->get_inv_masses(), 16);
    snapshot->movement = core->movement_state;
    if (!contacts)
        return 1; // Contact temporaries are only valid at the recorder seam.
    IVP_Friction_Info_For_Core *friction = core->moveable_core_has_friction_info();
    if (friction && friction->friction_springs.len())
    {
        snapshot->contacts = friction->friction_springs.len();
        IVP_Contact_Point *contact = friction->friction_springs.element_at(0);
        // These contact fields have the same audited transport offsets used by
        // the same-input kernel tests; pointers remain native and opaque.
        const unsigned char *bytes = reinterpret_cast<const unsigned char *>(contact);
        std::memcpy(snapshot->spring, bytes + 0x38, 8);
        std::memcpy(&snapshot->contact_inverse_mass, bytes + 0x30, 4);
        std::memcpy(&snapshot->destroyed_energy, bytes + 0x48, 4);
        std::memcpy(&snapshot->friction, bytes + 0x44, 4);
        std::memcpy(&snapshot->pressure, bytes + 0x54, 4);
        std::memcpy(&snapshot->gap, bytes + 0x58, 4);
        IVP_Impact_Solver_Long_Term *info = NULL;
        std::memcpy(&info, bytes + 0x40, sizeof(info));
        if (info)
        {
            std::memcpy(snapshot->normal, info->surf_normal.k, 12);
            std::memcpy(snapshot->contact_ws, info->contact_point_ws.k, 24);
            for (int i = 0; i < 2; ++i)
                std::memcpy(snapshot->span + i * 3, info->span_friction_v[i].k, 12);
            std::memcpy(snapshot->contact_cs, info->contact_point_cs[0].k, 12);
            std::memcpy(snapshot->cross_normal, info->contact_cross_nomal_cs[0].k, 12);
        }
    }
    return 1;
}

struct PhysicsRT_ObjectDebugAccess : IVP_Real_Object
{
    static const IVP_U_Quat *CoreRotation(const IVP_Real_Object *object)
    {
        auto member = &PhysicsRT_ObjectDebugAccess::q_core_f_object;
        return object->*member;
    }
};

PHYSICS_RT_DIAGNOSTIC int __cdecl PhysicsRT_ReadCore(void *opaque, void *entity, PhysicsRTCoreSnapshot *snapshot,
                                                        unsigned int size)
{
    PhysicsObject *object =
        static_cast<CKIpionManager *>(opaque)->GetPhysicsObject(static_cast<CK3dEntity *>(entity), FALSE);
    return object && object->m_RealObject ? ReadCoreSnapshot(object->m_RealObject->get_core(), snapshot, size) : 0;
}

PHYSICS_RT_DIAGNOSTIC int __cdecl PhysicsRT_DebugReadCore(void *core, PhysicsRTCoreSnapshot *snapshot,
                                                          unsigned int size)
{
    return ReadCoreSnapshot(static_cast<IVP_Core *>(core), snapshot, size, false);
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugInterpolationAddress()
{
    typedef void (IVP_U_Quat::*Interpolation)(const IVP_U_Quat *, const IVP_U_Quat *, IVP_DOUBLE);
    Interpolation method = &IVP_U_Quat::set_interpolate_smoothly;
    return PhysicsDiagnosticAddress(method);
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugNextPsiAddress()
{
    typedef void (IVP_Calc_Next_PSI_Solver::*Next)(IVP_Event_Sim *, IVP_U_Vector<IVP_Hull_Manager_Base> *);
    Next method = &IVP_Calc_Next_PSI_Solver::calc_next_PSI_matrix;
    return PhysicsDiagnosticAddress(method);
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugSynchronizeAddress()
{
    typedef void (IVP_Core::*Synchronize)();
    Synchronize method = &IVP_Core::synchronize_with_rot_z;
    return PhysicsDiagnosticAddress(method);
}

PHYSICS_RT_DIAGNOSTIC void __cdecl PhysicsRT_DebugReadSyncTimes(void *opaque, double *values)
{
    auto *core = static_cast<IVP_Core *>(opaque);
    values[0] = core->time_of_last_psi.get_time();
    values[1] = core->environment->get_current_time().get_time();
    values[2] = core->i_delta_time;
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugCoreDampAddress()
{
    typedef void (IVP_Core::*Damp)(IVP_DOUBLE);
    Damp method = &IVP_Core::global_damp_core;
    return PhysicsDiagnosticAddress(method);
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugMovementAddress()
{
    typedef IVP_Movement_Type (IVP_Core::*Movement)(IVP_Time);
    Movement method = &IVP_Core::calc_movement_state;
    void *address = NULL;
    static_assert(sizeof(method) == sizeof(address));
    std::memcpy(&address, &method, sizeof(address));
    return address;
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugSimulationUnitAddress()
{
    typedef void (IVP_Simulation_Unit::*Simulation)(IVP_Event_Sim *, IVP_U_Vector<IVP_Core> *);
    Simulation method = &IVP_Simulation_Unit::simulate_single_sim_unit_psi;
    void *address = NULL;
    static_assert(sizeof(method) == sizeof(address));
    std::memcpy(&address, &method, sizeof(address));
    return address;
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugForceAddress()
{
    return reinterpret_cast<void *>(&physics_rt_debug_force_entry);
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugAsyncPushAddress()
{
    auto method = &IVP_Core::async_push_core;
    void *address = nullptr;
    static_assert(sizeof(method) == sizeof(address));
    std::memcpy(&address, &method, sizeof(address));
    return address;
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugWorldAsyncPushAddress()
{
    auto method = &IVP_Core::async_push_core_ws;
    void *address = nullptr;
    static_assert(sizeof(method) == sizeof(address));
    std::memcpy(&address, &method, sizeof(address));
    return address;
}

PHYSICS_RT_DIAGNOSTIC void __cdecl PhysicsRT_DebugRigidCore(void *opaque, float *inertia, unsigned int *modes)
{
    auto *core = static_cast<IVP_Core *>(opaque);
    std::memcpy(inertia, core->get_rot_inertia(), 16);
    modes[0] = static_cast<unsigned int>(core->rot_inertias_are_equal);
    modes[1] = static_cast<unsigned int>(core->environment->get_env_state());
    modes[2] = core->car_wheel ? 1u : 0u;
}

PHYSICS_RT_DIAGNOSTIC unsigned int __cdecl PhysicsRT_DebugReadSimulationUnit(void *opaque, void *event, int *meta,
                                                                             void **cores, unsigned int capacity)
{
    auto *unit = static_cast<IVP_Simulation_Unit *>(opaque);
    auto *environment = static_cast<IVP_Event_Sim *>(event)->environment;
    meta[0] = IVP_Environment_External_Clock::Read(environment);
    meta[1] = ivp_srand_read();
    meta[2] = unit->get_unit_movement_type();
    meta[3] = unit->sim_unit_has_fast_objects;
    const unsigned int count = unit->sim_unit_cores.len();
    for (unsigned int i = 0; i < count && i < capacity; ++i)
        cores[i] = unit->sim_unit_cores.element_at(i);
    return count;
}

PHYSICS_RT_DIAGNOSTIC void __cdecl PhysicsRT_DebugReadFreeze(void *opaque, void *output)
{
    IVP_Core *core = static_cast<IVP_Core *>(opaque);
    auto *bytes = static_cast<unsigned char *>(output);
    for (int i = 0; i < 2; ++i)
    {
        const double time = core->time_of_calm_reference[i].get_time();
        std::memcpy(bytes + i * 8, &time, 8);
    }
    std::memcpy(bytes + 16, core->q_world_f_core_calm_reference, 32);
    std::memcpy(bytes + 48, core->position_world_f_core_calm_reference, 32);
    std::memcpy(bytes + 80, &core->upper_limit_radius, 4);
    std::memcpy(bytes + 84, &core->environment->get_freeze_manager()->freeze_check_dtime, 4);
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugCoreCommitAddress()
{
    typedef void (IVP_Core::*Commit)();
    Commit method = &IVP_Core::commit_all_async_pushes;
    return PhysicsDiagnosticAddress(method);
}

PHYSICS_RT_DIAGNOSTIC void __cdecl PhysicsRT_DebugReadDampFactors(void *opaque, float *factors)
{
    IVP_Core *core = static_cast<IVP_Core *>(opaque);
    std::memcpy(factors, core->rot_speed_damp_factor.k, 12);
    factors[3] = core->speed_damp_factor;
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugFindEntityCore(void *opaque, void *entity)
{
    PhysicsObject *object =
        static_cast<CKIpionManager *>(opaque)->GetPhysicsObject(static_cast<CK3dEntity *>(entity), FALSE);
    return object && object->m_RealObject ? object->m_RealObject->get_core() : NULL;
}

PHYSICS_RT_DIAGNOSTIC void __cdecl PhysicsRT_DebugObjectFrame(void *opaque, double *rotation, float *shift,
                                                              unsigned int *flags)
{
    auto *object = static_cast<IVP_Real_Object *>(opaque);
    const IVP_U_Quat *q = PhysicsRT_ObjectDebugAccess::CoreRotation(object);
    flags[0] = q ? 1u : 0u;
    flags[1] = static_cast<unsigned int>(object->flags.shift_core_f_object_is_zero);
    if (q)
        std::memcpy(rotation, q, 32);
    else
        std::memset(rotation, 0, 32);
    std::memcpy(shift, object->get_shift_core_f_object()->k, 12);
}

PHYSICS_RT_DIAGNOSTIC void __cdecl PhysicsRT_DebugObjectInterpolation(void *opaque, double *times, float *inverse,
                                                                      double *quaternions, float *delta,
                                                                      double *position, unsigned int *flags)
{
    auto *object = static_cast<IVP_Real_Object *>(opaque);
    IVP_Core *core = object->get_core();
    times[0] = core->environment->get_current_time() - IVP_Time(0.0);
    times[1] = core->time_of_last_psi - IVP_Time(0.0);
    *inverse = core->i_delta_time;
    std::memcpy(quaternions, &core->q_world_f_core_last_psi, 32);
    std::memcpy(quaternions + 4, &core->q_world_f_core_next_psi, 32);
    std::memcpy(delta, core->delta_world_f_core_psis.k, 12);
    std::memcpy(position, core->pos_world_f_core_last_psi.k, 24);
    flags[0] = static_cast<unsigned int>(object->flags.object_movement_state);
    flags[1] = core->environment->get_current_time_code();
}
