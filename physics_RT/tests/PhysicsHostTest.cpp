#include "PhysicsSDK.h"
#include "CKIpionManager.h"
#include "PhysicsState.h"
#include "Convert.h"
#include "RCKRenderObject.h"
#include "RCK3dEntity.h"
#include "RCK3dObject.h"
#include "RCKMesh.h"
#include "ivp_core.hxx"
#include "ivp_compact_surface.hxx"
#include "ivp_contact_situation.hxx"
#include "ivp_surbuild_pointsoup.hxx"
#include "ivp_surman_polygon.hxx"
#include <cstdio>
#include <cstring>

extern void SetProcessorSpecific_FunctionsPtr();
CKObjectDeclaration *FillBehaviorDeleteCollisionSurfacesDecl();
CKObjectDeclaration *FillBehaviorGetProfilerValuesDecl();
CKObjectDeclaration *FillBehaviorPhysicalizeDecl();
CKObjectDeclaration *FillBehaviorPhysicsBallJointDecl();
CKObjectDeclaration *FillBehaviorPhysicsBuoyancyDecl();
CKObjectDeclaration *FillBehaviorPhysicsCollDetectionDecl();
CKObjectDeclaration *FillBehaviorPhysicsContinuousContactDecl();
CKObjectDeclaration *FillBehaviorPhysicsForceDecl();
CKObjectDeclaration *FillBehaviorPhysicsHingeDecl();
CKObjectDeclaration *FillBehaviorPhysicsImpulseDecl();
CKObjectDeclaration *FillBehaviorPhysicsResetDecl();
CKObjectDeclaration *FillBehaviorPhysicsWakeUpDecl();
CKObjectDeclaration *FillBehaviorSetPhysicsGlobalsDecl();
CKObjectDeclaration *FillBehaviorSetPhysicsSliderDecl();
CKObjectDeclaration *FillBehaviorSetPhysicsSpringDecl();
extern int CKGet_TT_Physics_PluginInfoCount();
extern CKPluginInfo *CKGet_TT_Physics_PluginInfo(int);

static void Check(bool value, const char *message)
{
    if (!value)
        throw message;
}

void InitializePhysicsHostTests()
{
    SetProcessorSpecific_FunctionsPtr();
    Check(CKStartUp() == CK_OK, "start CK runtime");
    CKCLASSREGISTERCID(RCKRenderObject, CKCID_BEOBJECT);
    CKCLASSREGISTERCID(RCK3dEntity, CKCID_RENDEROBJECT);
    CKCLASSREGISTERCID(RCK3dObject, CKCID_3DENTITY);
    CKCLASSREGISTERCID(RCKMesh, CKCID_BEOBJECT);
    CKBuildClassHierarchyTable();
}

struct ScopedDeclaration
{
    CKObjectDeclaration *value;
    explicit ScopedDeclaration(CKObjectDeclaration *declaration) : value(declaration) {}
    ~ScopedDeclaration()
    {
        delete value;
    }
    CKObjectDeclaration *operator->() const
    {
        return value;
    }

  private:
    ScopedDeclaration(const ScopedDeclaration &);
    ScopedDeclaration &operator=(const ScopedDeclaration &);
};

