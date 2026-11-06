#include "RetailTestSupport.h"
#include "RetailContactKernels.h"

#include <ivp_physics.hxx>
#include <ivp_friction.hxx>
#include <ivp_great_matrix.hxx>
#include <ivp_friction_solver.hxx>
#include <ivu_memory.hxx>
#if IVP_ENABLE_X87_EMULATION
#include <ivu_x87.hxx>
#endif

#include <cmath>
#include <new>

#include "fixtures/linear_pressure_9990.hxx"
#include "fixtures/pressure_contacts_9990.hxx"
#include "fixtures/pressure_gauss_9990.hxx"
#include "fixtures/pressure_matrix_9990.hxx"
#include "fixtures/single_pressure_11400.hxx"

// clang-format off
#pragma comment(linker, "/alternatename:_parity_native_friction_entry=?friction_force_local_constraint_2d@IVP_Contact_Point@@AAEMPBVIVP_Event_Sim@@@Z")
#pragma comment(linker, "/alternatename:_parity_native_pressure_entry=?static_friction_single@IVP_Contact_Point@@AAEXPBVIVP_Event_Sim@@MM@Z")
// clang-format on

static bool lu_pressure_kernels_test()
{
    using Decompose = int(__thiscall *)(void *);
    using Solve = void(__thiscall *)(void *);
    using Change = int(__thiscall *)(void *, int);
    int differences[4] = {}, calls[4] = {}, decomposed = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            alignas(16) double el[324] = {}, al[324] = {}, eu[324], au[324], ei[18], ai[18], et[18] = {}, atmp[18] = {},
                               em[18] = {}, am[18] = {}, eo[18] = {}, ao[18] = {};
            std::memcpy(eu, retail_linear_pressure_matrix, sizeof(eu));
            eu[0] += sample * 0.00000001379;
            std::memcpy(au, eu, sizeof(au));
            std::memcpy(ei, retail_linear_pressure_desired, sizeof(ei));
            std::memcpy(ai, ei, sizeof(ai));
            IVP_Incr_L_U_Matrix expected{}, actual{};
            expected.MATRIX_EPS = actual.MATRIX_EPS = 1.0000000116860974e-6;
            expected.L_matrix = el;
            actual.L_matrix = al;
            expected.U_matrix = eu;
            actual.U_matrix = au;
            expected.input_vec = ei;
            actual.input_vec = ai;
            expected.out_vec = eo;
            actual.out_vec = ao;
            expected.temp_vec = et;
            actual.temp_vec = atmp;
            expected.mult_vec = em;
            actual.mult_vec = am;
            expected.aligned_row_len = actual.aligned_row_len = 18;
            expected.n_sub = actual.n_sub = 12;
            auto compare = [&](int stage, int e, int a) {
                ++calls[stage];
                const bool same =
                    e == a && expected.n_sub == actual.n_sub && std::memcmp(el, al, sizeof(al)) == 0 &&
                    std::memcmp(eu, au, sizeof(au)) == 0 &&
                    (stage != 1 || (std::memcmp(eo, ao, 12 * 8) == 0 && std::memcmp(et, atmp, 12 * 8) == 0 &&
                                    std::memcmp(em, am, 12 * 8) == 0));
                if (!same)
                {
                    ++differences[stage];
                    if (differences[stage] <= 2)
                    {
                        int cell = 0;
                        while (cell < 324 && eu[cell] == au[cell])
                            ++cell;
                        std::printf("LU kernel stage%d PC%d sample%d status%d / %d firstU%d out0 %.17g / %.17g\n",
                                    stage, precision, sample, e, a, cell, eo[0], ao[0]);
                    }
                }
                return same;
            };
            const int er = at<Decompose>(0x35AE0)(&expected), ar = actual.l_u_decomposition_with_pivoting();
            if (!compare(0, er, ar) || !er)
                continue;
            ++decomposed;
            at<Solve>(0x35FF0)(&expected);
            actual.solve_lin_equ();
            if (!compare(1, 1, 1))
                continue;
            alignas(16) double saved_l[324], saved_u[324];
            std::memcpy(saved_l, el, sizeof(saved_l));
            std::memcpy(saved_u, eu, sizeof(saved_u));
            const int ed = at<Change>(0x35D00)(&expected, sample % 12), ad = actual.decrement_l_u(sample % 12);
            compare(3, ed, ad);
            expected.n_sub = actual.n_sub = 12;
            std::memcpy(el, saved_l, sizeof(el));
            std::memcpy(al, saved_l, sizeof(al));
            std::memcpy(eu, saved_u, sizeof(eu));
            std::memcpy(au, saved_u, sizeof(au));
            for (int j = 0; j < 12; ++j)
                ei[j] = ai[j] = retail_linear_pressure_matrix[j * 18 + 12];
            const int ei_result = at<Decompose>(0x35BE0)(&expected), ai_result = actual.increment_l_u();
            compare(2, ei_result, ai_result);
        }
    }
    std::printf(
        "LU kernels mismatches decompose%d solve%d increment%d decrement%d / 384; calls %d %d %d %d, decomposed%d\n",
        differences[0], differences[1], differences[2], differences[3], calls[0], calls[1], calls[2], calls[3],
        decomposed);
    return differences[0] + differences[1] + differences[2] + differences[3] == 0 && calls[0] == 384 &&
           calls[1] == 384 && calls[2] == 384 && calls[3] == 384;
}

