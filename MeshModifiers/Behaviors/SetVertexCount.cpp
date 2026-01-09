/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            SetVertexCount
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKERROR CreateSetVertexCountBehaviorProto(CKBehaviorPrototype **);
int SetVertexCount(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorSetVertexCountDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Set Vertex Count");
    od->SetDescription("Set the number of Vertexs of a mesh.");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process.<BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Vertex Count: </SPAN>Number of Vertexs of the mesh.<BR>
    <BR>
    */
    od->SetCategory("Mesh Modifications/Basic");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x4bf4352e, 0x6c786db2));
    od->SetAuthorGuid(CKGUID());
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateSetVertexCountBehaviorProto);
    od->SetCompatibleClassId(CKCID_MESH);
    return od;
}

CKERROR CreateSetVertexCountBehaviorProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Set Vertex Count");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Vertex Count", CKPGUID_INT, "100");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);

    proto->SetFunction(SetVertexCount);
    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

    *pproto = proto;
    return CK_OK;
}

int SetVertexCount(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0);

    CKMesh *mesh = (CKMesh *)beh->GetTarget();
    if (!mesh)
        return CKBR_OWNERERROR;

    int Vertexc = 100;
    beh->GetInputParameterValue(0, &Vertexc);

    mesh->SetVertexCount(Vertexc);

    return CKBR_OK;
}
