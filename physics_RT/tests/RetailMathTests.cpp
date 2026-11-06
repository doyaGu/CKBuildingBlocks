#include "RetailTestSupport.h"

#include <ivp_physics.hxx>
#include <ivp_mindist.hxx>
#include <ivu_float.hxx>
#include <ivu_memory.hxx>
#if IVP_ENABLE_X87_EMULATION
#include <ivu_x87.hxx>
#endif

#include <cmath>
#include <new>

#include "fixtures/constraint_point.hxx"
#include "fixtures/kk_contact_10362.hxx"
#include "fixtures/matrix_product.hxx"
#include "fixtures/pk_kk_14391.hxx"
#include "fixtures/rescue_push.hxx"
#include "fixtures/world_async_push.hxx"

// clang-format off
#pragma comment(linker, "/alternatename:_parity_native_real_length_entry=?real_length@IVP_U_Point@@QBENXZ")
// clang-format on

static bool math_test()
{
    typedef double(__cdecl * DoubleFn)(double);
    typedef float(__cdecl * FloatFn)(float);
    typedef double(__cdecl * WideFloatFn)(float);
    const double values[] = {0.00001, 0.01, 0.7, 1.0, 2.0, 7.25, 25.0, 123.456, 10000.0};
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int i = 0; i < 9; ++i)
        {
            const double expected = at<DoubleFn>(0xDB80)(values[i]);
            const double actual = IVP_Inline_Math::isqrt_double(values[i]);
            const float expected_f = at<FloatFn>(0xDAE0)(static_cast<float>(values[i]));
            const float actual_f = IVP_Inline_Math::isqrt_float(static_cast<float>(values[i]));
            const volatile double expected_wide = at<WideFloatFn>(0xDAE0)(static_cast<float>(values[i]));
            const volatile double actual_wide = IVP_Inline_Math::isqrt_float(static_cast<float>(values[i]));
            if (expected != actual || expected_f != actual_f || expected_wide != actual_wide)
            {
                ++differences;
                std::printf("isqrt PC%d input %.9g retail %.17g rebuilt %.17g float %.9g / %.9g\n",
                            precision == 2   ? 64
                            : precision == 1 ? 53
                                             : 24,
                            values[i], expected, actual, expected_f, actual_f);
                std::printf("float-input ST0 result %.17g / %.17g\n", expected_wide, actual_wide);
            }
        }
    }
    std::printf("isqrt mismatches: %d / 27\n", differences);
    return differences == 0;
}

static bool settings_test()
{
    typedef void(__thiscall * Setter)(void *, double);
    const double tolerances[] = {0.01f, 0.02, 0.0037};
    int differences = 0;
    for (int i = 0; i < 3; ++i)
    {
        unsigned char storage[0x138] = {};
        at<Setter>(0x15FE0)(storage, tolerances[i]);
        IVP_Mindist_Settings settings;
        settings.set_collision_tolerance(tolerances[i]);
        // The retail revision ends here; later SDK-only bookkeeping is ignored.
        for (unsigned int j = 0; j < 75; ++j)
        {
            float expected = read<float>(storage, j * 4);
            float actual = read<float>(&settings, j * 4);
            if (expected != actual)
            {
                ++differences;
                std::printf("tolerance %.9g offset 0x%X retail %.9g rebuilt %.9g\n", tolerances[i], j * 4, expected,
                            actual);
            }
        }
    }
    std::printf("collision settings mismatches: %d\n", differences);
    return differences == 0;
}

static bool inverse_position_float_test()
{
    using Transform = void(__thiscall *)(const void *, const IVP_U_Point *, IVP_U_Float_Point *);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            const auto &row = retail_world_push_inputs[sample % (sizeof(retail_world_push_inputs) /
                                                                 sizeof(retail_world_push_inputs[0]))];
            IVP_U_Matrix matrix;
            for (int r = 0; r < 3; ++r)
                for (int c = 0; c < 3; ++c)
                    matrix.set_elem(r, c, row.matrix[r * 3 + c]);
            matrix.vv.set(173.1379, -19.731379, 81.91379);
            IVP_U_Point point(matrix.vv.k[0] + sample * 0.193179, matrix.vv.k[1] - sample * 0.713719,
                              matrix.vv.k[2] + sample * 0.391379);
            IVP_U_Float_Point expected, actual;
            at<Transform>(0xF370)(&matrix, &point, &expected);
            matrix.vimult4(&point, &actual);
            if (std::memcmp(expected.k, actual.k, 12) != 0)
            {
                ++differences;
                if (differences <= 4)
                    std::printf("inverse position float PC%d sample %d %.9g / %.9g\n", precision, sample, expected.k[0],
                                actual.k[0]);
            }
        }
    }
    std::printf("inverse position float mismatches: %d / 384\n", differences);
    return differences == 0;
}