static bool lu_row_normalization_test(bool left_only = false)
{
    using Normalize = int(__thiscall *)(void *, int);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            alignas(16) double expected_l[324] = {}, actual_l[324], expected_u[324], actual_u[324];
            std::memcpy(expected_u, retail_linear_pressure_matrix, sizeof(expected_u));
            const int row = sample % 12;
            expected_u[row * 18 + row] += (sample / 12) * 0.0000001379;
            for (int i = 0; i < 12; ++i)
                expected_l[i * 18 + i] = 1.0;
            if (left_only)
            {
                std::memcpy(expected_l, retail_linear_pressure_matrix, sizeof(expected_l));
                expected_l[row * 18 + row] += (sample / 12) * 0.0000001379;
            }
            std::memcpy(actual_l, expected_l, sizeof(actual_l));
            std::memcpy(actual_u, expected_u, sizeof(actual_u));
            IVP_Incr_L_U_Matrix expected{}, actual{};
            expected.MATRIX_EPS = actual.MATRIX_EPS = 1.0000000116860974e-6;
            expected.L_matrix = expected_l;
            actual.L_matrix = actual_l;
            expected.U_matrix = expected_u;
            actual.U_matrix = actual_u;
            expected.aligned_row_len = actual.aligned_row_len = 18;
            expected.n_sub = actual.n_sub = 12;
            const int expected_result = at<Normalize>(left_only ? 0x35780 : 0x357F0)(&expected, row),
                      actual_result = left_only ? actual.normize_row_L(row) : actual.normize_row(row);
            if (expected_result != actual_result || std::memcmp(expected_l, actual_l, sizeof(actual_l)) != 0 ||
                std::memcmp(expected_u, actual_u, sizeof(actual_u)) != 0)
            {
                ++differences;
                if (differences <= 4)
                {
                    const double *e = left_only ? expected_l : expected_u, *a = left_only ? actual_l : actual_u;
                    int cell = 0;
                    while (cell < 324 && e[cell] == a[cell])
                        ++cell;
                    std::printf("LU row normalization PC%d sample%d cell%d %.17g / %.17g\n", precision, sample, cell,
                                cell < 324 ? e[cell] : 0, cell < 324 ? a[cell] : 0);
                }
            }
        }
    }
    std::printf("LU row normalization mismatches %d / 384\n", differences);
    return differences == 0;
}

static bool linear_pressure_test()
{
    using Memory = void(__thiscall *)(void *);
    using Solve = int(__thiscall *)(void *, double *, double *, double *, int, int, void *);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            alignas(16) unsigned char old_solver[512] = {}, old_memory[64] = {};
            IVP_Linear_Constraint_Solver actual;
            IVP_U_Memory memory;
            alignas(16) double expected_a[324], actual_a[324], expected_b[18], actual_b[18], expected_x[18] = {},
                                                                                             actual_x[18] = {};
            std::memcpy(expected_a, retail_linear_pressure_matrix, sizeof(expected_a));
            std::memcpy(expected_b, retail_linear_pressure_desired, sizeof(expected_b));
            expected_b[0] += sample * 0.000000000001379;
            std::memcpy(actual_a, expected_a, sizeof(actual_a));
            std::memcpy(actual_b, expected_b, sizeof(actual_b));
            at<Memory>(0x20260)(old_memory);
            const int expected_result = at<Solve>(0x346E0)(old_solver, expected_a, expected_b, expected_x, 18,
                                                           retail_linear_pressure_active, old_memory);
            const int actual_result =
                actual.init_and_solve_lc(actual_a, actual_b, actual_x, 18, retail_linear_pressure_active, &memory);
            at<Memory>(0x20290)(old_memory);
            if (precision == 2 && sample == 0 &&
                std::memcmp(expected_x, retail_linear_pressure_solution, sizeof(expected_x)) != 0)
            {
                std::printf("recorded linear pressure solution reproduction failed X0 %.17g / %.17g\n", expected_x[0],
                            retail_linear_pressure_solution[0]);
                return false;
            }
            if (expected_result != actual_result || std::memcmp(expected_x, actual_x, sizeof(actual_x)) != 0)
            {
                ++differences;
                if (differences <= 4 || (precision == 2 && sample == 0))
                    std::printf("linear pressure PC%d sample%d status%d / %d X0 %.17g / %.17g\n", precision, sample,
                                expected_result, actual_result, expected_x[0], actual_x[0]);
            }
        }
    }
    std::printf("linear pressure mismatches %d / 384\n", differences);
    return differences == 0;
}

static bool pressure_gauss_settings_test()
{
    using Construct = void(__thiscall *)(void *);
    using Solve = int(__thiscall *)(void *);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        alignas(16) unsigned char old_matrix[32] = {};
        at<Construct>(0x34100)(old_matrix);
        IVP_Great_Matrix_Many_Zero actual;
        alignas(16) double expected_a[1] = {9.999999858590343e-10}, actual_a[1] = {expected_a[0]},
                           expected_b[1] = {expected_a[0]}, actual_b[1] = {expected_b[0]}, expected_x[1] = {},
                           actual_x[1] = {};
        write<int>(old_matrix, 8, 1);
        write<int>(old_matrix, 12, 1);
        write(old_matrix, 16, expected_a);
        write(old_matrix, 20, expected_b);
        write(old_matrix, 24, expected_x);
        actual.columns = 1;
        actual.aligned_row_len = 1;
        actual.matrix_values = actual_a;
        actual.desired_vector = actual_b;
        actual.result_vector = actual_x;
        const int expected_result = at<Solve>(0x33E50)(old_matrix),
                  actual_result = actual.solve_great_matrix_many_zero();
        if (read<double>(old_matrix, 0) != actual.MATRIX_EPS || expected_result != actual_result ||
            expected_x[0] != actual_x[0])
        {
            ++differences;
            std::printf("pressure Gauss settings PC%d EPS %.17g / %.17g resultX %.17g / %.17g\n", precision,
                        read<double>(old_matrix, 0), actual.MATRIX_EPS, expected_x[0], actual_x[0]);
        }
    }
    return differences == 0;
}

