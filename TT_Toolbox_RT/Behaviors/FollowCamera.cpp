///////////////////////////////////
///////////////////////////////////
//
//        TT FollowCamera
//
///////////////////////////////////
///////////////////////////////////
#include "CKAll.h"
#include "ToolboxGuids.h"

CKObjectDeclaration *FillBehaviorFollowCameraDecl();
CKERROR CreateFollowCameraProto(CKBehaviorPrototype **pproto);
int FollowCamera(const CKBehaviorContext &behcontext);
CKERROR FollowCameraCallBack(const CKBehaviorContext &behcontext);

#define LOCAL_TIME_BASED      0
#define LOCAL_INIT_ACT_POS    1
#define LOCAL_VIEW_POS_OLD    2
#define LOCAL_CAM_POS_OLD     3
#define LOCAL_OLD_DELTA_TIME  4
#define LOCAL_INIT            5

CKObjectDeclaration *FillBehaviorFollowCameraDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("TT FollowCamera");
    od->SetDescription("Kamera folgt einem Objekt");
    od->SetCategory("TT Toolbox/3D Transformations");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x595b4723, 0x576c6749));
    od->SetAuthorGuid(TERRATOOLS_GUID);
    od->SetAuthorName("Terratools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateFollowCameraProto);
    od->SetCompatibleClassId(CKCID_CAMERA);
    return od;
}

CKERROR CreateFollowCameraProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("TT FollowCamera");
    if (!proto) return CKERR_OUTOFMEMORY;

    proto->DeclareInput("On");
    proto->DeclareInput("Stop");

    proto->DeclareOutput("Exit On");
    proto->DeclareOutput("Exit Stop");

    proto->DeclareInParameter("Target Entity", CKPGUID_3DENTITY);
    proto->DeclareInParameter("View Offset", CKPGUID_VECTOR);
    proto->DeclareInParameter("View Attenuation", CKPGUID_PERCENTAGE, "100");
    proto->DeclareInParameter("Camera Offset", CKPGUID_VECTOR);
    proto->DeclareInParameter("Camera Attenuation", CKPGUID_PERCENTAGE, "100");
    proto->DeclareInParameter("FOV Angle", CKPGUID_ANGLE, "0:50");
    proto->DeclareInParameter("Back Attenuation", CKPGUID_PERCENTAGE, "25");

    proto->DeclareSetting("Time Based", CKPGUID_BOOL, "TRUE");
    proto->DeclareSetting("Init actPos", CKPGUID_BOOL, "FALSE");

    proto->DeclareLocalParameter("ViewPosOld", CKPGUID_VECTOR);
    proto->DeclareLocalParameter("CamPosOld", CKPGUID_VECTOR);
    proto->DeclareLocalParameter("oldDeltaTime", CKPGUID_FLOAT);
    proto->DeclareLocalParameter("init", CKPGUID_BOOL, "true");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(FollowCamera);

    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);
    proto->SetBehaviorCallbackFct(FollowCameraCallBack);

    *pproto = proto;
    return CK_OK;
}

