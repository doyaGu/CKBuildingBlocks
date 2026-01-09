/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            Taper
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKObjectDeclaration *FillBehaviorTaperDecl();
CKERROR CreateTaperProto(CKBehaviorPrototype **);
int Taper(const CKBehaviorContext &behcontext);
CKERROR MeshModificationsCallBack(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorTaperDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Taper");
    od->SetDescription("Modifies the mesh by scaling one end of it and changes the amount of tapering and the curvature of the effect.");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process.<BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Amount: </SPAN>amount of the effect (maximum of 10). Positive amounts produce an outward taper, negative amounts an inward taper.<BR>
    <SPAN CLASS=pin>Curve: </SPAN>curve of the taper. Positive values produce an outward curve along the tapered sides, negative values an inward curve.<BR>
    <SPAN CLASS=pin>Axis: </SPAN>primary axis of the taper. The effect axis can be either of the two remaining axes, or their combination.<BR>
    <SPAN CLASS=pin>Symmetry: </SPAN>When True, produces a symmetrical taper around the primary axis.<BR>
    <SPAN CLASS=pin>Modify First Axis: </SPAN>When True, the taper modifies the first remaining axis.<BR>
    <SPAN CLASS=pin>Modify Second Axis: </SPAN>When True, the taper modifies the second remaining axis.<BR>
    <SPAN CLASS=pin>Reset Mesh: </SPAN>if TRUE, the mesh is resetted to its initial conditions at activation.<BR>
    */
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0xe971a6b, 0x210f703a));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00020000);
    od->SetCreationFunction(CreateTaperProto);
    od->SetCompatibleClassId(CKCID_3DOBJECT);
    od->SetCategory("Mesh Modifications/Deformation");
    return od;
}

CKERROR CreateTaperProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = NULL;
    proto = CreateCKBehaviorPrototype("Taper");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Amount", CKPGUID_FLOAT, "0.1");
    proto->DeclareInParameter("Curve", CKPGUID_FLOAT, "0.0");
    // To change by Radio buttons
    proto->DeclareInParameter("Axis", CKPGUID_VECTOR, "0,0,1");
    proto->DeclareInParameter("Symmetry", CKPGUID_BOOL, "FALSE");
    proto->DeclareInParameter("Modify First Axis", CKPGUID_BOOL, "TRUE");
    proto->DeclareInParameter("Modify Second Axis", CKPGUID_BOOL, "TRUE");
    proto->DeclareInParameter("Reset Mesh", CKPGUID_BOOL, "TRUE");
    proto->DeclareInParameter("Normal Recalculation", CKPGUID_BOOL, "TRUE");

    proto->DeclareLocalParameter("data", CKPGUID_VOIDBUF);
    proto->DeclareLocalParameter("initialBox", CKPGUID_BOX);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(Taper);

    proto->SetBehaviorCallbackFct(MeshModificationsCallBack);
    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

    *pproto = proto;
    return CK_OK;
}

int Taper(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    // we get the camera entity
    CK3dEntity *ent = (CK3dEntity *)beh->GetTarget();
    if (!ent)
        return 0;

    // we get the amount value
    float amount;
    beh->GetInputParameterValue(0, &amount);

    // we get the curve value
    float curve;
    beh->GetInputParameterValue(1, &curve);

    // we get the axis
    VxVector axis;
    beh->GetInputParameterValue(2, &axis);

    int naxis = 0;
    int i = 0;
    while (axis[naxis] == 0.0f)
        naxis++;

    // we get the sym
    CKBOOL sym;
    beh->GetInputParameterValue(3, &sym);

    // we get the doX
    CKBOOL doX;
    beh->GetInputParameterValue(4, &doX);

    // we get the doY
    CKBOOL doY;
    beh->GetInputParameterValue(5, &doY);

    // we get the mesh
    CKMesh *mesh = ent->GetCurrentMesh();
    CKDWORD vStride = 0;
    CKBYTE *varray = (CKBYTE *)mesh->GetModifierVertices(&vStride);
    int pointsNumber = mesh->GetModifierVertexCount();

    CKBOOL resetmesh = TRUE;
    beh->GetInputParameterValue(6, &resetmesh);
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

    VxBbox bbox;
    beh->GetLocalParameterValue(1, &bbox);
    float l;
    l = (bbox.Max[naxis] - bbox.Min[naxis]);
    if (l == float(0))
        return 0;
    l = -l;

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
        mat = VxMatrix::Identity();
    }
    break; // z
    }
    VxMatrix invmat;
    Vx3DInverseMatrix(invmat, mat);

    VxVector ste;
    // we move all the points
    float z, f;
    VxVector v;
    for (i = 0; i < pointsNumber; i++, varray += vStride)
    {
        Vx3DMultiplyMatrixVector(&v, mat, (VxVector *)varray);
        z = v.z / l;
        if (sym && z < 0.0f)
            z = -z;
        f = float(1.0) + z * amount + curve * z * (float(1.0) - z);
        if (doX)
            v.x *= f;
        if (doY)
            v.y *= f;
        Vx3DMultiplyMatrixVector((VxVector *)varray, invmat, &v);
    }

    CKBOOL normalc = TRUE;
    beh->GetInputParameterValue(7, &normalc);

    mesh->ModifierVertexMove(normalc, TRUE);

    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0, TRUE);

    return CKBR_OK;
}