static bool pressure_gauss_test()
{
    using Solve = int(__thiscall *)(void *);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
            for (int case_id = 0; case_id < 2; ++case_id)
            {
                const int n = case_id ? 13 : 12;
                const double *matrix = case_id ? retail_pressure_gauss_13_matrix : retail_pressure_gauss_12_matrix;
                const double *desired = case_id ? retail_pressure_gauss_13_desired : retail_pressure_gauss_12_desired;
                const double *solution =
                    case_id ? retail_pressure_gauss_13_solution : retail_pressure_gauss_12_solution;
                const double epsilon =
                    case_id ? retail_pressure_gauss_13_epsilon[0] : retail_pressure_gauss_12_epsilon[0];
                IVP_Great_Matrix_Many_Zero expected, actual;
                alignas(16) double expected_a[169], actual_a[169], expected_b[13], actual_b[13], expected_x[13] = {},
                                                                                                 actual_x[13] = {};
                std::memcpy(expected_a, matrix, n * n * 8);
                std::memcpy(expected_b, desired, n * 8);
                expected_b[0] += sample * 0.0000000001379;
                std::memcpy(actual_a, expected_a, n * n * 8);
                std::memcpy(actual_b, expected_b, n * 8);
                expected.MATRIX_EPS = actual.MATRIX_EPS = epsilon;
                expected.columns = actual.columns = n;
                expected.aligned_row_len = actual.aligned_row_len = n;
                expected.matrix_values = expected_a;
                actual.matrix_values = actual_a;
                expected.desired_vector = expected_b;
                actual.desired_vector = actual_b;
                expected.result_vector = expected_x;
                actual.result_vector = actual_x;
                const int expected_result = at<Solve>(0x33E50)(&expected),
                          actual_result = actual.solve_great_matrix_many_zero();
                if (precision == 2 && sample == 0 && std::memcmp(expected_x, solution, n * 8) != 0)
                {
                    std::printf("recorded Gauss solution reproduction failed n%d\n", n);
                    return false;
                }
                const bool same =
                    expected_result == actual_result && std::memcmp(expected_a, actual_a, n * n * 8) == 0 &&
                    std::memcmp(expected_b, actual_b, n * 8) == 0 && std::memcmp(expected_x, actual_x, n * 8) == 0;
                if (!same)
                {
                    ++differences;
                    if (differences <= 4)
                    {
                        int cell = 0;
                        while (cell < n * n && expected_a[cell] == actual_a[cell])
                            ++cell;
                        std::printf("pressure Gauss PC%d n%d sample%d firstMatrixCell%d X0 %.17g / %.17g\n", precision,
                                    n, sample, cell, expected_x[0], actual_x[0]);
                    }
                }
            }
    }
    std::printf("pressure Gauss mismatches %d / 768\n", differences);
    return differences == 0;
}