struct HostFixture
{
    CKContext *context = NULL;
    CKIpionManager *manager = NULL;
    XArray<CKBehaviorPrototype *> prototypes;
    HostFixture()
    {
        Check(CKCreateContext(&context, NULL, 0, 0) == CK_OK, "create context");
        manager = new CKIpionManager(context);
        Check(manager->m_CollisionSurfaces != NULL, "initialize registered manager cache");
        manager->CreateEnvironment();
    }
    ~HostFixture()
    {
        CKCloseContext(context);
        for (CKBehaviorPrototype *prototype : prototypes)
            delete prototype;
    }
    CK3dEntity *Entity(const char *name)
    {
        CK3dEntity *entity = (CK3dEntity *)context->CreateObject(CKCID_3DOBJECT, const_cast<char *>(name));
        Check(entity != NULL, "create entity");
        return entity;
    }
    void Ball(CK3dEntity *entity)
    {
        IVP_Material *material = new IVP_Material_Simple(0.4f, 0.5f);
        const int result = manager->CreatePhysicsObjectOnParameters(
            entity, 0, NULL, 0, NULL, NULL, 0, NULL, 1.0f, NULL, NULL, FALSE, material, 1.0f,
            const_cast<char *>("HostTst"), TRUE, FALSE, TRUE, 0.1f, 0.1f);
        if (result != CK_OK)
            delete material;
        Check(result == CK_OK, "create ball");
        manager->OwnMaterial(entity, material);
    }
    CKBehavior *Behavior(CKObjectDeclaration *(*fill)(), CK3dEntity *owner)
    {
        ScopedDeclaration declaration(fill());
        CKBehaviorPrototype *prototype = NULL;
        Check(declaration->GetCreationFunction()(&prototype) == CK_OK, "create physics prototype");
        prototypes.PushBack(prototype);
        CKBehavior *behavior = (CKBehavior *)context->CreateObject(CKCID_BEHAVIOR, declaration->GetName());
        Check(behavior && behavior->InitFromPrototype(prototype) == CK_OK, "instantiate behavior");
        for (int i = 0; i < prototype->GetInParameterCount(); ++i)
        {
            CKPARAMETER_DESC *desc = prototype->GetInParameterList()[i];
            CKParameterLocal *source = context->CreateCKParameterLocal(desc->Name, desc->Guid);
            Check(source && behavior->GetInputParameter(i)->SetDirectSource(source) == CK_OK, "bind input source");
            if (desc->DefaultValueString)
                source->SetStringValue(desc->DefaultValueString);
            else if (desc->DefaultValue && desc->DefaultValueSize)
                source->SetValue(desc->DefaultValue, desc->DefaultValueSize);
        }
        behavior->SetPrototypeGuid(declaration->GetGuid());
        Check(behavior->SetOwner(owner, FALSE) == CK_OK, "set behavior owner");
        return behavior;
    }
    int Run(CKBehavior *behavior, int input = 0)
    {
        behavior->ActivateInput(input, TRUE);
        CKBehaviorContext call = {};
        call.Context = context;
        call.Behavior = behavior;
        call.TimeManager = context->GetTimeManager();
        return behavior->GetFunction()(call);
    }
};

static unsigned int Bits(float value)
{
    unsigned int result;
    std::memcpy(&result, &value, sizeof(result));
    return result;
}

static void SetObject(CKBehavior *behavior, int index, CKObject *object)
{
    CK_ID id = object ? object->GetID() : 0;
    CKParameterIn *input = behavior->GetInputParameter(index);
    Check(input && input->GetRealSource() && input->GetRealSource()->SetValue(&id) == CK_OK, "set object parameter");
}

static void TimingAndState()
{
    HostFixture f;
    f.manager->m_DeltaTime = 13.27400016784668f;
    f.manager->m_PhysicsTimeFactor = 0.0013699999544769526f;
    f.manager->SetDeltaTime(16.582000732421875f);
    Check(Bits(f.manager->m_DeltaTime) == 0x41619db2, "smoothed physics timestep rounds to float");
    Check(Bits(f.manager->m_PhysicsDeltaTime) == 0x3c9e418f, "physics delta must retain the double intermediate");
    const PhysicsRTStateApi *api = PhysicsRT_GetStateApi(1);
    Check(api && api->size == sizeof(*api) && api->version == 1, "state API v1 layout");
    Check(!PhysicsRT_GetStateApi(0) && !PhysicsRT_GetStateApi(2), "reject unsupported state API versions");
    api->restore_baseline(f.manager, 0.0017f, 17, 12345);
    float factor;
    short movement;
    int seed;
    api->read_baseline(f.manager, &factor, &movement, &seed);
    Check(factor == 0.0017f && movement == 17 && seed == 12345, "baseline round trip");
    const float random = api->random();
    api->set_seed(seed);
    Check(api->random() == random, "random generator replay");
    f.manager->Simulate(16.0f);
    api->reset_time(f.manager, 12.5f);
    Check(f.manager->GetSimulationTime().get_time() == 0.0, "state clock reset");
    Check(f.manager->GetEnvironment()->get_next_PSI_time().get_time() == 1.0 / 66.0, "next PSI after state reset");
    Check(f.manager->m_DeltaTime == 12.5f, "state clock smoothed delta");
    f.manager->Simulate(16.0f);
    f.manager->ResetSimulationClock();
    Check(f.manager->GetSimulationTime().get_time() == 0.0, "normal clock reset stays nonnegative");
    f.manager->SetSimulationTimeStep(1.0 / 77.0);
    Check(f.manager->GetSimulationTimeStep() == 1.0 / 77.0, "timestep must not narrow to float");
}

