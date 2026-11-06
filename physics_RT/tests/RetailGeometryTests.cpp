#include "RetailTestSupport.h"

#include <ivp_physics.hxx>
#include <ivp_templates.hxx>
#include <ivp_compact_surface.hxx>
#include <ivp_compact_ledge.hxx>
#include <ivp_compact_ledge_solver.hxx>
#include <ivp_surbuild_pointsoup.hxx>
#include <ivp_surbuild_ledge_soup.hxx>
#include <ivp_rot_inertia_solver.hxx>
#include <ivp_surman_polygon.hxx>
#include <qhull_a.hxx>
#include <ivu_memory.hxx>
#if IVP_ENABLE_X87_EMULATION
#include <ivu_x87.hxx>
#endif

#include <cmath>
#include <new>

#include "fixtures/level2_inertia_surface.hxx"
#include "fixtures/scene_7452_tree.hxx"
#include "fixtures/scene_pierce.hxx"
#include "fixtures/sr11_hull.hxx"

static bool hull_test()
{
    typedef IVP_Compact_Ledge *(__cdecl * Build)(void *);
    int differences = 0;
    for (int variant = 0; variant < 4; ++variant)
    {
        IVP_U_Point vertices[10];
        IVP_U_Vector<IVP_U_Point> points;
        for (int i = 0; i < 8; ++i)
        {
            double x = (i & 1) ? 1.0 : -1.0;
            double y = (i & 2) ? 1.0 : -1.0;
            double z = (i & 4) ? 1.0 : -1.0;
            if (variant == 1)
            {
                x *= 0.013;
                y *= 7.1;
                z *= 2.3;
            }
            if (variant == 2)
            {
                y = 0.0;
            }
            if (variant == 3)
            {
                x += i * 0.0031;
                z += i * i * 0.00027;
            }
            vertices[i].set(x, y, z);
            points.add(&vertices[i]);
        }
        vertices[8].set(0, 0, 0);
        vertices[9].set(&vertices[0]);
        points.add(&vertices[8]);
        points.add(&vertices[9]);
        unsigned int ignored;
        _controlfp_s(&ignored, _PC_24, _MCW_PC);
        IVP_Compact_Ledge *expected = at<Build>(0x3AE20)(&points);
        const unsigned int expected_precision = _controlfp(0, 0) & _MCW_PC;
        _controlfp_s(&ignored, _PC_24, _MCW_PC);
        IVP_Compact_Ledge *actual = IVP_SurfaceBuilder_Pointsoup::convert_pointsoup_to_compact_ledge(&points);
        const unsigned int actual_precision = _controlfp(0, 0) & _MCW_PC;
        bool same = expected_precision == _PC_64 && actual_precision == expected_precision && expected && actual &&
                    expected->get_n_triangles() == actual->get_n_triangles() &&
                    expected->get_size() == actual->get_size();
        if (same)
        {
            // Relative links, triangle ordering and point ordering affect contact
            // selection. Compare those too, rather than only hull volume/count.
            int count = expected->get_n_triangles();
            const IVP_Compact_Triangle *e = expected->get_first_triangle();
            const IVP_Compact_Triangle *a = actual->get_first_triangle();
            for (int j = 0; j < count; ++j)
                for (int k = 0; k < 3; ++k)
                {
                    const IVP_Compact_Edge *ee = e[j].get_edge(k), *ae = a[j].get_edge(k);
                    same = same && ee->get_start_point_index() == ae->get_start_point_index() &&
                           ee->get_opposite_index() == ae->get_opposite_index() &&
                           ee->get_is_virtual() == ae->get_is_virtual();
                    const IVP_Compact_Poly_Point *ep = ee->get_start_point(expected), *ap = ae->get_start_point(actual);
                    for (int n = 0; n < 3; ++n)
                        same = same && ep->k[n] == ap->k[n];
                }
        }
        std::printf("hull %d retail triangles %d rebuilt %d topology %s\n", variant,
                    expected ? expected->get_n_triangles() : -1, actual ? actual->get_n_triangles() : -1,
                    same ? "equal" : "DIFFERENT");
        differences += !same;
        if (actual)
            ivp_free_aligned(actual);
        if (expected)
            at<void(__cdecl *)(void *)>(0x20130)(expected);
    }
    return differences == 0;
}