static bool pressure_matrix_construction_test(bool desired_only = false)
{
    using Calculate = int(__thiscall *)(void *, void *, int *);
    int matrix_differences = 0, desired_differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            alignas(16) unsigned char old_solver[0x850] = {}, native_solver[sizeof(IVP_Friction_Solver)] = {},
                                      old_system[0x60] = {}, native_system[sizeof(IVP_Friction_System)] = {};
            alignas(16) unsigned char old_cores[3][0x200] = {}, native_cores[3][sizeof(IVP_Core)] = {},
                                      old_contacts[9][0x80] = {}, native_contacts[9][sizeof(IVP_Contact_Point)] = {},
                                      old_fr[3][32] = {};
            IVP_Impact_Solver_Long_Term old_info[9], native_info[9];
            IVP_Friction_Info_For_Core native_fr[3];
            void *old_springs[3][9] = {};
            int spring_counts[3] = {};
            IVP_Impact_Solver_Long_Term *old_infos[9];
            double expected_matrix[81] = {}, actual_matrix[81] = {}, expected_desired[9] = {}, actual_desired[9] = {};
            int expected_active[9] = {}, actual_active[9] = {};
            auto *solver = reinterpret_cast<IVP_Friction_Solver *>(native_solver);
            new (&solver->contact_info_vector) IVP_Vector_of_Contact_Info_512;
            solver->dist_change_mat.columns = 9;
            solver->dist_change_mat.aligned_row_len = 9;
            solver->dist_change_mat.matrix_values = actual_matrix;
            solver->dist_change_mat.desired_vector = actual_desired;
            write<int>(old_solver, 8, 9);
            write<int>(old_solver, 12, 9);
            write(old_solver, 16, expected_matrix);
            write(old_solver, 20, expected_desired);
            write<unsigned short>(old_solver, 0x32, 9);
            write(old_solver, 0x34, old_infos);
            for (int j = 0; j < 3; ++j)
            {
                auto *core = reinterpret_cast<IVP_Core *>(native_cores[j]);
                const float *values = retail_pressure_cores_9990[j];
                std::memcpy(old_cores[j] + 0x34, values, 16);
                std::memcpy(const_cast<IVP_U_Point_4 *>(core->get_inv_masses()), values, 16);
                core->speed.set(values[4], values[5], values[6]);
                core->rot_speed.set(values[7], values[8] + sample * 0.00000001379f, values[9]);
                std::memcpy(old_cores[j] + 0xA4, core->speed.k, 12);
                std::memcpy(old_cores[j] + 0x94, core->rot_speed.k, 12);
            }
            for (int i = 0; i < 9; ++i)
            {
                const float *values = retail_pressure_contacts_9990[i];
                auto &info = native_info[i];
                info.surf_normal.set(values[0], values[1], values[2]);
                info.index_in_fs = static_cast<short>(i);
                info.friction.dist_len = values[9];
                info.friction.has_negative_pull_since = static_cast<int>(values[10]);
                for (int j = 0; j < 2; ++j)
                {
                    info.contact_cross_nomal_cs[j].set(values[3 + j * 3], values[4 + j * 3], values[5 + j * 3]);
                    int core_id = static_cast<int>(values[11 + j]);
                    if (core_id >= 0)
                    {
                        info.contact_core[j] = reinterpret_cast<IVP_Core *>(native_cores[core_id]);
                        info.friction.friction_infos[j] = native_fr + core_id;
                        native_fr[core_id].friction_springs.add(
                            reinterpret_cast<IVP_Contact_Point *>(native_contacts[i]));
                        old_springs[core_id][spring_counts[core_id]++] = old_contacts[i];
                    }
                }
                old_info[i] = info;
                for (int j = 0; j < 2; ++j)
                {
                    int core_id = static_cast<int>(values[11 + j]);
                    if (core_id >= 0)
                    {
                        old_info[i].contact_core[j] = reinterpret_cast<IVP_Core *>(old_cores[core_id]);
                        old_info[i].friction.friction_infos[j] =
                            reinterpret_cast<IVP_Friction_Info_For_Core *>(old_fr[core_id]);
                    }
                }
                old_infos[i] = old_info + i;
                solver->contact_info_vector.add(native_info + i);
                write(old_contacts[i], 0x40, old_info + i);
                write(native_contacts[i], 0x40, native_info + i);
                if (i < 8)
                {
                    write(old_contacts[i], 0, old_contacts[i + 1]);
                    write(native_contacts[i], 0, native_contacts[i + 1]);
                }
            }
            for (int j = 0; j < 3; ++j)
            {
                write<unsigned short>(old_fr[j], 2, static_cast<unsigned short>(spring_counts[j]));
                write(old_fr[j], 4, old_springs[j]);
            }
            write(old_system, 0x20, old_contacts[0]);
            write(native_system, 0x20, native_contacts[0]);
            const int expected_count = at<Calculate>(0x36760)(old_solver, old_system, expected_active);
            const int actual_count =
                solver->calc_solver_PSI(reinterpret_cast<IVP_Friction_System *>(native_system), actual_active);
            if (std::memcmp(expected_matrix, actual_matrix, sizeof(actual_matrix)) != 0)
            {
                ++matrix_differences;
                if (matrix_differences <= 3)
                {
                    for (int j = 0; j < 81; ++j)
                        if (expected_matrix[j] != actual_matrix[j])
                        {
                            std::printf("pressure matrix PC%d sample%d cell%d %.17g / %.17g\n", precision, sample, j,
                                        expected_matrix[j], actual_matrix[j]);
                            break;
                        }
                }
            }
            if (std::memcmp(expected_desired, actual_desired, sizeof(actual_desired)) != 0 ||
                expected_count != actual_count ||
                std::memcmp(expected_active, actual_active, expected_count * sizeof(int)) != 0)
            {
                ++desired_differences;
                if (desired_differences <= 3)
                {
                    for (int j = 0; j < 9; ++j)
                        if (expected_desired[j] != actual_desired[j])
                        {
                            std::printf("pressure desired PC%d sample%d row%d %.17g / %.17g\n", precision, sample, j,
                                        expected_desired[j], actual_desired[j]);
                            break;
                        }
                }
            }
        }
    }
    std::printf("pressure construction mismatches matrix %d desired %d / 384\n", matrix_differences,
                desired_differences);
    return desired_only ? desired_differences == 0 : matrix_differences == 0;
}

