#include "RetailTestSupport.h"

#include <ivp_physics.hxx>
#include <ivp_surbuild_pointsoup.hxx>
#include <ivp_mindist.hxx>
#include <ivp_mindist_minimize.hxx>
#include <ivp_cache_ledge_point.hxx>
#include <ivp_compact_ledge_solver.hxx>
#include <ivp_friction.hxx>
#include <ivp_solver_core_reaction.hxx>
#include <ivp_calc_next_psi_solver.hxx>
#include <ivu_memory.hxx>
#if IVP_ENABLE_X87_EMULATION
#include <ivu_x87.hxx>
#endif

#include <cmath>
#include <new>

#include "fixtures/contact_center_projection.hxx"
#include "fixtures/early_box_kk.hxx"
#include "fixtures/kk_contact_10362.hxx"
#include "fixtures/kk_contact_12824.hxx"
#include "fixtures/kk_contact_9967.hxx"
#include "fixtures/level2_kk_14394.hxx"
#include "fixtures/small_body_contact_mass.hxx"
#include "fixtures/stacked_pf_12806.hxx"

static bool triangle_topology_test()
{
    typedef IVP_Compact_Ledge *(__cdecl * Build)(IVP_U_Vector<IVP_U_Point> *);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 64; ++sample)
        {
            IVP_U_Point p[3];
            IVP_U_Vector<IVP_U_Point> points;
            p[0].set(-2.5, 0.000013579, 2.5);
            p[1].set(2.5, 0.00008642, -2.5);
            p[2].set(8.9731 + sample * 0.0317, 0.000097531, 0.1234567 + sample * 0.019753);
            for (int k = 0; k < 3; ++k)
                points.add(&p[k]);
            IVP_Compact_Ledge *expected = at<Build>(0x3AE20)(&points),
                              *actual = IVP_SurfaceBuilder_Pointsoup::convert_pointsoup_to_compact_ledge(&points);
            bool same = expected && actual && expected->get_size() == actual->get_size() &&
                        expected->get_n_triangles() == actual->get_n_triangles();
            if (same)
                for (int t = 0; t < actual->get_n_triangles(); ++t)
                    for (int e = 0; e < 3; ++e)
                    {
                        const auto *ee = expected->get_first_triangle()[t].get_edge(e),
                                   *ae = actual->get_first_triangle()[t].get_edge(e);
                        same = same && ee->get_start_point_index() == ae->get_start_point_index() &&
                               ee->get_opposite_index() == ae->get_opposite_index() &&
                               ee->get_is_virtual() == ae->get_is_virtual();
                        const auto *ep = ee->get_start_point(expected), *ap = ae->get_start_point(actual);
                        for (int k = 0; k < 3; ++k)
                            same = same && ep->k[k] == ap->k[k];
                    }
            if (!same)
            {
                ++differences;
                if (differences <= 4)
                    std::printf("triangle topology PC%d case %d size %d / %d\n", precision, sample,
                                expected ? expected->get_size() : 0, actual ? actual->get_size() : 0);
            }
            if (expected)
                at<void(__cdecl *)(void *)>(0x20130)(expected);
            if (actual)
                ivp_free_aligned(actual);
        }
    }
    std::printf("triangle topology mismatches: %d / 192\n", differences);
    return differences == 0;
}

class RetailMinimizeFixture : public IVP_Mindist_Minimize_Solver
{
  public:
    explicit RetailMinimizeFixture(IVP_Mindist *md) : IVP_Mindist_Minimize_Solver(md) {}
    using IVP_Mindist_Minimize_Solver::calc_contact_center_projection;
    using IVP_Mindist_Minimize_Solver::p_minimize_KK;
};

