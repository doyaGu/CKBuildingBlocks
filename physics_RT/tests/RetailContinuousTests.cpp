#include "RetailTestSupport.h"

#include <ivp_physics.hxx>
#include <ivp_templates.hxx>
#include <ivp_range_manager.hxx>
#include <ivp_cache_object.hxx>
#include <ivp_3d_solver.hxx>
#include <ivp_mindist.hxx>
#include <ivp_mindist_event.hxx>
#include <ivp_calc_next_psi_solver.hxx>
#include <ivp_cache_ledge_point.hxx>
#include <ivu_memory.hxx>
#if IVP_ENABLE_X87_EMULATION
#include <ivu_x87.hxx>
#endif

#include <cmath>
#include <new>

#include "fixtures/continuous_pf.hxx"
#include "fixtures/continuous_pf_13114.hxx"
#include "fixtures/continuous_pf_13115.hxx"
#include "fixtures/continuous_pk_collision_13056.hxx"
#include "fixtures/continuous_vec_plane_13132.hxx"
#include "fixtures/early_events_12860.hxx"
#include "fixtures/event_speed_web_port.hxx"
#include "fixtures/pk_kk_14391.hxx"

// clang-format off
#pragma comment(linker, "/alternatename:_parity_native_pf_value_entry=?get_value@IVP_3D_Solver_PF_COLL@@UAENPAVIVP_U_Matrix@@0@Z")
#pragma comment(linker, "/alternatename:_parity_native_pk_kk_value_entry=?get_value@IVP_3D_Solver_PK_KK@@UAENPAVIVP_U_Matrix@@0@Z")
#pragma comment(linker, "/alternatename:_parity_native_vec_parallel_entry=?get_value@IVP_3D_Solver_VEC_PARALLEL_AREA@@UAENPAVIVP_U_Matrix@@0@Z")
#pragma comment(linker, "/alternatename:_parity_native_pk_collision_entry=?get_value@IVP_3D_Solver_PK_COLL@@UAENPAVIVP_U_Matrix@@0@Z")
#pragma comment(linker, "/alternatename:_parity_native_point_distance_entry=?get_value@IVP_3D_Solver_DISTANCE_OF_TWO_POINTS@@UAENPAVIVP_U_Matrix@@0@Z")
#pragma comment(linker, "/alternatename:_parity_native_s_values_entry=?get_value@IVP_3D_Solver_S_VALS@@UAENPAVIVP_U_Matrix@@0@Z")
// clang-format on