static bool pressure_normalization_test()
{
    using Normalize = void(__thiscall *)(void *);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            alignas(16) unsigned char old_solver[64] = {}, native_solver[sizeof(IVP_Friction_Solver)] = {};
            double expected_matrix[324], actual_matrix[324], expected_desired[18], actual_desired[18];
            std::memcpy(expected_matrix, retail_pressure_matrix_9990, sizeof(expected_matrix));
            std::memcpy(expected_desired, retail_pressure_desired_9990, sizeof(expected_desired));
            expected_desired[0] += sample * 0.00000001379;
            std::memcpy(actual_matrix, expected_matrix, sizeof(actual_matrix));
            std::memcpy(actual_desired, expected_desired, sizeof(actual_desired));
            write<int>(old_solver, 8, 18);
            write<int>(old_solver, 12, 18);
            write(old_solver, 16, expected_matrix);
            write(old_solver, 20, expected_desired);
            auto *solver = reinterpret_cast<IVP_Friction_Solver *>(native_solver);
            solver->dist_change_mat.columns = 18;
            solver->dist_change_mat.aligned_row_len = 18;
            solver->dist_change_mat.matrix_values = actual_matrix;
            solver->dist_change_mat.desired_vector = actual_desired;
            at<Normalize>(0x36190)(old_solver);
            solver->normize_constraint_equ();
            const bool same = std::memcmp(expected_matrix, actual_matrix, sizeof(actual_matrix)) == 0 &&
                              std::memcmp(expected_desired, actual_desired, sizeof(actual_desired)) == 0 &&
                              read<double>(old_solver, 32) == solver->correct_x_factor;
            if (!same)
            {
                ++differences;
                if (differences <= 4)
                    std::printf("pressure normalization PC%d sample%d matrix00 %.17g / %.17g desired0 %.17g / %.17g "
                                "factor %.17g / %.17g\n",
                                precision, sample, expected_matrix[0], actual_matrix[0], expected_desired[0],
                                actual_desired[0], read<double>(old_solver, 32), solver->correct_x_factor);
            }
        }
    }
    std::printf("pressure normalization mismatches %d / 384\n", differences);
    return differences == 0;
}

static bool async_pressure_push_test()
{
    typedef int(__thiscall * Push)(void *, void *);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            alignas(16) unsigned char old_cores[2][0x200] = {}, native_cores[2][sizeof(IVP_Core)] = {};
            alignas(16) unsigned char old_solver[0x850] = {}, old_system[0x60] = {}, old_contact[0x80] = {};
            IVP_Impact_Solver_Long_Term expected = {}, actual = {};
            alignas(16) unsigned char event_bytes[sizeof(IVP_Event_Sim)] = {};
            auto *event = reinterpret_cast<IVP_Event_Sim *>(event_bytes);
            event->i_delta_time = 66.0;
            double impulse = sample == 0 ? 0.30529915123456789 : (sample - 31) * 0.019753123456789;
            write(old_solver, 0x18, &impulse);
            write(old_solver, 0x2C, event);
            write(old_system, 0x20, old_contact);
            write(old_contact, 0x40, &expected);
            actual.surf_normal.set(-0.0005099062104709446f, -0.999983549118042f, 0.005710284691303968f);
            for (int j = 0; j < 2; ++j)
            {
                auto *core = reinterpret_cast<IVP_Core *>(native_cores[j]);
                actual.contact_core[j] = core;
                const float masses[4] = {0.33941125869750977f + j * 0.017f, 0.33941125869750977f, 0.33941125869750977f,
                                         1.0f};
                std::memcpy(old_cores[j] + 0x34, masses, 16);
                std::memcpy(const_cast<IVP_U_Point_4 *>(core->get_inv_masses()), masses, 16);
                core->rot_speed_change.set((sample % 7 - 3) * 0.0176543f, 0.007654321f, -0.0357913f);
                core->speed_change.set(-0.0164791f, (sample % 11 - 5) * 0.0234567f, 0.0123579f);
                std::memcpy(old_cores[j] + 0x74, core->rot_speed_change.k, 12);
                std::memcpy(old_cores[j] + 0x84, core->speed_change.k, 12);
                actual.contact_cross_nomal_cs[j].set(-1.2497138977050781f - j * 0.03217f, 1.6472087582819483e-10f,
                                                     0.841815173625946f);
            }
            expected = actual;
            for (int j = 0; j < 2; ++j)
                expected.contact_core[j] = reinterpret_cast<IVP_Core *>(old_cores[j]);
            at<Push>(0x364C0)(old_solver, old_system);
            IVP_Friction_Solver::async_apply_impulse(&actual, impulse);
            bool same = true;
            for (int j = 0; j < 2; ++j)
                for (int k = 0; k < 3; ++k)
                {
                    const auto *core = actual.contact_core[j];
                    same = same && read<float>(old_cores[j], 0x74 + k * 4) == core->rot_speed_change.k[k] &&
                           read<float>(old_cores[j], 0x84 + k * 4) == core->speed_change.k[k];
                }
            if (!same)
            {
                ++differences;
                if (differences <= 4)
                    std::printf("async pressure PC%d sample %d core1 torque X %.9g / %.9g\n", precision, sample,
                                read<float>(old_cores[1], 0x74), actual.contact_core[1]->rot_speed_change.k[0]);
            }
        }
    }
    std::printf("async pressure push mismatches: %d / 384\n", differences);
    return differences == 0;
}

