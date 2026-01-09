/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            AddMesh
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKObjectDeclaration *FillBehaviorAddMeshDecl();
CKERROR CreateAddMeshProto(CKBehaviorPrototype **);
int AddMesh(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorAddMeshDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Add Mesh");
    od->SetDescription("Adds a mesh to the selected 3d Entity.");
    /* rem:
    <SPAN CLASS=in>In : </SPAN>triggers the process.<BR>
    <SPAN CLASS=out>Out : </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Mesh : </SPAN>mesh to be added.<BR>
    <BR>
    Add as many mesh as you want to the selected 3d Entity and then, use 'Select Mesh' to select the displayed one.<BR>
    Useful to choose between several representation of an object at runtime (like a crushed car after a collision).<BR>
    See Also : Select Mesh, Remove Mesh<BR>
    */
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x7a1d593e, 0x12c5648d));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateAddMeshProto);
    od->SetCompatibleClassId(CKCID_3DOBJECT);
    od->SetCategory("Mesh Modifications/Multi Mesh");
    return od;
}

CKERROR CreateAddMeshProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = NULL;
    proto = CreateCKBehaviorPrototype("Add Mesh");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Mesh", CKPGUID_MESH);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(AddMesh);
    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

    *pproto = proto;
    return CK_OK;
}

int AddMesh(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    // SET IO
    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0);

    CK3dEntity *ent = (CK3dEntity *)beh->GetTarget();
    if (!ent)
        return CKBR_OWNERERROR;

    CKMesh *mesh = (CKMesh *)beh->GetInputParameterObject(0);

    ent->AddMesh(mesh);

    return CKBR_OK;
}