static void compare_scene_tree(const IVP_Compact_Ledgetree_Node *a, const IVP_Compact_Ledgetree_Node *b, int &bounds,
                               int &leaves, int &branches)
{
    const auto *ab = reinterpret_cast<const unsigned char *>(a), *bb = reinterpret_cast<const unsigned char *>(b);
    if (std::memcmp(ab + 8, bb + 8, 16) != 0 || std::memcmp(ab + 24, bb + 24, 3) != 0)
    {
        if (bounds < 3)
            std::printf("tree bound %.9g %.9g %.9g %.9g [%u,%u,%u] / %.9g %.9g %.9g %.9g [%u,%u,%u]\n",
                        read<float>(ab, 8), read<float>(ab, 12), read<float>(ab, 16), read<float>(ab, 20), ab[24],
                        ab[25], ab[26], read<float>(bb, 8), read<float>(bb, 12), read<float>(bb, 16),
                        read<float>(bb, 20), bb[24], bb[25], bb[26]);
        ++bounds;
    }
    if (a->is_terminal() != b->is_terminal())
    {
        ++branches;
        return;
    }
    if (!a->is_terminal())
    {
        compare_scene_tree(a->left_son(), b->left_son(), bounds, leaves, branches);
        compare_scene_tree(a->right_son(), b->right_son(), bounds, leaves, branches);
        return;
    }
    const auto *al = a->get_compact_ledge(), *bl = b->get_compact_ledge();
    if (al->get_n_triangles() != bl->get_n_triangles())
    {
        ++leaves;
        return;
    }
    bool same = true;
    for (int t = 0; t < al->get_n_triangles(); ++t)
        for (int e = 0; e < 3; ++e)
        {
            const auto *ap = al->get_first_triangle()[t].get_edge(e)->get_start_point(al),
                       *bp = bl->get_first_triangle()[t].get_edge(e)->get_start_point(bl);
            same = same && std::memcmp(ap->k, bp->k, 12) == 0;
        }
    if (!same)
        ++leaves;
}

static bool scene_tree_test()
{
    typedef void(__thiscall * Construct)(void *);
    typedef void(__thiscall * Insert)(void *, IVP_Compact_Ledge *);
    typedef IVP_Compact_Surface *(__thiscall * Compile)(void *, void *);
    typedef IVP_Compact_Ledge *(__cdecl * Build)(IVP_U_Vector<IVP_U_Point> *);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        alignas(16) unsigned char builder[0x200] = {};
        at<Construct>(0x38290)(builder);
        IVP_SurfaceBuilder_Ledge_Soup native_builder;
        for (int t = 0; t < 226; ++t)
        {
            IVP_U_Point vertices[3];
            IVP_U_Vector<IVP_U_Point> points;
            for (int k = 0; k < 3; ++k)
            {
                vertices[k].set(scene_7452_triangles[t][k * 3], scene_7452_triangles[t][k * 3 + 1],
                                scene_7452_triangles[t][k * 3 + 2]);
                points.add(vertices + k);
            }
            auto *ledge = at<Build>(0x3AE20)(&points);
            if (!ledge)
                return false;
            auto *native_ledge = IVP_SurfaceBuilder_Pointsoup::convert_pointsoup_to_compact_ledge(&points);
            if (!native_ledge)
                return false;
            at<Insert>(0x38430)(builder, ledge);
            native_builder.insert_ledge(native_ledge);
        }
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        active_call = "scene tree retail compile";
        active_frame = precision;
        auto *expected = at<Compile>(0x384B0)(builder, NULL);
        active_call = "scene tree native compile";
        auto *actual = native_builder.compile();
        active_call = "scene tree compare";
        if (!expected || !actual)
            return false;
        int bounds = 0, leaves = 0, branches = 0;
        compare_scene_tree(expected->get_compact_ledge_tree_root(), actual->get_compact_ledge_tree_root(), bounds,
                           leaves, branches);
        differences += bounds + leaves + branches;
        std::printf("scene tree PC%d bound/order/branch mismatches: %d / %d / %d\n", precision, bounds, leaves,
                    branches);
        active_call = "scene tree cleanup";
        at<Construct>(0x38340)(builder);
        at<void(__cdecl *)(void *)>(0x20130)(expected);
        ivp_free_aligned(actual);
    }
    return differences == 0;
}