// Same-input oracle for the spring easing preceding the L1 box angular drift.
static bool easing_kernel_test()
{
    typedef void(__cdecl * Ease)(void *, void *, IVP_U_Float_Point *, IVP_U_Float_Point *, double);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int i = 0; i < 128; ++i)
        {
            __declspec(align(16)) unsigned char expected_contacts[2][0x70] = {},
                                                native_contacts[2][sizeof(IVP_Contact_Point)] = {};
            __declspec(align(16)) unsigned char expected_objects[2][0xA8] = {},
                                                native_objects[2][sizeof(IVP_Real_Object)] = {};
            IVP_Impact_Solver_Long_Term info[2]{};
            IVP_U_Float_Point expected[2], actual[2];
            for (int j = 0; j < 2; ++j)
            {
                void *identity = reinterpret_cast<void *>(j && i % 2 ? 0x2000 : 0x1000);
                write<void *>(expected_objects[j], 0xA4, identity);
                IVP_Real_Object *object = reinterpret_cast<IVP_Real_Object *>(native_objects[j]);
                object->physical_core = reinterpret_cast<IVP_Core *>(identity);
                auto *contact = reinterpret_cast<IVP_Contact_Point *>(native_contacts[j]);
                contact->get_synapse(0)->l_obj = object;
                write<void *>(expected_contacts[j], 0x10, expected_objects[j]);
                info[j].contact_point_ws.set(-153.3236646204272 + j * 0.34157887258133 + i * 0.00012345,
                                             16.206178527379773 + j * 0.0303706908532,
                                             -366.5645699536357 - j * 0.61600050747276);
                info[j].span_friction_v[0].set(1.0f, 2.8419481168384664e-05f, -9.076590504264459e-05f);
                info[j].span_friction_v[1].set(9.072275133803487e-05f, 0.0015149291139096022f, 0.9999988675117493f);
                const float spring[2] = {
                    j ? 0.005259254481643438f + i * 0.000013579f : 0.0006315939244814217f - i * 0.000007531f,
                    j ? 0.00242899963632226f - i * 0.000008642f : -0.0009160332847386599f + i * 0.000005431f};
                std::memcpy(expected_contacts[j] + 0x38, spring, 8);
                std::memcpy(native_contacts[j] + 0x38, spring, 8);
                write<void *>(expected_contacts[j], 0x40, &info[j]);
                write<void *>(native_contacts[j], 0x40, &info[j]);
                expected[j].set_to_zero();
                actual[j].set_to_zero();
            }
            const double factor = 1.0 / (2.0 + 1e-10);
            at<Ease>(0x1BDF0)(expected_contacts[0], expected_contacts[1], &expected[0], &expected[1], factor);
            IVP_Friction_Solver::ease_two_mindists(reinterpret_cast<IVP_Contact_Point *>(native_contacts[0]),
                                                   reinterpret_cast<IVP_Contact_Point *>(native_contacts[1]),
                                                   &actual[0], &actual[1], factor);
            for (int j = 0; j < 2; ++j)
                for (int k = 0; k < 3; ++k)
                    if (expected[j].k[k] != actual[j].k[k])
                    {
                        ++differences;
                        if (differences <= 8)
                            std::printf("easing PC%d case %d contact %d axis %d %.9g / %.9g\n", precision, i, j, k,
                                        expected[j].k[k], actual[j].k[k]);
                    }
        }
    }
    std::printf("easing kernel mismatches: %d / 2304\n", differences);
    return differences == 0;
}

// Link the audited native private kernel without changing IVP's public API.
// The thunk supplies the MSVC x86 thiscall register/stack transport explicitly.
extern "C" void parity_native_friction_entry();

float native_friction(void *contact, const IVP_Event_Sim *event)
{
    float result;
    __asm {
        push event
        mov ecx, contact
        call parity_native_friction_entry
        fstp result
    }
    return result;
}

extern "C" void parity_native_pressure_entry();

void native_pressure(void *contact, const IVP_Event_Sim *event)
{
    const float gap = 0.02f, speedup = 1.0f;
    __asm {
        push speedup
        push gap
        push event
        mov ecx, contact
        call parity_native_pressure_entry
    }
}