// These two audited, branch-free instruction ranges use only register-based
// inputs, fixed stack output slots, and relocated constants in the loaded DLL.
// Execute copies with a return epilogue; the original DLL is never patched.
static void *event_speed_oracle_fragment(unsigned int begin, unsigned int end, int mode)
{
    const unsigned int length = end - begin;
    auto *code =
        static_cast<unsigned char *>(VirtualAlloc(NULL, length + 12, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (!code)
        return NULL;
    std::memcpy(code, retail + begin, length);
    unsigned int cursor = length;
    if (mode == 1)
    {
        // fstp tbyte ptr [esp+0x18], then discard the two remaining axis dots.
        const unsigned char store[] = {0xDB, 0x7C, 0x24, 0x18};
        std::memcpy(code + cursor, store, sizeof(store));
        cursor += sizeof(store);
        for (int i = 0; i < 2; ++i)
        {
            code[cursor++] = 0xDD;
            code[cursor++] = 0xD8;
        }
    }
    if (mode == 2)
    {
        const unsigned char result[] = {0x0F, 0x95, 0xC0, 0x0F, 0xB6, 0xC0}; // setnz al; movzx eax,al
        std::memcpy(code + cursor, result, sizeof(result));
        cursor += sizeof(result);
    }
    code[cursor++] = 0xC3;
    DWORD previous;
    if (!VirtualProtect(code, cursor, PAGE_EXECUTE_READ, &previous))
    {
        VirtualFree(code, 0, MEM_RELEASE);
        return NULL;
    }
    FlushInstructionCache(GetCurrentProcess(), code, cursor);
    return code;
}

static __declspec(naked) int __cdecl run_event_distance_oracle(void *, void **)
{
    __asm {
        sub esp, 50h
        mov edx, [esp+58h]
        mov ecx, [edx+4]
        mov eax, [edx+8]
        fld qword ptr [eax]
        fstp qword ptr [esp+44h]
        mov eax, [edx+0Ch]
        mov [esp+10h], eax
        mov eax, [edx]
        fld tbyte ptr [eax]
        mov eax, [edx+10h]
        fld dword ptr [eax]
        call dword ptr [esp+54h]
        add esp, 50h
        ret
    }
}

static __declspec(naked) void __cdecl run_event_speed_oracle(void *, void **, double *, bool, unsigned char *)
{
    __asm {
        push ebx
        push esi
        push edi
        sub esp, 40h
        mov edx, [esp+54h]
        mov esi, [edx]
        mov edi, [edx+4]
        mov ebx, [edx+8]
        mov ecx, [edx+0Ch]
        mov eax, [edx+10h]
        call dword ptr [esp+50h]
        mov edx, [esp+58h]
        cmp byte ptr [esp+5Ch], 0
        jne event_projected_outputs
        fld qword ptr [esp+1Ch]
        fstp qword ptr [edx]
        fld qword ptr [esp+34h]
        jmp event_outputs_done
    event_projected_outputs:
        mov ecx, [esp+60h]
        mov eax, [esp+14h]
        mov [ecx], eax
        mov eax, [esp+18h]
        mov [ecx+4], eax
        mov ax, [esp+1Ch]
        mov [ecx+8], ax
        fld qword ptr [esp+24h]
        fstp qword ptr [edx]
        fld qword ptr [esp+2Ch]
    event_outputs_done:
        fstp qword ptr [edx+8]
        add esp, 40h
        pop edi
        pop esi
        pop ebx
        ret
    }
}

static bool event_speed_test()
{
    void *bounds = event_speed_oracle_fragment(0x178B7, 0x178F0, false);
    void *projected = event_speed_oracle_fragment(0x1797E, 0x17A2E, true);
    void *distance = event_speed_oracle_fragment(0x17A87, 0x17AA4, 2);
    if (!bounds || !projected || !distance)
    {
        if (bounds)
            VirtualFree(bounds, 0, MEM_RELEASE);
        if (projected)
            VirtualFree(projected, 0, MEM_RELEASE);
        if (distance)
            VirtualFree(distance, 0, MEM_RELEASE);
        return false;
    }
    const int fixtures = sizeof(eventSpeedFixtures) / sizeof(eventSpeedFixtures[0]);
    int differences = 0, fixture_differences = 0, gate_checks = 0, gate_differences = 0;
    unsigned int seed = 0x25331793;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < fixtures + 128; ++sample)
        {
            float input[17];
            if (sample < fixtures)
                std::memcpy(input, eventSpeedFixtures[sample].input, sizeof(input));
            else
            {
                for (int k = 0; k < 17; ++k)
                {
                    seed = 1664525 * seed + 1013904223;
                    input[k] = (int(seed >> 8) - 8388608) * 0.0000001379f;
                }
                const int offsets[] = {0, 9, 12};
                for (int offset : offsets)
                {
                    const double length = std::sqrt(double(input[offset]) * input[offset] +
                                                    double(input[offset + 1]) * input[offset + 1] +
                                                    double(input[offset + 2]) * input[offset + 2]);
                    for (int k = 0; k < 3; ++k)
                        input[offset + k] = static_cast<float>(input[offset + k] / length);
                }
                input[15] = std::fabs(input[15]) * 13.179f;
                input[16] = sample % 4 ? std::fabs(input[16]) * 7.319f : 0.0f;
            }
            alignas(16) unsigned char old_mindist[0x90] = {}, old_cores[2][0x200] = {},
                                      native_cores[2][sizeof(IVP_Core)] = {}, old_objects[2][0xB0] = {};
            std::memcpy(old_mindist + 0x68, input, 12);
            auto *core0 = reinterpret_cast<IVP_Core *>(native_cores[0]),
                 *core1 = reinterpret_cast<IVP_Core *>(native_cores[1]);
            for (int side = 0; side < 2; ++side)
            {
                auto *core = reinterpret_cast<IVP_Core *>(native_cores[side]);
                std::memcpy(core->speed.k, input + 3 + side * 3, 12);
                std::memcpy(core->rotation_axis_world_space.k, input + 9 + side * 3, 12);
                core->current_speed = std::fabs(input[3 + side * 3]) + 0.73179f;
                core->max_surface_rot_speed = input[15 + side];
                std::memcpy(old_cores[side] + 0xA4, core->speed.k, 12);
                std::memcpy(old_cores[side] + 0x1A8, core->rotation_axis_world_space.k, 12);
                write<float>(old_cores[side], 0x1B8, core->current_speed);
                write<float>(old_cores[side], 0x1C0, core->max_surface_rot_speed);
                write<void *>(old_objects[side], 0xA4, old_cores[side]);
            }
            void *packet[] = {old_mindist, old_cores[0], old_cores[1], old_objects[0], old_objects[1]};
            double expected[4];
            unsigned char expected_extended[10], actual_extended[10] = {};
            IVP_U_Float80 *maximum_extended = NULL;
#if IVP_ENABLE_X87_EMULATION
            IVP_U_Float80 maximum_value;
            maximum_extended = &maximum_value;
#endif
            run_event_speed_oracle(bounds, packet, expected, false, NULL);
            run_event_speed_oracle(projected, packet, expected + 2, true, expected_extended);
            if (precision == 2 && sample < fixtures &&
                std::memcmp(expected + 2, eventSpeedFixtures[sample].expected, 16) != 0)
                ++fixture_differences;
            IVP_U_Float_Point plane;
            std::memcpy(plane.k, input, 12);
            IVP_Mindist_Event_Solver solver;
            solver.calculate_speed_bounds(core0, core1);
            solver.calculate_projected_speed(&plane, core0, core1, maximum_extended);
#if IVP_ENABLE_X87_EMULATION
            maximum_value.to_bytes(actual_extended);
#endif
            const double actual[] = {solver.sum_max_surface_rot_speed, solver.worst_case_speed,
                                     solver.projected_center_speed, solver.max_coll_speed};
            if (std::memcmp(expected, actual, sizeof(expected)) != 0 ||
                std::memcmp(expected_extended, actual_extended, 10) != 0)
            {
                ++differences;
                if (differences <= 5)
                    std::printf("event speed PC%d sample %d projection %.17g / %.17g max %.17g / %.17g\n", precision,
                                sample, expected[2], actual[2], expected[3], actual[3]);
            }
            if (expected[3] > 0.01)
            {
                unsigned char old_environment[0x130] = {};
                const double now = 17.713793179, next = now + 1.0 / 66.0;
                const float collision_distance = 0.001f;
                write<double>(old_environment, 0x128, next);
                const float boundary = static_cast<float>(collision_distance + (next - now) * expected[3]);
                for (int direction = -1; direction <= 1; ++direction)
                {
                    const float length =
                        direction == 0 ? boundary : std::nextafter(boundary, direction < 0 ? -FLT_MAX : FLT_MAX);
                    void *gate_input[] = {expected_extended, old_environment, const_cast<double *>(&now),
                                          const_cast<float *>(&collision_distance), const_cast<float *>(&length)};
                    const bool expected_gate = run_event_distance_oracle(distance, gate_input) != 0;
                    const bool actual_gate =
                        solver.collision_is_possible(maximum_extended, now, next, length, collision_distance);
                    ++gate_checks;
                    if (expected_gate != actual_gate)
                        ++gate_differences;
                }
            }
        }
    }
    VirtualFree(bounds, 0, MEM_RELEASE);
    VirtualFree(projected, 0, MEM_RELEASE);
    VirtualFree(distance, 0, MEM_RELEASE);
    std::printf("event speed mismatches: %d / %d; external reference mismatches: %d / %d\n", differences,
                3 * (fixtures + 128), fixture_differences, fixtures);
    std::printf("event distance gate mismatches: %d / %d\n", gate_differences, gate_checks);
    return differences == 0 && fixture_differences == 0 && gate_checks > 0 && gate_differences == 0;
}

