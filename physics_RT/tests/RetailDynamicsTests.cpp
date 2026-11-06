#include "RetailTestSupport.h"
#include "RetailContactKernels.h"

#include <ivp_physics.hxx>
#include <ivp_templates.hxx>
#include <ivp_anomaly_manager.hxx>
#include <ivp_sim_unit.hxx>
#include <ivp_calc_next_psi_solver.hxx>
#include <ivp_controller.hxx>
#include <ivp_solver_core_reaction.hxx>
#include <ivp_friction.hxx>
#include <ivp_mindist.hxx>
#include <ivp_mindist_event.hxx>
#include <ivp_constraint_local.hxx>
#include <ivp_anchor.hxx>
#include <ivp_actuator.hxx>
#include <ivp_actuator_spring.hxx>
#include <ivp_compact_surface.hxx>
#include <ivp_material.hxx>
#include <ivp_compact_ledge.hxx>
#include <ivp_surbuild_pointsoup.hxx>
#include <ivp_surbuild_ledge_soup.hxx>
#include <ivp_surman_polygon.hxx>
#include <ivu_memory.hxx>
#if IVP_ENABLE_X87_EMULATION
#include <ivu_x87.hxx>
#endif

#include <cmath>
#include <new>

#include "fixtures/constraint_18741.hxx"
#include "fixtures/constraint_sr09_2543.hxx"
#include "fixtures/constraint_sr09_world_2543.hxx"
#include "fixtures/continuous_pf.hxx"
#include "fixtures/rescue_push.hxx"
#include "fixtures/spring_18770.hxx"
#include "fixtures/world_async_push.hxx"

// clang-format off
#pragma comment(linker, "/alternatename:_parity_native_constraint_entry=?do_simulation_controller@IVP_Constraint_Local@@MAEXPAVIVP_Event_Sim@@PAV?$IVP_U_Vector@VIVP_Core@@@@@Z")
#pragma comment(linker, "/alternatename:_parity_native_rotation_axis_entry=?calc_psi_rotation_axis@IVP_Calc_Next_PSI_Solver@@AAEXPBVIVP_U_Quat@@@Z")
#pragma comment(linker, "/alternatename:_parity_native_rescue_push_entry=?do_rescue_push@IVP_Impact_Solver@@AAEXPAVIVP_U_Float_Point@@W4IVP_BOOL@@@Z")
#pragma comment(linker, "/alternatename:_parity_native_rescue_speed_entry=?get_rescue_speed_impact@IVP_Contact_Point@@AAEMPAVIVP_Environment@@@Z")
// clang-format on

// Use the existing clock-adapter friend for typed, read-only fixture values.
class IVP_Environment_External_Clock
{
  public:
    static double energy_damping(IVP_Environment *environment) { return environment->get_integrated_energy_damp(); }
    static void set_state(IVP_Environment *environment, IVP_ENV_STATE state) { environment->state = state; }
};

static bool energy_damping_lifecycle_test()
{
    using Construct = void(__thiscall *)(void *);
    using Manager = void *(__cdecl *)();
    using Environment = void *(__thiscall *)(void *, void *, const char *, unsigned int);
    using Setter = void(__thiscall *)(void *, double);
    const double steps[] = {1.0 / 120.0, 1.0 / 30.0, 1.0 / 66.0, 1.0 / 90.0,
                            1.0 / 240.0, 1.0 / 10.0, 0.001, 0.5};
    int differences = 0, write_scope_differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        const unsigned int mode = precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24;
        unsigned int ignored;
        _controlfp_s(&ignored, mode, _MCW_PC);
        unsigned char app[0x30];
        at<Construct>(0x15E70)(app);
        void *original = at<Environment>(0x137E0)(at<Manager>(0x138B0)(), app, "NeMo", 0x7EFAD621);
        const double original_damping = read<double>(original, 0x148);
        const double original_step = read<double>(original, 0xC0);
        _controlfp_s(&ignored, mode, _MCW_PC);
        IVP_Application_Environment rebuilt_app;
        auto *rebuilt =
            IVP_Environment_Manager::get_environment_manager()->create_environment(&rebuilt_app, "NeMo", 0x7EFAD621);
        const double rebuilt_damping = IVP_Environment_External_Clock::energy_damping(rebuilt);
        const double rebuilt_step = rebuilt->get_delta_PSI_time();
        if (std::memcmp(&original_damping, &rebuilt_damping, 8) != 0 ||
            std::memcmp(&original_step, &rebuilt_step, 8) != 0)
            ++differences;
        std::printf("damping constructor PC%d retail %.17g rebuilt %.17g\n",
                    precision == 2   ? 64
                    : precision == 1 ? 53
                                     : 24,
                    original_damping, rebuilt_damping);
        for (unsigned int i = 0; i < sizeof(steps) / sizeof(steps[0]); ++i)
        {
            _controlfp_s(&ignored, mode, _MCW_PC);
            at<Setter>(0x13240)(original, steps[i]);
            _controlfp_s(&ignored, mode, _MCW_PC);
            rebuilt->set_delta_PSI_time(steps[i]);
            const double expected = read<double>(original, 0x148),
                         actual = IVP_Environment_External_Clock::energy_damping(rebuilt);
            const double expected_step = read<double>(original, 0xC0), actual_step = rebuilt->get_delta_PSI_time();
            const double expected_inverse = read<double>(original, 0xC8),
                         actual_inverse = rebuilt->get_inv_delta_PSI_time();
            const bool same =
                std::memcmp(&expected, &original_damping, 8) == 0 && std::memcmp(&actual, &rebuilt_damping, 8) == 0 &&
                std::memcmp(&expected, &actual, 8) == 0 && std::memcmp(&expected_step, &actual_step, 8) == 0 &&
                std::memcmp(&expected_inverse, &actual_inverse, 8) == 0 && expected_step == steps[i];
            if (!same)
            {
                ++differences;
                std::printf(
                    "damping setter PC%d sample %u step %.17g / %.17g inverse %.17g / %.17g damping %.17g / %.17g\n",
                    precision == 2   ? 64
                    : precision == 1 ? 53
                                     : 24,
                    i, expected_step, actual_step, expected_inverse, actual_inverse, expected, actual);
            }

            // IDB 0x13240 writes only [this+0xC0, this+0xD0), independently of construction.
            unsigned char packet[0x160], before[sizeof(packet)];
            std::memset(packet, 0xA5, sizeof(packet));
            write<double>(packet, 0x148, 0.8123456789012345);
            std::memcpy(before, packet, sizeof(packet));
            at<Setter>(0x13240)(packet, steps[i]);
            for (unsigned int j = 0; j < sizeof(packet); ++j)
                if ((j < 0xC0 || j >= 0xD0) && packet[j] != before[j])
                    ++write_scope_differences;
        }
        delete rebuilt;
        at<Construct>(0x13270)(original);
        at<void(__cdecl *)(void *)>(0x60756)(original);
    }
    std::printf("damping lifecycle mismatches: %d / 27; setter writes outside step fields: %d\n", differences,
                write_scope_differences);
    return differences == 0 && write_scope_differences == 0;
}

static bool penetration_speed_test()
{
    // RVA 0x2FC80 returns the live x87 value despite its nominal float API.
    using Speed = double(__thiscall *)(void *, void *, void *);
    IVP_Anomaly_Manager manager(IVP_FALSE);
    Speed rebuilt_speed = reinterpret_cast<Speed>((*reinterpret_cast<void ***>(&manager))[5]);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            alignas(16) unsigned char original_environment[0x100] = {}, original_object[0xB0] = {};
            alignas(16) unsigned char native_environment[sizeof(IVP_Environment)] = {},
                                      native_object[sizeof(IVP_Real_Object)] = {};
            auto *environment = reinterpret_cast<IVP_Environment *>(native_environment);
            auto *object = reinterpret_cast<IVP_Real_Object *>(native_object);
            object->environment = environment;
            const double step = sample % 3 == 0 ? 1.0 / 66.0 : sample % 3 == 1 ? 1.0 / 120.0 : 1.0 / 30.0;
            IVP_U_Point gravity(sample % 7 ? 0.137193 * sample : 0.0, -9.81 - 0.003179 * sample,
                                0.731379 * (sample % 11));
            if (sample == 0)
                gravity.set(0.0, 0.0, 0.0);
            environment->set_delta_PSI_time(step);
            const_cast<IVP_U_Point *>(environment->get_gravity())->set(&gravity);
            write<double>(original_environment, 0xC0, step);
            std::memcpy(original_environment + 0xD0, &gravity, 24);
            write<void *>(original_object, 0x18, original_environment);
            const double expected = at<Speed>(0x2FC80)(NULL, original_object, original_object);
            const double actual = rebuilt_speed(&manager, object, object);
            if (std::memcmp(&expected, &actual, 8) != 0)
            {
                ++differences;
                if (differences <= 6)
                    std::printf("penetration speed PC%d sample %d %.17g / %.17g\n", precision, sample, expected,
                                actual);
            }
        }
    }
    std::printf("penetration speed mismatches: %d / 384\n", differences);
    return differences == 0;
}

static bool core_damping_test()
{
    using Damp = void(__thiscall *)(void *, double, const IVP_U_Float_Point *, double);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            alignas(16) unsigned char original[0x200] = {}, native[sizeof(IVP_Core)] = {};
            auto *core = reinterpret_cast<IVP_Core *>(native);
            core->speed.set(0.001357913f * (sample + 1), -0.7319753f * (sample + 1), 0.3197531f * (sample + 1));
            core->rot_speed.set(-0.3719531f * (sample + 1), 0.000137193f * (sample + 1),
                                -0.00026120391f * (sample + 1));
            std::memcpy(original + 0x94, core->rot_speed.k, 12);
            std::memcpy(original + 0xA4, core->speed.k, 12);
            const double step = sample % 3 == 0 ? 1.0 / 66.0 : sample % 3 == 1 ? 1.0 / 120.0 : 1.0 / 30.0;
            const float rate = sample % 4 == 0 ? 0.1f : sample % 4 == 1 ? 0.3f : sample % 4 == 2 ? 50.0f : 0.7137953f;
            IVP_U_Float_Point rotation(rate, rate * 0.731795f, rate * 1.137953f);
            const double linear = sample % 5 == 0 ? 0.1 : 0.1379531 * (sample + 1);
            at<Damp>(0xC6A0)(original, step, &rotation, linear);
            core->damp_object(step, &rotation, linear);
            if (std::memcmp(original + 0x94, core->rot_speed.k, 12) || std::memcmp(original + 0xA4, core->speed.k, 12))
            {
                ++differences;
                if (differences <= 4)
                    std::printf("core damping PC%d sample%d linearX %.9g / %.9g angularX %.9g / %.9g\n", precision,
                                sample, read<float>(original, 0xA4), core->speed.k[0], read<float>(original, 0x94),
                                core->rot_speed.k[0]);
            }
        }
    }
    std::printf("core damping mismatches: %d / 384\n", differences);
    return differences == 0;
}