static bool multi_friction_kernel_test()
{
    typedef void(__thiscall * Forces)(void *, const IVP_Event_Sim *);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 64; ++sample)
        {
            __declspec(align(16)) unsigned char original_core[0x200] = {}, native_core[sizeof(IVP_Core)] = {};
            __declspec(align(16)) unsigned char original_contacts[2][0x78] = {},
                                                native_contacts[2][sizeof(IVP_Contact_Point)] = {};
            unsigned char original_pair[0x40] = {}, original_system[0x48] = {},
                          native_system[sizeof(IVP_Friction_System)] = {};
            IVP_Core *core = reinterpret_cast<IVP_Core *>(native_core);
            IVP_Friction_System *system = reinterpret_cast<IVP_Friction_System *>(native_system);
            IVP_Friction_Core_Pair pair;
            IVP_Impact_Solver_Long_Term native_info[2], original_info[2];
            const double m[12] = {-0.3621027490594889,  -0.4709225749183902,    -0.80443366883527,
                                  -0.9115078590636997,  -0.0016719948638380666, 0.41127925707269786,
                                  -0.19502569571379613, 0.8821729608559724,     -0.42864419411201027,
                                  38.90438538352224,    15.168269770430479,     -155.18768453360707};
            const float masses[4] = {0.32894736528396606f, 0.32894736528396606f, 0.32894736528396606f,
                                     0.5263158082962036f},
                        speeds[3] = {-10.42862606048584f, 0.2690778970718384f, 6.229427337646484f},
                        angular[3] = {-1.2521344423294067f, 3.0161612033843994f, -4.970613479614258f};
            const double position[3] = {38.90499226462758, 13.242853896902952, -155.7287699556461};
            const float spans[6] = {6.068566563044442e-06f, 0.270542711019516f,    -0.9627079963684082f, 1.0f,
                                    0.0002904828288592398f, 8.793588494881988e-05f};
            std::memcpy(const_cast<IVP_U_Point_4 *>(core->get_inv_masses()), masses, 16);
            for (int r = 0; r < 3; ++r)
                for (int c = 0; c < 3; ++c)
                    core->m_world_f_core_last_psi.set_elem(r, c, m[r * 3 + c]);
            core->m_world_f_core_last_psi.vv.set(m[9], m[10], m[11]);
            core->speed.set(speeds[0], speeds[1], speeds[2]);
            core->rot_speed.set(angular[0], angular[1], angular[2]);
            std::memcpy(original_core + 0x34, masses, 16);
            std::memcpy(original_core + 0x128, &core->m_world_f_core_last_psi, sizeof(IVP_U_Matrix));
            std::memcpy(original_core + 0x94, &core->rot_speed, 12);
            std::memcpy(original_core + 0xA4, &core->speed, 12);
            void *original_distances[2] = {original_contacts[0], original_contacts[1]},
                 *original_pairs[1] = {original_pair};
            write<unsigned short>(original_pair, 2, 2);
            write<void *>(original_pair, 4, original_distances);
            write<unsigned short>(original_system, 0x36, 1);
            write<void *>(original_system, 0x38, original_pairs);
            for (int i = 0; i < 2; ++i)
            {
                native_info[i].contact_core[0] = core;
                native_info[i].contact_core[1] = NULL;
                native_info[i].contact_point_ws.set(position[0] + i * 0.0012345, position[1], position[2]);
                for (int d = 0; d < 2; ++d)
                    native_info[i].span_friction_v[d].set(spans[d * 3], spans[d * 3 + 1], spans[d * 3 + 2]);
                original_info[i] = native_info[i];
                original_info[i].contact_core[0] = reinterpret_cast<IVP_Core *>(original_core);
                write<float>(native_contacts[i], 0x30, i ? 1.9123456f : 1.8421053886413574f);
                write<float>(native_contacts[i], 0x38, i ? -0.0356789f : 0.0256789f);
                write<float>(native_contacts[i], 0x3C, i ? 0.0876543f : -0.0135791f);
                write<float>(native_contacts[i], 0x44, 0.5600000023841858f);
                write<float>(native_contacts[i], 0x54,
                             i ? 28.123456f + sample * 0.3579135f : 39.9155387878418f - sample * 0.1319753f);
                std::memcpy(original_contacts[i], native_contacts[i], 0x78);
                write<void *>(native_contacts[i], 0x40, &native_info[i]);
                write<void *>(original_contacts[i], 0x40, &original_info[i]);
                pair.add_fr_dist_obj_pairs(reinterpret_cast<IVP_Contact_Point *>(native_contacts[i]));
            }
            system->fr_pairs_of_objs.add(&pair);
            __declspec(align(16)) unsigned char event_packet[sizeof(IVP_Event_Sim)] = {};
            IVP_Event_Sim &event = *reinterpret_cast<IVP_Event_Sim *>(event_packet);
            event.delta_time = 1.0 / 66.0;
            event.i_delta_time = 66.0;
            at<Forces>(0x1BC20)(original_system, &event);
            system->calc_friction_forces(&event);
            for (int i = 0; i < 2; ++i)
                for (int offset = 0x38; offset <= 0x3C; offset += 4)
                {
                    float e = read<float>(original_contacts[i], offset), a = read<float>(native_contacts[i], offset);
                    if (e != a)
                    {
                        ++differences;
                        if (differences <= 8)
                            std::printf("multi friction PC%d spring %d+%x %.9g / %.9g\n", precision, i, offset, e, a);
                    }
                }
            for (int k = 0; k < 3; ++k)
                if (read<float>(original_core, 0x94 + k * 4) != core->rot_speed.k[k] ||
                    read<float>(original_core, 0xA4 + k * 4) != core->speed.k[k])
                {
                    ++differences;
                    if (differences <= 8)
                        std::printf("multi friction PC%d output %d torque %.9g / %.9g\n", precision, k,
                                    read<float>(original_core, 0x94 + k * 4), core->rot_speed.k[k]);
                }
            system->fr_pairs_of_objs.~IVP_U_Vector();
        }
    }
    std::printf("multi friction kernel mismatches: %d\n", differences);
    return differences == 0;
}