static void VelocityAndProfiler()
{
    HostFixture f;
    CK3dEntity *entity = f.Entity("velocity");
    const PhysicsRTStateApi *api = PhysicsRT_GetStateApi(1);
    Check(!api->read_velocity(f.manager, entity, NULL, NULL), "unphysicalized velocity result");
    Check(!api->is_sleeping(f.manager, entity), "unphysicalized sleep result");
    f.Ball(entity);
    IVP_Core *core = f.manager->GetPhysicsObject(entity)->m_RealObject->get_core();
    core->speed.set(1, 2, 3);
    core->speed_change.set(0.5f, -1, 4);
    core->rot_speed.set(2, 3, 4);
    core->rot_speed_change.set(-1, 0.5f, 2);
    float velocity[3], angular[3];
    f.manager->ResetProfiler();
    Check(api->read_velocity(f.manager, entity, velocity, angular) != 0, "read object velocity");
    Check(velocity[0] == 1.5f && velocity[1] == 1 && velocity[2] == 7, "include pending linear impulse");
    Check(angular[0] == 1 && angular[1] == 3.5f && angular[2] == 6, "include pending angular impulse");
    Check(api->read_velocity(f.manager, entity, NULL, angular) != 0, "optional velocity outputs");
    Check(f.manager->m_HasPhysicsCalls == 0, "internal reads must not change profiler counts");
    f.manager->GetPhysicsObject(entity, FALSE);
    Check(f.manager->m_HasPhysicsCalls == 1, "explicit profiled lookup counted once");
}

static void ConvexInputPrecision()
{
    HostFixture f;
    CK3dEntity *entity = f.Entity("convex");
    RCKMesh mesh(f.context, "precision hull");
    VxVector vertices[] = {{-1.137f, -0.791f, -1.123f}, {1.217f, -0.821f, -0.931f}, {1.337f, 1.311f, -0.743f},
                           {-0.913f, 1.273f, -1.073f},  {-0.997f, -1.217f, 1.331f}, {1.173f, -1.071f, 0.993f},
                           {0.873f, 1.193f, 1.237f},    {-1.311f, 0.873f, 1.119f}};
    Check(mesh.SetVertexCount(8) != FALSE, "allocate hull mesh");
    for (int i = 0; i < 8; ++i)
        mesh.SetVertexPosition(i, &vertices[i]);
    VxVector scale(1.137f, 0.791f, 1.319f);
    IVP_SurfaceBuilder_Ledge_Soup actualBuilder, expectedBuilder;
    Check(CKIpionManager::AddConvexSurface(&actualBuilder, &mesh, &scale) == 1, "host convex build");
    IVP_U_Point points[8];
    IVP_U_Vector<IVP_U_Point> list;
    for (int i = 0; i < 8; ++i)
    {
        points[i].set(double(vertices[i].x) * scale.x, double(vertices[i].y) * scale.y,
                      double(vertices[i].z) * scale.z);
        list.add(&points[i]);
    }
    IVP_Compact_Ledge *ledge = IVP_SurfaceBuilder_Pointsoup::convert_pointsoup_to_compact_ledge(&list);
    Check(ledge != NULL, "reference hull build");
    expectedBuilder.insert_ledge(ledge);
    IVP_Compact_Surface *actual = actualBuilder.compile(), *expected = expectedBuilder.compile();
    Check(actual && expected, "compile hull surfaces");
    const bool same =
        actual->get_size() == expected->get_size() && std::memcmp(actual, expected, actual->get_size()) == 0;
    ivp_free_aligned(actual);
    ivp_free_aligned(expected);
    Check(same, "host hull must match direct double-precision scaled vertices");
}