static bool gyro_integration_test()
{
    using Construct = void(__thiscall *)(void *);
    using Manager = void *(__cdecl *)();
    using Environment = void *(__thiscall *)(void *, void *, const char *, unsigned int);
    using Integrate = void(__thiscall *)(void *, void *, void *);
    unsigned char app[0x30];
    at<Construct>(0x15E70)(app);
    void *old_environment = at<Environment>(0x137E0)(at<Manager>(0x138B0)(), app, "NeMo", 0x7EFAD621);
    IVP_Application_Environment application;
    auto *environment =
        IVP_Environment_Manager::get_environment_manager()->create_environment(&application, "NeMo", 0x7EFAD621);
    IVP_Environment_External_Clock::set_state(environment, IVP_ES_PSI);
    write<int>(old_environment, 0x144, IVP_ES_PSI);
    int differences = 0;
    // Retail fast normalization requires 1e-12 convergence; arbitrary PC24
    // inputs can stall in that unrelated original loop. Replay uses PC64.
    for (int precision = 1; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 471; ++sample)
        {
            alignas(16) unsigned char original[0x300] = {}, native[sizeof(IVP_Core)] = {};
            auto *core = reinterpret_cast<IVP_Core *>(native);
            core->environment = environment;
            core->m_world_f_core_last_psi.init();
            core->q_world_f_core_next_psi.x = core->q_world_f_core_next_psi.y = core->q_world_f_core_next_psi.z = 0.0;
            core->q_world_f_core_next_psi.w = 1.0;
            core->speed.set(0.137193f, -0.7319753f, 0.3197531f);
            core->rot_speed.set(0.043f + sample * 0.137193f, -0.00238619f * (sample + 1),
                                -0.00026080815f * (sample + 1));
            IVP_U_Float_Point inertia(1.137193f + sample * 0.013719f, 0.537193f + sample * 0.031795f, 1.137193f);
            if (sample >= 128)
            {
                // SR09's captured angular input; reciprocal-consistent inertia
                // spans the same highly anisotropic range as its linked body.
                core->rot_speed.set(0.04279953986406326f, -0.0023898109793663025f, -0.00025971993454732f);
                inertia.set(13.020855903625488f, 0.5524281859397888f, 13.020855903625488f);
                int variation = sample - 128;
                for (int axis = 0; axis < 3; ++axis)
                {
                    const unsigned int bits = read<unsigned int>(inertia.k, axis * 4);
                    write<unsigned int>(inertia.k, axis * 4, bits + (variation % 7) - 3);
                    variation /= 7;
                }
            }
            float inverse[4] = {1.0f / inertia.k[0], 1.0f / inertia.k[1], 1.0f / inertia.k[2], 1.0f};
            if (sample >= 128)
            {
                inverse[0] = inverse[2] = 0.07679986953735352f;
                inverse[1] = 1.8101900815963745f;
            }
            std::memcpy(const_cast<IVP_U_Float_Point *>(core->get_rot_inertia()), inertia.k, 12);
            std::memcpy(const_cast<IVP_U_Point_4 *>(core->get_inv_masses()), inverse, 16);
            write<void *>(original, 0xC, old_environment);
            std::memcpy(original + 0x14, inertia.k, 12);
            std::memcpy(original + 0x34, inverse, 16);
            std::memcpy(original + 0x94, core->rot_speed.k, 12);
            std::memcpy(original + 0xA4, core->speed.k, 12);
            std::memcpy(original + 0x108, &core->q_world_f_core_next_psi, 32);
            std::memcpy(original + 0x128, &core->m_world_f_core_last_psi, 128);
            IVP_Event_Sim event(environment);
            event.delta_time = (sample % 2 || sample >= 128) ? 1.0 / 66.0 : 1.0 / 30.0;
            event.i_delta_time = 1.0 / event.delta_time;
            unsigned char old_event[sizeof(event)]{};
            std::memcpy(old_event, &event, sizeof(event));
            write<void *>(old_event, 16, old_environment);
            void *old_solver = original;
            at<Integrate>(0x1E300)(&old_solver, old_event, nullptr);
            IVP_Calc_Next_PSI_Solver solver(core);
            solver.calc_next_PSI_matrix(&event, nullptr);
            if (std::memcmp(original + 0x94, core->rot_speed.k, 12) ||
                std::memcmp(original + 0x108, &core->q_world_f_core_next_psi, 32))
            {
                ++differences;
                if (differences <= 4)
                {
                    std::printf("gyro integration PC%d sample%d angularZ %.9g / %.9g\n", precision, sample,
                                read<float>(original, 0x9C), core->rot_speed.k[2]);
                    for (int axis = 0; axis < 3; ++axis)
                        std::printf("  angular%d %.9g / %.9g\n", axis, read<float>(original, 0x94 + axis * 4),
                                    core->rot_speed.k[axis]);
                    for (int axis = 0; axis < 4; ++axis)
                        std::printf("  quaternion%d %.17g / %.17g\n", axis, read<double>(original, 0x108 + axis * 8),
                                    reinterpret_cast<double *>(&core->q_world_f_core_next_psi)[axis]);
                }
            }
        }
    }
    delete environment;
    at<Construct>(0x13270)(old_environment);
    at<void(__cdecl *)(void *)>(0x60756)(old_environment);
    std::printf("gyro integration mismatches: %d / 942 PC53/64 cases\n", differences);
    return differences == 0;
}

struct PenetrationOracleManager
{
    void **vtable;
    int calls;
    float value;
};

static float __fastcall penetration_oracle_speed(PenetrationOracleManager *manager, void *, void *, void *)
{
    ++manager->calls;
    return manager->value;
}

class PenetrationProbeManager : public IVP_Anomaly_Manager
{
  public:
    int calls;
    float value;
    PenetrationProbeManager() : IVP_Anomaly_Manager(IVP_FALSE), calls(0), value(0.0f) {}
    IVP_FLOAT get_push_speed_penetration(IVP_Real_Object *, IVP_Real_Object *) override
    {
        ++calls;
        return value;
    }
};

static bool penetration_simple_test()
{
    using Rescue = void(__thiscall *)(void *, void *, void *);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 64; ++sample)
        {
            alignas(16) unsigned char old_environment[0x180] = {}, native_environment[sizeof(IVP_Environment)] = {};
            alignas(16) unsigned char old_objects[2][0xC0] = {}, native_objects[2][sizeof(IVP_Real_Object)] = {};
            alignas(16) unsigned char old_cores[2][0x300] = {}, native_cores[2][sizeof(IVP_Core)] = {};
            auto *environment = reinterpret_cast<IVP_Environment *>(native_environment);
            const double step = sample % 2 ? 1.0 / 120.0 : 1.0 / 66.0;
            environment->set_delta_PSI_time(step);
            write<double>(old_environment, 0xC0, step);
            IVP_Real_Object *objects[2];
            for (int side = 0; side < 2; ++side)
            {
                auto *core = reinterpret_cast<IVP_Core *>(native_cores[side]);
                objects[side] = reinterpret_cast<IVP_Real_Object *>(native_objects[side]);
                objects[side]->environment = environment;
                objects[side]->physical_core = core;
                core->environment = environment;
                const auto movement = sample % 8 == 0 && side == 0 ? IVP_MT_NOT_SIM : IVP_MT_MOVING;
                core->movement_state = movement;
                objects[side]->set_movement_state(movement);
                const auto &row = retail_world_push_inputs[(sample + side) % (sizeof(retail_world_push_inputs) /
                                                                              sizeof(retail_world_push_inputs[0]))];
                for (int r = 0; r < 3; ++r)
                    for (int c = 0; c < 3; ++c)
                        core->m_world_f_core_last_psi.set_elem(r, c, row.matrix[r * 3 + c]);
                core->m_world_f_core_last_psi.vv.set(173.1379, -19.731379, 81.91379);
                if (side)
                {
                    const double distance = sample % 4 == 0 ? 0.0 : sample % 4 == 1 ? 0.001 : 1.0 + sample * 0.001379;
                    core->m_world_f_core_last_psi.vv.k[0] += distance * 0.193179;
                    core->m_world_f_core_last_psi.vv.k[1] += distance * 0.713719;
                    core->m_world_f_core_last_psi.vv.k[2] -= distance * 0.391379;
                }
                const float mass = 1.0f + side * 0.71379f + sample * 0.03179f;
                const float inverse[4] = {0.73179f, 1.37193f, 0.91379f, 1.0f / mass};
                const_cast<IVP_U_Float_Point *>(core->get_rot_inertia())->hesse_val = mass;
                std::memcpy(const_cast<IVP_U_Point_4 *>(core->get_inv_masses()), inverse, 16);
                core->rot_speed_change.set(0.13719f, -0.73179f, 0.39137f);
                core->speed_change.set(-0.31793f, 0.13791f, 0.73193f);
                write<void *>(old_objects[side], 0x18, old_environment);
                write<void *>(old_objects[side], 0xA4, old_cores[side]);
                write<void *>(old_cores[side], 0xC, old_environment);
                write<unsigned char>(old_objects[side], 0x80, static_cast<unsigned char>(movement));
                write<unsigned char>(old_cores[side], 0x60, static_cast<unsigned char>(movement));
                write<float>(old_cores[side], 0x20, mass);
                std::memcpy(old_cores[side] + 0x34, inverse, 16);
                std::memcpy(old_cores[side] + 0x128, &core->m_world_f_core_last_psi, 128);
                std::memcpy(old_cores[side] + 0x74, core->rot_speed_change.k, 12);
                std::memcpy(old_cores[side] + 0x84, core->speed_change.k, 12);
            }
            void *vtable[8] = {};
            vtable[5] = reinterpret_cast<void *>(&penetration_oracle_speed);
            PenetrationProbeManager manager;
            manager.value = 0.237193f + sample * 0.001379f;
            PenetrationOracleManager original = {vtable, 0, manager.value};
            if (sample & 8)
            {
                using Dispatch = void(__thiscall *)(void *, void *, void *, void *);
                at<Dispatch>(0x2F8F0)(&original, NULL, old_objects[0], old_objects[1]);
                manager.inter_penetration(NULL, objects[0], objects[1]);
            }
            else
            {
                at<Rescue>(0x2F780)(&original, old_objects[0], old_objects[1]);
                manager.solve_inter_penetration_simple(objects[0], objects[1]);
            }
            bool same = original.calls == 1 && manager.calls == 1;
            for (int side = 0; side < 2; ++side)
            {
                auto *core = reinterpret_cast<IVP_Core *>(native_cores[side]);
                same = same && std::memcmp(old_cores[side] + 0x74, core->rot_speed_change.k, 12) == 0 &&
                       std::memcmp(old_cores[side] + 0x84, core->speed_change.k, 12) == 0;
            }
            if (!same)
            {
                ++differences;
                if (differences <= 6)
                {
                    std::printf("simple penetration PC%d sample %d virtual calls %d / %d\n", precision, sample,
                                original.calls, manager.calls);
                    for (int side = 0; side < 2; ++side)
                        for (int k = 0; k < 3; ++k)
                        {
                            auto *core = reinterpret_cast<IVP_Core *>(native_cores[side]);
                            std::printf("  side %d axis %d angular %.9g / %.9g linear %.9g / %.9g\n", side, k,
                                        read<float>(old_cores[side], 0x74 + k * 4), core->rot_speed_change.k[k],
                                        read<float>(old_cores[side], 0x84 + k * 4), core->speed_change.k[k]);
                        }
                }
            }
        }
    }
    std::printf("simple penetration mismatches: %d / 192\n", differences);
    return differences == 0;
}

static bool sim_unit_split_timing_test()
{
    using Construct = void(__thiscall *)(void *);
    using Manager = void *(__cdecl *)();
    using Environment = void *(__thiscall *)(void *, void *, const char *, unsigned int);
    using Ball = void *(__thiscall *)(void *, const float *, void *, const IVP_U_Quat *, const IVP_U_Point *);
    using Revive = void(__thiscall *)(void *, void *);
    using Fuse = void(__thiscall *)(void *, void *);
    using Simulate = void(__thiscall *)(void *, void *, IVP_U_Vector<IVP_Core> *);
    int differences = 0;
    for (int scenario = 0; scenario < 7; ++scenario)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, _PC_64, _MCW_PC);
        unsigned char app[0x30];
        at<Construct>(0x15E70)(app);
        void *original = at<Environment>(0x137E0)(at<Manager>(0x138B0)(), app, "NeMo", 0x7EFAD621);
        IVP_Application_Environment rebuilt_app;
        auto *rebuilt =
            IVP_Environment_Manager::get_environment_manager()->create_environment(&rebuilt_app, "NeMo", 0x7EFAD621);
        IVP_U_Point gravity(0, 0, 0);
        at<void(__thiscall *)(void *, const IVP_U_Point *)>(0x13680)(original, &gravity);
        rebuilt->set_gravity(&gravity);
        const int bodies = scenario == 0 ? 2 : scenario + 1;
        void *old_cores[8] = {};
        IVP_Core *cores[8] = {};
        for (int i = 0; i < bodies; ++i)
        {
            unsigned char definition[0x70];
            at<Construct>(0x15EF0)(definition);
            IVP_Template_Real_Object native_definition;
            float radius = 0.5f;
            IVP_Template_Ball native_ball;
            native_ball.radius = radius;
            IVP_U_Point position(i * 3.0, 7.0, 1.0);
            void *old_ball = at<Ball>(0x13A10)(original, &radius, definition, NULL, &position);
            auto *ball = rebuilt->create_ball(&native_ball, &native_definition, NULL, &position);
            old_cores[i] = read<void *>(old_ball, 0xA4);
            cores[i] = ball->get_core();
            at<Revive>(0x11F80)(read<void *>(old_cores[i], 0x1D4), original);
            cores[i]->sim_unit_of_core->sim_unit_revive_for_simulation(rebuilt);
            const float velocity[3] = {0.71379f + i * 0.03179f, 0.19317f, -0.37193f};
            std::memcpy(static_cast<unsigned char *>(old_cores[i]) + 0xA4, velocity, 12);
            std::memcpy(cores[i]->speed.k, velocity, 12);
        }
        void *old_unit = read<void *>(old_cores[0], 0x1D4);
        auto *unit = cores[0]->sim_unit_of_core;
        at<Fuse>(0x11770)(old_unit, read<void *>(old_cores[1], 0x1D4));
        unit->fusion_simulation_unities(cores[1]->sim_unit_of_core);
        write<unsigned short>(old_unit, 0, read<unsigned short>(old_unit, 0) | 0x100);
        unit->union_find_needed_for_sim_unit = IVP_TRUE;
        IVP_U_Vector<IVP_Core> expected(16), actual(16);
        if (scenario == 0)
        {
            alignas(8) unsigned char event[24] = {};
            write<double>(event, 0, read<double>(original, 0xC0));
            write<double>(event, 8, read<double>(original, 0xC8));
            write<void *>(event, 16, original);
            IVP_Event_Sim native_event(rebuilt);
            at<Simulate>(0x121B0)(old_unit, event, &expected);
            unit->simulate_single_sim_unit_psi(&native_event, &actual);
        }
        else
        {
            at<Simulate>(0x124C0)(read<void *>(original, 8), original, &expected);
            rebuilt->get_sim_units_manager()->simulate_sim_units_psi(rebuilt, &actual);
        }
        const bool expected_split = read<void *>(old_cores[0], 0x1D4) != read<void *>(old_cores[1], 0x1D4);
        const bool actual_split = cores[0]->sim_unit_of_core != cores[1]->sim_unit_of_core;
        bool same = expected_split && actual_split && expected.len() == actual.len() &&
                    (read<unsigned short>(old_unit, 0) & 0x300) == 0 && !unit->union_find_needed_for_sim_unit;
        for (int i = 0; i < expected.len() && i < actual.len(); ++i)
        {
            int old_index = -1, new_index = -1;
            for (int j = 0; j < bodies; ++j)
            {
                if (expected.element_at(i) == old_cores[j])
                    old_index = j;
                if (actual.element_at(i) == cores[j])
                    new_index = j;
            }
            same = same && old_index >= 0 && old_index == new_index;
        }
        if (!same)
            ++differences;
        std::printf("split timing scenario %d bodies %d split %d / %d touched %d / %d\n", scenario, bodies,
                    expected_split, actual_split, expected.len(), actual.len());
        delete rebuilt;
        at<Construct>(0x13270)(original);
        at<void(__cdecl *)(void *)>(0x60756)(original);
    }
    std::printf("simulation split timing mismatches: %d / 7\n", differences);
    return differences == 0;
}