static bool range_intra_kernel_test()
{
    IVP_Application_Environment app;
    IVP_Environment *environment =
        IVP_Environment_Manager::get_environment_manager()->create_environment(&app, "NeMo", 0x7EFAD621);
    environment->set_delta_PSI_time(1.0 / 66.0);
    IVP_Range_Manager manager(environment, IVP_FALSE);
    alignas(16) unsigned char old_environment[0xD0] = {}, old_manager[0x80] = {};
    write<double>(old_environment, 0xC0, environment->get_delta_PSI_time());
    at<void(__thiscall *)(void *, void *, IVP_BOOL)>(0x2D8E0)(old_manager, old_environment, IVP_FALSE);
    for (int i = 0; i < 5; ++i)
        std::printf("retail intra setting%d %.17g\n", i, read<double>(old_manager, 16 + i * 8));
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            alignas(16) unsigned char old_cores[2][0x200] = {}, native_cores[2][sizeof(IVP_Core)] = {};
            alignas(16) unsigned char old_objects[2][0xB0] = {}, native_objects[2][sizeof(IVP_Real_Object)] = {};
            IVP_Real_Object *objects[2];
            for (int j = 0; j < 2; ++j)
            {
                IVP_Core *core = reinterpret_cast<IVP_Core *>(native_cores[j]);
                objects[j] = reinterpret_cast<IVP_Real_Object *>(native_objects[j]);
                objects[j]->physical_core = core;
                core->current_speed =
                    j ? 1.0856130123138428f + sample * 0.012531f : 15.998493194580078f + sample * 0.003179f;
                core->max_surface_rot_speed = j ? 1.0092753171920776f : 4.782212257385254f;
                core->upper_limit_radius = j ? 2.978996992111206f : 2.1265296936035156f;
                if (sample % 16 == 15)
                {
                    core->current_speed = sample * 0.0001931f;
                    core->max_surface_rot_speed = 0.0f;
                }
                if (sample % 16 == 14)
                    core->upper_limit_radius = 20.0f + j;
                write<void *>(old_objects[j], 0xA4, old_cores[j]);
                write<float>(old_cores[j], 0x1B8, core->current_speed);
                write<float>(old_cores[j], 0x1C0, core->max_surface_rot_speed);
                write<float>(old_cores[j], 4, core->upper_limit_radius);
            }
            double expected[2], actual[2];
            at<void(__thiscall *)(void *, void *, void *, double *, double *)>(0x2D990)(
                old_manager, old_objects[0], old_objects[1], expected, expected + 1);
            manager.get_coll_range_intra_objects(objects[0], objects[1], actual, actual + 1);
            if (expected[0] != actual[0] || expected[1] != actual[1])
            {
                ++differences;
                if (differences <= 6)
                    std::printf("intra range PC%d sample%d %.17g %.17g / %.17g %.17g\n", precision, sample, expected[0],
                                expected[1], actual[0], actual[1]);
            }
        }
    }
    std::printf("intra range mismatches %d / 384\n", differences);
    return differences == 0;
}