static bool scene_pierce_test()
{
    typedef IVP_Compact_Ledge *(__cdecl * Build)(IVP_U_Vector<IVP_U_Point> *);
    int differences = 0;
    for (int sample = 0; sample < 10; ++sample)
    {
        IVP_U_Point vertices[55];
        IVP_U_Vector<IVP_U_Point> points;
        const auto &fixture = scene_pierce_cases[sample];
        for (int p = 0; p < fixture.count; ++p)
        {
            vertices[p].set(fixture.points[p][0], fixture.points[p][1], fixture.points[p][2]);
            points.add(vertices + p);
        }
        auto *expected = at<Build>(0x3AE20)(&points),
             *actual = IVP_SurfaceBuilder_Pointsoup::convert_pointsoup_to_compact_ledge(&points);
        bool same = expected && actual && expected->get_n_triangles() == actual->get_n_triangles();
        int pierce_differences = 0;
        if (same)
            for (int t = 0; t < expected->get_n_triangles(); ++t)
            {
                const auto *et = expected->get_first_triangle() + t, *at = actual->get_first_triangle() + t;
                if (et->get_pierce_index() != at->get_pierce_index())
                    ++pierce_differences;
                for (int e = 0; e < 3; ++e)
                {
                    const auto *ee = et->get_edge(e), *ae = at->get_edge(e);
                    same = same && ee->get_opposite_index() == ae->get_opposite_index() &&
                           ee->get_is_virtual() == ae->get_is_virtual() &&
                           std::memcmp(ee->get_start_point(expected)->k, ae->get_start_point(actual)->k, 12) == 0;
                }
            }
        if (!same || pierce_differences)
            ++differences;
        std::printf("scene pierce case %d differences %d ordered edges %s\n", sample, pierce_differences,
                    same ? "match" : "differ");
        if (expected)
            at<void(__cdecl *)(void *)>(0x20130)(expected);
        if (actual)
            ivp_free_aligned(actual);
    }
    return differences == 0;
}

static bool hesse_normize_test()
{
    typedef void(__thiscall * Normize)(IVP_U_Hesse *);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            IVP_U_Hesse expected, actual;
            expected.set((sample % 17 - 8) * 0.13753123456789, (sample % 23 - 11) * 0.237913456789123,
                         (sample % 29 - 14) * 0.327159753123456);
            expected.hesse_val = (sample - 51) * 0.5137953123456789;
            if (sample == 0)
            {
                expected.set_to_zero();
                expected.hesse_val = 0.0;
            }
            actual = expected;
            at<Normize>(0xE740)(&expected);
            actual.normize();
            if (std::memcmp(&expected, &actual, 32) != 0)
            {
                ++differences;
                if (differences <= 4)
                    std::printf("Hesse normize PC%d case %d X %.17g / %.17g offset %.17g / %.17g\n", precision, sample,
                                expected.k[0], actual.k[0], expected.hesse_val, actual.hesse_val);
            }
        }
    }
    std::printf("Hesse normize mismatches: %d / 384\n", differences);
    return differences == 0;
}