int FollowCamera(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    CKCamera *camera = (CKCamera *)beh->GetTarget();
    if (!camera)
        return CKBR_OWNERERROR;

    // TT_Toolbox_RT.dll 0x10003250: the camera's own target (vtable +0x210,
    // CKCamera::GetTarget) is fetched first and is what the block moves to the
    // view point; the camera is never rotated with LookAt.
    CK3dEntity *cameraTarget = camera->GetTarget();

    // Stop is handled before any parameter is read or the camera touched.
    if (beh->IsInputActive(1))
    {
        beh->ActivateInput(1, FALSE);
        beh->ActivateOutput(1, TRUE);
        return CKBR_OK;
    }

    CK3dEntity *targetEntity = (CK3dEntity *)beh->GetInputParameterObject(0);

    VxVector viewOffset, cameraOffset;
    float viewAtt, cameraAtt;
    float fovAngle, backAtt;

    viewOffset = VxVector::axis0();
    beh->GetInputParameterValue(1, &viewOffset);
    viewAtt = 1.0f;
    beh->GetInputParameterValue(2, &viewAtt);
    cameraOffset = VxVector::axis0();
    beh->GetInputParameterValue(3, &cameraOffset);
    cameraAtt = 1.0f;
    beh->GetInputParameterValue(4, &cameraAtt);
    beh->GetInputParameterValue(5, &fovAngle);
    backAtt = 0.25f;
    beh->GetInputParameterValue(6, &backAtt);

    // The DLL takes |m[0][0]| and |m[1][1]| (elements 0 and 5) when m[0][0] < 0.
    VxMatrix mat = camera->GetWorldMatrix();
    if (mat[0][0] < 0.0f)
    {
        mat[0][0] = fabsf(mat[0][0]);
        mat[1][1] = fabsf(mat[1][1]);
        camera->SetWorldMatrix(mat);
    }

    camera->SetFov(fovAngle);

    VxVector targetViewPos = VxVector::axis0();
    VxVector targetCamPos = VxVector::axis0();

    if (targetEntity)
    {
        targetEntity->Transform(&targetViewPos, &viewOffset);
        targetEntity->Transform(&targetCamPos, &cameraOffset);
    }

    VxVector viewPosOld = VxVector::axis0();
    VxVector camPosOld = VxVector::axis0();

    if (beh->IsInputActive(0))
    {
        beh->ActivateInput(0, FALSE);
        beh->ActivateOutput(0, TRUE);

        CKBOOL init = FALSE;
        beh->GetLocalParameterValue(LOCAL_INIT, &init);

        if (!init)
        {
            viewPosOld = targetViewPos;
            camPosOld = targetCamPos;
        }
        else
        {
            // Start from where the camera and its target currently are.
            camera->GetPosition(&camPosOld);
            if (cameraTarget)
                cameraTarget->GetPosition(&viewPosOld);
            else
                viewPosOld = targetViewPos;
        }

        beh->SetLocalParameterValue(LOCAL_VIEW_POS_OLD, &viewPosOld);
        beh->SetLocalParameterValue(LOCAL_CAM_POS_OLD, &camPosOld);

        float deltaTime = behcontext.DeltaTime;
        beh->SetLocalParameterValue(LOCAL_OLD_DELTA_TIME, &deltaTime);
    }

    beh->GetLocalParameterValue(LOCAL_VIEW_POS_OLD, &viewPosOld);
    beh->GetLocalParameterValue(LOCAL_CAM_POS_OLD, &camPosOld);

    VxVector viewDelta = targetViewPos - viewPosOld;
    VxVector camDelta = targetCamPos - camPosOld;

    CKBOOL timeBased = FALSE;
    beh->GetLocalParameterValue(LOCAL_TIME_BASED, &timeBased);

    if (timeBased)
    {
        // The DLL reads the time manager's last delta time here (offset 0xA0).
        float deltaTime = beh->GetCKContext()->GetTimeManager()->GetLastDeltaTime();
        float oldDeltaTime = 0.0f;
        beh->GetLocalParameterValue(LOCAL_OLD_DELTA_TIME, &oldDeltaTime);

        float maxDelta = deltaTime * 1.1f;
        if (oldDeltaTime > maxDelta)
            deltaTime = maxDelta;

        float minDelta = deltaTime - deltaTime * 0.1f;
        if (oldDeltaTime >= minDelta)
            minDelta = deltaTime;

        float avgDelta = (oldDeltaTime * 4.0f + minDelta) * 0.2f;

        float viewFactor = avgDelta * viewAtt * 0.07f;
        if (viewFactor < 0.0f) viewFactor = 0.0f;
        if (viewFactor > 1.0f) viewFactor = 1.0f;

        float camFactor = avgDelta * cameraAtt * 0.07f;
        if (camFactor < 0.0f) camFactor = 0.0f;
        if (camFactor > 1.0f) camFactor = 1.0f;

        viewAtt = viewFactor;
        cameraAtt = camFactor;

        beh->SetLocalParameterValue(LOCAL_OLD_DELTA_TIME, &avgDelta);
    }

    viewPosOld += viewDelta * viewAtt;
    camPosOld += camDelta * cameraAtt;

    // Pull the camera back when it is closer to the followed entity than the
    // Camera Offset length (distance measured in the entity's local frame).
    VxVector camLocal(0.0f, 0.0f, 0.0f);
    if (targetEntity)
        targetEntity->InverseTransform(&camLocal, &camPosOld);
    if (Magnitude(cameraOffset) > Magnitude(camLocal))
    {
        VxVector diff = targetCamPos - camPosOld;
        camPosOld += diff * backAtt;
    }

    if (cameraTarget)
        cameraTarget->SetPosition(&viewPosOld);
    camera->SetPosition(&camPosOld);

    beh->SetLocalParameterValue(LOCAL_VIEW_POS_OLD, &viewPosOld);
    beh->SetLocalParameterValue(LOCAL_CAM_POS_OLD, &camPosOld);

    return CKBR_ACTIVATENEXTFRAME;
}

CKERROR FollowCameraCallBack(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    switch (behcontext.CallbackMessage)
    {
    case CKM_BEHAVIORATTACH:
    case CKM_BEHAVIORLOAD:
    case CKM_BEHAVIORNEWSCENE:
    case CKM_BEHAVIORACTIVATESCRIPT:
        {
            CKBOOL init = TRUE;
            beh->SetLocalParameterValue(LOCAL_INIT, &init);
        }
        break;
    default:
        break;
    }

    return CKBR_OK;
}