static bool early_box_kk_test()
{
    typedef IVP_MRC_TYPE(__thiscall * Minimize)(void *, const IVP_Compact_Edge *, const IVP_Compact_Edge *,
                                                IVP_Cache_Ledge_Point *, IVP_Cache_Ledge_Point *);
    unsigned int ignored;
    _controlfp_s(&ignored, _PC_64, _MCW_PC);
    const unsigned char *ledges[2] = {early_kk_ledge_0, early_kk_ledge_1};
    const double *matrices[2] = {early_kk_matrix_0, early_kk_matrix_1};
    alignas(16) unsigned char old_mindist[144] = {}, old_solver[2112] = {};
    alignas(16) unsigned char native_mindist[sizeof(IVP_Mindist)] = {};
    auto *md = reinterpret_cast<IVP_Mindist *>(native_mindist);
    write(old_solver, 0, old_mindist);
    write(old_solver, 4, 20);
    IVP_Cache_Object objects[2]{};
    IVP_Cache_Ledge_Point old_cache[2], native_cache[2];
    const IVP_Compact_Edge *edges[2]{};
    for (int i = 0; i < 2; ++i)
    {
        std::memcpy(&objects[i].m_world_f_object, matrices[i], 128);
        objects[i].core_pos.set(objects[i].m_world_f_object.get_position());
        auto *ledge = reinterpret_cast<const IVP_Compact_Ledge *>(ledges[i]);
        edges[i] = reinterpret_cast<const IVP_Compact_Edge *>(ledges[i] + early_kk_edges[i]);
        auto *synapse = old_mindist + 24 + i * 28;
        write(synapse, 20, edges[i]);
        write<short>(synapse, 26, 1);
        md->synapse[i].update_synapse(edges[i], IVP_ST_EDGE);
        for (int j = 0; j < 2; ++j)
        {
            auto *cache = j ? &native_cache[i] : &old_cache[i];
            write(cache, 0, ledge->get_point_array());
            write(cache, 4, ledge);
            cache->clp_cache_object = &objects[i];
            cache->clp_object = NULL;
            cache->tmp.synapse = j ? &md->synapse[i] : reinterpret_cast<IVP_Synapse_Real *>(synapse);
        }
    }
    int transform_differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int i = 0; i < 2; ++i)
            for (int t = 0; t < reinterpret_cast<const IVP_Compact_Ledge *>(ledges[i])->get_n_triangles(); ++t)
                for (int e = 0; e < 3; ++e)
                {
                    const auto *edge =
                        reinterpret_cast<const IVP_Compact_Ledge *>(ledges[i])->get_first_triangle()[t].get_edge(e);
                    IVP_U_Point expected_point, actual_point, expected_transform, actual_transform;
                    at<void(__cdecl *)(const IVP_Compact_Edge *, IVP_Cache_Ledge_Point *, IVP_Cache_Ledge_Point *,
                                       IVP_U_Point *)>(0x20710)(edge, old_cache + i, old_cache + 1 - i,
                                                                &expected_point);
                    IVP_CLS.calc_pos_other_space(edge, native_cache + i, native_cache + 1 - i, &actual_point);
                    const auto *point = edge->get_start_point(reinterpret_cast<const IVP_Compact_Ledge *>(ledges[i]));
                    at<void(__cdecl *)(const IVP_U_Float_Point *, IVP_Cache_Ledge_Point *, IVP_Cache_Ledge_Point *,
                                       IVP_U_Point *)>(0x208F0)(point, old_cache + i, old_cache + 1 - i,
                                                                &expected_transform);
                    IVP_CLS.transform_pos_other_space(point, native_cache + i, native_cache + 1 - i, &actual_transform);
                    for (int k = 0; k < 3; ++k)
                        transform_differences += (expected_point.k[k] != actual_point.k[k]) +
                                                 (expected_transform.k[k] != actual_transform.k[k]);
                }
    }
    std::printf("early KK point-transform component mismatches: %d / 756\n", transform_differences);
    _controlfp_s(&ignored, _PC_64, _MCW_PC);
    const auto expected = at<Minimize>(0x31440)(old_solver, edges[0], edges[1], old_cache, old_cache + 1);
    RetailMinimizeFixture solver(md);
    const auto actual = solver.p_minimize_KK(edges[0], edges[1], native_cache, native_cache + 1);
    bool same = expected == actual && transform_differences == 0;
    for (int i = 0; i < 2; ++i)
    {
        const auto *synapse = old_mindist + 24 + i * 28;
        const auto *edge = read<const IVP_Compact_Edge *>(synapse, 20);
        const int status = read<short>(synapse, 26);
        const auto *native_edge = md->synapse[i].get_edge();
        same = same && status == md->synapse[i].get_status() && edge == native_edge;
        std::printf("early KK feature %d status %d / %d edge %d / %d\n", i, status, md->synapse[i].get_status(),
                    int(reinterpret_cast<const unsigned char *>(edge) - ledges[i]),
                    int(reinterpret_cast<const unsigned char *>(native_edge) - ledges[i]));
    }
    std::printf("early KK result %d / %d\n", expected, actual);
    return same;
}