static bool qhull_facets_test()
{
    typedef int(__cdecl * Hull)(int, int, double *, unsigned int, char *, void *, void *);
    typedef void *(__cdecl * Vertices)(void *);
    typedef void(__cdecl * FreeHull)(unsigned int);
    const unsigned int facet_list_rva = 0x76BB0;
    double original_points[24], native_points[24];
    for (int i = 0; i < 8; ++i)
    {
        double x = (i & 1 ? 1.0 : -1.0) * 1.637, y = (i & 2 ? 1.0 : -1.0) * 1.531, z = (i & 4 ? 1.0 : -1.0) * 1.703;
        original_points[i * 3] = x + 0.0317;
        original_points[i * 3 + 1] = y + x * 0.012345;
        original_points[i * 3 + 2] = z + y * 0.009876;
    }
    std::memcpy(native_points, original_points, sizeof(native_points));
    char flags[] = "qhull Qs Pp C-0 W1e-14 E1.0e-18";
    unsigned int ignored;
    _controlfp_s(&ignored, _PC_64, _MCW_PC);
    if (at<Hull>(0x46C30)(3, 8, original_points, 0, flags, NULL, NULL) != 0 ||
        qh_new_qhull(3, 8, native_points, False, flags, NULL, NULL) != 0)
        return false;
    void *original = read<void *>(retail, facet_list_rva);
    facetT *native = qh facet_list;
    int differences = 0, ordinal = 0;
    typedef void(__cdecl * Normalize)(double *, int, unsigned int, double *, unsigned int *);
    for (int i = 0; i < 64; ++i)
    {
        double e[3] = {0.013579 * (i + 1), -0.987654 * (i + 1), 0.007654321 * (i + 1)}, a[3];
        std::memcpy(a, e, 24);
        at<Normalize>(0x4F890)(e, 3, 1, NULL, NULL);
        qh_normalize2(a, 3, True, NULL, NULL);
        if (std::memcmp(e, a, 24) != 0)
        {
            ++differences;
            if (differences <= 8)
                std::printf("Qhull normalize %d y %.17g / %.17g\n", i, e[1], a[1]);
        }
    }
    std::printf("Qhull normalization mismatches: %d / 64\n", differences);
    typedef void(__cdecl * Eliminate)(double **, int, int, unsigned int *, unsigned int *);
    typedef void(__cdecl * BackNormal)(double **, int, int, unsigned int, double *, unsigned int *);
    int elimination_differences = 0, back_differences = 0;
    for (int sample = 0; sample < 128; ++sample)
    {
        double expected_matrix[2][3] = {{3.274 + sample * 0.0013579, 0.04041743 + sample * 0.000013579, 0.0302479},
                                        {0.00076543, 3.062 + sample * 0.0019753, 3.406 + sample * 0.0008642}},
               actual_matrix[2][3];
        std::memcpy(actual_matrix, expected_matrix, sizeof(actual_matrix));
        double *expected_rows[2] = {expected_matrix[0], expected_matrix[1]},
               *actual_rows[2] = {actual_matrix[0], actual_matrix[1]};
        unsigned int expected_sign = 1, actual_sign = 1, expected_zero = 0, actual_zero = 0;
        at<Eliminate>(0x4F2F0)(expected_rows, 2, 3, &expected_sign, &expected_zero);
        qh_gausselim(actual_rows, 2, 3, &actual_sign, &actual_zero);
        elimination_differences += expected_sign != actual_sign || expected_zero != actual_zero;
        for (int r = 0; r < 2; ++r)
            for (int k = 0; k < 3; ++k)
                if (expected_rows[r][k] != actual_rows[r][k])
                {
                    ++elimination_differences;
                    if (elimination_differences <= 4)
                        std::printf("Qhull elimination case %d [%d,%d] %.17g / %.17g\n", sample, r, k,
                                    expected_rows[r][k], actual_rows[r][k]);
                }
        double expected_normal[3], actual_normal[3];
        expected_zero = actual_zero = 0;
        at<BackNormal>(0x4E4C0)(expected_rows, 2, 3, expected_sign, expected_normal, &expected_zero);
        qh_backnormal(expected_rows, 2, 3, expected_sign, actual_normal, &actual_zero);
        back_differences += expected_zero != actual_zero;
        for (int k = 0; k < 3; ++k)
            if (expected_normal[k] != actual_normal[k])
            {
                ++back_differences;
                if (back_differences <= 4)
                    std::printf("Qhull backnormal case %d axis %d %.17g / %.17g\n", sample, k, expected_normal[k],
                                actual_normal[k]);
            }
    }
    std::printf("Qhull elimination/backnormal mismatches: %d / %d\n", elimination_differences, back_differences);
    differences += elimination_differences + back_differences;
    typedef void(__cdecl * Plane)(int, double **, double *, unsigned int, double *, double *, unsigned int *);
    int plane_differences = 0;
    for (int sample = 0; sample < 128; ++sample)
    {
        double points[3][3] = {{-1.6052999496459961, 1.5107913017272949, 1.7181200981140137},
                               {-1.6052999496459961, 1.5107913017272949, -1.6878798007965088},
                               {1.6686999797821045 + sample * 0.000013579, 1.5512087345123291 + sample * 0.000002468,
                                -1.6878798007965088}};
        double *rows[3] = {points[0], points[1], points[2]}, expected_normal[3], actual_normal[3], expected_offset,
               actual_offset;
        unsigned int expected_zero = 0, actual_zero = 0;
        at<Plane>(0x501C0)(3, rows, points[0], sample % 2, expected_normal, &expected_offset, &expected_zero);
        qh_sethyperplane_det(3, rows, points[0], sample % 2, actual_normal, &actual_offset, &actual_zero);
        if (std::memcmp(expected_normal, actual_normal, 24) != 0 || expected_offset != actual_offset ||
            expected_zero != actual_zero)
        {
            ++plane_differences;
            if (plane_differences <= 4)
                std::printf("Qhull plane case %d offset %.17g / %.17g nearzero %u / %u\n", sample, expected_offset,
                            actual_offset, expected_zero, actual_zero);
        }
    }
    std::printf("Qhull determinant-plane mismatches: %d / 128\n", plane_differences);
    differences += plane_differences;
    int gauss_plane_differences = 0;
    for (int sample = 0; sample < 128; ++sample)
    {
        double em[2][3] = {{3.274 + sample * 0.0013579, 0.04041743 + sample * 0.000013579, 0.0302479},
                           {0.00076543, 3.062 + sample * 0.0019753, 3.406 + sample * 0.0008642}},
               am[2][3];
        std::memcpy(am, em, sizeof(am));
        double *er[2] = {em[0], em[1]}, *ar[2] = {am[0], am[1]};
        double point[3] = {-1.6052999496459961, 1.5107913017272949, 1.7181200981140137};
        double en[3], an[3], eo, ao;
        unsigned int ez = 0, az = 0;
        at<Plane>(0x50630)(3, er, point, sample % 2, en, &eo, &ez);
        qh_sethyperplane_gauss(3, ar, point, sample % 2, an, &ao, &az);
        if (std::memcmp(en, an, 24) != 0 || eo != ao || ez != az)
        {
            ++gauss_plane_differences;
            if (gauss_plane_differences <= 4)
                std::printf("Qhull Gauss-plane case %d offset %.17g / %.17g nearzero %u / %u\n", sample, eo, ao, ez,
                            az);
        }
    }
    std::printf("Qhull Gauss-plane mismatches: %d / 128\n", gauss_plane_differences);
    differences += gauss_plane_differences;
    while (original && read<void *>(original, 48) && native && native->next)
    {
        void *expected_set = at<Vertices>(0x40990)(original);
        setT *actual_set = qh_facet3vertex(native);
        const double *expected_normal = read<double *>(original, 24);
        if (std::memcmp(expected_normal, native->normal, 24) != 0)
        {
            ++differences;
            std::printf("Qhull facet %d normal %.17g %.17g %.17g / %.17g %.17g %.17g\n", ordinal, expected_normal[0],
                        expected_normal[1], expected_normal[2], native->normal[0], native->normal[1],
                        native->normal[2]);
        }
        const double expected_offset = read<double>(original, 16);
        if (expected_offset != native->offset)
        {
            ++differences;
            std::printf("Qhull facet %d offset %.17g / %.17g\n", ordinal, expected_offset, native->offset);
        }
        for (int v = 0; v < 8; ++v)
        {
            void *ev = read<void *>(expected_set, 4 + v * 4);
            vertexT *av = static_cast<vertexT *>(SETelem_(actual_set, v));
            if (!ev || !av)
            {
                if (ev != av)
                    ++differences;
                break;
            }
            const double *ep = read<double *>(ev, 8);
            if (std::memcmp(ep, av->point, 24) != 0)
            {
                ++differences;
                std::printf("Qhull facet %d vertex %d point %.9g %.9g %.9g / %.9g %.9g %.9g\n", ordinal, v, ep[0],
                            ep[1], ep[2], av->point[0], av->point[1], av->point[2]);
            }
        }
        qh_settempfree(&actual_set);
        original = read<void *>(original, 48);
        native = native->next;
        ++ordinal;
    }
    const bool original_remaining = original && read<void *>(original, 48);
    const bool native_remaining = native && native->next;
    if (original_remaining != native_remaining)
    {
        ++differences;
        std::printf("Qhull facet lists end at different lengths\n");
    }
    at<FreeHull>(0x43310)(0);
    qh_freeqhull(False);
    int curlong, totlong;
    qh_memfreeshort(&curlong, &totlong);
    std::printf("Qhull facet mismatches: %d across %d facets\n", differences, ordinal);
    return differences == 0;
}