static bool rotation_test()
{
    typedef void(__thiscall * Rotate)(void *, const IVP_U_Float_Point *, double);
    typedef void(__thiscall * Multiply)(void *, const IVP_U_Quat *, const IVP_U_Quat *);
    typedef void(__thiscall * Matrix)(const void *, IVP_U_Matrix3 *);
    unsigned int ignored;
    _controlfp_s(&ignored, _PC_64, _MCW_PC);
    double maximum = 0;
    int matrix_differences = 0, polynomial_differences = 0;
    for (int i = 0; i < 12; ++i)
    {
        IVP_U_Float_Point angles(0.13f * (i + 1), -0.27f * (i + 1), 0.19f * (i + 1));
        double dt = i < 6 ? 1.0 / 66.0 : 1.37;
        for (int mode = 0; mode < 2; ++mode)
        {
            IVP_U_Quat expected, actual;
            // The polynomial is used only for small gyro substeps.
            if (mode == 1)
                dt = 1.0 / 66.0;
            at<Rotate>(mode == 0 ? 0x19010 : 0x18F80)(&expected, &angles, dt);
            if (mode == 0)
                actual.set_fast_multiple_with_clip(&angles, dt);
            else
                actual.set_very_fast_multiple(&angles, dt);
            for (int n = 0; n < 4; ++n)
            {
                double e = read<double>(&expected, n * 8), a = read<double>(&actual, n * 8);
                maximum = ParityMaximum(maximum, std::fabs(e - a));
                if (mode == 1 && e != a)
                    ++polynomial_differences;
                if (std::fabs(e - a) > 2e-15)
                    std::printf("rotation case %d mode %d component %d %.17g / %.17g\n", i, mode, n, e, a);
            }
            IVP_U_Matrix3 expected_matrix, actual_matrix;
            at<Matrix>(0x190C0)(&expected, &expected_matrix);
            expected.set_matrix(&actual_matrix);
            for (int row = 0; row < 3; ++row)
                for (int column = 0; column < 3; ++column)
                    matrix_differences += expected_matrix.get_elem(row, column) != actual_matrix.get_elem(row, column);
            IVP_U_Quat a, b;
            a.set(0.31, -0.47, 0.29);
            b = expected;
            at<Multiply>(0x1E7C0)(&expected, &a, &b);
            actual.inline_set_mult_quat(&a, &b);
            for (int n = 0; n < 4; ++n)
                maximum = ParityMaximum(maximum, std::fabs(read<double>(&expected, n * 8) - read<double>(&actual, n * 8)));
        }
    }
    std::printf("rotation kernel max error %.17g\n", maximum);
    std::printf("quaternion matrix mismatches %d / 216\n", matrix_differences);
    std::printf("polynomial quaternion mismatches %d / 48\n", polynomial_differences);
    return maximum <= 2e-15 && matrix_differences == 0 && polynomial_differences == 0;
}

static bool vector_test()
{
    typedef double(__thiscall * Length)(const void *);
    typedef double(__thiscall * Dot)(const void *, const void *);
    unsigned int ignored;
    _controlfp_s(&ignored, _PC_64, _MCW_PC);
    double maximum = 0;
    for (int i = 1; i <= 16; ++i)
    {
        IVP_U_Float_Point a(0.173f * i, -0.287f * i, 0.1937f * i), b(-0.318f * i, 0.671f * i, 0.219f * i);
        double e = at<Length>(0x1E7A0)(&a), v = a.quad_length();
        maximum = ParityMaximum(maximum, std::fabs(e - v));
        e = at<Dot>(0x1B060)(&a, &b);
        v = a.dot_product(&b);
        maximum = ParityMaximum(maximum, std::fabs(e - v));
    }
    std::printf("vector kernel max error %.17g\n", maximum);
    return maximum <= 2e-15;
}