static bool sim_unit_sleep_order_test(bool reverse)
{
    typedef void(__thiscall *Construct)(void *);
    typedef void *(__cdecl *Manager)();
    typedef void *(__thiscall *Environment)(void *, void *, const char *, unsigned int);
    typedef void *(__thiscall *Ball)(void *, const float *, void *, const IVP_U_Quat *, const IVP_U_Point *);
    typedef void(__thiscall *Speed)(void *, const IVP_U_Float_Point *);
    typedef void(__thiscall *Wake)(void *);
    typedef void(__thiscall *Simulate)(void *, double);
    unsigned int ignored;
    _controlfp_s(&ignored, _PC_24, _MCW_PC);
    unsigned char application[0x30];
    at<Construct>(0x15E70)(application);
    void *original = at<Environment>(0x137E0)(at<Manager>(0x138B0)(), application, "NeMo", 0x7EFAD621);
    IVP_Application_Environment native_application;
    IVP_Environment *native = IVP_Environment_Manager::get_environment_manager()->create_environment(
        &native_application, "NeMo", 0x7EFAD621);
    IVP_U_Point gravity(0, 0, 0);
    at<void(__thiscall *)(void *, const IVP_U_Point *)>(0x13680)(original, &gravity);
    native->set_gravity(&gravity);
    const int count = 19;
    void *original_cores[count] = {};
    IVP_Core *native_cores[count] = {};
    int first_slow[count];
    for (int i = 0; i < count; ++i) first_slow[i] = -1;
    for (int i = 0; i < count; ++i)
    {
        const int role = reverse ? count - 1 - i : i;
        unsigned char definition[0x70];
        at<Construct>(0x15EF0)(definition);
        write<double>(definition, 0x38, 0.0);
        IVP_Template_Real_Object native_definition;
        native_definition.speed_damp_factor = 0.0;
        float radius = 0.5f;
        IVP_Template_Ball native_ball;
        native_ball.radius = radius;
        IVP_U_Point position(role * 4.0, 5.0, 1.0);
        void *object = at<Ball>(0x13A10)(original, &radius, definition, NULL, &position);
        IVP_Ball *native_object = native->create_ball(&native_ball, &native_definition, NULL, &position);
        IVP_U_Float_Point velocity(0.001379f * (role + 1), 0.0007317f * (role % 3), 0.0f);
        at<Speed>(0xA350)(object, &velocity);
        native_object->async_add_speed_object_ws(&velocity);
        at<Wake>(0xA460)(object);
        native_object->ensure_in_simulation();
        original_cores[role] = read<void *>(object, 0xA4);
        native_cores[role] = native_object->get_core();
    }
    int differences = 0, slow_samples = 0, sleeping_samples = 0;
    for (int frame = 0; frame < 1800; ++frame)
    {
        active_frame = frame;
        at<Simulate>(0x138C0)(original, 1.0 / 66.0);
        native->simulate_dtime(1.0 / 66.0);
        for (int role = 0; role < count; ++role)
        {
            void *expected = original_cores[role];
            IVP_Core *actual = native_cores[role];
            const unsigned char state = read<unsigned char>(expected, 0x60);
            const unsigned char unit_state = read<unsigned char>(read<void *>(expected, 0x1D4), 0);
            bool same = state == actual->movement_state &&
                        unit_state == actual->sim_unit_of_core->get_unit_movement_type();
            if (state == IVP_MT_SLOW)
            {
                ++slow_samples;
                if (first_slow[role] < 0) first_slow[role] = frame;
            }
            if (unit_state >= IVP_MT_NOT_SIM) ++sleeping_samples;
            for (int axis = 0; axis < 3; ++axis)
            {
                const double position = read<double>(expected, 0xB8 + axis * 8);
                const float speed = read<float>(expected, 0xA4 + axis * 4);
                same = same && memcmp(&position, &actual->pos_world_f_core_last_psi.k[axis], sizeof(position)) == 0 &&
                       memcmp(&speed, &actual->speed.k[axis], sizeof(speed)) == 0;
            }
            if (!same && ++differences <= 3)
                printf("sleep order %d frame %d role %d state %d / %d\n", reverse, frame, role, state,
                       actual->movement_state);
        }
    }
    printf("sleep order %d mismatches %d, slow samples %d, sleeping samples %d, first slow", reverse,
           differences, slow_samples, sleeping_samples);
    for (int i = 0; i < count; ++i) printf(" %d", first_slow[i]);
    printf("\n");
    delete native;
    at<Construct>(0x13270)(original);
    at<void(__cdecl *)(void *)>(0x60756)(original);
    return differences == 0 && slow_samples > 0 && sleeping_samples > 0;
}

static bool synchronize_kernel_test()
{
    typedef void(__thiscall * Construct)(void *);
    typedef void *(__cdecl * Manager)();
    typedef void *(__thiscall * Environment)(void *, void *, const char *, unsigned int);
    typedef void(__thiscall * Synchronize)(void *);
    unsigned char app[0x30];
    at<Construct>(0x15E70)(app);
    void *environment = at<Environment>(0x137E0)(at<Manager>(0x138B0)(), app, "NeMo", 0x7EFAD621);
    IVP_Application_Environment native_app;
    IVP_Environment *native_environment =
        IVP_Environment_Manager::get_environment_manager()->create_environment(&native_app, "NeMo", 0x7EFAD621);
    __declspec(align(16)) unsigned char original[0x300] = {}, native[sizeof(IVP_Core)] = {};
    IVP_Core *core = reinterpret_cast<IVP_Core *>(native);
    core->environment = native_environment;
    core->rot_inertias_are_equal = IVP_TRUE;
    core->i_delta_time = 66.0f;
    core->time_of_last_psi = IVP_Time(0.0);
    core->q_world_f_core_last_psi.x = 0.501167534376248;
    core->q_world_f_core_last_psi.y = -0.6819624758442903;
    core->q_world_f_core_last_psi.z = -0.48198983516297444;
    core->q_world_f_core_last_psi.w = 0.22681288064641933;
    core->q_world_f_core_next_psi.x = 0.5167720580479268;
    core->q_world_f_core_next_psi.y = -0.6687815316505946;
    core->q_world_f_core_next_psi.z = -0.4835107418174173;
    core->q_world_f_core_next_psi.w = 0.22780532366728823;
    core->rot_speed.set(-1.2761574983596802f, 3.0598504543304443f, -5.023659706115723f);
    write<void *>(original, 0xC, environment);
    write<unsigned char>(original, 0, 0x40);
    write<float>(original, 0x70, 66.0f);
    std::memcpy(original + 0xE8, &core->q_world_f_core_last_psi, 32);
    std::memcpy(original + 0x108, &core->q_world_f_core_next_psi, 32);
    std::memcpy(original + 0x94, &core->rot_speed, 12);
    unsigned int ignored;
    _controlfp_s(&ignored, _PC_64, _MCW_PC);
    at<Synchronize>(0xCFA0)(original);
    core->synchronize_with_rot_z();
    int differences = 0;
    for (int k = 0; k < 3; ++k)
        if (read<float>(original, 0x94 + k * 4) != core->rot_speed.k[k])
        {
            ++differences;
            std::printf("synchronize equal-inertia axis %d %.9g / %.9g\n", k, read<float>(original, 0x94 + k * 4),
                        core->rot_speed.k[k]);
        }
    std::printf("synchronize kernel mismatches: %d\n", differences);
    return differences == 0;
}

extern "C" void parity_native_constraint_entry();

static bool constraint_update_kernel_test(bool sr09 = false, bool world = false)
{
    using Update = void(__thiscall *)(void *, IVP_Event_Sim *, void *);
    int differences = 0, recorded_differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            alignas(16) unsigned char original[400], native[sizeof(IVP_Constraint_Local)];
            static_assert(sizeof(native) == sizeof(native_constraint_bytes));
            std::memcpy(original,
                        world  ? sr09_world_retail_constraint_bytes
                        : sr09 ? sr09_retail_constraint_bytes
                               : retail_constraint_bytes,
                        400);
            std::memcpy(native,
                        world  ? sr09_world_native_constraint_bytes
                        : sr09 ? sr09_native_constraint_bytes
                               : native_constraint_bytes,
                        sizeof(native));
            alignas(16) unsigned char old_cores[2][0x200] = {}, new_cores[2][sizeof(IVP_Core)] = {};
            alignas(16) unsigned char old_objects[2][0xB0] = {}, new_objects[2][sizeof(IVP_Real_Object)] = {};
            auto *constraint = reinterpret_cast<IVP_Constraint_Local *>(native);
            IVP_Real_Object *stored[2] = {constraint->get_objectR(), constraint->get_objectA()};
            for (int i = 0; i < 2; ++i)
            {
                if (!stored[i])
                {
                    write<void *>(original, i ? 376 : 240, nullptr);
                    continue;
                }
                auto *core = reinterpret_cast<IVP_Core *>(new_cores[i]);
                const auto &row = (world  ? sr09_world_constraint_cores
                                   : sr09 ? sr09_constraint_cores
                                          : retail_constraint_cores)[i];
                if (world && i == 1)
                {
                    core->physical_unmoveable = IVP_TRUE;
                    old_cores[i][0] = 4;
                }
                core->m_world_f_core_last_psi.init();
                for (int r = 0; r < 3; ++r)
                    for (int c = 0; c < 3; ++c)
                        core->m_world_f_core_last_psi.set_elem(r, c, row.matrix[r * 3 + c]);
                for (int k = 0; k < 3; ++k)
                    core->m_world_f_core_last_psi.vv.k[k] = row.matrix[9 + k];
                core->speed.set(row.speed[0] + sample * 0.000000001379f, row.speed[1], row.speed[2]);
                core->rot_speed.set(row.angular[0], row.angular[1], row.angular[2]);
                std::memcpy(old_cores[i] + 0x128, &core->m_world_f_core_last_psi, 128);
                std::memcpy(old_cores[i] + 0x34, row.inverse, 16);
                std::memcpy(const_cast<IVP_U_Point_4 *>(core->get_inv_masses()), row.inverse, 16);
                std::memcpy(old_cores[i] + 0x94, core->rot_speed.k, 12);
                std::memcpy(old_cores[i] + 0xA4, core->speed.k, 12);
                write(old_objects[i], 164, old_cores[i]);
                write(original, i ? 376 : 240, old_objects[i]);
                auto *object = reinterpret_cast<IVP_Real_Object *>(new_objects[i]);
                object->physical_core = core;
                int matches = 0;
                for (unsigned int off = 0; off + 4 <= sizeof(native); off += 4)
                    if (read<void *>(native, off) == stored[i])
                    {
                        write(native, off, object);
                        ++matches;
                    }
                if (matches != 1)
                {
                    std::printf("constraint object pointer is not unique\n");
                    return false;
                }
            }
            alignas(16) unsigned char event_bytes[sizeof(IVP_Event_Sim)] = {};
            auto *event = reinterpret_cast<IVP_Event_Sim *>(event_bytes);
            event->delta_time = (sr09 ? sr09_constraint_step : retail_constraint_step)[0];
            event->i_delta_time = (sr09 ? sr09_constraint_step : retail_constraint_step)[1];
            at<Update>(0x28960)(original, event, nullptr);
            reinterpret_cast<Update>(&parity_native_constraint_entry)(native, event, nullptr);
            bool same = true, recorded = true;
            for (int i = 0; i < 2; ++i)
            {
                auto *core = reinterpret_cast<IVP_Core *>(new_cores[i]);
                same = same && std::memcmp(old_cores[i] + 0x94, core->rot_speed.k, 12) == 0 &&
                       std::memcmp(old_cores[i] + 0xA4, core->speed.k, 12) == 0;
                const auto &row = (world  ? sr09_world_constraint_cores
                                   : sr09 ? sr09_constraint_cores
                                          : retail_constraint_cores)[i];
                recorded = recorded && std::memcmp(old_cores[i] + 0x94, row.expected_angular, 12) == 0 &&
                           std::memcmp(old_cores[i] + 0xA4, row.expected_speed, 12) == 0;
            }
            if (precision == 2 && sample == 0 && !recorded)
            {
                ++recorded_differences;
                if (world)
                    for (int side = 0; side < 2; ++side)
                        for (int axis = 0; axis < 3; ++axis)
                            std::printf("world recorded side%d axis%d speed %.9g / %.9g angular %.9g / %.9g\n", side,
                                        axis, sr09_world_constraint_cores[side].expected_speed[axis],
                                        read<float>(old_cores[side], 0xA4 + axis * 4),
                                        sr09_world_constraint_cores[side].expected_angular[axis],
                                        read<float>(old_cores[side], 0x94 + axis * 4));
            }
            if (!same)
            {
                ++differences;
                if (differences <= 4)
                    std::printf("constraint update PC%d sample %d core1 X %.9g / %.9g\n", precision, sample,
                                read<float>(old_cores[1], 0xA4),
                                reinterpret_cast<IVP_Core *>(new_cores[1])->speed.k[0]);
            }
        }
    }
    std::printf("constraint update mismatches: %d / 384; recorded retail output mismatches: %d\n", differences,
                recorded_differences);
    return differences == 0 && recorded_differences == 0;
}

