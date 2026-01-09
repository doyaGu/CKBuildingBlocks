/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            SetPatchMeshSteps
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKObjectDeclaration *FillBehaviorSetPatchMeshStepsDecl();
CKERROR CreateSetPatchMeshStepsProto(CKBehaviorPrototype **);
int SetPatchMeshSteps(const CKBehaviorContext &behcontext);
CKERROR MeshModificationsCallBack(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorSetPatchMeshStepsDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Set PatchMesh Steps");
    od->SetDescription("Sets number of steps used to render a PatchMesh.");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process.<BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Steps: </SPAN>number of steps used to tesselate the mesh.<BR>
    <BR>
    */
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x50c12830, 0x18033a52));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateSetPatchMeshStepsProto);
    od->SetCompatibleClassId(CKCID_PATCHMESH);
    od->SetCategory("Mesh Modifications/Deformation");
    return od;
}

CKERROR CreateSetPatchMeshStepsProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = NULL;
    proto = CreateCKBehaviorPrototype("Set PatchMesh Steps");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Steps", CKPGUID_INT, "1");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(SetPatchMeshSteps);
    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

    *pproto = proto;
    return CK_OK;
}

int SetPatchMeshSteps(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;
    CKObject *obj = beh->GetTarget();
    CKPatchMesh *pmesh = (CKPatchMesh *)obj;
    if (beh->GetCompatibleClassID() == CKCID_3DOBJECT) // old version that applies to a 3d object
    {
        pmesh = (CKPatchMesh *)((CK3dEntity *)obj)->GetCurrentMesh();
        if (!CKIsChildClassOf(pmesh, CKCID_PATCHMESH))
            pmesh = NULL;
    }

    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0, TRUE);

    if (!pmesh)
        return CKBR_OK;

    // we get the angle
    int steps = 1;
    beh->GetInputParameterValue(0, &steps);

    pmesh->SetIterationCount(steps);

    return CKBR_OK;
}
