/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            SelectMesh
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKObjectDeclaration *FillBehaviorSelectMeshDecl();
CKERROR CreateSelectMeshProto(CKBehaviorPrototype **);
int SelectMesh(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorSelectMeshDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Select Mesh");
    od->SetDescription("Selects which mesh should be used to represent the 3D entity.");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process.<BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Mesh : </SPAN>mesh to be displayed.<BR>
    <SPAN CLASS=pin>Add If Missing: </SPAN>tells whether you want the mesh to be added to the 3dEntity (if it hadn't been yet added with 'AddMesh'), or not.<BR>
    */
    /* warning:
    - If the selected mesh does not belong to the 3D Entity (i.e: if it hasn't been referenced with 'AddMesh') and if you don't want it to be added, then you won't be able to see any change.
    */
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x43890176, 0x58b2244e));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateSelectMeshProto);
    od->SetCompatibleClassId(CKCID_3DOBJECT);
    od->SetCategory("Mesh Modifications/Multi Mesh");
    return od;
}

CKERROR CreateSelectMeshProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = NULL;
    proto = CreateCKBehaviorPrototype("Select Mesh");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Mesh", CKPGUID_MESH);
    proto->DeclareInParameter("Add If Missing", CKPGUID_BOOL, "TRUE");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(SelectMesh);
    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

    *pproto = proto;
    return CK_OK;
}

int SelectMesh(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    // SET IO
    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0);

    CK3dEntity *ent = (CK3dEntity *)beh->GetTarget();
    if (!ent)
        return CKBR_OWNERERROR;

    CKMesh *mesh = (CKMesh *)beh->GetInputParameterObject(0);
    CKBOOL b = TRUE;
    beh->GetInputParameterValue(1, &b);

    ent->SetCurrentMesh(mesh, b);

    return CKBR_OK;
}