static bool double_point_interpolation_test()
{
    typedef void(__thiscall * Interpolate)(IVP_U_Point *, const IVP_U_Point *, const IVP_U_Point *, double);
    IVP_U_Matrix matrix;
    std::memcpy(&matrix, retail_kk_contact_matrices, 128);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            IVP_U_Float_Point local[2];
            IVP_U_Point points[2], expected, actual;
            for (int i = 0; i < 2; ++i)
            {
                std::memcpy(local[i].k, retail_kk_contact_points + i * 3, 12);
                matrix.vmult4(local + i, points + i);
            }
            const double fraction = (sample + 1) * 0.003791357913579;
            at<Interpolate>(0xDC40)(&expected, points, points + 1, fraction);
            actual.set_interpolate(points, points + 1, fraction);
            bool same = std::memcmp(expected.k, actual.k, 24) == 0;
            expected = points[0];
            actual = points[0];
            at<Interpolate>(0xDC40)(&expected, &expected, points + 1, fraction);
            actual.set_interpolate(&actual, points + 1, fraction);
            same = same && std::memcmp(expected.k, actual.k, 24) == 0;
            if (!same)
            {
                ++differences;
                if (differences <= 4)
                    std::printf("double interpolation PC%d sample%d Z %.17g / %.17g\n", precision, sample,
                                expected.k[2], actual.k[2]);
            }
        }
    }
    std::printf("double point interpolation mismatches %d / 384\n", differences);
    return differences == 0;
}

static bool length_normize_test()
{
    typedef double(__thiscall * FloatLength)(IVP_U_Float_Point *);
    typedef double(__thiscall * DoubleLength)(IVP_U_Point *);
    int differences = 0;
    const double scales[] = {0.0, 1e-11, 1e-10, 1e-9, 1e-8, 1e-7, 1e-6, 1e-5, 0.01, 1.0};
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 100; ++sample)
        {
            const double scale = scales[sample % 10];
            IVP_U_Float_Point expected_f, actual_f;
            IVP_U_Point expected_d, actual_d;
            expected_f.set((sample % 7 - 3) * scale * 0.137531, (sample % 11 - 5) * scale * 0.237913,
                           (sample % 13 - 6) * scale * 0.327159);
            if (sample == 66)
                expected_f.set(1.5458271036550286e-06f, -7.644295692443848e-06f, 2.610249794088304e-10f);
            expected_d.set(expected_f.k[0], expected_f.k[1], expected_f.k[2]);
            actual_f = expected_f;
            actual_d = expected_d;
            const volatile double ef = at<FloatLength>(0xDF30)(&expected_f), af = actual_f.real_length_plus_normize();
            const volatile double ed = at<DoubleLength>(0xDE30)(&expected_d), ad = actual_d.real_length_plus_normize();
            bool same = ef == af && ed == ad;
            for (int k = 0; k < 3; ++k)
                same = same && expected_f.k[k] == actual_f.k[k] && expected_d.k[k] == actual_d.k[k];
            if (!same)
            {
                ++differences;
                if (differences <= 4)
                    std::printf("length normize PC%d sample %d float %.17g / %.17g double %.17g / %.17g\n", precision,
                                sample, ef, af, ed, ad);
            }
        }
    }
    std::printf("length normize mismatches: %d / 300\n", differences);
    return differences == 0;
}

static bool inverse_quaternion_kernel_test()
{
    typedef void(__thiscall * Inverse)(IVP_U_Quat *, const IVP_U_Quat *, const IVP_U_Quat *);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int i = 0; i < 128; ++i)
        {
            IVP_U_Quat from, to, expected, actual;
            from.x = -0.46456418196629545;
            from.y = -0.46768450048744015;
            from.z = -0.7248456004481669;
            from.w = -0.20012542154541652;
            to.x = -0.4439953289609428 + i * 0.0000013579;
            to.y = -0.472418359192289 - i * 0.000002468;
            to.z = -0.7247763469918909;
            to.w = -0.23321296833585292;
            at<Inverse>(0x19620)(&expected, &from, &to);
            actual.set_invert_mult(&from, &to);
            if (std::memcmp(&expected, &actual, 32) != 0)
            {
                if (differences < 8)
                    std::printf("inverse quaternion PC%d case %d y %.17g / %.17g\n", precision, i, expected.y,
                                actual.y);
                ++differences;
            }
        }
    }
    std::printf("inverse quaternion mismatches: %d / 384\n", differences);
    return differences == 0;
}