static bool spring_update_kernel_test()
{
    using Update = void(__thiscall *)(void *, IVP_Event_Sim *, void *);
    int differences = 0, recorded_differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            alignas(16) unsigned char original[sizeof(retail_spring_bytes)], native[sizeof(IVP_Actuator_Spring)];
            static_assert(sizeof(native) == sizeof(native_spring_bytes));
            std::memcpy(original, retail_spring_bytes, sizeof(original));
            std::memcpy(native, native_spring_bytes, sizeof(native));
            alignas(16) unsigned char old_cores[2][0x200] = {}, new_cores[2][sizeof(IVP_Core)] = {};
            alignas(16) unsigned char old_objects[2][0xB0] = {}, new_objects[2][sizeof(IVP_Real_Object)] = {};
            auto *spring = reinterpret_cast<IVP_Actuator_Spring *>(native);
            for (int i = 0; i < 2; ++i)
            {
                auto *core = reinterpret_cast<IVP_Core *>(new_cores[i]);
                const auto &row = retail_spring_cores[i];
                core->m_world_f_core_last_psi.init();
                for (int r = 0; r < 3; ++r)
                    for (int c = 0; c < 3; ++c)
                        core->m_world_f_core_last_psi.set_elem(r, c, row.matrix[r * 3 + c]);
                for (int k = 0; k < 3; ++k)
                    core->m_world_f_core_last_psi.vv.k[k] = row.matrix[9 + k];
                core->speed.set(row.speed[0] + sample * 0.000000001379f, row.speed[1], row.speed[2]);
                core->rot_speed.set(row.angular[0], row.angular[1], row.angular[2]);
                std::memcpy(old_cores[i] + 0x128, &core->m_world_f_core_last_psi, 128);
                std::memcpy(old_cores[i] + 0x34, row.inverse, 16);
                std::memcpy(const_cast<IVP_U_Point_4 *>(core->get_inv_masses()), row.inverse, 16);
                std::memcpy(old_cores[i] + 0x94, core->rot_speed.k, 12);
                std::memcpy(old_cores[i] + 0xA4, core->speed.k, 12);
                std::memcpy(old_cores[i] + 0x74, row.angular_change, 12);
                std::memcpy(core->rot_speed_change.k, row.angular_change, 12);
                std::memcpy(old_cores[i] + 0x84, row.speed_change, 12);
                std::memcpy(core->speed_change.k, row.speed_change, 12);
                write(old_objects[i], 164, old_cores[i]);
                write(original, i ? 68 : 20, old_objects[i]);
                auto *object = reinterpret_cast<IVP_Real_Object *>(new_objects[i]);
                object->physical_core = core;
                spring->get_actuator_anchor(i)->l_anchor_object = object;
            }
            alignas(16) unsigned char event_bytes[sizeof(IVP_Event_Sim)] = {};
            auto *event = reinterpret_cast<IVP_Event_Sim *>(event_bytes);
            event->delta_time = retail_spring_step[0];
            event->i_delta_time = retail_spring_step[1];
            at<Update>(0x146D0)(original, event, nullptr);
            spring->IVP_Actuator_Spring::do_simulation_controller(event, nullptr);
            bool same = true, recorded = true;
            for (int i = 0; i < 2; ++i)
            {
                auto *core = reinterpret_cast<IVP_Core *>(new_cores[i]);
                same = same && std::memcmp(old_cores[i] + 0x74, core->rot_speed_change.k, 12) == 0 &&
                       std::memcmp(old_cores[i] + 0x84, core->speed_change.k, 12) == 0;
                recorded = recorded &&
                           std::memcmp(old_cores[i] + 0x74, retail_spring_cores[i].expected_angular_change, 12) == 0 &&
                           std::memcmp(old_cores[i] + 0x84, retail_spring_cores[i].expected_speed_change, 12) == 0;
            }
            if (precision == 2 && sample == 0 && !recorded)
                ++recorded_differences;
            if (!same)
            {
                ++differences;
                if (differences <= 4)
                    std::printf("spring PC%d sample%d core1 Z %.9g / %.9g\n", precision, sample,
                                read<float>(old_cores[1], 0x8C),
                                reinterpret_cast<IVP_Core *>(new_cores[1])->speed_change.k[2]);
            }
        }
    }
    std::printf("spring update mismatches: %d / 384; recorded retail output mismatches: %d\n", differences,
                recorded_differences);
    return differences == 0 && recorded_differences == 0;
}

static bool world_async_push_kernel_test()
{
    using Push = void(__thiscall *)(void *, const IVP_U_Point *, const IVP_U_Float_Point *);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (const auto &row : retail_world_push_inputs)
            for (int sample = 0; sample < 128; ++sample)
            {
                alignas(16) unsigned char original[0x200] = {}, native[sizeof(IVP_Core)] = {};
                auto *core = reinterpret_cast<IVP_Core *>(native);
                core->m_world_f_core_last_psi.init();
                for (int r = 0; r < 3; ++r)
                    for (int c = 0; c < 3; ++c)
                        core->m_world_f_core_last_psi.set_elem(r, c, row.matrix[r * 3 + c]);
                for (int k = 0; k < 3; ++k)
                    core->m_world_f_core_last_psi.vv.k[k] = row.matrix[9 + k];
                std::memcpy(original + 0x128, &core->m_world_f_core_last_psi, 128);
                std::memcpy(original + 0x34, row.inverse, 16);
                std::memcpy(const_cast<IVP_U_Point_4 *>(core->get_inv_masses()), row.inverse, 16);
                std::memcpy(original + 0x74, row.angular_change, 12);
                std::memcpy(core->rot_speed_change.k, row.angular_change, 12);
                std::memcpy(original + 0x84, row.speed_change, 12);
                std::memcpy(core->speed_change.k, row.speed_change, 12);
                IVP_U_Point point;
                point.set(row.point[0], row.point[1], row.point[2]);
                IVP_U_Float_Point impulse;
                impulse.set(row.impulse[0] + sample * 0.00000001379f, row.impulse[1], row.impulse[2]);
                at<Push>(0xC940)(original, &point, &impulse);
                core->async_push_core_ws(&point, &impulse);
                const bool same = std::memcmp(original + 0x74, core->rot_speed_change.k, 12) == 0 &&
                                  std::memcmp(original + 0x84, core->speed_change.k, 12) == 0;
                if (!same)
                {
                    ++differences;
                    if (differences <= 4)
                        std::printf("world async push PC%d sample %d Z %.9g / %.9g\n", precision, sample,
                                    read<float>(original, 0x7C), core->rot_speed_change.k[2]);
                }
            }
    }
    std::printf("world async push mismatches: %d / 1536\n", differences);
    return differences == 0;
}

static bool inertia_mode_kernel_test()
{
    using Calculate = void(__thiscall *)(void *);
    int differences = 0, scale_differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            alignas(16) unsigned char original[0x50] = {}, native[sizeof(IVP_Core)] = {};
            auto *core = reinterpret_cast<IVP_Core *>(native);
            float inertia[4] = {0.013579f + sample * 0.537951f, 0.023751f + sample * 0.97531f,
                                0.016531f + sample * 0.719753f, 4.0f};
            if (sample == 0)
            {
                inertia[0] = 33.586883544921875f;
                inertia[1] = 56.2310676574707f;
                inertia[2] = 45.33760452270508f;
            }
            if (sample % 13 == 0 && sample != 0)
                inertia[1] = inertia[2] = inertia[0];
            std::memcpy(original + 20, inertia, 16);
            std::memcpy(const_cast<IVP_U_Float_Point *>(core->get_rot_inertia()), inertia, 16);
            core->upper_limit_radius = 2.126521110534668f;
            write(original, 4, core->upper_limit_radius);
            at<Calculate>(0xD1A0)(original);
            core->calc_calc();
            const unsigned int expected = (original[0] >> 6) & 3, actual = core->rot_inertias_are_equal;
            const bool same = expected == actual && std::memcmp(original + 0x34, core->get_inv_masses(), 16) == 0 &&
                              read<float>(original, 0x48) == core->inv_object_diameter;
            if (!same)
            {
                ++differences;
                if (differences <= 4)
                    std::printf("inertia mode PC%d sample %d equal flag %u / %u\n", precision, sample, expected,
                                actual);
            }
        }
        // Pin the original's scale-dependent classification, independently of
        // the newer SDK's scale-invariant inverse-inertia comparison.
        for (int large = 0; large < 2; ++large)
        {
            alignas(16) unsigned char original[0x50] = {}, native[sizeof(IVP_Core)] = {};
            auto *core = reinterpret_cast<IVP_Core *>(native);
            const float scale = large ? 10.0f : 1.0f;
            const float inertia[4] = {scale, scale, 2.0f * scale, 4.0f};
            std::memcpy(original + 0x14, inertia, 16);
            std::memcpy(const_cast<IVP_U_Float_Point *>(core->get_rot_inertia()), inertia, 16);
            core->upper_limit_radius = 2.0f;
            write<float>(original, 4, 2.0f);
            at<Calculate>(0xD1A0)(original);
            core->calc_calc();
            const unsigned int observed = (original[0] >> 6) & 3, rebuilt = core->rot_inertias_are_equal;
            if (observed != static_cast<unsigned int>(large) || rebuilt != observed)
                ++scale_differences;
            std::printf("inertia scale PC%d I=(%.0f,%.0f,%.0f) equal flag retail %u rebuilt %u expected %d\n",
                        precision == 2   ? 64
                        : precision == 1 ? 53
                                         : 24,
                        inertia[0], inertia[1], inertia[2], observed, rebuilt, large);
        }
    }
    std::printf("inertia mode mismatches: %d / 384; scale behavior mismatches: %d / 6\n", differences,
                scale_differences);
    return differences == 0 && scale_differences == 0;
}

extern "C" void *__cdecl PhysicsRT_DebugForceAddress();
extern "C" void __cdecl PhysicsRT_DebugForceLayout(unsigned int *);

static bool constant_force_kernel_test()
{
    using Controller = void(__thiscall *)(void *, void *, IVP_U_Vector<IVP_Core> *);
    auto query = &PhysicsRT_DebugForceAddress;
    auto layout_fn = &PhysicsRT_DebugForceLayout;
    unsigned int layout[5];
    layout_fn(layout);
    if (layout[0] > 128 || layout[3] + layout[4] > layout[0] || (layout[4] != 16 && layout[4] != 32))
    {

        return false;
    }
    IVP_Application_Environment app;
    auto *environment =
        IVP_Environment_Manager::get_environment_manager()->create_environment(&app, "NeMo", 0x7EFAD621);
    Controller run = reinterpret_cast<Controller>(query());
    int differences = 0, by_precision[3] = {};
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            alignas(16) unsigned char old_controller[64] = {}, native_controller[128] = {};
            alignas(16) unsigned char old_core[0x200] = {}, native_core[sizeof(IVP_Core)] = {},
                                      old_environment[0x130] = {};
            auto *core = reinterpret_cast<IVP_Core *>(native_core);
            core->environment = environment;
            core->movement_state = IVP_MT_NOT_SIM;
            core->m_world_f_core_last_psi.init();
            for (int row = 0; row < 3; ++row)
                for (int col = 0; col < 3; ++col)
                    core->m_world_f_core_last_psi.set_elem(row, col, retail_rescue_matrices[1][row * 3 + col]);
            std::memcpy(old_core + 0x128, &core->m_world_f_core_last_psi, 128);
            write(old_core, 12, old_environment);
            write<unsigned char>(old_core, 0x60, IVP_MT_NOT_SIM);
            const float masses[4] = {1.6246339082717896f, 1.8529869318008423f, 3.318126916885376f, 5.0f};
            std::memcpy(old_core + 0x34, masses, 16);
            std::memcpy(const_cast<IVP_U_Point_4 *>(core->get_inv_masses()), masses, 16);
            IVP_U_Point position, force;
            position.set(0.913579123456789, -0.7531975312345678, 1.23579135791357);
            force.set((sample - 61) * 0.019753123456789, (sample % 17 - 8) * 0.23795123456789,
                      (sample % 11 - 5) * 0.13579135791357);
            IVP_U_Float_Point rounded_force;
            rounded_force.set(&force);
            write(old_controller, 8, old_core);
            std::memcpy(old_controller + 16, &position, 24);
            std::memcpy(old_controller + 48, &rounded_force, 12);
            write(native_controller, layout[1], core);
            std::memcpy(native_controller + layout[2], &position, 24);
            if (layout[4] == 16)
                std::memcpy(native_controller + layout[3], &rounded_force, 12);
            else
                std::memcpy(native_controller + layout[3], &force, 24);
            IVP_U_Vector<IVP_Core> cores;
            cores.add(core);
            at<Controller>(0x46B0)(old_controller, nullptr, &cores);
            run(native_controller, nullptr, &cores);
            const bool same = std::memcmp(old_core + 0x74, core->rot_speed_change.k, 12) == 0 &&
                              std::memcmp(old_core + 0x84, core->speed_change.k, 12) == 0;
            if (!same)
            {
                ++differences;
                ++by_precision[precision];
                if (differences <= 4)
                    std::printf("constant force PC%d sample %d torque Y %.9g / %.9g\n", precision, sample,
                                read<float>(old_core, 0x78), core->rot_speed_change.k[1]);
            }
        }
    }
    delete environment;

    std::printf("constant force by precision: %d / %d / %d\n", by_precision[0], by_precision[1], by_precision[2]);
    std::printf("constant force mismatches: %d / 384\n", differences);
    return differences == 0;
}