static void MatrixConversions()
{
    VxMatrix matrix;
    for (int row = 0; row < 4; ++row)
        for (int column = 0; column < 4; ++column)
            matrix[row][column] = float(row * 4 + column + 1);
    matrix[0][3] = matrix[1][3] = matrix[2][3] = 0;
    matrix[3][3] = 1;
    IVP_U_Matrix converted;
    VxMatrix roundtrip;
    VxConvertMatrix(matrix, converted);
    VxConvertMatrix(converted, roundtrip);
    Check(std::memcmp(&matrix, &roundtrip, sizeof(matrix)) == 0, "matrix transpose round trip");
}

struct BehaviorPrototypeCounts
{
    CKObjectDeclaration *(*fill)();
    int inputs, outputs, parameters, outputParameters, locals;
};
static const BehaviorPrototypeCounts PrototypeCounts[] = {
    {FillBehaviorPhysicalizeDecl, 2, 2, 12, 0, 4},
    {FillBehaviorPhysicsImpulseDecl, 1, 1, 5, 0, 2},
    {FillBehaviorPhysicsForceDecl, 2, 2, 5, 0, 1},
    {FillBehaviorPhysicsWakeUpDecl, 1, 1, 0, 0, 0},
    {FillBehaviorSetPhysicsGlobalsDecl, 2, 1, 2, 0, 0},
    {FillBehaviorPhysicsHingeDecl, 2, 2, 5, 0, 1},
    {FillBehaviorSetPhysicsSpringDecl, 2, 2, 9, 0, 1},
    {FillBehaviorSetPhysicsSliderDecl, 2, 2, 6, 0, 1},
    {FillBehaviorPhysicsBallJointDecl, 2, 2, 3, 0, 2},
    {FillBehaviorPhysicsCollDetectionDecl, 2, 1, 4, 4, 2},
    {FillBehaviorPhysicsContinuousContactDecl, 2, 10, 2, 0, 2},
    {FillBehaviorPhysicsBuoyancyDecl, 1, 1, 12, 0, 1},
    {FillBehaviorPhysicsResetDecl, 1, 1, 0, 0, 0},
    {FillBehaviorGetProfilerValuesDecl, 2, 2, 0, 5, 0},
    {FillBehaviorDeleteCollisionSurfacesDecl, 1, 1, 0, 0, 0}
};

static void BehaviorPrototypes()
{
    HostFixture f;
    CK3dEntity *owner = f.Entity("owner");
    XArray<CKGUID> guids;
    for (const BehaviorPrototypeCounts &prototype : PrototypeCounts)
    {
        ScopedDeclaration declaration(prototype.fill());
        Check(declaration->GetManagerNeededCount() == 1 && declaration->GetManagerNeeded(0) == TT_PHYSICS_MANAGER_GUID,
              "declare physics manager dependency");
        for (CKGUID guid : guids)
            Check(guid != declaration->GetGuid(), "unique behavior GUIDs");
        guids.PushBack(declaration->GetGuid());
        CKBehavior *behavior = f.Behavior(prototype.fill, owner);
        Check(behavior->GetInputCount() == prototype.inputs && behavior->GetOutputCount() == prototype.outputs &&
                  behavior->GetInputParameterCount() == prototype.parameters &&
                  behavior->GetOutputParameterCount() == prototype.outputParameters &&
                  behavior->GetLocalParameterCount() == prototype.locals,
              declaration->GetName());
    }
    Check(CKGet_TT_Physics_PluginInfoCount() == 2 && !CKGet_TT_Physics_PluginInfo(-1) &&
              !CKGet_TT_Physics_PluginInfo(2),
          "plugin discovery bounds");
}

class TestCallback : public PhysicsCallback
{
  public:
    TestCallback(CKIpionManager *manager, CKBehavior *behavior, int &calls, int &deleted, int action = 0)
        : PhysicsCallback(manager, behavior, 2), calls(calls), deleted(deleted), action(action)
    {
    }
    ~TestCallback()
    {
        ++deleted;
    }
    int Execute()
    {
        ++calls;
        if (action == 1)
            m_IpionManager->ClearBehaviorCallbacks(m_BehaviorID);
        if (action == 2)
            m_IpionManager->Reset();
        return 0;
    }
    int &calls, &deleted, action;
};