static bool qhull_distance_test()
{
    using Distance = void(__cdecl *)(double *, void *, double *);
    const int saved_dim = read<int>(retail, 0x765EC), saved_native_dim = qh hull_dim;
    int differences = 0, cases = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int dim = 2; dim <= 8; ++dim)
            for (int sample = 0; sample < 128; ++sample)
            {
                double point[8], normal[8];
                for (int k = 0; k < 8; ++k)
                {
                    point[k] = (sample + 1) * 0.0013579 + (k + 1) * 1.123456789;
                    normal[k] = (k % 2 ? -1.0 : 1.0) * ((sample % 7) + 1) * 0.0123456789;
                }
                facetT facet{};
                facet.normal = normal;
                facet.offset = -(point[0] * normal[0] + point[1] * normal[1]);
                unsigned char opaque[88]{};
                write<double>(opaque, 16, facet.offset);
                write<double *>(opaque, 24, normal);
                write<int>(retail, 0x765EC, dim);
                qh hull_dim = dim;
                double expected, actual;
                at<Distance>(0x4E6A0)(point, opaque, &expected);
                qh_distplane(point, &facet, &actual);
                ++cases;
                if (std::memcmp(&expected, &actual, 8))
                {
                    ++differences;
                    if (differences <= 4)
                        std::printf("Qhull distance PC%d dimension%d sample%d %.17g / %.17g\n", precision, dim, sample,
                                    expected, actual);
                }
            }
    }
    write<int>(retail, 0x765EC, saved_dim);
    qh hull_dim = saved_native_dim;
    std::printf("Qhull distance mismatches: %d / %d\n", differences, cases);
    return differences == 0;
}