extern "C" void parity_native_pf_value_entry();

extern "C" void parity_native_pk_kk_value_entry();

static bool continuous_pk_kk_kernel_test()
{
    using Value = void(__thiscall *)(void *, IVP_U_Matrix *, IVP_U_Matrix *);
    Value original_value = at<Value>(0x2AC40);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            alignas(16) unsigned char solver[168];
            std::memcpy(solver, retail_pk_kk_solver, sizeof(solver));
            IVP_U_Matrix matrix_a, matrix_b;
            std::memcpy(&matrix_a, retail_pk_kk_matrix_a, 128);
            std::memcpy(&matrix_b, retail_pk_kk_matrix_b, 128);
            write(solver, 72, read<double>(solver, 72) + sample * 0.0000000001379);
            void *packet = solver;
            IVP_U_Matrix *a = &matrix_a, *b = &matrix_b;
            unsigned char expected[10], actual[10];
            __asm {
                push b
                push a
                mov ecx, packet
                call original_value
                fstp tbyte ptr expected
                push b
                push a
                mov ecx, packet
                call parity_native_pk_kk_value_entry
                fstp tbyte ptr actual
            }
            if (std::memcmp(expected, actual, 10) != 0)
            {
                ++differences;
                if (sample == 0 || differences <= 4)
                {
                    double e, a;
                    __asm {
                        fld tbyte ptr expected
                        fstp e
                        fld tbyte ptr actual
                        fstp a
                    }
                    std::printf("PK KK value PC%d sample%d %.17g / %.17g; raw x87 return differs\n", precision, sample,
                                e, a);
                }
            }
        }
    }
    std::printf("PK KK value mismatches: %d / 384\n", differences);
    return differences == 0;
}