static bool rotation_push_kernel_test()
{
    using Push = void(__thiscall *)(void *, const IVP_U_Float_Point *);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            alignas(16) unsigned char original[0xB0] = {}, native[sizeof(IVP_Core)] = {};
            auto *core = reinterpret_cast<IVP_Core *>(native);
            const float masses[4] = {1.0917569398880005f, 1.0862270593643188f, 10.532702445983887f, 2.0f};
            std::memcpy(original + 0x34, masses, 16);
            std::memcpy(const_cast<IVP_U_Point_4 *>(core->get_inv_masses()), masses, 16);
            IVP_U_Float_Point impulse;
            impulse.set((sample - 61) * 0.0197531f, (sample % 17 - 8) * 0.237951f, (sample % 11 - 5) * 0.135791f);
            core->rot_speed.set(-impulse.k[0] * masses[0] + 0.000001379f, (sample % 7 - 3) * 0.113579f, 0.137951f);
            std::memcpy(original + 0x94, core->rot_speed.k, 12);
            at<Push>(0xCB70)(original, &impulse);
            core->rot_push_core_cs(&impulse);
            if (std::memcmp(original + 0x94, core->rot_speed.k, 12) != 0)
            {
                ++differences;
                if (differences <= 4)
                    std::printf("rotation push PC%d sample %d X %.9g / %.9g\n", precision, sample,
                                read<float>(original, 0x94), core->rot_speed.k[0]);
            }
        }
    }
    std::printf("rotation push mismatches: %d / 384\n", differences);
    return differences == 0;
}

extern "C" void parity_native_rotation_axis_entry();

static bool rotation_axis_kernel_test()
{
    typedef void(__thiscall * Axis)(void *, const IVP_U_Quat *);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            __declspec(align(16)) unsigned char original_core[0x300] = {}, native_core[sizeof(IVP_Core)] = {};
            IVP_Core *core = reinterpret_cast<IVP_Core *>(native_core);
            core->i_delta_time = 66.0f;
            core->max_surface_deviation = 2.048567f;
            std::memcpy(&core->m_world_f_core_last_psi, retail_continuous_pf[0] + 9, 128);
            write<float>(original_core, 0x70, core->i_delta_time);
            write<float>(original_core, 8, core->max_surface_deviation);
            std::memcpy(original_core + 0x128, &core->m_world_f_core_last_psi, 128);
            IVP_U_Quat q;
            q.x = (sample - 63) * 0.000001357913579;
            q.y = (sample % 13 - 6) * 0.00000246813579;
            q.z = (sample % 17 - 8) * 0.0000001357913579;
            q.w = 1.0;
            if (sample == 0)
                q.x = q.y = q.z = 0.0;
            if (sample == 1)
                q.x = q.y = q.z = 1e-10;
            void *original = original_core;
            at<Axis>(0x1E870)(&original, &q);
            reinterpret_cast<Axis>(&parity_native_rotation_axis_entry)(&core, &q);
            bool same = read<float>(original_core, 0x1BC) == core->abs_omega &&
                        read<float>(original_core, 0x1C0) == core->max_surface_rot_speed;
            for (int k = 0; k < 3; ++k)
                same = same && read<float>(original_core, 0x1A8 + k * 4) == core->rotation_axis_world_space.k[k];
            if (!same)
            {
                ++differences;
                if (differences <= 4)
                    std::printf("rotation axis PC%d sample %d omega %.9g / %.9g\n", precision, sample,
                                read<float>(original_core, 0x1BC), core->abs_omega);
            }
        }
    }
    std::printf("rotation axis mismatches: %d / 384\n", differences);
    return differences == 0;
}

extern "C" void parity_native_rescue_push_entry();

static bool rescue_push_kernel_test()
{
    using Rescue = void(__thiscall *)(void *, IVP_U_Float_Point *, IVP_BOOL);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            alignas(16) unsigned char old_cores[2][0x200] = {}, native_cores[2][sizeof(IVP_Core)] = {};
            alignas(16) unsigned char expected[296] = {}, actual[296] = {};
            IVP_U_Matrix matrices[2]{};
            IVP_U_Float_Point points[2], direction;
            direction.set(retail_rescue_direction[0], retail_rescue_direction[1], retail_rescue_direction[2]);
            for (int i = 0; i < 2; ++i)
            {
                for (int row = 0; row < 3; ++row)
                    for (int col = 0; col < 3; ++col)
                        matrices[i].set_elem(row, col, retail_rescue_matrices[i][row * 3 + col]);
                for (int k = 0; k < 3; ++k)
                    matrices[i].vv.k[k] = retail_rescue_matrices[i][9 + k];
                points[i].set(retail_rescue_points[i][0], retail_rescue_points[i][1], retail_rescue_points[i][2]);
                auto *core = reinterpret_cast<IVP_Core *>(native_cores[i]);
                std::memcpy(old_cores[i] + 0x128, &matrices[i], 128);
                core->m_world_f_core_last_psi = matrices[i];
                std::memcpy(old_cores[i] + 0x34, retail_rescue_masses[i], 16);
                std::memcpy(const_cast<IVP_U_Point_4 *>(core->get_inv_masses()), retail_rescue_masses[i], 16);
                for (int solverIndex = 0; solverIndex < 2; ++solverIndex)
                {
                    unsigned char *solver = solverIndex ? actual : expected;
                    write(solver, 48 + i * 4, &matrices[i]);
                    write(solver, 264 + i * 4, &points[i]);
                    write(solver, 284, &direction);
                }
                write(expected, 256 + i * 4, old_cores[i]);
                write(actual, 256 + i * 4, core);
            }
            for (int solverIndex = 0; solverIndex < 2; ++solverIndex)
            {
                unsigned char *solver = solverIndex ? actual : expected;
                write(solver, 0, retail_rescue_min_speed);
                for (int i = 0; i < 4; ++i)
                    std::memcpy(solver + 56 + i * 16, retail_rescue_speeds[i], 12);
                write(solver, 108, read<float>(solver, 108) + (sample - 0) * 0.00000001379f);
            }
            const IVP_BOOL panic = sample % 4 == 3 ? IVP_TRUE : IVP_FALSE;
            at<Rescue>(0x22A20)(expected, &direction, panic);
            reinterpret_cast<Rescue>(&parity_native_rescue_push_entry)(actual, &direction, panic);
            bool same = true;
            for (int vec = 0; vec < 8; ++vec)
                same = same && std::memcmp(expected + 56 + vec * 16, actual + 56 + vec * 16, 12) == 0;
            if (!same)
            {
                ++differences;
                if (differences <= 4)
                    std::printf("rescue push PC%d sample %d main Y %.9g / %.9g\n", precision, sample,
                                read<float>(expected, 92), read<float>(actual, 92));
            }
        }
    }
    std::printf("rescue push mismatches: %d / 384\n", differences);
    return differences == 0;
}

extern "C" void parity_native_rescue_speed_entry();

static bool rescue_speed_kernel_test()
{
    typedef float(__thiscall * Rescue)(void *, void *);
    IVP_Application_Environment app;
    auto *environment =
        IVP_Environment_Manager::get_environment_manager()->create_environment(&app, "NeMo", 0x7EFAD621);
    environment->set_delta_PSI_time(1.0 / 66.0);
    ivp_mindist_settings.min_coll_dists = read<float>(retail, 0x75DB4);
    __declspec(align(16)) unsigned char original_environment[0xD0] = {};
    write<double>(original_environment, 0xC8, environment->get_inv_delta_PSI_time());
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            __declspec(align(16)) unsigned char original_cores[2][0x300] = {}, native_cores[2][sizeof(IVP_Core)] = {};
            unsigned char original_contact[0x78] = {}, native_contact[sizeof(IVP_Contact_Point)] = {};
            IVP_Impact_Solver_Long_Term original_info, actual_info;
            for (int i = 0; i < 2; ++i)
            {
                auto *core = reinterpret_cast<IVP_Core *>(native_cores[i]);
                core->environment = environment;
                core->upper_limit_radius = i ? 2.126521110534668f : 2.0f;
                core->rot_speed.set(i ? -0.2319401502609253f + sample * 0.00013579f : 0.2632350027561188f,
                                    i ? -1.9312939643859863f : -3.395444869995117f,
                                    i ? 0.26223278045654297f : 3.833409309387207f);
                write<void *>(original_cores[i], 12, original_environment);
                write<float>(original_cores[i], 4, core->upper_limit_radius);
                std::memcpy(original_cores[i] + 0x94, &core->rot_speed, 12);
                original_info.contact_core[i] = reinterpret_cast<IVP_Core *>(original_cores[i]);
                actual_info.contact_core[i] = core;
                const unsigned char status = i ? 2 : sample % 4 ? 0 : 3;
                write<unsigned char>(original_contact, 22 + i * 20, status);
                write<unsigned char>(native_contact, 22 + i * 20, status);
            }
            const float gap = sample % 3 ? 0.01f - sample * 0.000003579f : 0.009999990463256836f;
            write<float>(original_contact, 0x58, gap);
            write<float>(native_contact, 0x58, gap);
            write<void *>(original_contact, 0x40, &original_info);
            write<void *>(native_contact, 0x40, &actual_info);
            const float expected = at<Rescue>(0x24930)(original_contact, original_environment);
            const float actual =
                reinterpret_cast<Rescue>(&parity_native_rescue_speed_entry)(native_contact, environment);
            if (expected != actual)
            {
                ++differences;
                if (differences <= 4)
                    std::printf("rescue speed PC%d sample %d %.9g / %.9g\n", precision, sample, expected, actual);
            }
        }
    }
    std::printf("rescue speed mismatches: %d / 384\n", differences);
    return differences == 0;
}