static bool sr11_hull_test()
{
    using Build = IVP_Compact_Ledge *(__cdecl *)(void *);
    unsigned int ignored;
    _controlfp_s(&ignored, _PC_64, _MCW_PC);
    IVP_U_Point vertices[10];
    IVP_U_Vector<IVP_U_Point> points;
    for (int i = 0; i < 10; ++i)
    {
        vertices[i].set(sr11_hull_points[i][0], sr11_hull_points[i][1], sr11_hull_points[i][2]);
        points.add(vertices + i);
    }
    auto *expected = at<Build>(0x3AE20)(&points);
    auto *actual = IVP_SurfaceBuilder_Pointsoup::convert_pointsoup_to_compact_ledge(&points);
    if (!expected || !actual || expected->get_n_triangles() != actual->get_n_triangles())
        return false;
    int differences = 0;
    for (int t = 0; t < actual->get_n_triangles(); ++t)
        for (int e = 0; e < 3; ++e)
        {
            auto *ee = expected->get_first_triangle()[t].get_edge(e), *ae = actual->get_first_triangle()[t].get_edge(e);
            if (ee->get_start_point_index() != ae->get_start_point_index() ||
                ee->get_opposite_index() != ae->get_opposite_index())
            {
                ++differences;
                std::printf("SR11 hull triangle %d edge %d point %d / %d opposite %d / %d\n", t, e,
                            ee->get_start_point_index(), ae->get_start_point_index(), ee->get_opposite_index(),
                            ae->get_opposite_index());
            }
            differences += std::memcmp(ee->get_start_point(expected)->k, ae->get_start_point(actual)->k, 12) != 0;
        }
    ivp_free_aligned(actual);
    at<void(__cdecl *)(void *)>(0x20130)(expected);
    using Hull = int(__cdecl *)(int, int, double *, unsigned int, char *, void *, void *);
    using Vertices = void *(__cdecl *)(void *);
    double ep[30], ap[30];
    std::memcpy(ep, sr11_hull_points, sizeof(ep));
    std::memcpy(ap, ep, sizeof(ap));
    char flags[] = "qhull Qs Pp C-0 W1e-14 E1.0e-18";
    if (at<Hull>(0x46C30)(3, 10, ep, 0, flags, NULL, NULL) || qh_new_qhull(3, 10, ap, False, flags, NULL, NULL))
        return false;
    void *ef = read<void *>(retail, 0x76BB0);
    auto *af = qh facet_list;
    int facet_differences = 0, ordinal = 0;
    while (ef && read<void *>(ef, 48) && af && af->next)
    {
        void *ev = at<Vertices>(0x40990)(ef);
        auto *av = qh_facet3vertex(af);
        for (int v = 0; v < 10; ++v)
        {
            void *e = read<void *>(ev, 4 + v * 4);
            auto *a = static_cast<vertexT *>(SETelem_(av, v));
            if (!e || !a)
            {
                facet_differences += e != a;
                break;
            }
            if (std::memcmp(read<double *>(e, 8), a->point, 24))
            {
                ++facet_differences;
                std::printf("SR11 Qhull facet %d vertex %d point %d / %d\n", ordinal, v,
                            int((read<double *>(e, 8) - ep) / 3), int((a->point - ap) / 3));
            }
        }
        facet_differences += std::memcmp(read<double *>(ef, 24), af->normal, 24) != 0;
        qh_settempfree(&av);
        ef = read<void *>(ef, 48);
        af = af->next;
        ++ordinal;
    }
    at<void(__cdecl *)(unsigned int)>(0x43310)(0);
    qh_freeqhull(False);
    int curlong, totlong;
    qh_memfreeshort(&curlong, &totlong);
    std::printf("SR11 Qhull facet differences: %d\n", facet_differences);
    differences += facet_differences;
    std::printf("SR11 hull differences: %d\n", differences);
    return differences == 0;
}