// Recorded single-contact pressure input after 2D friction at L1 frame 11355.
static bool single_pressure_kernel_test()
{
    typedef void(__thiscall * Pressure)(void *, const IVP_Event_Sim *, float, float);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            __declspec(align(16)) unsigned char original_core[0x200] = {}, native_core[sizeof(IVP_Core)] = {};
            unsigned char original_contact[0x78] = {}, native_contact[sizeof(IVP_Contact_Point)] = {};
            IVP_Core *core = reinterpret_cast<IVP_Core *>(native_core);
            IVP_Impact_Solver_Long_Term actual, expected;
            const float masses[4] = {4.51443338394165f, 4.516299724578857f, 4.508676052093506f, 5.0f};
            core->speed.set(0.35022491216659546f, -0.26439186930656433f, 0.004652271047234535f);
            core->rot_speed.set(-0.02298710122704506f, -0.04401024430990219f, -0.2163946032524109f);
            std::memcpy(const_cast<IVP_U_Point_4 *>(core->get_inv_masses()), masses, 16);
            std::memcpy(original_core + 0x34, masses, 16);
            std::memcpy(original_core + 0x94, &core->rot_speed, 12);
            std::memcpy(original_core + 0xA4, &core->speed, 12);
            actual.contact_core[0] = core;
            actual.contact_core[1] = NULL;
            actual.surf_normal.set(-0.0f, -1.0f, 0.00014140616985969245f);
            actual.contact_cross_nomal_cs[0].set(-0.07874153554439545f, -0.042734816670417786f, -0.156248539686203f);
            actual.virtual_mass = 0.1943139135837555f;
            expected = actual;
            expected.contact_core[0] = reinterpret_cast<IVP_Core *>(original_core);
            const float gap = 0.012887340039014816f + sample * 0.00003125f;
            write<float>(original_contact, 0x58, gap);
            write<float>(native_contact, 0x58, gap);
            write<void *>(original_contact, 0x40, &expected);
            write<void *>(native_contact, 0x40, &actual);
            __declspec(align(16)) unsigned char event_packet[sizeof(IVP_Event_Sim)] = {};
            IVP_Event_Sim &event = *reinterpret_cast<IVP_Event_Sim *>(event_packet);
            event.delta_time = 1.0 / 66.0;
            event.i_delta_time = 66.0;
            at<Pressure>(0x1AE40)(original_contact, &event, 0.02f, 1.0f);
            native_pressure(native_contact, &event);
            if (sample == 0)
                std::printf("single pressure recorded PC%d output Y %.9g / %.9g\n", precision,
                            read<float>(original_core, 0xA8), core->speed.k[1]);
            bool same = read<float>(original_contact, 0x54) == read<float>(native_contact, 0x54);
            for (int k = 0; k < 3; ++k)
                same = same && read<float>(original_core, 0x94 + k * 4) == core->rot_speed.k[k] &&
                       read<float>(original_core, 0xA4 + k * 4) == core->speed.k[k];
            if (!same)
            {
                ++differences;
                if (differences <= 4)
                    std::printf("single pressure PC%d sample %d velocity Y %.9g / %.9g\n", precision, sample,
                                read<float>(original_core, 0xA8), core->speed.k[1]);
            }
        }
    }
    std::printf("single pressure mismatches: %d / 384\n", differences);
    return differences == 0;
}

static bool single_pressure_cancellation_test()
{
    typedef void(__thiscall * Pressure)(void *, const IVP_Event_Sim *, float, float);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            alignas(16) unsigned char old_core[0x200] = {}, native_core[sizeof(IVP_Core)] = {};
            unsigned char old_contact[0x78] = {}, native_contact[sizeof(IVP_Contact_Point)] = {};
            IVP_Core *core = reinterpret_cast<IVP_Core *>(native_core);
            IVP_Impact_Solver_Long_Term actual, expected;
            std::memcpy(&actual, l4_pressure_info, sizeof(actual));
            actual.contact_core[0] = core;
            actual.contact_core[1] = NULL;
            expected = actual;
            expected.contact_core[0] = reinterpret_cast<IVP_Core *>(old_core);
            std::memcpy(&core->speed, l4_pressure_speed, 12);
            core->speed.k[1] -= sample * 0.000931f;
            std::memcpy(&core->rot_speed, l4_pressure_angular, 12);
            std::memcpy(const_cast<IVP_U_Point_4 *>(core->get_inv_masses()), l4_pressure_inverse_mass, 16);
            std::memcpy(old_core + 0x34, core->get_inv_masses(), 16);
            std::memcpy(old_core + 0x94, &core->rot_speed, 12);
            std::memcpy(old_core + 0xA4, &core->speed, 12);
            std::memcpy(old_contact, l4_pressure_contact, sizeof(l4_pressure_contact));
            std::memcpy(native_contact, l4_pressure_contact, sizeof(l4_pressure_contact));
            write<void *>(old_contact, 0x40, &expected);
            write<void *>(native_contact, 0x40, &actual);
            alignas(16) unsigned char event_bytes[sizeof(IVP_Event_Sim)] = {};
            IVP_Event_Sim &event = *reinterpret_cast<IVP_Event_Sim *>(event_bytes);
            event.delta_time = l4_pressure_event[0];
            event.i_delta_time = l4_pressure_event[1];
            at<Pressure>(0x1AE40)(old_contact, &event, l4_pressure_event[2], l4_pressure_event[3]);
            native_pressure(native_contact, &event);
            if (sample == 0 && precision == 2 && read<float>(old_core, 0xA8) != 2.555998079856181e-08f)
            {
                std::printf("L4 recorded pressure reproduction differs %.17g\n", read<float>(old_core, 0xA8));
                return false;
            }
            bool same = read<float>(old_contact, 0x54) == read<float>(native_contact, 0x54);
            for (int k = 0; k < 3; ++k)
                same = same && read<float>(old_core, 0xA4 + k * 4) == core->speed.k[k] &&
                       read<float>(old_core, 0x94 + k * 4) == core->rot_speed.k[k];
            if (!same)
            {
                ++differences;
                if (differences <= 6)
                    std::printf("pressure cancellation PC%d sample%d Y %.17g / %.17g\n", precision, sample,
                                read<float>(old_core, 0xA8), core->speed.k[1]);
            }
        }
    }
    std::printf("pressure cancellation mismatches %d / 384\n", differences);
    return differences == 0;
}

#define RETAIL_REGISTER_pressure RETAIL_REGISTER
#include "RetailTestRegistration.inc"