static bool matrix_quaternion_kernel_test()
{
    using Convert = void(__thiscall *)(IVP_U_Quat *, const IVP_U_Matrix3 *);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            IVP_U_Quat input, expected, actual;
            input.x = (sample % 7 - 3) * 0.21753123456789;
            input.y = (sample % 11 - 5) * 0.13579123456789;
            input.z = (sample % 17 - 8) * 0.019753123456789;
            input.w = (sample - 61) * 0.0319753123456789;
            if (sample == 0)
            {
                input.x = 0;
                input.y = 1;
                input.z = 0;
                input.w = 4.371138828673793e-8;
            }
            input.normize_quat();
            IVP_U_Matrix3 matrix;
            input.set_matrix(&matrix);
            at<Convert>(0x191B0)(&expected, &matrix);
            actual.set_quaternion(&matrix);
            if (std::memcmp(&expected, &actual, 32) != 0)
            {
                ++differences;
                if (differences <= 4)
                    std::printf("matrix quaternion PC%d sample %d W %.17g / %.17g\n", precision, sample, expected.w,
                                actual.w);
            }
        }
    }
    std::printf("matrix quaternion mismatches: %d / 384\n", differences);
    return differences == 0;
}

static bool normize_quaternion_kernel_test()
{
    using Normalize = void(__thiscall *)(IVP_U_Quat *);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            IVP_U_Quat expected, actual;
            if (sample == 0)
            {
                expected.x = 0.0;
                expected.y = 1.0;
                expected.z = 0.0;
                expected.w = 4.371138828673793e-8;
            }
            else
            {
                expected.x = (sample - 61) * 0.019753123456789;
                expected.y = (sample % 17 - 8) * 0.23795123456789;
                expected.z = (sample % 11 - 5) * 0.13579135791357;
                expected.w = (sample % 7 - 3) * 0.2753123456789;
                if (sample % 16 == 0)
                {
                    expected.x *= 1e-11;
                    expected.y *= 1e-11;
                    expected.z *= 1e-11;
                    expected.w *= 1e-11;
                }
            }
            actual = expected;
            at<Normalize>(0x194E0)(&expected);
            actual.normize_quat();
            if (std::memcmp(&expected, &actual, 32) != 0)
            {
                ++differences;
                if (differences <= 4)
                    std::printf("quaternion normize PC%d sample %d Y %.17g / %.17g\n", precision, sample, expected.y,
                                actual.y);
            }
        }
    }
    std::printf("quaternion normize mismatches: %d / 384\n", differences);
    return differences == 0;
}

static bool fast_normize_quaternion_test()
{
    using Normalize = void(__thiscall *)(IVP_U_Quat *);
    int differences = 0;
    for (int precision = 1; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : _PC_53, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            IVP_U_Quat expected, actual;
            expected.x = (sample % 7 - 3) * 0.12753123456789;
            expected.y = (sample % 11 - 5) * 0.06579123456789;
            expected.z = (sample % 17 - 8) * 0.019753123456789;
            expected.w = std::sqrt(1.0 - expected.x * expected.x - expected.y * expected.y - expected.z * expected.z) *
                         (0.97 + sample * 0.00047193);
            actual = expected;
            at<Normalize>(0x19540)(&expected);
            actual.fast_normize_quat();
            if (std::memcmp(&expected, &actual, 32))
            {
                ++differences;
                if (differences <= 4)
                    std::printf("fast quaternion normalize PC%d sample%d W %.17g / %.17g\n", precision, sample,
                                expected.w, actual.w);
            }
        }
    }
    std::printf("fast quaternion normalize mismatches: %d / 256 PC53/64 cases\n", differences);
    return differences == 0;
}

static bool mixed_point_transform_kernel_test()
{
    using Transform = void(__thiscall *)(const IVP_U_Matrix *, const IVP_U_Float_Point *, IVP_U_Point *);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            IVP_U_Matrix matrix;
            std::memcpy(&matrix, constraint_point_matrix, sizeof(matrix));
            IVP_U_Float_Point input;
            input.set(constraint_point_input[0] + sample * 0.000001379f, constraint_point_input[1],
                      constraint_point_input[2]);
            IVP_U_Point expected, actual;
            at<Transform>(0xF5F0)(&matrix, &input, &expected);
            matrix.vmult4(&input, &actual);
            if (std::memcmp(expected.k, actual.k, 24) != 0)
            {
                ++differences;
                if (differences <= 4)
                    std::printf("mixed point transform PC%d sample %d Y %.17g / %.17g\n", precision, sample,
                                expected.k[1], actual.k[1]);
            }
        }
    }
    std::printf("mixed point transform mismatches: %d / 384\n", differences);
    return differences == 0;
}