static bool scene_inertia_kernel_test()
{
    using Calculate = int(__cdecl *)(const IVP_Compact_Surface *, IVP_U_Point *, IVP_U_Point *);
    using Integrate = void(__thiscall *)(IVP_Compact_Ledge_Mass_Center_Solver *, const IVP_Compact_Ledge *,
                                         const IVP_Compact_Triangle *, int, int, int);
    const auto *surface = reinterpret_cast<const IVP_Compact_Surface *>(level2_inertia_surface);
    int differences = 0, recorded_differences = 0, metadata_differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        IVP_U_Point center, inertia, native_center, native_inertia;
        at<Calculate>(0x3BBF0)(surface, &center, &inertia);
        IVP_Rot_Inertia_Solver::calc_mass_center_and_rotation_inertia(surface, &native_center, &native_inertia);
        if (precision == 2)
            for (int i = 0; i < 3; ++i)
                recorded_differences += static_cast<float>(inertia.k[i]) != surface->rotation_inertia.k[i];
        for (int i = 0; i < 3; ++i)
        {
            metadata_differences += static_cast<float>(inertia.k[i]) != static_cast<float>(native_inertia.k[i]);
            metadata_differences += static_cast<float>(center.k[i]) != static_cast<float>(native_center.k[i]);
        }
        std::printf("scene inertia PC%d X %.17g / %.17g\n", precision, inertia.k[0], native_inertia.k[0]);
        IVP_U_BigVector<IVP_Compact_Ledge> ledges(128);
        IVP_Compact_Ledge_Solver::get_all_ledges(surface, &ledges);
        for (int sample = 0; sample < 128; ++sample)
        {
            IVP_U_Matrix transform;
            transform.init();
            transform.vv.set(&center);
            transform.vv.k[0] += sample * 0.000001379;
            bool same = true;
            for (int axis = 0; axis < 3; ++axis)
            {
                IVP_Compact_Ledge_Mass_Center_Solver expected(&transform), actual(&transform);
                for (int l = ledges.len() - 1; l >= 0; --l)
                {
                    auto *ledge = ledges.element_at(l);
                    for (int t = 0; t < ledge->get_n_triangles(); ++t)
                    {
                        auto *triangle = ledge->get_first_triangle() + t;
                        at<Integrate>(0x3B550)(&expected, ledge, triangle, axis, (axis + 1) % 3, (axis + 2) % 3);
                        actual.integrate_triangle(ledge, triangle, axis, (axis + 1) % 3, (axis + 2) % 3);
                        if (expected.msum != actual.msum || expected.mzsum != actual.mzsum ||
                            expected.mzzsum != actual.mzzsum)
                        {
                            same = false;
                            if (sample == 0)
                                std::printf(
                                    "scene integral PC%d axis%d triangle%d mass %.17g / %.17g inertia %.17g / %.17g\n",
                                    precision, axis, t, expected.msum, actual.msum, expected.mzzsum, actual.mzzsum);
                            break;
                        }
                    }
                    if (!same)
                        break;
                }
                if (!same)
                    break;
            }
            if (!same)
                ++differences;
        }
    }
    std::printf("scene integral mismatches: %d / 384; recorded inertia mismatches: %d\n", differences,
                recorded_differences);
    std::printf("scene metadata mismatches: %d\n", metadata_differences);
    return differences == 0 && recorded_differences == 0 && metadata_differences == 0;
}

