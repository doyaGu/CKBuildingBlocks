/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            RemoveMesh
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKObjectDeclaration *FillBehaviorRemoveMeshDecl();
CKERROR CreateRemoveMeshProto(CKBehaviorPrototype **);
int RemoveMesh(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorRemoveMeshDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Remove Mesh");
    od->SetDescription("Removes a mesh from the selected 3d Entity.");
    /* rem:
    <SPAN CLASS=in>In : </SPAN>triggers the process.<BR>
    <SPAN CLASS=out>Out : </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Mesh : </SPAN>mesh to remove.<BR>
    */
    /* warning:
    - This buidling block removes only the reference to the inner mesh. It does not remove the mesh from the Level nor from the memory.<BR>
    */
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x2145e, 0x6e1b2592));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateRemoveMeshProto);
    od->SetCompatibleClassId(CKCID_3DOBJECT);
    od->SetCategory("Mesh Modifications/Multi Mesh");
    return od;
}

CKERROR CreateRemoveMeshProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = NULL;
    proto = CreateCKBehaviorPrototype("Remove Mesh");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Mesh", CKPGUID_MESH);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(RemoveMesh);
    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

    *pproto = proto;
    return CK_OK;
}

int RemoveMesh(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    // SET IO
    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0);

    CK3dEntity *ent = (CK3dEntity *)beh->GetTarget();
    if (!ent)
        return CKBR_OWNERERROR;

    CKMesh *mesh = (CKMesh *)beh->GetInputParameterObject(0);

    ent->RemoveMesh(mesh);

    return CKBR_OK;
}