static bool reaction_kernel_fixture(const double *m, const float *masses, const float *speeds, const float *angular,
                                    const double *position, const float *spans)
{
    typedef void(__thiscall * Init)(void *, void *, void *, IVP_U_Point &, IVP_U_Float_Point *, IVP_U_Float_Point *,
                                    IVP_U_Float_Point *);
    typedef void(__thiscall * Exert)(void *, void *, void *, IVP_U_Float_Point &);
    __declspec(align(16)) unsigned char retail_core[0x200] = {}, native_core[sizeof(IVP_Core)] = {};
    IVP_Core *core = reinterpret_cast<IVP_Core *>(native_core);
    std::memcpy(const_cast<IVP_U_Point_4 *>(core->get_inv_masses()), masses, 16);
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            core->m_world_f_core_last_psi.set_elem(r, c, m[r * 3 + c]);
    core->m_world_f_core_last_psi.vv.set(m[9], m[10], m[11]);
    std::memcpy(retail_core + 0x34, masses, 16);
    std::memcpy(retail_core + 0x128, &core->m_world_f_core_last_psi, sizeof(IVP_U_Matrix));
    IVP_U_Point contact;
    contact.set(position[0], position[1], position[2]);
    IVP_U_Float_Point directions[2];
    for (int d = 0; d < 2; ++d)
        directions[d].set(spans[d * 3], spans[d * 3 + 1], spans[d * 3 + 2]);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        core->speed.set(speeds[0], speeds[1], speeds[2]);
        core->rot_speed.set(angular[0], angular[1], angular[2]);
        std::memcpy(retail_core + 0xA4, &core->speed, 12);
        std::memcpy(retail_core + 0x94, &core->rot_speed, 12);
        IVP_Solver_Core_Reaction expected = {}, actual = {};
        at<Init>(0x33A30)(&expected, retail_core, NULL, contact, &directions[0], &directions[1], NULL);
        actual.init_reaction_solver_translation_ws(core, NULL, contact, &directions[0], &directions[1], NULL);
        for (int d = 0; d < 2; ++d)
            for (int k = 0; k < 3; ++k)
            {
                float e = expected.cr_mult_inv0[d].k[k], a = actual.cr_mult_inv0[d].k[k];
                if (e != a)
                {
                    ++differences;
                    std::printf("reaction PC%d inertia %d,%d %.9g / %.9g\n", precision, d, k, e, a);
                }
                e = expected.cross_direction_position_cs0[d].k[k];
                a = actual.cross_direction_position_cs0[d].k[k];
                if (e != a)
                {
                    ++differences;
                    std::printf("reaction PC%d cross %d,%d %.9g / %.9g\n", precision, d, k, e, a);
                }
            }
        for (int r = 0; r < 2; ++r)
            for (int c = r; c < 2; ++c)
            {
                double e = expected.m_velocity_ds_f_impulse_ds.get_elem(r, c),
                       a = actual.m_velocity_ds_f_impulse_ds.get_elem(r, c);
                if (e != a)
                {
                    ++differences;
                    std::printf("reaction PC%d matrix %d,%d %.17g / %.17g\n", precision, r, c, e, a);
                }
            }
        for (int d = 0; d < 2; ++d)
            if (expected.delta_velocity_ds.k[d] != actual.delta_velocity_ds.k[d])
            {
                ++differences;
                std::printf("reaction PC%d velocity %d %.9g / %.9g\n", precision, d, expected.delta_velocity_ds.k[d],
                            actual.delta_velocity_ds.k[d]);
            }
        IVP_U_Float_Point impulse;
        impulse.set(0.07654321f, -0.01234567f, 0.0f);
        at<Exert>(0x33AD0)(&expected, retail_core, NULL, impulse);
        actual.exert_impulse_dim2(core, NULL, impulse);
        for (int k = 0; k < 3; ++k)
            if (read<float>(retail_core, 0x94 + k * 4) != core->rot_speed.k[k] ||
                read<float>(retail_core, 0xA4 + k * 4) != core->speed.k[k])
            {
                ++differences;
                std::printf("reaction PC%d output %d torque %.9g / %.9g\n", precision, k,
                            read<float>(retail_core, 0x94 + k * 4), core->rot_speed.k[k]);
            }
    }
    std::printf("reaction kernel mismatches: %d\n", differences);
    return differences == 0;
}

static bool reaction_kernel_test()
{
    bool ok = true;
    { // Captured Level 01 trace frame 1000.
        const double m[12] = {-0.9494754066664564, -0.3138403636321477,   -0.0008235841235794826, -0.31375312856009724,
                              0.9492672047138598,  -0.021230835440131154, 0.007444894513582059,   -0.019899754017981653,
                              -0.9997742611888473, 52.092223282112286,    16.17360828636799,      -153.3094597178732};
        const float masses[4] = {0.32894736528396606f, 0.32894736528396606f, 0.32894736528396606f, 0.5263158082962036f};
        const float speeds[3] = {-4.907885551452637f, 0.3488386869430542f, -2.1628823280334473f};
        const float angular[3] = {0.6675915718078613f, 0.2255774289369583f, -2.131891965866089f};
        const double position[3] = {51.95049973870675, 14.178635938154795, -153.3094597178732};
        const float spans[6] = {-0.9824064373970032f, 0.06979049742221832f,   -0.17322486639022827f,
                                0.17278940975666046f, -0.012275020591914654f, -0.9848822951316833f};
        ok = reaction_kernel_fixture(m, masses, speeds, angular, position, spans) && ok;
    }
    { // Captured Level 01 trace frame 1197.
        const double m[12] = {-0.3621027490594889,    -0.4709225749183902, -0.80443366883527,    -0.9115078590636997,
                              -0.0016719948638380666, 0.41127925707269786, -0.19502569571379613, 0.8821729608559724,
                              -0.42864419411201027,   38.90438538352224,   15.168269770430479,   -155.18768453360707};
        const float masses[4] = {0.32894736528396606f, 0.32894736528396606f, 0.32894736528396606f, 0.5263158082962036f};
        const float speeds[3] = {-10.42862606048584f, 0.2690778970718384f, 6.229427337646484f};
        const float angular[3] = {-1.2521344423294067f, 3.0161612033843994f, -4.970613479614258f};
        const double position[3] = {38.90499226462758, 13.242853896902952, -155.7287699556461};
        const float spans[6] = {6.068566563044442e-06f, 0.270542711019516f,    -0.9627079963684082f, 1.0f,
                                0.0002904828288592398f, 8.793588494881988e-05f};
        ok = reaction_kernel_fixture(m, masses, speeds, angular, position, spans) && ok;
    }
    return ok;
}

// Same-input oracle for the impact torque kernel first exposed by recorded frame 996.
static bool push_kernel_test()
{
    typedef void(__thiscall * Push)(const void *, const IVP_U_Float_Point *, const IVP_U_Float_Point *,
                                    const IVP_U_Float_Point *, IVP_U_Float_Point *, IVP_U_Float_Point *);
    __declspec(align(16)) unsigned char original[0x50] = {}, native[sizeof(IVP_Core)] = {};
    IVP_Core *core = reinterpret_cast<IVP_Core *>(native);
    int differences = 0, samples = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int i = 0; i < 64; ++i)
        {
            IVP_U_Float_Point point, impulse, world, expected_speed, expected_rot, actual_speed, actual_rot;
            // The first contact observed in Level 01, plus oblique impulses of
            // different scales to exercise all three cross-product components.
            point.set(0.6402314901351929f, -1.8947569131851196f, -5.675428838003427e-7f);
            impulse.set((i - 31) * 0.013579f, (i % 11 - 5) * 0.1234567f, (i % 17 - 8) * 0.0987654f);
            world.set(impulse.k[2], -impulse.k[0], impulse.k[1]);
            const float masses[4] = {0.32894736528396606f, 0.32894736528396606f,
                                     i % 2 ? 0.32894736528396606f : 0.71234567f, 0.5263158082962036f};
            std::memcpy(original + 0x34, masses, sizeof(masses));
            std::memcpy(const_cast<IVP_U_Point_4 *>(core->get_inv_masses()), masses, sizeof(masses));
            at<Push>(0xCA80)(original, &point, &impulse, &world, &expected_speed, &expected_rot);
            core->test_push_core(&point, &impulse, &world, &actual_speed, &actual_rot);
            for (int axis = 0; axis < 3; ++axis)
            {
                const bool same =
                    expected_speed.k[axis] == actual_speed.k[axis] && expected_rot.k[axis] == actual_rot.k[axis];
                if (!same && differences < 8)
                    std::printf("push PC%d case %d axis %d torque %.9g / %.9g\n", precision, i, axis,
                                expected_rot.k[axis], actual_rot.k[axis]);
                differences += !same;
                ++samples;
            }
        }
    }
    std::printf("push kernel mismatches: %d / %d\n", differences, samples);
    return differences == 0;
}

static bool energy_init_kernel_test()
{
    typedef void(__thiscall * Init)(void *, void *, void *);
    int differences = 0;
    const double matrix[12] = {0.99162775217158472, 0.1152135508242683,  -0.058311566859362722, -0.127985699144431,
                               0.93689558027438291, -0.3253403333078127, 0.017148234242667058,  0.33007955006148321,
                               0.94379734514013514, -148.00155460277944, 18.171561697474196,    -361.54110068526035};
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        int before = differences;
        for (int sample = 0; sample < 128; ++sample)
        {
            __declspec(align(16)) unsigned char expected[160] = {}, actual[sizeof(IVP_Mutual_Energizer)] = {};
            __declspec(align(16)) unsigned char expected_cores[2][0x200] = {}, actual_cores[2][sizeof(IVP_Core)] = {};
            IVP_Core *cores[2] = {reinterpret_cast<IVP_Core *>(actual_cores[0]),
                                  reinterpret_cast<IVP_Core *>(actual_cores[1])};
            for (int j = 0; j < 2; ++j)
            {
                const float inverse[4] = {4.51443338394165f, 4.516299724578857f, 4.508676052093506f, 5.0f};
                const float inertia[4] = {1.0f / inverse[0], 1.0f / inverse[1], 1.0f / inverse[2], 0.2f};
                if (j == 0)
                {
                    expected_cores[j][0] = 4;
                    cores[j]->physical_unmoveable = IVP_TRUE;
                }
                std::memcpy(expected_cores[j] + 0x14, inertia, 16);
                std::memcpy(const_cast<IVP_U_Float_Point *>(cores[j]->get_rot_inertia()), inertia, 16);
                std::memcpy(expected_cores[j] + 0x34, inverse, 16);
                std::memcpy(const_cast<IVP_U_Point_4 *>(cores[j]->get_inv_masses()), inverse, 16);
                for (int r = 0; r < 3; ++r)
                    for (int k = 0; k < 3; ++k)
                        cores[j]->m_world_f_core_last_psi.set_elem(r, k, j ? matrix[r * 3 + k] : (r == k ? 1.0 : 0.0));
                cores[j]->m_world_f_core_last_psi.vv.set(0, 0, 0);
                std::memcpy(expected_cores[j] + 0x128, &cores[j]->m_world_f_core_last_psi, sizeof(IVP_U_Matrix));
                if (j == 1)
                {
                    cores[j]->speed.set(-0.040293533354997635f + sample * 0.0013579f, -0.08594444394111633f,
                                        0.7729243636131287f);
                    cores[j]->rot_speed.set(0.297102689743042f, -0.04810178279876709f + sample * 0.00008642f,
                                            -0.007944159209728241f);
                }
                std::memcpy(expected_cores[j] + 0xA4, cores[j]->speed.k, 12);
                std::memcpy(expected_cores[j] + 0x94, cores[j]->rot_speed.k, 12);
            }
            at<Init>(0x1CDB0)(expected, expected_cores[0], expected_cores[1]);
            reinterpret_cast<IVP_Mutual_Energizer *>(actual)->init_mutual_energizer(cores[0], cores[1]);
            for (int v = 0; v < 3; ++v)
                for (int k = 0; k < 3; ++k)
                {
                    float e = read<float>(expected, v * 16 + k * 4), a = read<float>(actual, v * 16 + k * 4);
                    if (e != a)
                    {
                        ++differences;
                        if (differences <= 8)
                            std::printf("energy init PC%d case %d vector %d axis %d %.9g / %.9g\n", precision, sample,
                                        v, k, e, a);
                    }
                }
            for (int off = 48; off < 128; off += 8)
            {
                double e = read<double>(expected, off), a = read<double>(actual, off);
                if (e != a)
                {
                    ++differences;
                    if (differences <= 8)
                        std::printf("energy init PC%d case %d field %x %.17g / %.17g\n", precision, sample, off, e, a);
                }
            }
        }
        std::printf("energy init PC%d mismatches: %d\n", precision, differences - before);
    }
    return differences == 0;
}