static bool surface_metadata_test()
{
    typedef IVP_Compact_Ledge *(__cdecl * Build)(void *);
    typedef void(__thiscall * Construct)(void *);
    typedef void(__thiscall * Insert)(void *, void *);
    typedef void *(__thiscall * Compile)(void *, void *);
    typedef void(__thiscall * ReadPoint)(const void *, IVP_U_Float_Point *);
    int differences = 0;
    for (int sample = 0; sample < 8; ++sample)
    {
        IVP_U_Point vertices[8];
        IVP_U_Vector<IVP_U_Point> points;
        for (int i = 0; i < 8; ++i)
        {
            double x = (i & 1 ? 1.0 : -1.0) * (1.5 + sample * 0.137),
                   y = (i & 2 ? 1.0 : -1.0) * (1.49 + sample * 0.041),
                   z = (i & 4 ? 1.0 : -1.0) * (1.51 + sample * 0.193);
            vertices[i].set(x + sample * 0.0317, y + x * 0.012345, z + y * 0.009876);
            points.add(&vertices[i]);
        }
        IVP_Compact_Ledge *expected_ledge = at<Build>(0x3AE20)(&points);
        IVP_Compact_Ledge *actual_ledge = IVP_SurfaceBuilder_Pointsoup::convert_pointsoup_to_compact_ledge(&points);
        if (!expected_ledge || !actual_ledge)
            return false;
        if (expected_ledge->get_n_triangles() != actual_ledge->get_n_triangles() ||
            expected_ledge->get_size() != actual_ledge->get_size())
        {
            ++differences;
            std::printf("surface input hull structure differs sample %d\n", sample);
            continue;
        }
        int hull_differences = 0;
        for (int t = 0; t < actual_ledge->get_n_triangles(); ++t)
            for (int e = 0; e < 3; ++e)
            {
                const IVP_Compact_Poly_Point *ep =
                    expected_ledge->get_first_triangle()[t].get_edge(e)->get_start_point(expected_ledge);
                const IVP_Compact_Poly_Point *ap =
                    actual_ledge->get_first_triangle()[t].get_edge(e)->get_start_point(actual_ledge);
                const IVP_Compact_Edge *ee = expected_ledge->get_first_triangle()[t].get_edge(e),
                                       *ae = actual_ledge->get_first_triangle()[t].get_edge(e);
                hull_differences += ee->get_start_point_index() != ae->get_start_point_index() ||
                                    ee->get_opposite_index() != ae->get_opposite_index() ||
                                    ee->get_is_virtual() != ae->get_is_virtual();
                for (int k = 0; k < 3; ++k)
                    hull_differences += ep->k[k] != ap->k[k];
            }
        differences += hull_differences != 0;
        std::printf("surface input hull coordinate/order differences sample %d: %d\n", sample, hull_differences);
        typedef void(__thiscall * Integrate)(void *, const IVP_Compact_Ledge *, const IVP_Compact_Triangle *);
        IVP_Compact_Ledge_Find_Mass_Center original_center, native_center;
        at<Construct>(0x3B530)(&original_center);
        for (int t = 0; t < actual_ledge->get_n_triangles(); ++t)
        {
            const IVP_Compact_Triangle *triangle = actual_ledge->get_first_triangle() + t;
            at<Integrate>(0x3B8E0)(&original_center, actual_ledge, triangle);
            native_center.integrate_triangle(actual_ledge, triangle);
            if (std::memcmp(original_center.sum_mass.k, native_center.sum_mass.k, 12) != 0 ||
                original_center.sum_det != native_center.sum_det ||
                original_center.qsum_surface != native_center.qsum_surface)
            {
                ++differences;
                std::printf(
                    "center kernel first drift sample %d triangle %d mass %.9g %.9g / %.9g %.9g det %.17g / %.17g\n",
                    sample, t, original_center.sum_mass.k[0], original_center.sum_mass.k[1],
                    native_center.sum_mass.k[0], native_center.sum_mass.k[1], original_center.sum_det,
                    native_center.sum_det);
                break;
            }
        }
        unsigned char builder[0x200];
        at<Construct>(0x38290)(builder);
        at<Insert>(0x38430)(builder, expected_ledge);
        void *surface = at<Compile>(0x384B0)(builder, NULL);
        at<Construct>(0x38340)(builder);
        IVP_SurfaceBuilder_Ledge_Soup actual_builder;
        actual_builder.insert_ledge(actual_ledge);
        IVP_Compact_Surface *actual_surface = actual_builder.compile();
        if (!surface || !actual_surface)
            return false;
        unsigned char manager[8];
        write<void *>(manager, 0, retail + 0x631E0);
        write<void *>(manager, 4, surface);
        IVP_SurfaceManager_Polygon actual_manager(actual_surface);
        IVP_U_Float_Point expected_mass, expected_inertia, actual_mass, actual_inertia;
        at<ReadPoint>(0xBC00)(manager, &expected_mass);
        at<ReadPoint>(0xBC20)(manager, &expected_inertia);
        actual_manager.get_mass_center(&actual_mass);
        actual_manager.get_rotation_inertia(&actual_inertia);
        for (int k = 0; k < 3; ++k)
            if (expected_mass.k[k] != actual_mass.k[k] || expected_inertia.k[k] != actual_inertia.k[k])
            {
                ++differences;
                std::printf("surface metadata %d axis %d inertia %.9g / %.9g mass center %.9g / %.9g\n", sample, k,
                            expected_inertia.k[k], actual_inertia.k[k], expected_mass.k[k], actual_mass.k[k]);
            }
    }
    std::printf("surface metadata mismatches: %d\n", differences);
    return differences == 0;
}

#define RETAIL_REGISTER_geometry RETAIL_REGISTER
#include "RetailTestRegistration.inc"