static void CallbackMutation()
{
    HostFixture f;
    CKBehavior *behavior = f.Behavior(FillBehaviorPhysicsResetDecl, f.Entity("owner"));
    int calls = 0, deleted = 0;
    f.manager->m_PreSimulateCallbacks->Process(new TestCallback(f.manager, behavior, calls, deleted, 1));
    Check(calls == 1 && deleted == 1 && !f.manager->m_PreSimulateCallbacks->m_HasCallbacks,
          "self-cancelling callback destroyed once");
    f.manager->m_PreSimulateCallbacks->Process(new TestCallback(f.manager, behavior, calls, deleted));
    f.manager->m_PreSimulateCallbacks->Process(new TestCallback(f.manager, behavior, calls, deleted, 1));
    Check(deleted == 3 && !f.manager->m_PreSimulateCallbacks->m_HasCallbacks, "cancel queued and active callbacks");
    f.manager->m_PreSimulateCallbacks->Process(new TestCallback(f.manager, behavior, calls, deleted, 2));
    Check(f.manager->m_ResetRequested && f.manager->m_CallbackProcessingDepth == 0, "reset deferred during Execute");
    Check(f.manager->ProcessPendingReset() && !f.manager->m_ResetRequested, "reset runs after callbacks finish");
    Check(deleted == 4 && f.manager->GetEnvironment(), "reset released pending callback");
}

static void StopCancelsDeferredCreation()
{
    HostFixture f;
    CK3dEntity *owner = f.Entity("not yet physicalized");
    CKBehavior *behavior = f.Behavior(FillBehaviorPhysicsForceDecl, owner);
    Check(f.Run(behavior) == CKBR_OK, "start deferred force");
    Check(f.manager->m_PreSimulateCallbacks->m_HasCallbacks != FALSE, "force waiting for physicalization");
    Check(f.Run(behavior, 1) == CKBR_OK && !f.manager->m_PreSimulateCallbacks->m_HasCallbacks,
          "stop removes deferred force");
    f.Ball(owner);
    f.manager->Simulate(16);
    void *handle = NULL;
    behavior->GetLocalParameterValue(0, &handle);
    Check(!handle, "stopped force must not appear after physicalization");
}

static void ObjectAndConstraintLifetimes()
{
    CKObjectDeclaration *(*fills[])() = {FillBehaviorPhysicsForceDecl, FillBehaviorPhysicsBallJointDecl,
                                         FillBehaviorPhysicsHingeDecl, FillBehaviorSetPhysicsSliderDecl,
                                         FillBehaviorSetPhysicsSpringDecl};
    for (auto fill : fills)
    {
        HostFixture f;
        CK3dEntity *a = f.Entity("A"), *b = f.Entity("B"), *axis = f.Entity("axis");
        VxVector position(0, 0, 2);
        axis->SetPosition(&position);
        f.Ball(a);
        f.Ball(b);
        CKBehavior *behavior = f.Behavior(fill, a);
        if (fill != FillBehaviorPhysicsForceDecl)
            SetObject(behavior, 0, b);
        if (fill == FillBehaviorPhysicsHingeDecl)
            SetObject(behavior, 1, axis);
        if (fill == FillBehaviorSetPhysicsSliderDecl)
        {
            SetObject(behavior, 1, a);
            SetObject(behavior, 2, axis);
        }
        Check(f.Run(behavior) == CKBR_OK, "create force/constraint");
        void *handle = NULL;
        behavior->GetLocalParameterValue(0, &handle);
        Check(handle != NULL, "created force/constraint handle");
        f.manager->GetPhysicsObject(a)->m_RealObject->delete_silently();
        behavior->GetLocalParameterValue(0, &handle);
        Check(!handle, "deleted physical object invalidates its behavior handle");
        Check(f.Run(behavior, 1) == CKBR_OK, "stop after object deletion");
        f.manager->Reset();
        Check(f.manager->GetPhysicsObjectCount() == 0 && f.manager->m_MaterialOwners.len() == 0 &&
                  f.manager->m_MovableObjects.len() == 0,
              "reset releases object resources");
    }
}