static bool level2_kk_case(int precision, bool compare_metadata = false)
{
    typedef IVP_MRC_TYPE(__thiscall * Minimize)(void *, const IVP_Compact_Edge *, const IVP_Compact_Edge *,
                                                IVP_Cache_Ledge_Point *, IVP_Cache_Ledge_Point *);
    unsigned int ignored;
    _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
    const unsigned char *ledges[2] = {l2_kk_ledge_0, l2_kk_ledge_1};
    const double *matrices[2] = {l2_kk_matrix_0, l2_kk_matrix_1};
    alignas(16) unsigned char old_mindist[144] = {}, old_solver[2112] = {};
    alignas(16) unsigned char native_mindist[sizeof(IVP_Mindist)] = {};
    auto *md = reinterpret_cast<IVP_Mindist *>(native_mindist);
    write(old_solver, 0, old_mindist);
    write(old_solver, 4, 20);
    IVP_Cache_Object objects[2]{};
    IVP_Cache_Ledge_Point old_cache[2], native_cache[2];
    const IVP_Compact_Edge *edges[2]{};
    for (int i = 0; i < 2; ++i)
    {
        std::memcpy(&objects[i].m_world_f_object, matrices[i], 128);
        objects[i].core_pos.set(objects[i].m_world_f_object.get_position());
        auto *ledge = reinterpret_cast<const IVP_Compact_Ledge *>(ledges[i]);
        edges[i] = reinterpret_cast<const IVP_Compact_Edge *>(ledges[i] + l2_kk_edges[i]);
        auto *synapse = old_mindist + 24 + i * 28;
        write(synapse, 20, edges[i]);
        write<short>(synapse, 26, 1);
        md->synapse[i].update_synapse(edges[i], IVP_ST_EDGE);
        for (int j = 0; j < 2; ++j)
        {
            auto *cache = j ? &native_cache[i] : &old_cache[i];
            write(cache, 0, ledge->get_point_array());
            write(cache, 4, ledge);
            cache->clp_cache_object = &objects[i];
            cache->clp_object = NULL;
            cache->tmp.synapse = j ? &md->synapse[i] : reinterpret_cast<IVP_Synapse_Real *>(synapse);
        }
    }
    int transform_differences = 0;
    {
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int i = 0; i < 2; ++i)
            for (int t = 0; t < reinterpret_cast<const IVP_Compact_Ledge *>(ledges[i])->get_n_triangles(); ++t)
                for (int e = 0; e < 3; ++e)
                {
                    const auto *edge =
                        reinterpret_cast<const IVP_Compact_Ledge *>(ledges[i])->get_first_triangle()[t].get_edge(e);
                    IVP_U_Point expected_point, actual_point, expected_transform, actual_transform;
                    at<void(__cdecl *)(const IVP_Compact_Edge *, IVP_Cache_Ledge_Point *, IVP_Cache_Ledge_Point *,
                                       IVP_U_Point *)>(0x20710)(edge, old_cache + i, old_cache + 1 - i,
                                                                &expected_point);
                    IVP_CLS.calc_pos_other_space(edge, native_cache + i, native_cache + 1 - i, &actual_point);
                    const auto *point = edge->get_start_point(reinterpret_cast<const IVP_Compact_Ledge *>(ledges[i]));
                    at<void(__cdecl *)(const IVP_U_Float_Point *, IVP_Cache_Ledge_Point *, IVP_Cache_Ledge_Point *,
                                       IVP_U_Point *)>(0x208F0)(point, old_cache + i, old_cache + 1 - i,
                                                                &expected_transform);
                    IVP_CLS.transform_pos_other_space(point, native_cache + i, native_cache + 1 - i, &actual_transform);
                    for (int k = 0; k < 3; ++k)
                        transform_differences += (expected_point.k[k] != actual_point.k[k]) +
                                                 (expected_transform.k[k] != actual_transform.k[k]);
                }
    }
    std::printf("level2 KK point-transform component mismatches: %d / 5616\n", transform_differences);
    _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
    const auto expected = at<Minimize>(0x31440)(old_solver, edges[0], edges[1], old_cache, old_cache + 1);
    RetailMinimizeFixture solver(md);
    const auto actual = solver.p_minimize_KK(edges[0], edges[1], native_cache, native_cache + 1);
    bool same = expected == actual && transform_differences == 0;
    if (compare_metadata)
    {
        const float original_values[5] = {read<float>(old_mindist, 0x54), read<float>(old_mindist, 0x58),
                                          read<float>(old_mindist, 0x68), read<float>(old_mindist, 0x6C),
                                          read<float>(old_mindist, 0x70)};
        const float native_values[5] = {md->len_numerator, md->contact_dot_diff_center, md->contact_plane.k[0],
                                        md->contact_plane.k[1], md->contact_plane.k[2]};
        for (int k = 0; k < 5; ++k)
            if (original_values[k] != native_values[k])
            {
                same = false;
                std::printf("level2 contact PC%d field%d %.9g / %.9g\n", precision, k, original_values[k],
                            native_values[k]);
            }
    }

    for (int i = 0; i < 2; ++i)
    {
        const auto *synapse = old_mindist + 24 + i * 28;
        const auto *edge = read<const IVP_Compact_Edge *>(synapse, 20);
        const int status = read<short>(synapse, 26);
        const auto *native_edge = md->synapse[i].get_edge();
        same = same && status == md->synapse[i].get_status() && edge == native_edge;
        std::printf("level2 KK feature %d status %d / %d edge %d / %d\n", i, status, md->synapse[i].get_status(),
                    int(reinterpret_cast<const unsigned char *>(edge) - ledges[i]),
                    int(reinterpret_cast<const unsigned char *>(native_edge) - ledges[i]));
    }
    std::printf("level2 KK result %d / %d\n", expected, actual);
    return same;
}