static bool continuous_pf_data_test(const double (*rows)[41], int row_count)
{
    typedef double(__thiscall * Value)(void *, IVP_U_Matrix *, IVP_U_Matrix *);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int row_index = 0; row_index < row_count; ++row_index)
            for (int sample = 0; sample < 128; ++sample)
            {
                const double *row = rows[row_index];
                __declspec(align(16)) unsigned char solver[136] = {};
                for (int i = 0; i < 3; ++i)
                    std::memcpy(solver + 40 + i * 32, row + i * 3, 24);
                IVP_U_Matrix a, b;
                std::memcpy(&a, row + 9, 128);
                std::memcpy(&b, row + 25, 128);
                a.vv.k[1] += sample * 0.0000001379;
                unsigned char expected[10], actual[10];
                Value original = at<Value>(0x2A9C0);
                void *receiver = solver, *matrix_a = &a, *matrix_b = &b;
                __asm {
                mov ecx, receiver
                push matrix_b
                push matrix_a
                call original
                fstp tbyte ptr expected
                mov ecx, receiver
                push matrix_b
                push matrix_a
                call parity_native_pf_value_entry
                fstp tbyte ptr actual
                }
                if (std::memcmp(expected, actual, 10) != 0)
                {
                    ++differences;
                    if (differences <= 4)
                        std::printf("continuous PF PC%d sample %d extended result differs\n", precision, sample);
                }
            }
    }
    std::printf("continuous PF mismatches: %d / %d\n", differences, row_count * 384);
    return differences == 0;
}

static bool continuous_pf_kernel_test()
{
    return continuous_pf_data_test(retail_continuous_pf,
                                   sizeof(retail_continuous_pf) / sizeof(retail_continuous_pf[0]));
}

static bool continuous_pf_level4_test()
{
    return continuous_pf_data_test(retail_pf_13114, sizeof(retail_pf_13114) / sizeof(retail_pf_13114[0]));
}

static bool continuous_pf_time_drift_test()
{
    return continuous_pf_data_test(retail_pf_13115, sizeof(retail_pf_13115) / sizeof(retail_pf_13115[0]));
}

extern "C" void parity_native_vec_parallel_entry();

extern "C" void parity_native_pk_collision_entry();

extern "C" void parity_native_point_distance_entry();

extern "C" void parity_native_s_values_entry();

static bool continuous_early_event_test(int kind)
{
    typedef double(__thiscall * Value)(void *, IVP_U_Matrix *, IVP_U_Matrix *);
    const unsigned char *inputs[2] = {kind ? l4_early_event_solver_10 : l4_early_event_solver_00,
                                      kind ? l4_early_event_solver_11 : l4_early_event_solver_01};
    const double *matrices_a[2] = {kind ? l4_early_event_matrix_a10 : l4_early_event_matrix_a00,
                                   kind ? l4_early_event_matrix_a11 : l4_early_event_matrix_a01};
    const double *matrices_b[2] = {kind ? l4_early_event_matrix_b10 : l4_early_event_matrix_b00,
                                   kind ? l4_early_event_matrix_b11 : l4_early_event_matrix_b01};
    void(__cdecl * native)() = kind ? &parity_native_s_values_entry : &parity_native_point_distance_entry;
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int row = 0; row < 2; ++row)
            for (int sample = 0; sample < 128; ++sample)
            {
                alignas(16) unsigned char solver[136];
                std::memcpy(solver, inputs[row], 136);
                IVP_U_Matrix a, b;
                std::memcpy(&a, matrices_a[row], 128);
                std::memcpy(&b, matrices_b[row], 128);
                a.vv.k[1] += sample * 0.0000001379;
                unsigned char expected[10], actual[10];
                Value original = at<Value>(kind ? 0x2ABC0 : 0x2AB30);
                void *receiver = solver, *matrix_a = &a, *matrix_b = &b;
                __asm {
                mov ecx, receiver
                push matrix_b
                push matrix_a
                call original
                fstp tbyte ptr expected
                mov ecx, receiver
                push matrix_b
                push matrix_a
                call native
                fstp tbyte ptr actual
                }
                if (std::memcmp(expected, actual, 10) != 0)
                {
                    ++differences;
                    if (differences <= 4 || (sample == 0 && precision == 2))
                        std::printf("early event kind%d PC%d row%d sample%d extended result differs\n", kind, precision,
                                    row, sample);
                }
            }
    }
    std::printf("early event kind%d mismatches %d / 768\n", kind, differences);
    return differences == 0;
}

