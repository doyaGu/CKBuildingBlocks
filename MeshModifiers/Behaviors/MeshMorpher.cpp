/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            MeshMorpher
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKObjectDeclaration *FillBehaviorMeshMorpherDecl();
CKERROR CreateMeshMorpherProto(CKBehaviorPrototype **);
int MeshMorpher(const CKBehaviorContext &behcontext);
CKERROR MeshMorpherCallBackObject(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorMeshMorpherDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Mesh Morpher");
    od->SetDescription("Makes a mesh out of two (that must have the same number of vertices).");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process.<BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Start Mesh: </SPAN>first mesh.<BR>
    <SPAN CLASS=pin>End Mesh: </SPAN>second mesh.<BR>
    <SPAN CLASS=pin>Effect Value: </SPAN>percentage value to determine which mesh takes precedence.<BR>
    "0" the hybrid has the same form as the first, "100" the hybrid has the same form as the second.
    "50" is a even mix of the two meshes.<BR>
    <SPAN CLASS=pin>Calculate Vertex Normals: </SPAN>interpolates the vertices normals too if checked.<BR>
    <SPAN CLASS=pin>Calculate Face Normals: </SPAN>calculates the faces normals too if checked.<BR>
    <BR>
    This behavior takes two meshes in parameter, creates an hybrid mesh of these two using linear interpolation on the vertices and sets it as the current mesh of the attached object.
    If the object is prelit, or if the geometry of the two mesh are quite similar, you can spare a lot of time by not calculating the normals.<BR>
    */
    /*
    warning:
    - the two meshes must have the same number of vertices, and the vertices index must be equal from a topology to an other. To do so in your 3d modeling software (like Max or Softimage), make sure to build the different meshes by always deforming the same initial mesh.<BR>
    */
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0xff0d010a, 0x001d010a));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateMeshMorpherProto);
    od->SetCompatibleClassId(CKCID_3DOBJECT);
    od->SetCategory("Mesh Modifications/Deformation");
    return od;
}

CKERROR CreateMeshMorpherProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = NULL;
    proto = CreateCKBehaviorPrototype("Mesh Morpher");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Start Mesh", CKPGUID_MESH);
    proto->DeclareInParameter("End Mesh", CKPGUID_MESH);
    proto->DeclareInParameter("Effect Value", CKPGUID_PERCENTAGE, "50");
    proto->DeclareInParameter("Calculate Vertices Normals", CKPGUID_BOOL, "FALSE");
    proto->DeclareInParameter("Calculate Faces Normals", CKPGUID_BOOL, "FALSE");

    proto->DeclareLocalParameter("initial mesh", CKPGUID_MESH);
    proto->DeclareLocalParameter("created mesh", CKPGUID_MESH);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(MeshMorpher);

    proto->SetBehaviorCallbackFct(MeshMorpherCallBackObject);

    *pproto = proto;
    return CK_OK;
}