static bool level2_kk_test()
{
    bool same = true;
    for (int precision = 0; precision < 3; ++precision)
    {
        std::printf("level2 KK PC%d\n", precision);
        same = level2_kk_case(precision) && same;
    }
    return same;
}

static bool level2_contact_metadata_test()
{
    bool same = true;
    for (int precision = 0; precision < 3; ++precision)
        same = level2_kk_case(precision, true) && same;
    return same;
}

static bool kk_contact_geometry_test(unsigned int fixture = 0)
{
    typedef void(__thiscall * Calculate)(void *, const IVP_Compact_Edge *, const IVP_Compact_Edge *,
                                         IVP_Cache_Ledge_Point *, IVP_Cache_Ledge_Point *,
                                         IVP_Impact_Solver_Long_Term *, IVP_U_Float_Point *);
    auto query = &PhysicsRT_DebugKKContactAddress;
    const auto calculate = reinterpret_cast<Calculate>(query());
    const double *matrix_sets[] = {retail_kk_contact_matrices, retail_kk_contact_l6_matrices,
                                   retail_kk_contact_stacked_matrices};
    const float *point_sets[] = {retail_kk_contact_points, retail_kk_contact_l6_points,
                                 retail_kk_contact_stacked_points};
    const float *normals[] = {retail_kk_contact_normal, retail_kk_contact_l6_normal, retail_kk_contact_stacked_normal};
    const double *matrices = matrix_sets[fixture];
    const float *points = point_sets[fixture];
    const float *normal = normals[fixture];
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
        {
            alignas(16) unsigned char ledges[2][80] = {}, old_contact[0x78] = {},
                                      native_contact[sizeof(IVP_Contact_Point)] = {};
            IVP_Cache_Object objects[2]{};
            IVP_Cache_Ledge_Point caches[2];
            const IVP_Compact_Edge *edges[2];
            for (int j = 0; j < 2; ++j)
            {
                write<int>(ledges[j], 0, 32);
                write<unsigned short>(ledges[j], 12, 1);
                write<unsigned int>(ledges[j], 20, 0);
                write<unsigned int>(ledges[j], 24, 1);
                write<unsigned int>(ledges[j], 28, 2);
                std::memcpy(ledges[j] + 32, points + j * 6, 12);
                std::memcpy(ledges[j] + 48, points + j * 6 + 3, 12);
                std::memcpy(&objects[j].m_world_f_object, matrices + j * 16, 128);
                if (j == 1)
                    objects[j].m_world_f_object.vv.k[1] += sample * 0.0000001379;
                auto *ledge = reinterpret_cast<const IVP_Compact_Ledge *>(ledges[j]);
                edges[j] = reinterpret_cast<const IVP_Compact_Edge *>(ledges[j] + 20);
                write<const IVP_Compact_Poly_Point *>(&caches[j], 0, ledge->get_point_array());
                write(&caches[j], 4, ledge);
                caches[j].clp_cache_object = &objects[j];
                caches[j].clp_object = NULL;
            }
            IVP_Impact_Solver_Long_Term expected = {}, actual = {};
            IVP_U_Float_Point old_diff, native_diff;
            old_diff.set_to_zero();
            native_diff.set_to_zero();
            at<Calculate>(0x1F3B0)(old_contact, edges[0], edges[1], caches, caches + 1, &expected, &old_diff);
            calculate(native_contact, edges[0], edges[1], caches, caches + 1, &actual, &native_diff);
            if (sample == 0 && precision == 2 && std::memcmp(expected.surf_normal.k, normal, 12) != 0)
            {
                std::printf("recorded KK normal reproduction %.9g %.9g %.9g\n", expected.surf_normal.k[0],
                            expected.surf_normal.k[1], expected.surf_normal.k[2]);

                return false;
            }
            const bool same = std::memcmp(expected.surf_normal.k, actual.surf_normal.k, 12) == 0 &&
                              std::memcmp(expected.contact_point_ws.k, actual.contact_point_ws.k, 24) == 0 &&
                              std::memcmp(expected.span_friction_v[0].k, actual.span_friction_v[0].k, 12) == 0 &&
                              read<float>(old_contact, 0x58) == read<float>(native_contact, 0x58);
            if (!same)
            {
                ++differences;
                if (differences <= 4 || (sample == 0 && precision == 2))
                    std::printf("KK contact PC%d sample%d normalZ %.9g / %.9g pointZ %.17g / %.17g\n", precision,
                                sample, expected.surf_normal.k[2], actual.surf_normal.k[2],
                                expected.contact_point_ws.k[2], actual.contact_point_ws.k[2]);
            }
        }
    }

    std::printf("KK contact geometry mismatches %d / 384\n", differences);
    return differences == 0;
}

