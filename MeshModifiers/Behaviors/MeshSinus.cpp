/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            MeshSinus
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKERROR CreateMeshSinusBehaviorProto(CKBehaviorPrototype **);
int DoSinus(const CKBehaviorContext &behcontext);
CKERROR MeshModificationsCallBack(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorMeshSinusDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Sine Deform");
    od->SetDescription("Applies a time-dependent sinusoidal deformation to each vertice of the mesh.");
    /* rem:
    <SPAN CLASS=in>In : </SPAN>triggers the process.<BR>
    <SPAN CLASS=out>Out : </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Deform Axis: </SPAN>sets the amount of deformation on each local axis.<BR>
    <SPAN CLASS=pin>Magnitude: </SPAN>global amount of deformation.<BR>
    <SPAN CLASS=pin>Wave Length: </SPAN>wave length of the sine fonction.<BR>
    <SPAN CLASS=pin>Reset Mesh: </SPAN>if TRUE, the mesh is resetted to its initial conditions at activation.<BR>
    <BR>
    Loop this behavior to observe organic deformations.
    */
    /* warning:
    - The initial Mesh is saved when the building block is attached
    to the 3dObject (that's why we can't applied this building block to another 3d Object than the one
    the building block is applied to), therefore you needn't to put Initials Conditions to the mesh if
    you wish to recover the initial mesh.
    - Beware, not all the building block works with this specification
    */
    od->SetCategory("Mesh Modifications/Deformation");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x45123704, 0x72f25769));
    od->SetAuthorGuid(CKGUID());
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00020000);
    od->SetCreationFunction(CreateMeshSinusBehaviorProto);
    od->SetCompatibleClassId(CKCID_3DOBJECT);
    return od;
}

CKERROR DaMeshSinusCB(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    switch (behcontext.CallbackMessage)
    {
    case CKM_BEHAVIORATTACH:
    {
        CK3dEntity *object = (CK3dEntity *)beh->GetTarget();
        if (!object)
            return 0;
        CKMesh *mesh = object->GetCurrentMesh();
        if (!mesh)
            return 0;
    }
    case CKM_BEHAVIORLOAD:
    case CKM_BEHAVIORRESET:
    {
        float time = 0.0f;
        beh->SetLocalParameterValue(0, &time);

        return 0;
    }
    break;

    case CKM_BEHAVIORDELETE:
    {
        if (beh->GetVersion() < 0x00020000)
        { // Old Version
            // we get the initial mesh vertices
            CK3dEntity *object = (CK3dEntity *)beh->GetTarget();
            if (!object)
                return 0;
            CKMesh *mesh = object->GetCurrentMesh();
            if (!mesh)
                return 0;

            int nbvert = mesh->GetModifierVertexCount();
            CKDWORD vStride = 0;
            CKBYTE *varray = (CKBYTE *)mesh->GetModifierVertices(&vStride);

            VxVector *savePos = (VxVector *)beh->GetLocalParameterWriteDataPtr(1);
            if (!savePos)
                return 0;

            for (int i = 0; i < nbvert; i++, varray += vStride)
            {
                *(VxVector *)varray = savePos[i];
            }

            mesh->ModifierVertexMove(TRUE, TRUE);
        }
    }
    break;
    }
    return CKBR_OK;
}

CKERROR CreateMeshSinusBehaviorProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Sine Deform");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Deform Axis", CKPGUID_VECTOR, "1,1,1");
    proto->DeclareInParameter("Magnitude", CKPGUID_FLOAT, "1");
    proto->DeclareInParameter("Wave Length", CKPGUID_FLOAT, "1");
    proto->DeclareInParameter("Reset Mesh", CKPGUID_BOOL, "TRUE");

    proto->DeclareLocalParameter("Time", CKPGUID_FLOAT);
    proto->DeclareLocalParameter("Vertex Array", CKPGUID_VOIDBUF);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);

    proto->SetFunction(DoSinus);
    proto->SetBehaviorCallbackFct(DaMeshSinusCB);

    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

    *pproto = proto;
    return CK_OK;
}

int DoSinus(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    CK3dEntity *object = (CK3dEntity *)beh->GetTarget();
    CKMesh *mesh = object->GetCurrentMesh();
    if (!mesh)
        return CKBR_OWNERERROR;

    float time;
    beh->GetLocalParameterValue(0, &time);

    VxVector vect;
    beh->GetInputParameterValue(0, &vect);

    float str;
    beh->GetInputParameterValue(1, &str);

    float l;
    beh->GetInputParameterValue(2, &l);

    CKDWORD vStride = 0;
    CKBYTE *vxv = (CKBYTE *)mesh->GetModifierVertices(&vStride);
    int vcount = mesh->GetModifierVertexCount();

    CKBOOL resetmesh = TRUE;
    beh->GetInputParameterValue(3, &resetmesh);
    if (resetmesh)
    { // The Mesh must be resetted
        if (beh->GetVersion() < 0x00020000)
        { // Old Version with vertices stuffed inside
            // we get the saved position
            VxVector *savePos = (VxVector *)beh->GetLocalParameterWriteDataPtr(1);

            CKBYTE *temparray = vxv;
            for (int i = 0; i < vcount; i++, temparray += vStride)
            {
                *(VxVector *)temparray = savePos[i];
            }
        }
        else
        { // new version : based on ICs
            CKScene *scn = behcontext.CurrentScene;
            CKStateChunk *chunk = scn->GetObjectInitialValue(mesh);
            if (chunk)
                CKReadObjectState(mesh, chunk);
            vxv = (CKBYTE *)mesh->GetModifierVertices(&vStride);
            vcount = mesh->GetModifierVertexCount();
        }
    }

    for (int i = 0; i < vcount; i++, vxv += vStride)
    {
        VxVector *v = (VxVector *)vxv;
        v->x = v->x + v->x * vect.x * str * (float)cos(l * time + v->x);
        v->y = v->y + v->y * vect.y * str * (float)cos(l * time + v->y);
        v->z = v->z + v->z * vect.z * str * (float)cos(l * time + v->z);
    }

    time += (behcontext.DeltaTime * 0.001f);

    if (l * time > 2 * PI)
        time -= (2 * PI / l);

    beh->SetLocalParameterValue(0, &time);

    // reinitiate the Bounding Box
    mesh->ModifierVertexMove(TRUE, TRUE);

    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0);

    return CKBR_OK;
}