int MeshMorpher(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;
    CKContext *ctx = behcontext.Context;

    // we get the 3d entity
    CK3dEntity *ent = (CK3dEntity *)beh->GetTarget();

    // we get the starting mesh
    CKMesh *sm = (CKMesh *)beh->GetInputParameterObject(0);
    if (!sm)
    {
        behcontext.Context->OutputToConsole("You must provide a starting mesh");
        return CKBR_PARAMETERERROR;
    }

    // we get the ending mesh
    CKMesh *em = (CKMesh *)beh->GetInputParameterObject(1);
    if (!em)
    {
        ctx->OutputToConsole("You must provide an ending mesh");
        return CKBR_PARAMETERERROR;
    }
    if (em->GetClassID() != sm->GetClassID())
    {
        ctx->OutputToConsole("Starting mesh and Ending Mesh must have the same type");
        return CKBR_PARAMETERERROR;
    }

    if (em->GetModifierVertexCount() != sm->GetModifierVertexCount())
    {
        ctx->OutputToConsole("Starting mesh and Ending Mesh must have the same vertices number");
        return CKBR_PARAMETERERROR;
    }

    // we get the mesh on which interpolation will be applied
    CKMesh *im = (CKMesh *)beh->GetLocalParameterObject(1);
    // we keep the old mesh to restore it after
    if (ent->GetCurrentMesh() != im)
    { // if the intepolated mesh wasn't put in place yet
        CKMesh *oldmesh = ent->GetCurrentMesh();
        beh->SetLocalParameterObject(0, oldmesh);
    }

    // we now have to test if the morphing mesh has the same number of vertices
    if (!im || im->GetModifierVertexCount() != sm->GetModifierVertexCount())
    {
        // if The mesh was already created, we destroy it....
        if (im)
            CKDestroyObject(im);
        // We create the morphingmesh
        im = (CKMesh *)ctx->CopyObject(sm, NULL, NULL);
        XString name = im->GetName();
        name << "_Morpher";
        im->SetName(name.Str());
        im->ModifyObjectFlags(CK_OBJECT_NOTTOBELISTEDANDSAVED, 0);
        CKMaterial *mat = sm->GetFaceMaterial(0);
        // We stock the mesh in a local parameter
        beh->SetLocalParameterObject(1, im);
    }
    // We set the mesh as current without adding it to the object
    ent->SetCurrentMesh(im, FALSE);

    CKBOOL vnormal, fnormal;
    beh->GetInputParameterValue(3, &vnormal);
    beh->GetInputParameterValue(4, &fnormal);
    if (CKIsChildClassOf(im, CKCID_PATCHMESH))
        vnormal = FALSE;

    // we get the effect value
    float effect;
    beh->GetInputParameterValue(2, &effect);

    CKDWORD iStride = 0;
    CKBYTE *ivarray = (CKBYTE *)im->GetModifierVertices(&iStride);
    CKDWORD sStride = 0;
    CKBYTE *svarray = (CKBYTE *)sm->GetModifierVertices(&sStride);
    CKDWORD eStride = 0;
    CKBYTE *evarray = (CKBYTE *)em->GetModifierVertices(&eStride);

    CKDWORD inStride = 0;
    CKBYTE *invarray = (CKBYTE *)im->GetNormalsPtr(&inStride);
    CKDWORD snStride = 0;
    CKBYTE *snvarray = (CKBYTE *)sm->GetNormalsPtr(&snStride);
    CKDWORD enStride = 0;
    CKBYTE *envarray = (CKBYTE *)em->GetNormalsPtr(&enStride);

    int pointsNumber = im->GetModifierVertexCount();

    VxVector ste;
    // we move all the points
    for (int i = 0; i < pointsNumber; i++, ivarray += iStride, svarray += sStride, evarray += eStride, invarray += inStride, snvarray += snStride, envarray += enStride)
    {
        ste = *(VxVector *)evarray - *(VxVector *)svarray;
        ste *= effect;

        *(VxVector *)ivarray = *(VxVector *)svarray + ste;
        if (vnormal)
        {
            ste = *(VxVector *)envarray - *(VxVector *)snvarray;
            ste *= effect;

            *(VxVector *)invarray = *(VxVector *)snvarray + ste;
            ((VxVector *)invarray)->Normalize();
        }
    }

    // calculate the face normals
    im->ModifierVertexMove(CKIsChildClassOf(im, CKCID_PATCHMESH), fnormal);

    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0);

    return CKBR_OK;
}

/*******************************************************/
/*                     CALLBACK                        */
/*******************************************************/
CKERROR MeshMorpherCallBackObject(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;
    CK3dObject *object = (CK3dObject *)beh->GetOwner();

    CKMesh *initialmesh, *createdmesh;

    switch (behcontext.CallbackMessage)
    {

    case CKM_BEHAVIORLOAD:
    case CKM_BEHAVIORATTACH:
    {
        initialmesh = object->GetCurrentMesh();
        beh->SetLocalParameterObject(0, initialmesh);
        createdmesh = NULL;
        beh->SetLocalParameterObject(1, createdmesh);
    }
    break;

    case CKM_BEHAVIORPRESAVE:
    {
        // we get the mesh on which interpolation will be applied
        initialmesh = (CKMesh *)beh->GetLocalParameterObject(0);
        object->SetCurrentMesh(initialmesh);
    }
    break;

    case CKM_BEHAVIORPOSTSAVE:
    {
        // we get the mesh on which interpolation will be applied
        createdmesh = (CKMesh *)beh->GetLocalParameterObject(1);
        object->SetCurrentMesh(createdmesh);
    }
    break;

    case CKM_BEHAVIORDELETE:
    {
        initialmesh = (CKMesh *)beh->GetLocalParameterObject(0);
        if (object)
        { // the user press the cancel button
            if (initialmesh)
                object->SetCurrentMesh(initialmesh);
        }
        createdmesh = (CKMesh *)beh->GetLocalParameterObject(1);
        CKDestroyObject(createdmesh);
    }
    break;
    }

    return CKBR_OK;
}