static void CollisionOutputs()
{
    HostFixture f;
    CK3dEntity *a = f.Entity("A"), *b = f.Entity("B");
    f.Ball(a);
    f.Ball(b);
    CKBehavior *behavior = f.Behavior(FillBehaviorPhysicsCollDetectionDecl, a);
    float zero = 0;
    behavior->GetInputParameter(2)->GetRealSource()->SetValue(&zero);
    Check(f.Run(behavior) == CKBR_ACTIVATENEXTFRAME, "install collision listener");
    IVP_Listener_Collision *listener = NULL;
    behavior->GetLocalParameterValue(0, &listener);
    Check(listener != NULL, "collision listener handle");
    IVP_Contact_Situation situation;
    situation.objects[0] = f.manager->GetPhysicsObject(b)->m_RealObject;
    situation.objects[1] = f.manager->GetPhysicsObject(a)->m_RealObject;
    situation.speed.set(3, 0, 0);
    situation.surf_normal.set(0, 1, 0);
    situation.contact_point_ws.set(7, 8, 9);
    IVP_Event_Collision event = {};
    event.environment = f.manager->GetEnvironment();
    event.contact_situation = &situation;
    listener->event_post_collision(&event);
    VxVector position, normal;
    behavior->GetOutputParameterValue(3, &position);
    behavior->GetOutputParameterValue(2, &normal);
    Check(position == VxVector(7, 8, 9) && normal == VxVector(0, -1, 0), "collision position and oriented normal");
    Check(situation.surf_normal.k[1] == 1, "collision callback must not modify shared normal");
    Check(behavior->GetOutputParameterObject(0) == b, "collision other entity output");
}

static void ContactSettingsAndCleanup()
{
    HostFixture f;
    CK3dEntity *owner = f.Entity("contact");
    f.Ball(owner);
    CKBehavior *behavior = f.Behavior(FillBehaviorPhysicsContinuousContactDecl, owner);
    Check(f.Run(behavior) == CKBR_ACTIVATENEXTFRAME, "install continuous contact");
    PhysicsObject *object = f.manager->GetPhysicsObject(owner);
    Check(object->m_ContactData != NULL, "contact data attached");
    f.manager->m_ContactManager->AddRecord(object, 0, IVP_Time(0));
    f.manager->m_ContactManager->AddRecord(object, -1, IVP_Time(0));
    Check(f.manager->m_ContactManager->GetRecordCount() == 1, "contact index validation");
    int groups = 2;
    behavior->SetLocalParameterValue(0, &groups);
    behavior->CallCallbackFunction(CKM_BEHAVIORSETTINGSEDITED);
    Check(!object->m_ContactData && !f.manager->m_ContactManager->GetRecordCount() && behavior->GetOutputCount() == 4,
          "editing contact groups releases previous listeners and records");
    f.Run(behavior);
    behavior->SetOwner(NULL, FALSE);
    f.manager->GetPhysicsObject(owner)->m_RealObject->delete_silently();
    Check(!f.manager->m_ContactManager->GetRecordCount(), "dephysicalization clears contacts");
    void *handle = reinterpret_cast<void *>(1);
    behavior->GetLocalParameterValue(1, &handle);
    Check(!handle, "detached behavior must not retain freed contact data");
}

static CKParameter *InputValue(CKBehavior *behavior, int index)
{
    CKParameterIn *input = behavior->GetInputParameter(index);
    Check(input != NULL, "input parameter exists");
    if (!input->GetRealSource())
    {
        CKParameterLocal *source = behavior->GetCKContext()->CreateCKParameterLocal(input->GetName(), input->GetGUID());
        Check(source && input->SetDirectSource(source) == CK_OK, "bind dynamic input");
    }
    return input->GetRealSource();
}

