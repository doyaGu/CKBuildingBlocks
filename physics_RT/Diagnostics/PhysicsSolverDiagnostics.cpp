#include "PhysicsDiagnosticAccess.h"

#include "ivp_physics.hxx"
#include "ivp_core.hxx"
#include "ivp_mindist.hxx"
#include "ivp_mindist_minimize.hxx"
#include "ivp_surbuild_pointsoup.hxx"
#include "ivp_surbuild_ledge_soup.hxx"
#include "ivp_mindist_event.hxx"
#include "ivp_great_matrix.hxx"
#include "ivp_constraint_local.hxx"
#include "ivp_anchor.hxx"
#include "ivp_actuator.hxx"
#include "ivp_actuator_spring.hxx"

// clang-format off
extern "C" void physics_rt_debug_constraint_entry();
#pragma comment(linker, "/alternatename:_physics_rt_debug_constraint_entry=?do_simulation_controller@IVP_Constraint_Local@@MAEXPAVIVP_Event_Sim@@PAV?$IVP_U_Vector@VIVP_Core@@@@@Z")
extern "C" void physics_rt_debug_pf_value_entry();
#pragma comment(linker, "/alternatename:_physics_rt_debug_pf_value_entry=?get_value@IVP_3D_Solver_PF_COLL@@UAENPAVIVP_U_Matrix@@0@Z")
extern "C" void physics_rt_debug_poly_event_entry();
#pragma comment(linker, "/alternatename:_physics_rt_debug_poly_event_entry=?next_event_default_poly_poly@IVP_Mindist_Event_Solver@@KAXPAV1@@Z")
extern "C" int physics_rt_debug_minimize_pp();
extern "C" int physics_rt_debug_minimize_pk();
extern "C" int physics_rt_debug_minimize_pf();
extern "C" int physics_rt_debug_minimize_kk();
#pragma comment(linker, "/alternatename:_physics_rt_debug_minimize_pp=?p_minimize_PP@IVP_Mindist_Minimize_Solver@@IAE?AW4IVP_MRC_TYPE@@PBVIVP_Compact_Edge@@0PAVIVP_Cache_Ledge_Point@@1@Z")
#pragma comment(linker, "/alternatename:_physics_rt_debug_minimize_pk=?p_minimize_PK@IVP_Mindist_Minimize_Solver@@IAE?AW4IVP_MRC_TYPE@@PBVIVP_Compact_Edge@@0PAVIVP_Cache_Ledge_Point@@1@Z")
#pragma comment(linker, "/alternatename:_physics_rt_debug_minimize_pf=?p_minimize_PF@IVP_Mindist_Minimize_Solver@@IAE?AW4IVP_MRC_TYPE@@PBVIVP_Compact_Edge@@0PAVIVP_Cache_Ledge_Point@@1@Z")
#pragma comment(linker, "/alternatename:_physics_rt_debug_minimize_kk=?p_minimize_KK@IVP_Mindist_Minimize_Solver@@IAE?AW4IVP_MRC_TYPE@@PBVIVP_Compact_Edge@@0PAVIVP_Cache_Ledge_Point@@1@Z")
extern "C" void physics_rt_debug_minimize_bf();
#pragma comment(linker, "/alternatename:_physics_rt_debug_minimize_bf=?p_minimize_BF@IVP_Mindist_Minimize_Solver@@IAE?AW4IVP_MRC_TYPE@@PAVIVP_Cache_Ball@@PBVIVP_Compact_Edge@@PAVIVP_Cache_Ledge_Point@@@Z")
extern "C" void physics_rt_debug_spring_entry();
#pragma comment(linker, "/alternatename:_physics_rt_debug_spring_entry=?do_simulation_controller@IVP_Actuator_Spring@@UAEXPAVIVP_Event_Sim@@PAV?$IVP_U_Vector@VIVP_Core@@@@@Z")
extern "C" void physics_rt_debug_pk_kk_value_entry();
#pragma comment(linker, "/alternatename:_physics_rt_debug_pk_kk_value_entry=?get_value@IVP_3D_Solver_PK_KK@@UAENPAVIVP_U_Matrix@@0@Z")
extern "C" void physics_rt_debug_event_value_0();
#pragma comment(linker, "/alternatename:_physics_rt_debug_event_value_0=?get_value@IVP_3D_Solver_DISTANCE_OF_TWO_POINTS@@UAENPAVIVP_U_Matrix@@0@Z")
extern "C" void physics_rt_debug_event_value_1();
#pragma comment(linker, "/alternatename:_physics_rt_debug_event_value_1=?get_value@IVP_3D_Solver_S_VALS@@UAENPAVIVP_U_Matrix@@0@Z")
extern "C" void physics_rt_debug_event_value_2();
#pragma comment(linker, "/alternatename:_physics_rt_debug_event_value_2=?get_value@IVP_3D_Solver_PK_COLL@@UAENPAVIVP_U_Matrix@@0@Z")
extern "C" void physics_rt_debug_event_value_3();
#pragma comment(linker, "/alternatename:_physics_rt_debug_event_value_3=?get_value@IVP_3D_Solver_PF_NPF@@UAENPAVIVP_U_Matrix@@0@Z")
extern "C" void physics_rt_debug_event_value_4();
#pragma comment(linker, "/alternatename:_physics_rt_debug_event_value_4=?get_value@IVP_3D_Solver_KK_COLL@@UAENPAVIVP_U_Matrix@@0@Z")
extern "C" void physics_rt_debug_event_value_5();
#pragma comment(linker, "/alternatename:_physics_rt_debug_event_value_5=?get_value@IVP_3D_Solver_KK_PARALLEL@@UAENPAVIVP_U_Matrix@@0@Z")
extern "C" void physics_rt_debug_event_value_6();
#pragma comment(linker, "/alternatename:_physics_rt_debug_event_value_6=?get_value@IVP_3D_Solver_VEC_PARALLEL_AREA@@UAENPAVIVP_U_Matrix@@0@Z")
extern "C" void physics_rt_debug_find_time_max();
extern "C" void physics_rt_debug_find_time_coll();
#pragma comment(linker, "/alternatename:_physics_rt_debug_find_time_max=?find_first_t_for_value_max_dev@IVP_3D_Solver@@QAE?AW4IVP_BOOL@@NVIVP_Time@@0HPAVIVP_U_Matrix_Cache@@1PANPAV3@@Z")
#pragma comment(linker, "/alternatename:_physics_rt_debug_find_time_coll=?find_first_t_for_value_coll@IVP_3D_Solver@@QAE?AW4IVP_BOOL@@NNVIVP_Time@@0PAVIVP_U_Matrix_Cache@@1PANPAV3@@Z")
extern "C" void physics_rt_debug_find_time_root();
#pragma comment(linker, "/alternatename:_physics_rt_debug_find_time_root=?calc_nullstelle@IVP_3D_Solver@@IAE?AVIVP_Time@@V2@0NNNPAVIVP_Real_Object@@1@Z")
// clang-format on

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugConstraintMatrixAddress()
{
    auto method = &IVP_Great_Matrix_Many_Zero::solve_great_matrix_many_zero;
    void *address = nullptr;
    static_assert(sizeof(method) == sizeof(address));
    static_assert(sizeof(IVP_Great_Matrix_Many_Zero) == 32);
    static_assert(offsetof(IVP_Great_Matrix_Many_Zero, matrix_values) == 16);
    std::memcpy(&address, &method, sizeof(address));
    return address;
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugMatrixMultiplyAddress(unsigned int kind)
{
    using Multiply = void (IVP_U_Matrix3::*)(const IVP_U_Matrix3 *, IVP_U_Matrix3 *) const;
    Multiply method = kind ? &IVP_U_Matrix3::mi2mult3 : &IVP_U_Matrix3::mmult3;
    void *address = nullptr;
    static_assert(sizeof(method) == sizeof(address));
    static_assert(sizeof(IVP_U_Matrix3) == 96);
    std::memcpy(&address, &method, sizeof(address));
    return address;
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugConstraintAddress()
{
    static_assert(sizeof(IVP_Constraint_Local) <= 512);
    return reinterpret_cast<void *>(&physics_rt_debug_constraint_entry);
}

PHYSICS_RT_DIAGNOSTIC unsigned int __cdecl PhysicsRT_DebugConstraintBytes()
{
    return sizeof(IVP_Constraint_Local);
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugConstraintCore(void *opaque, unsigned int index)
{
    auto *constraint = static_cast<IVP_Constraint_Local *>(opaque);
    auto *object = index ? constraint->get_objectA() : constraint->get_objectR();
    return object ? object->get_core() : nullptr;
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugPointTransformAddress(unsigned int kind)
{
    auto address = [](auto method)
    {
        void *value = nullptr;
        static_assert(sizeof(method) == sizeof(value));
        std::memcpy(&value, &method, sizeof(value));
        return value;
    };
    if (kind == 0)
    {
        using Method = void (IVP_U_Matrix::*)(const IVP_U_Float_Point *, IVP_U_Point *) const;
        return address(static_cast<Method>(&IVP_U_Matrix::vmult4));
    }
    using Method = void (IVP_U_Matrix::*)(const IVP_U_Point *, IVP_U_Float_Point *) const;
    return address(static_cast<Method>(&IVP_U_Matrix::vimult4));
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugPFValueAddress()
{
    return reinterpret_cast<void *>(&physics_rt_debug_pf_value_entry);
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugPolyEventAddress()
{
    static_assert(sizeof(IVP_Mindist_Event_Solver) == 72);
    return reinterpret_cast<void *>(&physics_rt_debug_poly_event_entry);
}

PHYSICS_RT_DIAGNOSTIC void __cdecl PhysicsRT_DebugReadPolyEvent(void *opaque, float *values, float *limits,
                                                                void **cores)
{
    auto *solver = static_cast<IVP_Mindist_Event_Solver *>(opaque);
    IVP_Mindist *mindist = solver->mindist;
    values[0] = mindist->sum_extra_radius;
    values[1] = mindist->len_numerator;
    values[2] = mindist->contact_dot_diff_center;
    std::memcpy(values + 3, mindist->contact_plane.k, 12);
    for (int i = 0; i < 2; ++i)
    {
        auto *core = mindist->get_sorted_synapse(i)->get_core();
        cores[i] = core;
        std::memcpy(limits + i * 8, core->rotation_axis_world_space.k, 12);
        limits[i * 8 + 3] = core->current_speed;
        limits[i * 8 + 4] = core->abs_omega;
        limits[i * 8 + 5] = core->max_surface_rot_speed;
        limits[i * 8 + 6] = core->max_surface_deviation;
        limits[i * 8 + 7] = core->upper_limit_radius;
    }
}

PHYSICS_RT_DIAGNOSTIC void __cdecl PhysicsRT_DebugReadMinimizeState(void *opaque, int *state)
{
    const IVP_Mindist_Minimize_Solver *solver = static_cast<const IVP_Mindist_Minimize_Solver *>(opaque);
    state[0] = solver->P_Finish_Counter;
    state[1] = solver->loop_hash_size;
    state[2] = solver->mindist->get_sorted_synapse(0) == solver->mindist->get_synapse(0) ? 0 : 1;
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugBallFaceAddress()
{
    return reinterpret_cast<void *>(&physics_rt_debug_minimize_bf);
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugMinimizeAddress(unsigned int kind)
{
    return kind == 0   ? reinterpret_cast<void *>(&physics_rt_debug_minimize_pp)
           : kind == 1 ? reinterpret_cast<void *>(&physics_rt_debug_minimize_pk)
           : kind == 2 ? reinterpret_cast<void *>(&physics_rt_debug_minimize_pf)
           : kind == 3 ? reinterpret_cast<void *>(&physics_rt_debug_minimize_kk)
                       : NULL;
}

PHYSICS_RT_DIAGNOSTIC void __cdecl PhysicsRT_DebugReadMindist(void *opaque, float *values)
{
    IVP_Mindist *mindist = static_cast<IVP_Mindist_Minimize_Solver *>(opaque)->mindist;
    values[0] = mindist->sum_extra_radius;
    values[1] = mindist->len_numerator;
    values[2] = mindist->contact_dot_diff_center;
    std::memcpy(values + 3, mindist->contact_plane.k, 12);
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugPointsoupAddress()
{
    return reinterpret_cast<void *>(&IVP_SurfaceBuilder_Pointsoup::convert_pointsoup_to_compact_ledge);
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugCompileSurfaceAddress()
{
    typedef IVP_Compact_Surface *(IVP_SurfaceBuilder_Ledge_Soup::*Compile)(IVP_Template_Surbuild_LedgeSoup *);
    Compile method = &IVP_SurfaceBuilder_Ledge_Soup::compile;
    return PhysicsDiagnosticAddress(method);
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugSpringAddress()
{
    // Bind the implementation rather than the member pointer's virtual-call thunk.
    return reinterpret_cast<void *>(&physics_rt_debug_spring_entry);
}

PHYSICS_RT_DIAGNOSTIC unsigned int __cdecl PhysicsRT_DebugSpringBytes()
{
    return sizeof(IVP_Actuator_Spring);
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugSpringCore(void *opaque, unsigned int index)
{
    if (index > 1)
        return nullptr;
    return static_cast<IVP_Actuator_Spring *>(opaque)->get_actuator_anchor(index)->l_anchor_object->get_core();
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugFindTimeAddress(unsigned int kind)
{
    return kind == 0   ? reinterpret_cast<void *>(&physics_rt_debug_find_time_max)
           : kind == 1 ? reinterpret_cast<void *>(&physics_rt_debug_find_time_coll)
           : kind == 2 ? reinterpret_cast<void *>(&physics_rt_debug_find_time_root)
                       : NULL;
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugEventValueAddress(unsigned int kind)
{
    static void *const addresses[] = {reinterpret_cast<void *>(&physics_rt_debug_event_value_0),
                                      reinterpret_cast<void *>(&physics_rt_debug_event_value_1),
                                      reinterpret_cast<void *>(&physics_rt_debug_event_value_2),
                                      reinterpret_cast<void *>(&physics_rt_debug_event_value_3),
                                      reinterpret_cast<void *>(&physics_rt_debug_event_value_4),
                                      reinterpret_cast<void *>(&physics_rt_debug_event_value_5),
                                      reinterpret_cast<void *>(&physics_rt_debug_event_value_6)};
    return kind < 7 ? addresses[kind] : NULL;
}

PHYSICS_RT_DIAGNOSTIC void *__cdecl PhysicsRT_DebugPKKKValueAddress()
{
    return reinterpret_cast<void *>(&physics_rt_debug_pk_kk_value_entry);
}