static bool energy_pair_kernel_test()
{
    typedef void(__thiscall * Remove)(void *);
    IVP_Application_Environment app;
    IVP_Environment *environment =
        IVP_Environment_Manager::get_environment_manager()->create_environment(&app, "NeMo", 0x7EFAD621);
    const double matrix[12] = {0.99162775217158472, 0.1152135508242683,  -0.058311566859362722, -0.127985699144431,
                               0.93689558027438291, -0.3253403333078127, 0.017148234242667058,  0.33007955006148321,
                               0.94379734514013514, -148.00155460277944, 18.171561697474196,    -361.54110068526035};
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        const int before = differences;
        for (int sample = 0; sample < 128; ++sample)
        {
            __declspec(align(16)) unsigned char original_pair[0x38] = {}, original_environment[0x200] = {};
            __declspec(align(16)) unsigned char original[2][0x200] = {}, native[2][sizeof(IVP_Core)] = {};
            IVP_Core *cores[2] = {reinterpret_cast<IVP_Core *>(native[0]), reinterpret_cast<IVP_Core *>(native[1])};
            IVP_Friction_Core_Pair pair;
            for (int j = 0; j < 2; ++j)
            {
                pair.objs[j] = cores[j];
                write<void *>(original_pair, 0x2C + j * 4, original[j]);
                cores[j]->environment = environment;
                write<void *>(original[j], 0xC, original_environment);
                const float inverse[4] = {4.51443338394165f, 4.516299724578857f, 4.508676052093506f, 5.0f};
                const float inertia[4] = {1.0f / inverse[0], 1.0f / inverse[1], 1.0f / inverse[2], 0.2f};
                std::memcpy(original[j] + 0x14, inertia, 16);
                std::memcpy(const_cast<IVP_U_Float_Point *>(cores[j]->get_rot_inertia()), inertia, 16);
                std::memcpy(original[j] + 0x34, inverse, 16);
                std::memcpy(const_cast<IVP_U_Point_4 *>(cores[j]->get_inv_masses()), inverse, 16);
                if (j == 0)
                {
                    original[j][0] = 4;
                    cores[j]->physical_unmoveable = IVP_TRUE;
                }
                for (int r = 0; r < 3; ++r)
                    for (int k = 0; k < 3; ++k)
                        cores[j]->m_world_f_core_last_psi.set_elem(r, k, j ? matrix[r * 3 + k] : (r == k ? 1.0 : 0.0));
                cores[j]->m_world_f_core_last_psi.vv.set(0, 0, 0);
                std::memcpy(original[j] + 0x128, &cores[j]->m_world_f_core_last_psi, sizeof(IVP_U_Matrix));
                if (j == 1)
                {
                    cores[j]->speed.set(-0.040293533354997635f, -0.08594444394111633f, 0.7729243636131287f);
                    cores[j]->rot_speed.set(0.297102689743042f, -0.04810178279876709f + sample * 0.00008642f,
                                            -0.007944159209728241f);
                }
                std::memcpy(original[j] + 0xA4, cores[j]->speed.k, 12);
                std::memcpy(original[j] + 0x94, cores[j]->rot_speed.k, 12);
            }
            const float energy = sample == 0 ? 0.0f : sample == 1 ? -1.3137e-9f : sample * 1.3137e-9f;
            pair.integrated_anti_energy = energy;
            write<float>(original_pair, 0x28, energy);
            write<double>(original_environment, 0x148, IVP_Environment_External_Clock::energy_damping(environment));
            environment->get_statistic_manager()->sum_energy_destr = 0.0;
            at<Remove>(0x1CCD0)(original_pair);
            pair.remove_energy_gained_by_real_friction();
            if (read<float>(original_pair, 0x28) != pair.integrated_anti_energy ||
                read<double>(original_environment, 0x70) != environment->get_statistic_manager()->sum_energy_destr)
            {
                ++differences;
                if (differences <= 8)
                    std::printf("pair energy PC%d case %d remainder %.9g / %.9g\n", precision, sample,
                                read<float>(original_pair, 0x28), pair.integrated_anti_energy);
            }
            for (int j = 0; j < 2; ++j)
                for (int k = 0; k < 3; ++k)
                    if (read<float>(original[j], 0x74 + k * 4) != cores[j]->rot_speed_change.k[k] ||
                        read<float>(original[j], 0x84 + k * 4) != cores[j]->speed_change.k[k])
                    {
                        ++differences;
                        if (differences <= 8)
                            std::printf("pair energy push PC%d case %d core %d axis %d %.9g / %.9g\n", precision,
                                        sample, j, k, read<float>(original[j], 0x74 + k * 4),
                                        cores[j]->rot_speed_change.k[k]);
                    }
        }
        std::printf("pair energy PC%d mismatches: %d\n", precision, differences - before);
    }
    return differences == 0;
}

// Same-input oracle for mutual-energy removal before normal-pressure solving.
static bool energy_kernel_test()
{
    typedef void(__thiscall * Potential)(void *);
    typedef void(__thiscall * Destroy)(void *, double);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        const int before = differences;
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            __declspec(align(16)) unsigned char expected[160] = {}, actual[sizeof(IVP_Mutual_Energizer)] = {};
            __declspec(align(16)) unsigned char expected_cores[2][0x200] = {}, actual_cores[2][sizeof(IVP_Core)] = {};
            IVP_Core *cores[2] = {reinterpret_cast<IVP_Core *>(actual_cores[0]),
                                  reinterpret_cast<IVP_Core *>(actual_cores[1])};
            const float vectors[12] = {-0.051773f, -0.110424f, 0.992533f,  0.0f,      0.987624f, -0.159896f,
                                       -0.026379f, 0.0f,       -0.987624f, 0.159896f, 0.026379f, 0.0f};
            std::memcpy(expected, vectors, 48);
            const double values[10] = {0.77874213579 + sample * 0.0013579, 2000.0, 0.2,    0.0005,       5.0,
                                       0.30123456789 + sample * 0.0008642, 2214.0, 0.2214, 1.0 / 2214.0, 1.0 / 0.2214};
            std::memcpy(expected + 48, values, sizeof(values));
            std::memcpy(actual, expected, 128);
            for (int j = 0; j < 2; ++j)
            {
                write<void *>(expected, 144 + j * 4, expected_cores[j]);
                write<void *>(actual, 144 + j * 4, cores[j]);
                if (j == 0)
                {
                    expected_cores[j][0] = 4;
                    cores[j]->physical_unmoveable = IVP_TRUE;
                }
            }
            at<Potential>(0x1D060)(expected);
            auto *energizer = reinterpret_cast<IVP_Mutual_Energizer *>(actual);
            energizer->calc_energy_potential();
            for (int off = 128; off <= 152; off += 8)
            {
                if (off == 144)
                    continue;
                double e = read<double>(expected, off), a = read<double>(actual, off);
                if (e != a)
                {
                    ++differences;
                    if (differences <= 8)
                        std::printf("energy PC%d case %d field %x %.17g / %.17g\n", precision, sample, off, e, a);
                }
            }
            const double fraction = 0.0001 + sample * 0.003137;
            at<Destroy>(0x1D0E0)(expected, fraction);
            energizer->destroy_percent_energy(fraction);
            for (int j = 0; j < 2; ++j)
                for (int k = 0; k < 3; ++k)
                {
                    float er = read<float>(expected_cores[j], 0x74 + k * 4), ar = cores[j]->rot_speed_change.k[k];
                    float ev = read<float>(expected_cores[j], 0x84 + k * 4), av = cores[j]->speed_change.k[k];
                    if (er != ar || ev != av)
                    {
                        ++differences;
                        if (differences <= 8)
                            std::printf("energy push PC%d case %d core %d axis %d %.9g / %.9g\n", precision, sample, j,
                                        k, er, ar);
                    }
                }
        }
        std::printf("energy PC%d mismatches: %d\n", precision, differences - before);
    }
    std::printf("energy kernel mismatches: %d\n", differences);
    return differences == 0;
}

enum Contact_Kernel
{
    LOCAL_FRICTION,
    PRETENSION,
    PRESSURE,
    SURFACE_SPEED
};

static bool friction_kernel(void *contact, IVP_Core *core, const IVP_Event_Sim &event, int frame, Contact_Kernel kernel)
{
    // The original kernel reads only these audited core/long-term fields.
    // Build independent retail transports from identical native input values.
    __declspec(align(16)) unsigned char core_packet[0x200] = {}, info_packet[0xE0] = {}, contact_packet[0x70] = {};
    const IVP_Impact_Solver_Long_Term *info = read<IVP_Impact_Solver_Long_Term *>(contact, 0x40);
    if (!info || info->contact_core[0] != core || info->contact_core[1])
        return false;
    IVP_Friction_System *system = core->moveable_core_has_friction_info()->l_friction_system;
    if (kernel == PRETENSION && (system->get_fr_dist_number() != 1 || system->get_first_friction_dist() != contact))
        return false;
    unsigned char saved_contact[0x70];
    std::memcpy(saved_contact, contact, sizeof(saved_contact));
    const float spring0 = read<float>(contact, 0x38), spring1 = read<float>(contact, 0x3C);
    if (frame % 4 == 1)
    {
        write<float>(contact, 0x38, 0.012345f);
        write<float>(contact, 0x3C, -0.004321f);
    }
    if (frame % 4 == 2)
    {
        write<float>(contact, 0x38, -0.000193f);
        write<float>(contact, 0x3C, 0.000129f);
    }
    std::memcpy(core_packet + 0x34, core->get_inv_masses(), 16);
    std::memcpy(core_packet + 0x94, &core->rot_speed, 16);
    std::memcpy(core_packet + 0xA4, &core->speed, 16);
    std::memcpy(core_packet + 0x128, core->get_m_world_f_core_PSI(), sizeof(IVP_U_Matrix));
    std::memcpy(info_packet, info, sizeof(info_packet));
    write<void *>(info_packet, 0x78, core_packet);
    write<void *>(info_packet, 0x7C, NULL);
    std::memcpy(contact_packet, contact, sizeof(contact_packet));
    write<void *>(contact_packet, 0x40, info_packet);
    const IVP_U_Float_Point speed = core->speed, angular = core->rot_speed;
    const float energy = read<float>(contact, 0x50);
    typedef float(__thiscall * Friction)(void *, const IVP_Event_Sim *);
    float expected = 0, actual = 0;
    bool surface_equal = true;
    if (kernel == PRETENSION)
    {
        unsigned char system_packet[0x48] = {};
        write<void *>(system_packet, 0x20, contact_packet);
        write<short>(system_packet, 0x3E, 1);
        typedef void(__thiscall * Controller)(void *, const IVP_Event_Sim *, void *);
        at<Controller>(0x1D750)(system_packet, &event, NULL);
        IVP_Event_Sim local_event = event;
        system->do_simulation_controller(&local_event, NULL);
        expected = read<float>(contact_packet, 0x48);
        actual = read<float>(contact, 0x48);
    }
    else if (kernel == PRESSURE)
    {
        typedef void(__thiscall * Pressure)(void *, const IVP_Event_Sim *, float, float);
        at<Pressure>(0x1AE40)(contact_packet, &event, 0.02f, 1.0f);
        native_pressure(contact, &event);
        expected = read<float>(contact_packet, 0x54);
        actual = read<float>(contact, 0x54);
    }
    else if (kernel == SURFACE_SPEED)
    {
        typedef void(__thiscall * Surface_Speed)(void *, const IVP_U_Float_Point *, const IVP_U_Float_Point *,
                                                 const IVP_U_Float_Point *, IVP_U_Float_Point *);
        IVP_U_Float_Point expected_speed, actual_speed;
        at<Surface_Speed>(0xBF90)(core_packet, &info->contact_point_cs[0], &speed, &angular, &expected_speed);
        core->get_surface_speed_on_test(&info->contact_point_cs[0], &speed, &angular, &actual_speed);
        for (int i = 0; i < 3; ++i)
            if (expected_speed.k[i] != actual_speed.k[i])
            {
                surface_equal = false;
                std::printf("surface speed frame %d axis %d %.9g / %.9g\n", frame, i, expected_speed.k[i],
                            actual_speed.k[i]);
            }
    }
    else
    {
        expected = at<Friction>(0x1B080)(contact_packet, &event);
        actual = native_friction(contact, &event);
    }
    bool same = surface_equal && expected == actual;
    for (int i = 0; i < 3; ++i)
        same = same && read<float>(core_packet, 0xA4 + i * 4) == core->speed.k[i] &&
               read<float>(core_packet, 0x94 + i * 4) == core->rot_speed.k[i];
    if (!same)
        std::printf("%s kernel frame %d energy %.9g / %.9g speed X %.9g / %.9g angular Z %.9g / %.9g\n",
                    kernel == PRETENSION      ? "pretension"
                    : kernel == PRESSURE      ? "pressure"
                    : kernel == SURFACE_SPEED ? "surface"
                                              : "friction",
                    frame, expected, actual, read<float>(core_packet, 0xA4), core->speed.k[0],
                    read<float>(core_packet, 0x9C), core->rot_speed.k[2]);
    core->speed = speed;
    core->rot_speed = angular;
    write<float>(contact, 0x50, energy);
    write<float>(contact, 0x38, spring0);
    write<float>(contact, 0x3C, spring1);
    std::memcpy(contact, saved_contact, sizeof(saved_contact));
    return same;
}