static bool matrix_products_kernel_test()
{
    using Multiply = void(__thiscall *)(const IVP_U_Matrix3 *, const IVP_U_Matrix3 *, IVP_U_Matrix3 *);
    int differences[2] = {};
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
            for (int kind = 0; kind < 2; ++kind)
            {
                IVP_U_Matrix3 a, b, expected, actual;
                std::memcpy(&a, matrix_product_a, 96);
                std::memcpy(&b, matrix_product_b, 96);
                a.set_elem(0, 2, a.get_elem(0, 2) + sample * 0.00000001379);
                auto native = [&](const IVP_U_Matrix3 *x, const IVP_U_Matrix3 *y, IVP_U_Matrix3 *z) {
                    if (kind)
                        x->mi2mult3(y, z);
                    else
                        x->mmult3(y, z);
                };
                auto equal = [](const IVP_U_Matrix3 &x, const IVP_U_Matrix3 &y) {
                    for (int r = 0; r < 3; ++r)
                        for (int c = 0; c < 3; ++c)
                            if (x.get_elem(r, c) != y.get_elem(r, c))
                                return false;
                    return true;
                };
                at<Multiply>(kind ? 0xF140 : 0xEF10)(&a, &b, &expected);
                native(&a, &b, &actual);
                bool same = equal(expected, actual);
                IVP_U_Matrix3 alias = a;
                native(&alias, &b, &alias);
                same = same && equal(expected, alias);
                alias = b;
                native(&a, &alias, &alias);
                same = same && equal(expected, alias);
                if (!same)
                {
                    ++differences[kind];
                    if (differences[kind] <= 3)
                        std::printf("matrix product kind%d PC%d sample%d row2 col1 %.17g / %.17g\n", kind, precision,
                                    sample, expected.get_elem(2, 1), actual.get_elem(2, 1));
                }
            }
    }
    std::printf("matrix product mismatches: %d / 384; transpose: %d / 384\n", differences[0], differences[1]);
    return differences[0] == 0 && differences[1] == 0;
}

static bool double_transform_kernel_test()
{
    using Transform = void(__thiscall *)(const IVP_U_Matrix3 *, const IVP_U_Point *, IVP_U_Point *);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            IVP_U_Matrix3 matrix;
            matrix.init3();
            for (int row = 0; row < 3; ++row)
                for (int col = 0; col < 3; ++col)
                    matrix.set_elem(row, col, retail_rescue_matrices[0][row * 3 + col] + sample * 0.000013579);
            IVP_U_Point input, expected, actual;
            input.set((sample - 61) * 0.019753123456789, (sample % 17 - 8) * 0.23795123456789,
                      (sample % 11 - 5) * 0.13579135791357);
            if (sample % 16 == 0)
                input.mult(1e-11);
            at<Transform>(0xF6F0)(&matrix, &input, &expected);
            matrix.vmult3(&input, &actual);
            bool same = std::memcmp(expected.k, actual.k, 24) == 0;
            expected = input;
            actual = input;
            at<Transform>(0xF6F0)(&matrix, &expected, &expected);
            matrix.vmult3(&actual, &actual);
            same = same && std::memcmp(expected.k, actual.k, 24) == 0;
            if (!same)
            {
                ++differences;
                if (differences <= 4)
                    std::printf("double transform PC%d sample %d Y %.17g / %.17g\n", precision, sample, expected.k[1],
                                actual.k[1]);
            }
        }
    }
    std::printf("double transform mismatches: %d / 384\n", differences);
    return differences == 0;
}

static bool inverse_double_transform_kernel_test()
{
    using Transform = void(__thiscall *)(const IVP_U_Matrix3 *, const IVP_U_Point *, IVP_U_Point *);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            IVP_U_Matrix3 matrix;
            matrix.init3();
            for (int row = 0; row < 3; ++row)
                for (int col = 0; col < 3; ++col)
                    matrix.set_elem(row, col, retail_rescue_matrices[0][row * 3 + col] + sample * 0.000013579);
            IVP_U_Point input, expected, actual;
            input.set((sample - 61) * 0.019753123456789, (sample % 17 - 8) * 0.23795123456789,
                      (sample % 11 - 5) * 0.13579135791357);
            if (sample % 16 == 0)
                input.mult(1e-11);
            at<Transform>(0xF760)(&matrix, &input, &expected);
            matrix.vimult3(&input, &actual);
            bool same = std::memcmp(expected.k, actual.k, 24) == 0;
            expected = input;
            actual = input;
            at<Transform>(0xF760)(&matrix, &expected, &expected);
            matrix.vimult3(&actual, &actual);
            same = same && std::memcmp(expected.k, actual.k, 24) == 0;
            if (!same)
            {
                ++differences;
                if (differences <= 4)
                    std::printf("inverse double transform PC%d sample %d Y %.17g / %.17g\n", precision, sample,
                                expected.k[1], actual.k[1]);
            }
        }
    }
    std::printf("inverse double transform mismatches: %d / 384\n", differences);
    return differences == 0;
}