static void PhysicalizeAndGlobals()
{
    HostFixture f;
    CK3dEntity *owner = f.Entity("physicalize");
    CKBehavior *physicalize = f.Behavior(FillBehaviorPhysicalizeDecl, owner);
    int convexes = 0, balls = 1;
    physicalize->SetLocalParameterValue(0, &convexes);
    physicalize->SetLocalParameterValue(1, &balls);
    physicalize->CallCallbackFunction(CKM_BEHAVIORSETTINGSEDITED);
    VxVector center(0, 0, 0);
    float radius = 1;
    InputValue(physicalize, 11)->SetValue(&center);
    InputValue(physicalize, 12)->SetValue(&radius);
    Check(f.Run(physicalize) == CKBR_OK && f.manager->GetPhysicsObjectCount() == 1, "physicalize sphere behavior");
    Check(f.Run(physicalize) == CKBR_OK && f.manager->GetPhysicsObjectCount() == 1, "repeated physicalization");
    Check(f.manager->m_HasPhysicsCalls == 2, "BB lookups retain profiler counts");
    CKBehavior *wake = f.Behavior(FillBehaviorPhysicsWakeUpDecl, owner);
    Check(f.Run(wake) == CKBR_OK, "wake behavior");
    CKBehavior *impulse = f.Behavior(FillBehaviorPhysicsImpulseDecl, owner);
    Check(f.Run(impulse) == CKBR_OK, "impulse behavior");
    float velocity[3];
    PhysicsRT_GetStateApi(1)->read_velocity(f.manager, owner, velocity, NULL);
    Check(velocity[2] > 9.0f, "impulse changes linear speed");
    CKBehavior *globals = f.Behavior(FillBehaviorSetPhysicsGlobalsDecl, owner);
    VxVector gravity(0, -3, 0);
    float factor = 2;
    InputValue(globals, 0)->SetValue(&gravity);
    InputValue(globals, 1)->SetValue(&factor);
    Check(f.Run(globals) == CKBR_OK, "set globals behavior");
    VxVector observed;
    f.manager->GetGravity(observed);
    Check(observed == gravity && f.manager->m_PhysicsTimeFactor == 0.002f, "globals values and units");
    CKBehavior *profiler = f.Behavior(FillBehaviorGetProfilerValuesDecl, owner);
    Check(f.Run(profiler, 1) == CKBR_OK && f.manager->m_PhysicalizeCalls == 0, "reset profiler behavior");
    Check(f.Run(profiler) == CKBR_OK, "read profiler behavior");
    Check(f.Run(physicalize, 1) == CKBR_OK && !f.manager->GetPhysicsObject(owner), "unphysicalize behavior");
    CKBehavior *clear = f.Behavior(FillBehaviorDeleteCollisionSurfacesDecl, owner);
    Check(f.Run(clear) == CKBR_OK, "delete collision surfaces behavior");
    Check(f.Run(physicalize) == CKBR_OK, "physicalize after clearing surfaces");
    CKBehavior *reset = f.Behavior(FillBehaviorPhysicsResetDecl, owner);
    Check(f.Run(reset) == CKBR_OK && f.manager->GetPhysicsObjectCount() == 0 &&
              f.manager->m_PhysicsTimeFactor == 0.001f,
          "physics reset behavior");
}

static void MultiBallAndDoubleAnchor()
{
    HostFixture f;
    CK3dEntity *a = f.Entity("multiball"), *b = f.Entity("anchor");
    VxVector centers[] = {{-1, 0, 0}, {1, 0, 0}};
    float radii[] = {0.5f, 0.75f};
    IVP_Material_Simple material(0.4f, 0.5f);
    Check(f.manager->CreatePhysicsObjectOnParameters(a, 0, NULL, 2, centers, radii, 0, NULL, 1, NULL, NULL, FALSE,
                                                     &material, 1, const_cast<char *>("Multi"), TRUE, FALSE, TRUE, 0.1f,
                                                     0.1f) == CK_OK,
          "local multiball support");
    Check(f.manager->m_PrivateCollisionSurfaceOwners.len() == 1, "private multiball surface owned");
    f.Ball(b);
    CKBehavior *joint = f.Behavior(FillBehaviorPhysicsBallJointDecl, a);
    SetObject(joint, 0, b);
    CKBOOL enabled = TRUE;
    joint->SetLocalParameterValue(1, &enabled);
    joint->CallCallbackFunction(CKM_BEHAVIORSETTINGSEDITED);
    Check(joint->GetInputParameterCount() == 5, "double-anchor setting preserved");
    VxVector second(0, 2, 0);
    InputValue(joint, 3)->SetValue(&second);
    Check(f.Run(joint) == CKBR_OK, "double-anchor joint behavior");
    void *constraint = NULL;
    joint->GetLocalParameterValue(0, &constraint);
    Check(constraint != NULL, "double-anchor constraint created");
    f.manager->GetPhysicsObject(a)->m_RealObject->delete_silently();
    Check(f.manager->m_PrivateCollisionSurfaceOwners.len() == 0, "private surface released with owner");
}