static bool stacked_face_minimum_test()
{
    using Minimize = IVP_MRC_TYPE(__thiscall *)(void *, const IVP_Compact_Edge *, const IVP_Compact_Edge *,
                                                IVP_Cache_Ledge_Point *, IVP_Cache_Ledge_Point *);
    auto query = &PhysicsRT_DebugMinimizeAddress;
    unsigned int ignored;
    _controlfp_s(&ignored, _PC_64, _MCW_PC);
    const unsigned char *ledges[2] = {stacked_pf_ledge_0, stacked_pf_ledge_1};
    const double *matrices[2] = {stacked_pf_matrix_0, stacked_pf_matrix_1};
    alignas(16) unsigned char old_mindist[144] = {}, old_solver[2112] = {}, native_mindist[sizeof(IVP_Mindist)] = {};
    auto *md = reinterpret_cast<IVP_Mindist *>(native_mindist);
    RetailMinimizeFixture solver(md);
    write(old_solver, 0, old_mindist);
    write(old_solver, 4, 20);
    IVP_Cache_Object objects[2]{};
    IVP_Cache_Ledge_Point old_cache[2], native_cache[2];
    const IVP_Compact_Edge *edges[2];
    for (int i = 0; i < 2; ++i)
    {
        std::memcpy(&objects[i].m_world_f_object, matrices[i], 128);
        objects[i].core_pos.set(objects[i].m_world_f_object.get_position());
        const auto *ledge = reinterpret_cast<const IVP_Compact_Ledge *>(ledges[i]);
        edges[i] = reinterpret_cast<const IVP_Compact_Edge *>(ledges[i] + stacked_pf_edges[i]);
        auto *synapse = old_mindist + 24 + i * 28;
        write(synapse, 20, edges[i]);
        write<short>(synapse, 26, 0);
        md->synapse[i].update_synapse(edges[i], IVP_ST_POINT);
        for (int j = 0; j < 2; ++j)
        {
            auto *cache = j ? &native_cache[i] : &old_cache[i];
            write(cache, 0, ledge->get_point_array());
            write(cache, 4, ledge);
            cache->clp_cache_object = &objects[i];
            cache->clp_object = NULL;
            cache->tmp.synapse = j ? &md->synapse[i] : reinterpret_cast<IVP_Synapse_Real *>(synapse);
        }
    }
    IVP_U_Point old_normal, native_normal, old_world, native_world, old_local, native_local;
    at<void(__cdecl *)(const IVP_Compact_Edge *, const IVP_Compact_Ledge *, IVP_U_Point *)>(0x21280)(
        edges[1], reinterpret_cast<const IVP_Compact_Ledge *>(ledges[1]), &old_normal);
    IVP_CLS.calc_hesse_vec_object_not_normized(edges[1], reinterpret_cast<const IVP_Compact_Ledge *>(ledges[1]),
                                               &native_normal);
    at<IVP_RETURN_TYPE(__thiscall *)(IVP_U_Point *)>(0xDD50)(&old_normal);
    native_normal.fast_normize();
    using Transform = void(__thiscall *)(const IVP_Cache_Object *, const IVP_U_Point *, IVP_U_Point *);
    at<Transform>(0x18EA0)(objects + 1, &old_normal, &old_world);
    objects[1].transform_vector_to_world_coords(&native_normal, &native_world);
    at<Transform>(0x18DC0)(objects, &old_world, &old_local);
    objects[0].transform_vector_to_object_coords(&native_world, &native_local);
    const bool transforms_same = std::memcmp(old_normal.k, native_normal.k, 24) == 0 &&
                                 std::memcmp(old_world.k, native_world.k, 24) == 0 &&
                                 std::memcmp(old_local.k, native_local.k, 24) == 0;
    if (!transforms_same)
        for (int k = 0; k < 3; ++k)
            std::printf("stacked normal%d %.17g / %.17g world %.17g / %.17g local %.17g / %.17g\n", k, old_normal.k[k],
                        native_normal.k[k], old_world.k[k], native_world.k[k], old_local.k[k], native_local.k[k]);
    const auto expected = at<Minimize>(0x30C60)(old_solver, edges[0], edges[1], old_cache, old_cache + 1);
    const auto actual =
        reinterpret_cast<Minimize>(query(2))(&solver, edges[0], edges[1], native_cache, native_cache + 1);
    bool same = expected == actual && transforms_same;
    for (int i = 0; i < 2; ++i)
    {
        const auto *synapse = old_mindist + 24 + i * 28;
        const auto *edge = read<const IVP_Compact_Edge *>(synapse, 20);
        const auto *native_edge = md->synapse[i].get_edge();
        const int status = read<short>(synapse, 26);
        same = same && edge == native_edge && status == md->synapse[i].get_status();
        std::printf("stacked face feature%d edge %d / %d status %d / %d\n", i,
                    int(reinterpret_cast<const unsigned char *>(edge) - ledges[i]),
                    int(reinterpret_cast<const unsigned char *>(native_edge) - ledges[i]), status,
                    md->synapse[i].get_status());
    }

    return same;
}

