/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            Stretch
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKObjectDeclaration *FillBehaviorStretchDecl();
CKERROR CreateStretchProto(CKBehaviorPrototype **);
int Stretch(const CKBehaviorContext &behcontext);
CKERROR MeshModificationsCallBack(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorStretchDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Stretch");
    od->SetDescription("Stretches an object, equivalent to scaling it with a stretch factor along the selected axis and with the opposite factor along the two other axes.");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process.<BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Stretch: </SPAN>base scale factor for all three axes. A value of 1.5 corresponds to a scale factor of 1.5+1=2.5, or 250%.<BR>
    <SPAN CLASS=pin>Amplify: </SPAN>change the scale factor applied to the minor axes. It can be a negative value.<BR>
    <SPAN CLASS=pin>Axis: </SPAN>axis along which the stretch will occur.<BR>
    <SPAN CLASS=pin>Reset Mesh: </SPAN>if TRUE, the mesh is resetted to its initial conditions at activation.<BR>
    */
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x22d774f6, 0x4ac542c3));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00020000);
    od->SetCreationFunction(CreateStretchProto);
    od->SetCompatibleClassId(CKCID_3DOBJECT);
    od->SetCategory("Mesh Modifications/Deformation");
    return od;
}

CKERROR CreateStretchProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = NULL;
    proto = CreateCKBehaviorPrototype("Stretch");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Stretch", CKPGUID_FLOAT, "0.1");
    proto->DeclareInParameter("Amplify", CKPGUID_FLOAT, "0.0");
    // To change by Radio buttons
    proto->DeclareInParameter("Axis", CKPGUID_VECTOR, "0,0,1");
    proto->DeclareInParameter("Reset Mesh", CKPGUID_BOOL, "TRUE");

    proto->DeclareLocalParameter("data", CKPGUID_VOIDBUF);
    proto->DeclareLocalParameter("initialBox", CKPGUID_BOX);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(Stretch);

    proto->SetBehaviorCallbackFct(MeshModificationsCallBack);
    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

    *pproto = proto;
    return CK_OK;
}

int Stretch(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    // we get the camera entity
    CK3dEntity *ent = (CK3dEntity *)beh->GetTarget();
    if (!ent)
        return 0;

    // we get the amount value
    float stretch;
    beh->GetInputParameterValue(0, &stretch);

    // we get the amplification
    float amplify;
    beh->GetInputParameterValue(1, &amplify);

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

    amplify = (amplify >= 0) ? amplify + 1 : 1.0F / (-amplify + 1.0F);
    float heightMin, heightMax;

    VxBbox bbox;
    beh->GetLocalParameterValue(1, &bbox);

    heightMax = bbox.Max[naxis];
    heightMin = bbox.Min[naxis];

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
        Vx3DMatrixFromRotation(mat, vx, -HALFPI);
    }
    break; // y
    case 2:
    {
        Vx3DMatrixIdentity(mat);
    }
    break; // z
    }
    VxMatrix invmat;
    Vx3DInverseMatrix(invmat, mat);

    VxVector ste;
    // we move all the points
    float fraction, normHeight;
    float xyScale, zScale, a, b, c;
    VxVector v;
    for (i = 0; i < pointsNumber; i++, varray += vStride)
    {
        Vx3DMultiplyMatrixVector(&v, mat, (VxVector *)varray);
        normHeight = (v.z - heightMin) / (heightMax - heightMin);
        if (stretch < 0) // Squash
        {
            xyScale = (amplify * -stretch + 1.0F);
            zScale = (-1.0F / (stretch - 1.0F));
        }
        else // Stretch
        {
            xyScale = 1.0F / (amplify * stretch + 1.0F);
            zScale = stretch + 1.0F;
        }
        a = 4.0F * (1.0F - xyScale);
        b = -4.0F * (1.0F - xyScale);
        c = 1.0F;
        fraction = (((a * normHeight) + b) * normHeight) + c;
        v.x *= fraction;
        v.y *= fraction;
        v.z *= zScale;

        Vx3DMultiplyMatrixVector((VxVector *)varray, invmat, &v);
    }

    mesh->ModifierVertexMove(TRUE, TRUE);
    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0, TRUE);

    return CKBR_OK;
}
