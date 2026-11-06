#include "PhysicsDiagnosticAccess.h"

#include "ivp_physics.hxx"
#include "ivp_core.hxx"
#include "ivp_friction.hxx"
#include "ivp_impact.hxx"
#include "ivp_great_matrix.hxx"
#include "ivp_friction_solver.hxx"

// clang-format off
extern "C" void physics_rt_debug_friction_entry();
#pragma comment(linker, "/alternatename:_physics_rt_debug_friction_entry=?friction_force_local_constraint_2d@IVP_Contact_Point@@AAEMPBVIVP_Event_Sim@@@Z")
extern "C" void physics_rt_debug_static_pressure_entry();
#pragma comment(linker, "/alternatename:_physics_rt_debug_static_pressure_entry=?static_friction_single@IVP_Contact_Point@@AAEXPBVIVP_Event_Sim@@MM@Z")
extern "C" void physics_rt_debug_kk_contact_entry();
#pragma comment(linker, "/alternatename:_physics_rt_debug_kk_contact_entry=?p_calc_friction_ss_KK@IVP_Contact_Point@@AAEXPBVIVP_Compact_Edge@@0PAVIVP_Cache_Ledge_Point@@1PAVIVP_Impact_Solver_Long_Term@@PAVIVP_U_Float_Point@@@Z")
extern "C" void physics_rt_debug_world_push_entry();
#pragma comment(                                                                                                       \
    linker, "/alternatename:_physics_rt_debug_world_push_entry=?get_world_push_direction@IVP_Impact_Solver@@AAEXXZ")
extern "C" void physics_rt_debug_impact_push_entry();
#pragma comment(linker, "/alternatename:_physics_rt_debug_impact_push_entry=?do_push@IVP_Impact_Solver@@AAEXN@Z")
extern "C" void physics_rt_debug_rescue_push_entry();
#pragma comment(linker, "/alternatename:_physics_rt_debug_rescue_push_entry=?do_rescue_push@IVP_Impact_Solver@@AAEXPAVIVP_U_Float_Point@@W4IVP_BOOL@@@Z")
extern "C" void physics_rt_debug_contact_pk();
#pragma comment(linker, "/alternatename:_physics_rt_debug_contact_pk=?p_calc_friction_s_PK@IVP_Contact_Point@@AAEXPBVIVP_U_Point@@PBVIVP_Compact_Edge@@PAVIVP_Cache_Ledge_Point@@PAVIVP_Impact_Solver_Long_Term@@PAVIVP_U_Float_Point@@@Z")
// clang-format on

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugImpactAddress()
{
    typedef void (IVP_Impact_Solver_Long_Term::*Impact)(IVP_Core *[2], IVP_FLOAT, IVP_Contact_Point *);
    Impact method = &IVP_Impact_Solver_Long_Term::do_impact_long_term;
    return PhysicsDiagnosticAddress(method);
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugFrictionAddress()
{
    return reinterpret_cast<void *>(&physics_rt_debug_friction_entry);
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugStaticPressureAddress()
{
    return reinterpret_cast<void *>(&physics_rt_debug_static_pressure_entry);
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugFrictionForcesAddress()
{
    typedef void (IVP_Friction_System::*Forces)(const IVP_Event_Sim *);
    Forces method = &IVP_Friction_System::calc_friction_forces;
    return PhysicsDiagnosticAddress(method);
}

PHYSICS_RT_DIAGNOSTIC unsigned int __cdecl PhysicsRT_DebugFrictionPairsOffset()
{
    return static_cast<unsigned int>(offsetof(IVP_Friction_System, fr_pairs_of_objs));
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugRecalcContactAddress()
{
    typedef void (IVP_Contact_Point::*Recalc)();
    Recalc method = &IVP_Contact_Point::recalc_friction_s_vals;
    return PhysicsDiagnosticAddress(method);
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugKKContactAddress()
{
    return reinterpret_cast<void *>(&physics_rt_debug_kk_contact_entry);
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugLinearPressureAddress()
{
    auto method = &IVP_Linear_Constraint_Solver::init_and_solve_lc;
    void *address = NULL;
    static_assert(sizeof(method) == sizeof(address));
    std::memcpy(&address, &method, sizeof(address));
    return address;
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugPressureMatrixAddress()
{
    auto method = &IVP_Friction_Solver::normize_constraint_equ;
    void *address = NULL;
    static_assert(sizeof(method) == sizeof(address));
    std::memcpy(&address, &method, sizeof(address));
    return address;
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugPressureResultsAddress()
{
    auto method = &IVP_Friction_Solver::do_resulting_pushes;
    void *address = NULL;
    static_assert(sizeof(method) == sizeof(address));
    std::memcpy(&address, &method, sizeof(address));
    return address;
}

PHYSICS_RT_DIAGNOSTIC void __cdecl PhysicsRT_DebugPressureResults(void *opaque, void *system, unsigned int *count,
                                                                  void **first, const double **values)
{
    auto *solver = static_cast<IVP_Friction_Solver *>(opaque);
    auto *friction = static_cast<IVP_Friction_System *>(system);
    *count = friction->get_fr_dist_number();
    *first = friction->get_first_friction_dist();
    *values = solver->dist_change_mat.result_vector;
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugPressureSystemAddress()
{
    typedef void (IVP_Friction_System::*Solve)(const IVP_Event_Sim *);
    Solve method = &IVP_Friction_System::do_friction_system;
    return PhysicsDiagnosticAddress(method);
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugConfirmPushesAddress()
{
    typedef void (IVP_Friction_System::*Commit)();
    Commit method = &IVP_Friction_System::confirm_complex_pushes;
    return PhysicsDiagnosticAddress(method);
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugWorldPushAddress()
{
    // The diagnostic host copies only the audited Win32 scalar/vector layout.
    static_assert(sizeof(IVP_Impact_Solver) == 296);
    static_assert(offsetof(IVP_Impact_Solver, core) == 256);
    static_assert(offsetof(IVP_Impact_Solver, surf_normal) == 284);
    return reinterpret_cast<void *>(&physics_rt_debug_world_push_entry);
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugImpactPushAddress()
{
    return reinterpret_cast<void *>(&physics_rt_debug_impact_push_entry);
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugRescuePushAddress()
{
    return reinterpret_cast<void *>(&physics_rt_debug_rescue_push_entry);
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugEnergyInitAddress()
{
    typedef void (IVP_Mutual_Energizer::*Init)(IVP_Core *, IVP_Core *);
    Init method = &IVP_Mutual_Energizer::init_mutual_energizer;
    return PhysicsDiagnosticAddress(method);
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugEnergyPotentialAddress()
{
    typedef void (IVP_Mutual_Energizer::*Potential)();
    Potential method = &IVP_Mutual_Energizer::calc_energy_potential;
    return PhysicsDiagnosticAddress(method);
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugEnergyPercentAddress()
{
    typedef void (IVP_Mutual_Energizer::*Destroy)(IVP_DOUBLE);
    Destroy method = &IVP_Mutual_Energizer::destroy_percent_energy;
    return PhysicsDiagnosticAddress(method);
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugContactPKAddress()
{
    return reinterpret_cast<void *>(&physics_rt_debug_contact_pk);
}
