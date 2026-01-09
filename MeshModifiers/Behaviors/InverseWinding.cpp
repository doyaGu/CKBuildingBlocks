/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            InverseWinding
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKERROR CreateInverseWindingBehaviorProto(CKBehaviorPrototype **);
int InverseWinding(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorInverseWindingDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Invert Winding");
    od->SetDescription("Inverse the orientation of the faces of a mesh.");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process.<BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <BR>
    This behavior also change the normals of the faces and the normal of the vertices.<BR>
    */
    od->SetCategory("Mesh Modifications/Local Deformation");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x31ce61cd, 0x69a44beb));
    od->SetAuthorGuid(CKGUID());
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateInverseWindingBehaviorProto);
    od->SetCompatibleClassId(CKCID_MESH);
    return od;
}

CKERROR CreateInverseWindingBehaviorProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Invert Winding");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(InverseWinding);
    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

    *pproto = proto;
    return CK_OK;
}

int InverseWinding(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0);

    CKMesh *mesh = (CKMesh *)beh->GetTarget();
    if (!mesh)
        return CKBR_OWNERERROR;

    mesh->InverseWinding();

    return CKBR_OK;
}