class ParityRootSolver : public IVP_3D_Solver
{
    IVP_DOUBLE get_value(IVP_U_Matrix *, IVP_U_Matrix *)
    {
        const int step = ++calls;
        return step >= finish_after ? target : target + (step & 1 ? 1.0 : -1.0) * response_scale / (step + 1);
    }

  public:
    int calls = 0, finish_after = 1;
    double target = 0.0, response_scale = 1.0;
    using IVP_3D_Solver::calc_nullstelle;
};

static bool continuous_secant_test(bool iterated = false)
{
    typedef void(__thiscall * Construct)(void *);
    typedef void *(__cdecl * Manager)();
    typedef void *(__thiscall * Environment)(void *, void *, const char *, unsigned int);
    typedef void *(__thiscall * Ball)(void *, const float *, void *, const IVP_U_Quat *, const IVP_U_Point *);
    // Retail returns IVP_Time through an explicit first stack output pointer.
    typedef void(__thiscall * Root)(void *, double *, double, double, double, double, double, void *, void *);
    unsigned char old_app[0x30], old_template[0x70];
    at<Construct>(0x15E70)(old_app);
    at<Construct>(0x15EF0)(old_template);
    void *old_environment = at<Environment>(0x137E0)(at<Manager>(0x138B0)(), old_app, "NeMo", 0x7EFAD621);
    write<IVP_BOOL>(old_template, 0x0C, IVP_TRUE);
    float radius = 0.5f;
    IVP_U_Point position(0, 0, 0);
    void *old_ball = at<Ball>(0x13A10)(old_environment, &radius, old_template, NULL, &position);
    IVP_Application_Environment app;
    IVP_Environment *environment =
        IVP_Environment_Manager::get_environment_manager()->create_environment(&app, "NeMo", 0x7EFAD621);
    IVP_Template_Real_Object object_template;
    object_template.physical_unmoveable = IVP_TRUE;
    IVP_Template_Ball ball_template;
    ball_template.radius = radius;
    IVP_Ball *ball = environment->create_ball(&ball_template, &object_template, NULL, &position);
    ParityRootSolver solver;
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            IVP_Time t0(sample % 4 ? 0.0 : 99.73), t1(t0.get_seconds() + (1 + sample % 5) * 0.000537912345);
            const double v0 = (sample % 13 + 1) * 0.13753123456789, v1 = -(sample % 17 + 1) * 0.237913456789;
            solver.target = iterated ? 0.02500000037252903 : 0.0;
            solver.finish_after = iterated ? 4 + sample % 13 : 1;
            solver.response_scale = ParityMinimum(v0, -v1);
            solver.calls = 0;
            double expected = 0.0;
            at<Root>(0x37790)(&solver, &expected, t0.get_seconds(), t1.get_seconds(), solver.target, v0 + solver.target,
                              v1 + solver.target, old_ball, old_ball);
            const int expected_calls = solver.calls;
            solver.calls = 0;
            const IVP_Time actual =
                solver.calc_nullstelle(t0, t1, solver.target, v0 + solver.target, v1 + solver.target, ball, ball);
            if (expected_calls != solver.calls)
            {
                ++differences;
                std::printf("secant call count %d / %d\n", expected_calls, solver.calls);
            }
            if (expected != actual.get_seconds())
            {
                ++differences;
                if (differences <= 6)
                    std::printf("secant PC%d sample%d %.17g / %.17g\n", precision, sample, expected,
                                actual.get_seconds());
            }
        }
    }
    std::printf("secant time mismatches %d / 384\n", differences);
    return differences == 0;
}