static bool cache_world_position_test()
{
    using Transform = void(__thiscall *)(const IVP_Cache_Object *, const IVP_U_Float_Point *, IVP_U_Point *);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int sample = 0; sample < 128; ++sample)
            for (int endpoint = 0; endpoint < 4; ++endpoint)
            {
                IVP_Cache_Object object{};
                std::memcpy(&object.m_world_f_object, retail_kk_contact_l6_matrices + (endpoint / 2) * 16, 128);
                object.m_world_f_object.vv.k[1] += sample * 0.0000001379;
                IVP_U_Float_Point local;
                std::memcpy(local.k, retail_kk_contact_l6_points + endpoint * 3, 12);
                IVP_U_Point expected, actual;
                at<Transform>(0x18D10)(&object, &local, &expected);
                object.transform_position_to_world_coords(&local, &actual);
                if (std::memcmp(expected.k, actual.k, 24) != 0)
                {
                    ++differences;
                    if (differences <= 4)
                        std::printf("cache world PC%d sample%d endpoint%d XYZ %.17g %.17g %.17g / %.17g %.17g %.17g\n",
                                    precision, sample, endpoint, expected.k[0], expected.k[1], expected.k[2],
                                    actual.k[0], actual.k[1], actual.k[2]);
                }
            }
    }
    std::printf("cache world position mismatches %d / 1536\n", differences);
    return differences == 0;
}