static bool matrix_inverse_kernel_test()
{
    using Invert = IVP_RETURN_TYPE(__thiscall *)(IVP_U_Matrix *, double);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            IVP_U_Matrix expected, actual;
            expected.init();
            if (sample == 0)
            {
                expected.set_elem(0, 0, -0.999999999999996);
                expected.set_elem(0, 2, 8.742277657347569e-8);
                expected.set_elem(2, 0, -8.742277657347569e-8);
                expected.set_elem(2, 2, -0.999999999999996);
            }
            else
            {
                for (int row = 0; row < 3; ++row)
                    for (int col = 0; col < 3; ++col)
                        expected.set_elem(row, col,
                                          retail_rescue_matrices[0][row * 3 + col] +
                                              (row == col ? sample * 0.000013579 : 0.0));
            }
            expected.vv.set(720.6406859760884, -13.98040771484375, -873.9077758789062);
            actual = expected;
            const auto e = at<Invert>(0xE8F0)(&expected, 1e-19), r = actual.real_invert(1e-19);
            bool same = e == r;
            for (int row = 0; row < 3; ++row)
                for (int col = 0; col < 3; ++col)
                    same = same && expected.get_elem(row, col) == actual.get_elem(row, col);
            for (int k = 0; k < 3; ++k)
                same = same && expected.vv.k[k] == actual.vv.k[k];
            if (!same)
            {
                ++differences;
                if (differences <= 4)
                    std::printf("matrix inverse PC%d sample %d Y diagonal %.17g / %.17g\n", precision, sample,
                                expected.get_elem(1, 1), actual.get_elem(1, 1));
            }
        }
    }
    const double determinants[] = {0.0, 0.5e-19, 1e-19, -1e-19, 2e-19, -2e-19};
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (double determinant : determinants)
        {
            IVP_U_Matrix expected, actual;
            expected.init();
            expected.set_elem(1, 1, determinant);
            expected.vv.set(720.6406859760884, -13.98040771484375, -873.9077758789062);
            actual = expected;
            const IVP_U_Matrix original = expected;
            const auto e = at<Invert>(0xE8F0)(&expected, 1e-19), r = actual.real_invert(1e-19);
            const bool same = e == r && std::memcmp(&expected, &actual, 128) == 0 &&
                              (e != IVP_FAULT || std::memcmp(&actual, &original, 128) == 0);
            if (!same)
            {
                ++differences;
                std::printf("matrix inverse threshold PC%d determinant %.17g return %d / %d\n", precision, determinant,
                            int(e), int(r));
            }
        }
    }
    std::printf("matrix inverse mismatches: %d / 402\n", differences);
    return differences == 0;
}

static bool double_normize_kernel_test()
{
    using Normalize = IVP_RETURN_TYPE(__thiscall *)(IVP_U_Point *);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            IVP_U_Point expected, actual;
            if (sample == 0)
                expected.set(8.742277657347569e-8, 0.0, 0.999999999999996);
            else
            {
                expected.set((sample - 61) * 0.019753123456789, (sample % 17 - 8) * 0.23795123456789,
                             (sample % 11 - 5) * 0.13579135791357);
                if (sample % 16 == 0)
                    expected.mult(1e-11);
            }
            actual = expected;
            const auto e = at<Normalize>(0xE120)(&expected), r = actual.normize();
            if (e != r || std::memcmp(expected.k, actual.k, 24) != 0)
            {
                ++differences;
                if (differences <= 4)
                    std::printf("double normize PC%d sample %d Z %.17g / %.17g\n", precision, sample, expected.k[2],
                                actual.k[2]);
            }
        }
    }
    std::printf("double normize mismatches: %d / 384\n", differences);
    return differences == 0;
}