static bool continuous_pk_collision_test()
{
    typedef double(__thiscall * Value)(void *, IVP_U_Matrix *, IVP_U_Matrix *);
    const unsigned char *inputs[2] = {l4_pk_collision_solver0, l4_pk_collision_solver1};
    const double *matrices_a[2] = {l4_pk_collision_matrix_a0, l4_pk_collision_matrix_a1},
                 *matrices_b[2] = {l4_pk_collision_matrix_b0, l4_pk_collision_matrix_b1};
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int row = 0; row < 2; ++row)
            for (int sample = 0; sample < 128; ++sample)
            {
                alignas(16) unsigned char solver[144];
                std::memcpy(solver, inputs[row], 144);
                IVP_U_Matrix a, b;
                std::memcpy(&a, matrices_a[row], 128);
                std::memcpy(&b, matrices_b[row], 128);
                a.vv.k[1] += sample * 0.0000001379;
                unsigned char expected[10], actual[10];
                Value original = at<Value>(0x2AD90);
                void *receiver = solver, *matrix_a = &a, *matrix_b = &b;
                __asm {
                mov ecx, receiver
                push matrix_b
                push matrix_a
                call original
                fstp tbyte ptr expected
                mov ecx, receiver
                push matrix_b
                push matrix_a
                call parity_native_pk_collision_entry
                fstp tbyte ptr actual
                }
                if (std::memcmp(expected, actual, 10) != 0)
                {
                    ++differences;
                    if (differences <= 4 || (sample == 0 && precision == 2))
                        std::printf("PK collision PC%d row%d sample%d extended result differs\n", precision, row,
                                    sample);
                }
            }
    }
    std::printf("PK collision mismatches %d / 768\n", differences);
    return differences == 0;
}

static bool continuous_vec_parallel_test(bool recorded = false)
{
    typedef double(__thiscall * Value)(void *, IVP_U_Matrix *, IVP_U_Matrix *);
    int differences = 0;
    const int row_count = recorded ? int(sizeof(retail_vec_plane_13132) / sizeof(retail_vec_plane_13132[0])) : 1;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int row_index = 0; row_index < row_count; ++row_index)
            for (int sample = 0; sample < 128; ++sample)
            {
                alignas(16) unsigned char solver[104] = {};
                std::memcpy(solver + 40, retail_pk_kk_solver + 72, 24);
                std::memcpy(solver + 72, retail_pf_13114[0] + 3, 24);
                write<double>(solver, 40, read<double>(solver, 40) + sample * 0.000001379);
                IVP_U_Matrix a, b;
                std::memcpy(&a, retail_pf_13114[0] + 9, 128);
                std::memcpy(&b, retail_pf_13114[0] + 25, 128);
                if (recorded)
                {
                    const double *row = retail_vec_plane_13132[row_index];
                    std::memcpy(solver + 40, row, 24);
                    std::memcpy(solver + 72, row + 3, 24);
                    std::memcpy(&a, row + 6, 128);
                    std::memcpy(&b, row + 22, 128);
                    write<double>(solver, 40, read<double>(solver, 40) + sample * 0.000001379);
                }
                unsigned char expected[10], actual[10];
                Value original = at<Value>(0x2AAE0);
                void *receiver = solver, *matrix_a = &a, *matrix_b = &b;
                __asm {
                mov ecx, receiver
                push matrix_b
                push matrix_a
                call original
                fstp tbyte ptr expected
                mov ecx, receiver
                push matrix_b
                push matrix_a
                call parity_native_vec_parallel_entry
                fstp tbyte ptr actual
                }
                if (std::memcmp(expected, actual, 10) != 0)
                {
                    ++differences;
                    if (differences <= 4)
                        std::printf("vector parallel PC%d sample%d extended return differs\n", precision, sample);
                }
            }
    }
    std::printf("vector parallel mismatches %d / %d\n", differences, row_count * 384);
    return differences == 0;
}

#define RETAIL_REGISTER_continuous RETAIL_REGISTER
#include "RetailTestRegistration.inc"
