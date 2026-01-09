/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            SetFaceCount
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKERROR CreateSetFaceCountBehaviorProto(CKBehaviorPrototype **);
int SetFaceCount(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorSetFaceCountDecl()
{
	CKObjectDeclaration *od = CreateCKObjectDeclaration("Set Face Count");
	od->SetDescription("Set the number of faces of a mesh.");
	/* rem:
	<SPAN CLASS=in>In: </SPAN>triggers the process.<BR>
	<SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
	<BR>
	<SPAN CLASS=pin>Face Count: </SPAN>Number of faces of the mesh.<BR>
	<BR>
	*/
	od->SetCategory("Mesh Modifications/Basic");
	od->SetType(CKDLL_BEHAVIORPROTOTYPE);
	od->SetGuid(CKGUID(0x15a14196, 0x69530840));
	od->SetAuthorGuid(CKGUID());
	od->SetAuthorName("Virtools");
	od->SetVersion(0x00010000);
	od->SetCreationFunction(CreateSetFaceCountBehaviorProto);
	od->SetCompatibleClassId(CKCID_MESH);
	return od;
}

CKERROR CreateSetFaceCountBehaviorProto(CKBehaviorPrototype **pproto)
{
	CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Set Face Count");
	if (!proto)
		return CKERR_OUTOFMEMORY;

	proto->DeclareInput("In");
	proto->DeclareOutput("Out");

	proto->DeclareInParameter("Face Count", CKPGUID_INT, "100");

	proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);

	proto->SetFunction(SetFaceCount);
	proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

	*pproto = proto;
	return CK_OK;
}

int SetFaceCount(const CKBehaviorContext &behcontext)
{
	CKBehavior *beh = behcontext.Behavior;

	beh->ActivateInput(0, FALSE);
	beh->ActivateOutput(0);

	CKMesh *mesh = (CKMesh *)beh->GetTarget();
	if (!mesh)
		return CKBR_OWNERERROR;

	int facec = 100;
	beh->GetInputParameterValue(0, &facec);

	mesh->SetFaceCount(facec);

	return CKBR_OK;
}