static bool normize_kernel_test()
{
    typedef IVP_RETURN_TYPE(__thiscall * Normize)(IVP_U_Float_Point *);
    unsigned int seed = 123456789;
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int i = 0; i < 100000; ++i)
        {
            IVP_U_Float_Point expected, actual;
            for (int k = 0; k < 3; ++k)
            {
                seed = 1664525 * seed + 1013904223;
                expected.k[k] = (int(seed >> 8) - 8388608) * 0.000001234567f;
            }
            actual = expected;
            const IVP_RETURN_TYPE e = at<Normize>(0xE030)(&expected), a = actual.normize();
            if (e != a || std::memcmp(expected.k, actual.k, 12) != 0)
            {
                if (differences < 8)
                    std::printf("normize PC%d case %d x %.9g / %.9g y %.9g / %.9g z %.9g / %.9g\n", precision, i,
                                expected.k[0], actual.k[0], expected.k[1], actual.k[1], expected.k[2], actual.k[2]);
                ++differences;
            }
        }
    }
    std::printf("normize mismatches: %d / 300000\n", differences);
    return differences == 0;
}

static bool double_point_transform_test()
{
    using Transform = void(__thiscall *)(const IVP_U_Matrix *, const IVP_U_Point *, IVP_U_Point *);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            IVP_U_Matrix matrix;
            std::memcpy(&matrix, retail_pk_kk_matrix_a, 128);
            IVP_U_Point input, expected, actual;
            std::memcpy(input.k, retail_pk_kk_solver + 40, 24);
            input.k[0] += sample * 0.00000001379;
            at<Transform>(0xF550)(&matrix, &input, &expected);
            matrix.vmult4(&input, &actual);
            bool same = std::memcmp(expected.k, actual.k, 24) == 0;
            expected = input;
            actual = input;
            at<Transform>(0xF550)(&matrix, &expected, &expected);
            matrix.vmult4(&actual, &actual);
            same = same && std::memcmp(expected.k, actual.k, 24) == 0;
            if (!same)
            {
                ++differences;
                if (differences <= 4)
                    std::printf("double point transform PC%d sample%d Z %.17g / %.17g\n", precision, sample,
                                expected.k[2], actual.k[2]);
            }
        }
    }
    std::printf("double point transform mismatches: %d / 384\n", differences);
    return differences == 0;
}

extern "C" void parity_native_real_length_entry();

static bool double_real_length_test()
{
    typedef double(__thiscall * Length)(IVP_U_Point *);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            IVP_U_Point vector;
            vector.set((sample % 17 - 8) * 0.13753123456789 + 0.00791, (sample % 23 - 11) * 0.237913456789123,
                       (sample % 29 - 14) * 0.327159753123456);
            unsigned char expected[10], actual[10];
            Length original = at<Length>(0xDD20);
            void *receiver = &vector;
            __asm {
                mov ecx, receiver
                call original
                fstp tbyte ptr expected
                mov ecx, receiver
                call parity_native_real_length_entry
                fstp tbyte ptr actual
            }
            if (std::memcmp(expected, actual, 10) != 0)
            {
                ++differences;
                if (differences <= 4)
                    std::printf("double real length PC%d sample%d extended return differs\n", precision, sample);
            }
        }
    }
    std::printf("double real length mismatches %d / 384\n", differences);
    return differences == 0;
}

static bool interpolate_kernel_test()
{
    typedef void(__thiscall * Interpolate)(IVP_U_Quat *, const IVP_U_Quat *, const IVP_U_Quat *, double);
    const double fractions[] = {0.0, 0.123456789012345, 0.5, 0.712345678901234, 1.0};
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int i = 0; i < 5; ++i)
            for (int sign = -1; sign <= 1; sign += 2)
            {
                IVP_U_Quat from, to, expected, actual;
                from.x = -0.12712033954351706;
                from.y = 0.9918873017003091;
                from.z = 1.3417153996787672e-7;
                from.w = 5.249360060255856e-8;
                to.x = -0.142943538115755 * sign;
                to.y = 0.9897167660135355 * sign;
                to.z = -0.005175473452641218 * sign;
                to.w = -0.0010404120333647872 * sign;
                at<Interpolate>(0x19320)(&expected, &from, &to, fractions[i]);
                actual.set_interpolate_smoothly(&from, &to, fractions[i]);
                if (std::memcmp(&expected, &actual, 32) != 0)
                {
                    if (differences < 8)
                        std::printf("interpolation PC%d fraction %.15g sign %d x %.17g / %.17g\n", precision,
                                    fractions[i], sign, expected.x, actual.x);
                    ++differences;
                }
            }
    }
    unsigned int ignored;
    _controlfp_s(&ignored, _PC_64, _MCW_PC);
    IVP_U_Quat from, to, expected, actual;
    from.x = 0.5660137283389004;
    from.y = -0.6240316479891731;
    from.z = -0.48704121826197894;
    from.w = 0.23022557059044005;
    to.x = 0.5979192252758208;
    to.y = -0.5921305605010913;
    to.z = -0.4882445388388551;
    to.w = 0.23128179707934574;
    for (int i = 0; i < 5; ++i)
    {
        const double fraction = i == 0 ? 0.80001300573349 : fractions[i];
        at<Interpolate>(0x19320)(&expected, &from, &to, fraction);
        actual.set_interpolate_smoothly(&from, &to, fraction);
        if (std::memcmp(&expected, &actual, 32) != 0)
        {
            ++differences;
            std::printf("spherical interpolation fraction %.17g y %.17g / %.17g\n", fraction, expected.y, actual.y);
        }
    }
    std::printf("interpolation kernel mismatches: %d / 35\n", differences);
    return differences == 0;
}

