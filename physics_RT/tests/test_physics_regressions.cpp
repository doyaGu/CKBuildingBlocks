#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#if defined(_WIN32)
#include <windows.h>
#endif

#include "CKContext.h"
#include "CKGlobals.h"
#include "CKIpionManager.h"
#include "Convert.h"
#include "RCK3dObject.h"
#include "RCKMesh.h"
#include "VxQuaternion.h"
#include "ivp_material.hxx"

void InitializePhysicsHostTests();
int RunPhysicsHostTests(int &total);

static void Check(bool condition, const char *message)
{
    if (!condition)
        throw std::runtime_error(message);
}

static bool NearlyEqual(float lhs, float rhs, float epsilon = 0.0001f)
{
    return std::fabs(lhs - rhs) <= epsilon;
}

static bool NearlyEqual(const VxVector &lhs, const VxVector &rhs, float epsilon = 0.0001f)
{
    return NearlyEqual(lhs.x, rhs.x, epsilon) &&
           NearlyEqual(lhs.y, rhs.y, epsilon) &&
           NearlyEqual(lhs.z, rhs.z, epsilon);
}

static std::string ReadSource(const char *relativePath)
{
    std::ifstream file(std::string(CKBB_SOURCE_DIR) + "/" + relativePath, std::ios::binary);
    Check(static_cast<bool>(file), "Unable to read physics source file");
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

static void FillTriangle(RCKMesh &mesh)
{
    Check(mesh.SetVertexCount(3) == TRUE, "Unable to allocate collision mesh vertices");

    VxVector positions[3] = {
        VxVector(-1.0f, 0.0f, -1.0f),
        VxVector(1.0f, 0.0f, -1.0f),
        VxVector(0.0f, 0.0f, 1.0f),
    };
    for (int i = 0; i < 3; ++i)
        mesh.SetVertexPosition(i, &positions[i]);
}

static CK3dEntity *AsEntity(RCK3dObject &object)
{
    return reinterpret_cast<CK3dEntity *>(&object);
}

struct PhysicsFixture
{
    PhysicsFixture()
        : context(new CKContext(NULL, 0, 0)), manager(new CKIpionManager(context))
    {
        manager->CreateEnvironment();
    }

    ~PhysicsFixture() { delete context; }

    CKContext *context;
    CKIpionManager *manager;
};

static PhysicsFixture &GetPhysicsFixture()
{
    // CKContext owns registered managers; destroy it after all local entities.
    static PhysicsFixture fixture;
    return fixture;
}

static void DeletePhysicsObject(CKIpionManager &manager, CK3dEntity &entity)
{
    PhysicsObject *object = manager.GetPhysicsObject(&entity);
    if (object && object->m_RealObject)
        object->m_RealObject->delete_silently();
}

static int CreateNamedPolygon(CKIpionManager &manager, CK3dEntity &entity,
                       CKMesh **convexes, int convexCount,
                       const char *surfaceName, IVP_Material *material)
{
    return manager.CreatePhysicsObjectOnParameters(
        &entity,
        convexCount,
        convexes,
        0,
        NULL,
        NULL,
        0,
        NULL,
        1.0f,
        const_cast<char *>(surfaceName),
        NULL,
        TRUE,
        material,
        1.0f,
        const_cast<char *>("RegTest"),
        TRUE,
        TRUE,
        TRUE,
        0.1f,
        0.1f);
}

static void NamedSurfaceReuseDoesNotNeedGeometry()
{
    PhysicsFixture &fixture = GetPhysicsFixture();
    CKIpionManager *manager = fixture.manager;

    RCK3dObject entity(fixture.context, "PaperBall");
    RCKMesh mesh(fixture.context, "Ball_Paper_Mesh");
    FillTriangle(mesh);
    entity.SetCurrentMesh(&mesh);

    CKMesh *convexes[] = {&mesh};
    IVP_Material_Simple firstMaterial(0.4f, 0.5f);
    Check(CreateNamedPolygon(*manager, *AsEntity(entity), convexes, 1,
                             "Ball_Paper_Mesh", &firstMaterial) == CK_OK,
          "Initial named collision surface creation failed");
    Check(manager->GetCollisionSurface("Ball_Paper_Mesh") != NULL,
          "Named collision surface was not cached");
    DeletePhysicsObject(*manager, *AsEntity(entity));

    VxVector changedScale(1.5f, 1.5f, 1.5f);
    entity.SetScale(&changedScale, FALSE, FALSE);

    IVP_Material_Simple respawnMaterial(0.4f, 0.5f);
    Check(CreateNamedPolygon(*manager, *AsEntity(entity), NULL, 0,
                             "Ball_Paper_Mesh", &respawnMaterial) == CK_OK,
          "A cached named collision surface must be reusable without geometry inputs");
    const PhysicsObject *respawned = manager->GetPhysicsObject(AsEntity(entity));
    Check(respawned && respawned->m_RealObject, "Respawned physics object is missing");
    CKIpionManager::UpdateObjectWorldMatrix(respawned->m_RealObject);
    VxVector respawnScale;
    entity.GetScale(&respawnScale, FALSE);
    Check(NearlyEqual(respawnScale, changedScale),
          "Rephysicalization must capture the new world scale even when reusing a collision surface");
    DeletePhysicsObject(*manager, *AsEntity(entity));
}

static void UnknownNamedSurfaceDoesNotGuessCurrentMesh()
{
    PhysicsFixture &fixture = GetPhysicsFixture();
    CKIpionManager *manager = fixture.manager;

    RCK3dObject entity(fixture.context, "PaperBall");
    RCKMesh mesh(fixture.context, "MissingSurface");
    FillTriangle(mesh);
    entity.SetCurrentMesh(&mesh);

    IVP_Material_Simple material(0.4f, 0.5f);
    Check(CreateNamedPolygon(*manager, *AsEntity(entity), NULL, 0,
                             "MissingSurface", &material) == CKERR_INVALIDPARAMETER,
          "A cache miss without explicit geometry must fail instead of guessing the current mesh");
}

static void PhysicsWorldMatrixUpdatesKeepWorldScaleStable()
{
    PhysicsFixture &fixture = GetPhysicsFixture();
    CKIpionManager *manager = fixture.manager;

    RCK3dObject parent(fixture.context, "Parent");
    RCK3dObject entity(fixture.context, "Child");

    VxVector origin(0.0f, 0.0f, 0.0f);
    VxVector parentScale(1.75f, 0.6f, 1.2f);
    VxQuaternion parentRotation;
    parentRotation.FromEulerAngles(0.2f, 0.4f, -0.3f);
    Check(parent.ConstructWorldMatrix(&origin, &parentScale, &parentRotation) == TRUE,
          "Unable to construct parent transform");

    Check(entity.SetParent(AsEntity(parent), FALSE) == TRUE, "Unable to parent physics entity");
    VxVector localScale(0.8f, 1.1f, 1.35f);
    VxQuaternion localRotation;
    localRotation.FromEulerAngles(-0.15f, 0.3f, 0.25f);
    Check(entity.ConstructLocalMatrix(&origin, &localScale, &localRotation) == TRUE,
          "Unable to construct child transform");

    VxVector initialWorldScale;
    entity.GetScale(&initialWorldScale, FALSE);

    IVP_Material_Simple material(0.4f, 0.5f);
    Check(manager->CreatePhysicsObjectOnParameters(
              AsEntity(entity), 0, NULL, 0, NULL, NULL, 0, NULL, 1.0f, NULL, NULL,
              TRUE, &material, 1.0f, const_cast<char *>("RegTest"),
              TRUE, FALSE, TRUE, 0.1f, 0.1f) == CK_OK,
          "Unable to create physics object for transform test");

    PhysicsObject *physicsObject = manager->GetPhysicsObject(AsEntity(entity));
    Check(physicsObject && physicsObject->m_RealObject,
          "Transform test physics object is missing");
    IVP_U_Point beamPosition(0.0, 0.0, 0.0);
    IVP_U_Quat initialRotation;
    initialRotation.set(0.2, 0.4, -0.3);
    physicsObject->m_RealObject->beam_object_to_new_position(&initialRotation, &beamPosition);
    CKIpionManager::UpdateObjectWorldMatrix(physicsObject->m_RealObject);
    const VxMatrix firstWorldMatrix = entity.GetWorldMatrix();
    for (int i = 0; i < 1000; ++i)
        CKIpionManager::UpdateObjectWorldMatrix(physicsObject->m_RealObject);

    const VxMatrix finalWorldMatrix = entity.GetWorldMatrix();
    Check(std::memcmp(&firstWorldMatrix, &finalWorldMatrix, sizeof(VxMatrix)) == 0,
          "Repeated synchronization of a fixed physics pose must not accumulate scale error");

    for (int i = 0; i < 5000; ++i)
    {
        VxQuaternion rotation;
        rotation.FromEulerAngles(float(i) * 0.017f, float(i) * 0.013f, float(i) * -0.019f);
        IVP_U_Quat beamRotation;
        beamRotation.x = rotation.x;
        beamRotation.y = rotation.y;
        beamRotation.z = rotation.z;
        beamRotation.w = rotation.w;
        physicsObject->m_RealObject->beam_object_to_new_position(&beamRotation, &beamPosition);
        CKIpionManager::UpdateObjectWorldMatrix(physicsObject->m_RealObject);
    }
    physicsObject->m_RealObject->beam_object_to_new_position(&initialRotation, &beamPosition);
    CKIpionManager::UpdateObjectWorldMatrix(physicsObject->m_RealObject);
    const VxMatrix returnedWorldMatrix = entity.GetWorldMatrix();
    Check(std::memcmp(&firstWorldMatrix, &returnedWorldMatrix, sizeof(VxMatrix)) == 0,
          "Rotating and returning to the same physics pose must preserve the original world scale");

    VxVector finalWorldScale;
    entity.GetScale(&finalWorldScale, FALSE);
    Check(NearlyEqual(initialWorldScale, finalWorldScale, 0.00001f),
          "Physics matrix updates must not feed local-scale error back into world scale");
    DeletePhysicsObject(*manager, *AsEntity(entity));
}

static void CollisionCacheReusesShapesBySurfaceName()
{
    const std::string managerSource = ReadSource("physics_RT/CKIpionManager.cpp");
    const std::string behaviorSource = ReadSource("physics_RT/Behaviors/Physicalize.cpp");

    Check(managerSource.find("BuildCollisionSurfaceKey") == std::string::npos,
          "Collision cache keys must not be derived from mesh or transform state");
    Check(managerSource.find("GetCollisionSurface(collisionSurface)") != std::string::npos,
          "Collision Surface must be used directly as the cache key");
    Check(behaviorSource.find("recoveredShapeMesh") == std::string::npos &&
              behaviorSource.find("currentMeshName") == std::string::npos,
          "Physicalize's runtime path must not infer collision geometry from the current render mesh");
}

static void UnitRotationDoesNotIntroduceVisualScale()
{
    PhysicsFixture &fixture = GetPhysicsFixture();
    RCK3dObject entity(fixture.context, "UnitRotation");
    VxVector origin(0.0f, 0.0f, 0.0f), scale(1.0f, 1.0f, 1.0f);
    bool roundedUnit = false;
    for (int i = 1; i < 200 && !roundedUnit; ++i)
    {
        VxQuaternion rotation;
        rotation.FromEulerAngles(i * 0.017f, i * 0.013f, i * -0.019f);
        entity.ConstructWorldMatrix(&origin, &scale, &rotation);
        VxVector measured;
        entity.GetScale(&measured, FALSE);
        roundedUnit = measured.x != 1.0f || measured.y != 1.0f || measured.z != 1.0f;
    }
    Check(roundedUnit, "Find a float rotation whose measured axis lengths contain rounding error");
    IVP_Material_Simple material(0.4f, 0.5f);
    Check(fixture.manager->CreatePhysicsObjectOnParameters(
              AsEntity(entity), 0, NULL, 0, NULL, NULL, 0, NULL, 1.0f, NULL, NULL,
              TRUE, &material, 1.0f, const_cast<char *>("RegTest"),
              TRUE, FALSE, TRUE, 0.1f, 0.1f) == CK_OK, "Create unit-scale rotated object");
    PhysicsObject *object = fixture.manager->GetPhysicsObject(AsEntity(entity));
    IVP_U_Matrix pose;
    object->m_RealObject->get_m_world_f_object_AT(&pose);
    VxMatrix expected;
    VxConvertMatrix(pose, expected);
    CKIpionManager::UpdateObjectWorldMatrix(object->m_RealObject);
    const VxMatrix actual = entity.GetWorldMatrix();
    Check(std::memcmp(&expected, &actual, sizeof(VxMatrix)) == 0,
          "A nominal unit-scale rotation must retain the original physics pose matrix");
    DeletePhysicsObject(*fixture.manager, *AsEntity(entity));
}

struct PhysicsTestCase
{
    const char *name;
    void (*run)();
};

int main()
{
    std::setvbuf(stdout, NULL, _IONBF, 0);
#if defined(_WIN32)
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter([](EXCEPTION_POINTERS *exception) -> LONG {
        std::fprintf(stderr, "Exception 0x%lx at %p\n", exception->ExceptionRecord->ExceptionCode,
                     exception->ExceptionRecord->ExceptionAddress);
        std::fprintf(stderr, "Image base %p\n", GetModuleHandleA(NULL));
        return EXCEPTION_EXECUTE_HANDLER;
    });
#endif
    InitializePhysicsHostTests();
    const PhysicsTestCase tests[] = {
        {"Collision cache reuses shapes by surface name", &CollisionCacheReusesShapesBySurfaceName},
        {"Named surface reuse does not need geometry", &NamedSurfaceReuseDoesNotNeedGeometry},
        {"Unknown named surface does not guess current mesh", &UnknownNamedSurfaceDoesNotGuessCurrentMesh},
        {"Physics world matrix updates keep scale stable", &PhysicsWorldMatrixUpdatesKeepWorldScaleStable},
        {"Unit rotation does not introduce visual scale", &UnitRotationDoesNotIntroduceVisualScale},
    };

    int failed = 0;
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i)
    {
        std::printf("Running test: %s... ", tests[i].name);
        std::fflush(stdout);
        try
        {
            tests[i].run();
            std::printf("PASSED\n");
        }
        catch (const std::exception &error)
        {
            ++failed;
            std::printf("FAILED: %s\n", error.what());
        }
    }

    int total = sizeof(tests) / sizeof(tests[0]);
    failed += RunPhysicsHostTests(total);
    delete GetPhysicsFixture().context;
    GetPhysicsFixture().context = NULL;
    CKShutdown();
    std::printf("\n=== Test Summary ===\n");
    std::printf("Total tests: %d\n", total);
    std::printf("Passed: %d\n", total - failed);
    std::printf("Failed: %d\n", failed);
    return failed == 0 ? 0 : 1;
}