static bool dynamics_test(const char *scenario)
{
    typedef void(__thiscall * Construct)(void *);
    typedef void *(__cdecl * Manager)();
    typedef void *(__thiscall * Environment)(void *, void *, const char *, unsigned int);
    typedef void(__thiscall * Gravity)(void *, const IVP_U_Point *);
    typedef void *(__thiscall * Ball)(void *, const float *, void *, const IVP_U_Quat *, const IVP_U_Point *);
    typedef void(__thiscall * Enable)(void *, IVP_BOOL);
    typedef void(__thiscall * Wake)(void *);
    typedef void(__thiscall * Simulate)(void *, double);
    unsigned char app[0x30];
    at<Construct>(0x15E70)(app);
    void *env = at<Environment>(0x137E0)(at<Manager>(0x138B0)(), app, "NeMo", 0x7EFAD621);
    IVP_Application_Environment rebuilt_app;
    IVP_Environment *rebuilt_env =
        IVP_Environment_Manager::get_environment_manager()->create_environment(&rebuilt_app, "NeMo", 0x7EFAD621);
    IVP_U_Point gravity(0, -9.81, 0);
    at<Gravity>(0x13680)(env, &gravity);
    rebuilt_env->set_gravity(&gravity);
    std::printf("default PSI retail %.17g rebuilt %.17g\n", read<double>(env, 0xC0), rebuilt_env->get_delta_PSI_time());
    std::printf("retail energy damping %.17g\n", read<double>(env, 0x148));
    unsigned char object_template[0x70];
    at<Construct>(0x15EF0)(object_template);
    IVP_Template_Real_Object rebuilt_template;
    typedef void(__thiscall * Material)(void *, double, double);
    unsigned char material[0x30];
    at<Material>(0xBF00)(material, 0.5, 0.5);
    IVP_Material_Simple rebuilt_material(0.5, 0.5);
    write<void *>(object_template, 0x14, material);
    rebuilt_template.material = &rebuilt_material;
    const bool damped = std::strcmp(scenario, "damped") == 0;
    const bool impact = std::strcmp(scenario, "impact") == 0;
    const bool pretension = std::strcmp(scenario, "pretension") == 0 || std::strcmp(scenario, "slope_pretension") == 0;
    const bool pressure = std::strcmp(scenario, "pressure") == 0 || std::strcmp(scenario, "slope_pressure") == 0;
    const bool surface = std::strcmp(scenario, "surface") == 0 || std::strcmp(scenario, "slope_surface") == 0;
    const bool kernel = pretension || pressure || surface || std::strcmp(scenario, "friction") == 0 ||
                        std::strcmp(scenario, "slope_friction") == 0;
    const bool slope = std::strncmp(scenario, "slope", 5) == 0;
    const bool rolling = (kernel && !slope) || std::strcmp(scenario, "rolling") == 0;
    const bool gyro = std::strcmp(scenario, "gyro") == 0;
    const bool pair = std::strcmp(scenario, "pair") == 0;
    const bool mesh = rolling || slope || std::strcmp(scenario, "mesh") == 0;
    double damping = damped ? 0.37 : 0.0;
    write<double>(object_template, 0x38, damping);
    rebuilt_template.speed_damp_factor = damping;
    if (gyro)
    {
        write<float>(object_template, 0x24, 1.0f);
        write<float>(object_template, 0x28, 0.6f);
        write<float>(object_template, 0x2C, 1.8f);
        rebuilt_template.rot_inertia.set(1.0f, 0.6f, 1.8f);
    }
    float radius = 0.5f;
    IVP_Template_Ball rebuilt_ball;
    rebuilt_ball.radius = radius;
    IVP_U_Point position(0, rolling ? 0.53 : 5.0, 0);
    void *ball = at<Ball>(0x13A10)(env, &radius, object_template, NULL, &position);
    IVP_Ball *rebuilt = rebuilt_env->create_ball(&rebuilt_ball, &rebuilt_template, NULL, &position);
    void *partner = NULL;
    IVP_Ball *rebuilt_partner = NULL;
    if (pair)
    {
        IVP_U_Point partner_position(3, 5, 0.3);
        partner = at<Ball>(0x13A10)(env, &radius, object_template, NULL, &partner_position);
        rebuilt_partner = rebuilt_env->create_ball(&rebuilt_ball, &rebuilt_template, NULL, &partner_position);
        typedef void(__thiscall * Speed)(void *, const IVP_U_Float_Point *);
        IVP_U_Float_Point velocity0(2.5f, 0, 0), velocity1(-2.5f, 0, 0);
        IVP_U_Float_Point angular0(1.3f, -0.8f, 2.1f), angular1(-0.4f, 1.7f, 0.6f);
        at<Speed>(0xA350)(ball, &velocity0);
        rebuilt->async_add_speed_object_ws(&velocity0);
        at<Speed>(0xA350)(partner, &velocity1);
        rebuilt_partner->async_add_speed_object_ws(&velocity1);
        at<Speed>(0xA3A0)(ball, &angular0);
        rebuilt->async_add_rot_speed_object_cs(&angular0);
        at<Speed>(0xA3A0)(partner, &angular1);
        rebuilt_partner->async_add_rot_speed_object_cs(&angular1);
        at<Enable>(0x9350)(partner, IVP_TRUE);
        rebuilt_partner->enable_collision_detection(IVP_TRUE);
        at<Wake>(0xA460)(partner);
        rebuilt_partner->ensure_in_simulation();
    }
    if (impact)
    {
        unsigned char floor_template[0x70];
        at<Construct>(0x15EF0)(floor_template);
        write<int>(floor_template, 0xC, IVP_TRUE);
        IVP_Template_Real_Object rebuilt_floor;
        rebuilt_floor.physical_unmoveable = IVP_TRUE;
        write<void *>(floor_template, 0x14, material);
        rebuilt_floor.material = &rebuilt_material;
        float floor_radius = 100.0f;
        IVP_Template_Ball rebuilt_floor_ball;
        rebuilt_floor_ball.radius = floor_radius;
        IVP_U_Point floor_position(0, -100, 0);
        void *floor = at<Ball>(0x13A10)(env, &floor_radius, floor_template, NULL, &floor_position);
        IVP_Ball *rebuilt_floor_object =
            rebuilt_env->create_ball(&rebuilt_floor_ball, &rebuilt_floor, NULL, &floor_position);
        at<Enable>(0x9350)(floor, IVP_TRUE);
        rebuilt_floor_object->enable_collision_detection(IVP_TRUE);
    }
    // The surface manager's compact_surface member is the audited retail +4
    // transport field; its vtable is the original eleven-slot table.
    unsigned char surface_manager[8];
    IVP_SurfaceManager_Polygon *rebuilt_surface_manager = NULL;
    IVP_Compact_Surface *rebuilt_surface = NULL;
    void *retail_surface = NULL;
    if (mesh)
    {
        typedef IVP_Compact_Ledge *(__cdecl * Build)(void *);
        typedef void(__thiscall * Insert)(void *, void *);
        typedef void *(__thiscall * Compile)(void *, void *);
        typedef void *(__thiscall * Polygon)(void *, void *, void *, const IVP_U_Quat *, const IVP_U_Point *);
        IVP_U_Point vertices[8];
        IVP_U_Vector<IVP_U_Point> points;
        for (int i = 0; i < 8; ++i)
        {
            double x = (i & 1) ? 30.0 : -30.0, z = (i & 4) ? 10.0 : -10.0;
            double y = ((i & 2) ? 0.0 : -2.0) + (slope ? x * 0.15 : 0.0);
            vertices[i].set(x, y, z);
            points.add(&vertices[i]);
        }
        IVP_Compact_Ledge *retail_ledge = at<Build>(0x3AE20)(&points);
        IVP_Compact_Ledge *rebuilt_ledge = IVP_SurfaceBuilder_Pointsoup::convert_pointsoup_to_compact_ledge(&points);
        unsigned char builder[0x200];
        at<Construct>(0x38290)(builder);
        at<Insert>(0x38430)(builder, retail_ledge);
        retail_surface = at<Compile>(0x384B0)(builder, NULL);
        at<Construct>(0x38340)(builder);
        IVP_SurfaceBuilder_Ledge_Soup rebuilt_builder;
        rebuilt_builder.insert_ledge(rebuilt_ledge);
        rebuilt_surface = rebuilt_builder.compile();
        if (!retail_surface || !rebuilt_surface)
        {
            std::fprintf(stderr, "floor construction failed\n");
            return false;
        }
        write<void *>(surface_manager, 0, retail + 0x631E0);
        write<void *>(surface_manager, 4, retail_surface);
        rebuilt_surface_manager = new IVP_SurfaceManager_Polygon(rebuilt_surface);
        unsigned char floor_template[0x70];
        at<Construct>(0x15EF0)(floor_template);
        write<int>(floor_template, 0xC, IVP_TRUE);
        write<void *>(floor_template, 0x14, material);
        IVP_Template_Real_Object rebuilt_floor;
        rebuilt_floor.physical_unmoveable = IVP_TRUE;
        rebuilt_floor.material = &rebuilt_material;
        IVP_U_Point origin(0, 0, 0);
        void *floor = at<Polygon>(0x139A0)(env, surface_manager, floor_template, NULL, &origin);
        IVP_Real_Object *rebuilt_floor_object =
            rebuilt_env->create_polygon(rebuilt_surface_manager, &rebuilt_floor, NULL, &origin);
        at<Enable>(0x9350)(floor, IVP_TRUE);
        rebuilt_floor_object->enable_collision_detection(IVP_TRUE);
        // Default ledge-soup parameters transfer ownership of both input ledges.
    }
    if (rolling)
    {
        typedef void(__thiscall * Speed)(void *, const IVP_U_Float_Point *);
        IVP_U_Float_Point initial_speed(2.5f, 0, 0);
        at<Speed>(0xA350)(ball, &initial_speed);
        rebuilt->async_add_speed_object_ws(&initial_speed);
    }
    if (gyro)
    {
        typedef void(__thiscall * Speed)(void *, const IVP_U_Float_Point *);
        IVP_U_Float_Point initial_speed(12.0f, 17.0f, 9.0f);
        at<Speed>(0xA3A0)(ball, &initial_speed);
        rebuilt->async_add_rot_speed_object_cs(&initial_speed);
    }
    at<Enable>(0x9350)(ball, IVP_TRUE);
    rebuilt->enable_collision_detection(IVP_TRUE);
    at<Wake>(0xA460)(ball);
    rebuilt->ensure_in_simulation();
    void *core = read<void *>(ball, 0xA4);
    void *retail_cores[2] = {core, partner ? read<void *>(partner, 0xA4) : NULL};
    IVP_Core *native_cores[2] = {rebuilt->get_core(), rebuilt_partner ? rebuilt_partner->get_core() : NULL};
    double max_position = 0, max_speed = 0, max_rotation_speed = 0, max_rotation = 0;
    int first = -1, first_quaternion = -1, first_matrix = -1;
    int kernel_cases = 0, kernel_differences = 0;
    unsigned int pair_collision_mask = 0;
    for (int frame = 0; frame < 240; ++frame)
    {
        active_frame = frame;
        active_call = "retail simulate";
        at<Simulate>(0x138C0)(env, 1.0 / 60.0);
        active_call = "rebuilt simulate";
        rebuilt_env->simulate_dtime(1.0 / 60.0);
        for (int body = 0; body < (pair ? 2 : 1); ++body)
        {
            core = retail_cores[body];
            IVP_Core *native_core = native_cores[body];
            if (pair && std::fabs(native_core->speed.k[0] - (body ? -2.5f : 2.5f)) > 0.25f)
                pair_collision_mask |= 1u << body;
            for (int row = 0; row < 3; ++row)
                for (int column = 0; column < 3; ++column)
                {
                    const double e = read<double>(core, 0x128 + row * 32 + column * 8),
                                 a = native_core->get_m_world_f_core_PSI()->get_elem(row, column);
                    if (first_matrix < 0 && e != a)
                    {
                        first_matrix = frame;
                        std::printf("first matrix difference frame %d body %d element %d/%d %.17g / %.17g\n", frame,
                                    body, row, column, e, a);
                    }
                }
            if (kernel)
            {
                IVP_Friction_Info_For_Core *fi = native_core->moveable_core_has_friction_info();
                if (fi && fi->friction_springs.len())
                {
                    IVP_Event_Sim event(rebuilt_env);
                    ++kernel_cases;
                    kernel_differences +=
                        !friction_kernel(fi->friction_springs.element_at(0), native_core, event, frame,
                                         pretension ? PRETENSION
                                         : pressure ? PRESSURE
                                         : surface  ? SURFACE_SPEED
                                                    : LOCAL_FRICTION);
                }
            }
            for (int axis = 0; axis < 3; ++axis)
            {
                double ep = read<double>(core, 0xB8 + axis * 8), ap = native_core->pos_world_f_core_last_psi.k[axis];
                double ev = read<float>(core, 0xA4 + axis * 4), av = native_core->speed.k[axis];
                max_position = ParityMaximum(max_position, std::fabs(ep - ap));
                max_speed = ParityMaximum(max_speed, std::fabs(ev - av));
                const double er = read<float>(core, 0x94 + axis * 4), ar = native_core->rot_speed.k[axis];
                max_rotation_speed = ParityMaximum(max_rotation_speed, std::fabs(er - ar));
                if (first < 0 && (ev != av || er != ar || std::fabs(ep - ap) > 1e-7))
                {
                    first = frame;
                    std::printf("first divergence frame %d body %d axis %d pos %.17g / %.17g speed %.9g / %.9g angular "
                                "%.9g / %.9g\n",
                                frame, body, axis, ep, ap, ev, av, er, ar);
                }
            }
            for (int component = 0; component < 4; ++component)
            {
                double er = read<double>(core, 0x108 + component * 8),
                       ar = read<double>(&native_core->q_world_f_core_next_psi, component * 8);
                max_rotation = ParityMaximum(max_rotation, std::fabs(er - ar));
                if (first_quaternion < 0 && er != ar)
                {
                    first_quaternion = frame;
                    std::printf("first quaternion difference frame %d body %d component %d %.17g / %.17g\n", frame,
                                body, component, er, ar);
                }
            }
        }
    }
    std::printf("%s 240 frames max position error %.9g max speed error %.9g\n", scenario, max_position, max_speed);
    std::printf("max angular speed error %.9g max quaternion error %.9g\n", max_rotation_speed, max_rotation);
    delete rebuilt_env;
    at<Construct>(0x13270)(env);
    at<void(__cdecl *)(void *)>(0x60756)(env);
    delete rebuilt_surface_manager;
    if (rebuilt_surface)
        ivp_free_aligned(rebuilt_surface);
    if (retail_surface)
        at<void(__cdecl *)(void *)>(0x20130)(retail_surface);
    if (kernel)
    {
        std::printf("friction kernel mismatches %d / %d\n", kernel_differences, kernel_cases);
        return kernel_cases > 0 && kernel_differences == 0;
    }
    return (!pair || pair_collision_mask == 3) && max_position <= 1e-7 && max_speed <= 1e-6 &&
           max_rotation_speed <= 1e-6 && max_rotation <= 1e-7;
}

#define RETAIL_REGISTER_dynamics RETAIL_REGISTER
#include "RetailTestRegistration.inc"