static bool matrix_float_kernel_test()
{
    typedef void(__thiscall * Transform)(const IVP_U_Matrix3 *, const IVP_U_Float_Point *, IVP_U_Float_Point *);
    const double matrix[12] = {0.99162775217158472, 0.1152135508242683,  -0.058311566859362722, -0.127985699144431,
                               0.93689558027438291, -0.3253403333078127, 0.017148234242667058,  0.33007955006148321,
                               0.94379734514013514, -148.00155460277944, 18.171561697474196,    -361.54110068526035};
    IVP_U_Matrix3 m;
    for (int r = 0; r < 3; ++r)
        for (int k = 0; k < 3; ++k)
            m.set_elem(r, k, matrix[r * 3 + k]);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            IVP_U_Float_Point input(0.297102689743042f + sample * 0.0013579f,
                                    -0.04810178279876709f + sample * 0.0008642f, -0.007944159209728241f);
            for (int transpose = 0; transpose < 2; ++transpose)
                for (int alias = 0; alias < 2; ++alias)
                {
                    IVP_U_Float_Point e = input, a = input;
                    at<Transform>(transpose ? 0xF7D0 : 0xF690)(&m, alias ? &e : &input, &e);
                    if (transpose)
                        m.vimult3(alias ? &a : &input, &a);
                    else
                        m.vmult3(alias ? &a : &input, &a);
                    for (int k = 0; k < 3; ++k)
                        if (e.k[k] != a.k[k])
                        {
                            ++differences;
                            if (differences <= 8)
                                std::printf("float matrix PC%d case %d transpose %d alias %d axis %d %.9g / %.9g\n",
                                            precision, sample, transpose, alias, k, e.k[k], a.k[k]);
                        }
                }
        }
    }
    std::printf("float matrix mismatches: %d / 4608\n", differences);
    return differences == 0;
}

static bool polynomial_quaternion_test()
{
    typedef void(__thiscall * Rotate)(IVP_U_Quat *, const IVP_U_Float_Point *, double);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int variant = 0; variant < 3; ++variant)
            for (int i = 0; i < 128; ++i)
            {
                IVP_U_Float_Point angles(0.3141677975654602f + i * 0.003579f, -0.03505023196339607f - i * 0.001234f,
                                         -0.2566875219345093f + i * 0.002468f);
                if (variant)
                    angles.set(0.043f + i * 0.137193f, -0.00238619f * (i + 1), -0.00026080815f * (i + 1));
                IVP_U_Quat expected, actual;
                const double step = variant == 0 ? 1.0 / 66.0 : variant == 1 ? 1.0 / 210.0 : 1.0 / 30.0;
                at<Rotate>(0x18F80)(&expected, &angles, step);
                actual.set_very_fast_multiple(&angles, step);
                if (std::memcmp(&expected, &actual, 32) != 0)
                {
                    ++differences;
                    if (differences <= 8)
                        std::printf("polynomial quaternion PC%d case %d w %.17g / %.17g\n", precision, i, expected.w,
                                    actual.w);
                }
            }
    }
    std::printf("polynomial quaternion mismatches: %d / 1152\n", differences);
    return differences == 0;
}

#define RETAIL_REGISTER_math RETAIL_REGISTER
#include "RetailTestRegistration.inc"