static void BuoyancyReset()
{
    HostFixture f;
    CK3dEntity *volume = f.Entity("buoyancy volume");
    f.Ball(volume);
    CKBehavior *behavior = f.Behavior(FillBehaviorPhysicsBuoyancyDecl, volume);
    CK3dEntity *points[5];
    const VxVector positions[] = {{0, 0, 0}, {0, 0, 1}, {1, 0, 0}, {0, 0, 0}, {1, 0, 0}};
    for (int i = 0; i < 5; ++i)
    {
        points[i] = f.Entity("liquid point");
        points[i]->SetPosition(&positions[i]);
        SetObject(behavior, i, points[i]);
    }
    Check(f.Run(behavior) == CKBR_OK && f.manager->m_LiquidSurfaces.len() == 1, "install buoyancy");
    Check(f.Run(behavior) == CKBR_OK && f.manager->m_LiquidSurfaces.len() == 1, "buoyancy installation is idempotent");
    f.manager->Reset();
    CKBOOL installed = TRUE;
    behavior->GetLocalParameterValue(0, &installed);
    Check(!installed && !f.manager->m_LiquidSurfaces.len(), "reset clears buoyancy installation state and descriptor");
    f.Ball(volume);
    Check(f.Run(behavior) == CKBR_OK && f.manager->m_LiquidSurfaces.len() == 1, "reinstall buoyancy after reset");
}

static void CollisionShapeScaleRounding()
{
    HostFixture f;
    CK3dEntity *entity = f.Entity("rotated collision scale");
    VxMatrix rotation = VxMatrix::Identity();
    rotation[0].Set(0.9848077297210693f, 0.1736481636762619f, 0);
    rotation[1].Set(-0.1736481636762619f, 0.9848077297210693f, 0);
    entity->SetWorldMatrix(rotation);
    VxVector collision;
    entity->GetScale(&collision, TRUE);
    const VxVector expected(Magnitude(rotation[0]), Magnitude(rotation[1]), Magnitude(rotation[2]));
    Check(collision == expected, "collision shape scale uses VxMath vector magnitude rounding");
}

static void HingeAxisNormalizationRounding()
{
    HostFixture f;
    CK3dEntity *entity = f.Entity("hinge direction");
    const float data[3][2] = {{-0.963630438f, 0.267238319f},
                              {0.968147635f, 0.250380158f},
                              {-0.965925813f, 0.258819014f}};
    for (int i = 0; i < 3; ++i)
    {
        VxMatrix matrix = VxMatrix::Identity();
        matrix[2].Set(data[i][0], 0, data[i][1]);
        entity->SetWorldMatrix(matrix);
        VxVector axis;
        entity->GetOrientation(&axis, NULL);
        Check(axis == Normalize(VxVector(data[i][0], 0, data[i][1])),
              "hinge orientation uses VxMath vector normalization rounding");
    }
}

int RunPhysicsHostTests(int &total)
{
    struct Test
    {
        const char *name;
        void (*run)();
    };
    const Test tests[] = {{"Timing and versioned state", TimingAndState},
                          {"Velocity and profiler isolation", VelocityAndProfiler},
                          {"Convex input precision", ConvexInputPrecision},
                          {"Matrix conversions", MatrixConversions},
                          {"Collision shape scale rounding", CollisionShapeScaleRounding},
                          {"Hinge axis normalization rounding", HingeAxisNormalizationRounding},
                          {"All 15 building block prototypes", BehaviorPrototypes},
                          {"Callback mutation and pending reset", CallbackMutation},
                          {"Stop cancels deferred creation", StopCancelsDeferredCreation},
                          {"Object and constraint lifetimes", ObjectAndConstraintLifetimes},
                          {"Collision output isolation", CollisionOutputs},
                          {"Contact settings and cleanup", ContactSettingsAndCleanup},
                          {"Physicalize, impulse and global behaviors", PhysicalizeAndGlobals},
                          {"Local multiball and double-anchor support", MultiBallAndDoubleAnchor},
                          {"Buoyancy reinstall after reset", BuoyancyReset}};
    int failed = 0;
    for (const Test &test : tests)
    {
        ++total;
        std::printf("Running test: %s... ", test.name);
        std::fflush(stdout);
        try
        {
            test.run();
            std::puts("PASSED");
        }
        catch (const char *error)
        {
            ++failed;
            std::printf("FAILED: %s\n", error);
        }
    }
    return failed;
}
