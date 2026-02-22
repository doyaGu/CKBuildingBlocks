/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            Twist
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKObjectDeclaration *FillBehaviorTwistDecl();
CKERROR CreateTwistProto(CKBehaviorPrototype **);
int Twist(const CKBehaviorContext &behcontext);
CKERROR MeshModificationsCallBack(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorTwistDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Twist");
    od->SetDescription("Twists an object around an axis. You can choose the angle of twisting and the bias, ranging from -100 to 100.");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process.<BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Angle: </SPAN>amount of twist around the axis.<BR>
    <SPAN CLASS=pin>Bias: </SPAN>causes the twist rotation to bunch up at either end of the object (-100 to 100).<BR>
    <SPAN CLASS=pin>Axis: </SPAN>axis along which the twist will occur.<BR>
    <SPAN CLASS=pin>Reset Mesh: </SPAN>if TRUE, the mesh is resetted to its initial conditions at activation.<BR>
    */
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x75b066af, 0x70371b90));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00020000);
    od->SetCreationFunction(CreateTwistProto);
    od->SetCompatibleClassId(CKCID_3DOBJECT);
    od->SetCategory("Mesh Modifications/Deformation");
    return od;
}

CKERROR CreateTwistProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = NULL;
    proto = CreateCKBehaviorPrototype("Twist");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Angle", CKPGUID_ANGLE, "0:90");
    // Should be a range from -100 to 100
    proto->DeclareInParameter("Bias", CKPGUID_FLOAT, "0.0");
    // To change by Radio buttons
    proto->DeclareInParameter("Axis", CKPGUID_VECTOR, "0,1,0");
    proto->DeclareInParameter("Reset Mesh", CKPGUID_BOOL, "TRUE");
    proto->DeclareInParameter("Normal Recalculation", CKPGUID_BOOL, "TRUE");

    proto->DeclareLocalParameter("data", CKPGUID_VOIDBUF);
    proto->DeclareLocalParameter("initialBox", CKPGUID_BOX);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(Twist);

    proto->SetBehaviorCallbackFct(MeshModificationsCallBack);
    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

    *pproto = proto;
    return CK_OK;
}

int Twist(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    // we get the camera entity
    CK3dEntity *ent = (CK3dEntity *)beh->GetTarget();
    if (!ent)
        return 0;

    // we get the angle value
    float angle;
    beh->GetInputParameterValue(0, &angle);

    // we get the bias value
    float bias;
    beh->GetInputParameterValue(1, &bias);

    // we get the amount value
    VxVector axis;
    beh->GetInputParameterValue(2, &axis);

    int naxis = 0;
    int i = 0;
    while (naxis < 3 && axis[naxis] == 0.0f)
        naxis++;
    if (naxis == 3)
        naxis = 2;

    // we get the mesh
    CKMesh *mesh = ent->GetCurrentMesh();
    CKDWORD vStride = 0;
    CKBYTE *varray = (CKBYTE *)mesh->GetModifierVertices(&vStride);
    int pointsNumber = mesh->GetModifierVertexCount();

    CKBOOL resetmesh = TRUE;
    beh->GetInputParameterValue(3, &resetmesh);
    if (resetmesh) // The Mesh must be resetted
    {
        if (beh->GetVersion() < 0x00020000) // Old Version with vertices stuffed inside
        {
            // we get the saved position
            VxVector *savePos = (VxVector *)beh->GetLocalParameterWriteDataPtr(0);

            CKBYTE *temparray = varray;
            for (int i = 0; i < pointsNumber; i++, temparray += vStride)
            {
                *(VxVector *)temparray = savePos[i];
            }
        }
        else // new version : based on ICs
        {
            CKScene *scn = behcontext.CurrentScene;
            CKStateChunk *chunk = scn->GetObjectInitialValue(mesh);
            if (chunk)
                mesh->LoadVertices(chunk);
            varray = (CKBYTE *)mesh->GetModifierVertices(&vStride);
            pointsNumber = mesh->GetModifierVertexCount();
        }
    }

    CKBOOL doBias;
    if (bias != 0.0f)
    {
        bias = 1.0f - (bias + 100.0f) / 200.0f;
        if (bias < 0.00001f)
            bias = 0.00001f;
        if (bias > 0.99999f)
            bias = 0.99999f;
        bias = float(log(bias) / log(0.5));
        doBias = true;
    }
    else
    {
        bias = 1.0f;
        doBias = false;
    }

    VxMatrix mat;
    switch (naxis)
    {
    case 0:
    {
        VxVector vy(0.0f, 1.0f, 0.0f);
        Vx3DMatrixFromRotation(mat, vy, -HALFPI);
    }
    break; // x
    case 1:
    {
        VxVector vx(1.0f, 0.0f, 0.0f);
        Vx3DMatrixFromRotation(mat, vx, HALFPI);
    }
    break; // y
    case 2:
    {
        mat = VxMatrix::Identity();
    }
    break; // z
    }
    VxMatrix invmat;
    Vx3DInverseMatrix(invmat, mat);

    VxBbox bbox;
    beh->GetLocalParameterValue(1, &bbox);

    float height, angleOverHeight;
    switch (naxis)
    {
    case 0:
        height = bbox.Max.x - bbox.Min.x;
        break;
    case 1:
        height = bbox.Max.y - bbox.Min.y;
        break;
    case 2:
        height = bbox.Max.z - bbox.Min.z;
        break;
    }
    if (height == 0.0f)
    {
        angle = 0.0f;
        angleOverHeight = 0.0f;
    }
    else
    {
        angleOverHeight = angle / height;
    }

    // we move all the points
    float x, y, z, cosine, sine, a;
    VxVector v;
    for (i = 0; i < pointsNumber; i++, varray += vStride)
    {
        Vx3DMultiplyMatrixVector(&v, mat, (VxVector *)varray);
        x = v.x;
        y = v.y;
        z = v.z;
        if (doBias)
        {
            float u = z / height;
            a = angle * (float)pow(fabs(u), bias);
            if (u < 0.0)
                a = -a;
        }
        else
        {
            a = z * angleOverHeight;
        }
        cosine = float(cos(a));
        sine = float(sin(a));
        v.x = cosine * x + sine * y;
        v.y = -sine * x + cosine * y;
        Vx3DMultiplyMatrixVector((VxVector *)varray, invmat, &v);
    }

    CKBOOL normalc = TRUE;
    beh->GetInputParameterValue(4, &normalc);

    mesh->ModifierVertexMove(normalc, TRUE);

    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0, TRUE);

    return CKBR_OK;
}