static bool virtual_mass_kernel_test()
{
    typedef double(__thiscall * Mass)(const void *, const IVP_U_Float_Point *);
    __declspec(align(16)) unsigned char original[0x50] = {}, native[sizeof(IVP_Core)] = {};
    IVP_Core *core = reinterpret_cast<IVP_Core *>(native);
    const float masses[4] = {0.32894736528396606f, 0.32894736528396606f, 0.32894736528396606f, 0.5263158082962036f};
    std::memcpy(original + 0x34, masses, 16);
    std::memcpy(const_cast<IVP_U_Point_4 *>(core->get_inv_masses()), masses, 16);
    int differences = 0;
    for (int precision = 0; precision < 3; ++precision)
    {
        unsigned int ignored;
        _controlfp_s(&ignored, precision == 2 ? _PC_64 : precision == 1 ? _PC_53 : _PC_24, _MCW_PC);
        for (int i = 0; i < 64; ++i)
        {
            IVP_U_Float_Point point;
            if (i == 0)
                point.set(0.6402314901351929f, -1.8947569131851196f, -5.675428838003427e-7f);
            else
                point.set((i - 31) * 0.04123456f, (i % 17 - 8) * 0.2534567f, (i % 5 - 2) * 0.1376543f);
            const volatile double expected = at<Mass>(0xC200)(original, &point);
            const volatile double actual = core->calc_virt_mass_worst_case(&point);
            if (expected != actual)
            {
                if (differences < 8)
                    std::printf("virtual mass PC%d case %d %.17g / %.17g\n", precision, i, expected, actual);
                ++differences;
            }
        }
    }
    std::printf("virtual mass kernel mismatches: %d / 192\n", differences);
    return differences == 0;
}

static bool contact_inverse_mass_test()
{
    unsigned int ignored;
    _controlfp_s(&ignored, _PC_64, _MCW_PC);
    int differences = 0;
    for (const auto &row : retail_small_body_contact_mass)
    {
        IVP_U_Float_Point cross;
        cross.set(row[0], row[1], row[2]);
        const float actual = ivp_contact_inverse_mass(&cross, row + 3);
        if (actual != row[7])
        {
            ++differences;
            if (differences <= 4)
                std::printf("contact inverse mass %.9g / %.9g\n", row[7], actual);
        }
    }
    std::printf("recorded contact inverse mass mismatches: %d / %u\n", differences,
                unsigned(sizeof(retail_small_body_contact_mass) / sizeof(retail_small_body_contact_mass[0])));
    return differences == 0;
}

static bool contact_center_projection_test()
{
    unsigned int ignored;
    _controlfp_s(&ignored, _PC_64, _MCW_PC);
    int differences = 0;
    for (const auto &row : retail_center_projection)
    {
        IVP_U_Point a, b;
        a.set(row.a[0], row.a[1], row.a[2]);
        b.set(row.b[0], row.b[1], row.b[2]);
        IVP_U_Float_Point normal;
        normal.set(row.normal[0], row.normal[1], row.normal[2]);
        float actual = RetailMinimizeFixture::calc_contact_center_projection(&a, &b, &normal);
        if (actual != row.expected)
        {
            ++differences;
            if (differences <= 4)
                std::printf("contact center projection %.9g / %.9g\n", row.expected, actual);
        }
    }
    std::printf("contact center projection mismatches: %d / %u captured PC64 cases\n", differences,
                unsigned(sizeof(retail_center_projection) / sizeof(retail_center_projection[0])));
    return differences == 0;
}

#define RETAIL_REGISTER_contact RETAIL_REGISTER
#include "RetailTestRegistration.inc"
