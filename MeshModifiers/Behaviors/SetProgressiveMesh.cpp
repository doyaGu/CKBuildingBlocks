/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            SetProgressiveMeshOptions
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKObjectDeclaration *FillBehaviorSetProgressiveMeshOptionsDecl();
CKERROR CreateSetProgressiveMeshOptionsProto(CKBehaviorPrototype **);
int SetProgressiveMeshOptions(const CKBehaviorContext &behcontext);
CKERROR MeshModificationsCallBack(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorSetProgressiveMeshOptionsDecl()
{
	CKObjectDeclaration *od = CreateCKObjectDeclaration("Set Progressive Mesh Options");
	od->SetDescription("Sets the options of the progressive mesh.");
	/* rem:
	<SPAN CLASS=in>In: </SPAN>triggers the process.<BR>
	<SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
	<BR>
	<SPAN CLASS=pin>Active: </SPAN>should the progressive mesh be active for this mesh.<BR>
	<SPAN CLASS=pin>GeoMorph: </SPAN>should the geomorphic transition be enabled.<BR>
	<SPAN CLASS=pin>GeoMorph Steps: </SPAN>geomorphic steps (0 means dichotomic transitions).<BR>
	<BR>
	Activating the progressive mesh can take a long time, depending on the mesh complexity.
	*/
	od->SetType(CKDLL_BEHAVIORPROTOTYPE);
	od->SetGuid(CKGUID(0x10357830, 0x18099a52));
	od->SetAuthorGuid(VIRTOOLS_GUID);
	od->SetAuthorName("Virtools");
	od->SetVersion(0x00010000);
	od->SetCreationFunction(CreateSetProgressiveMeshOptionsProto);
	od->SetCompatibleClassId(CKCID_MESH);
	od->SetCategory("Optimizations/Level Of Detail");
	return od;
}

CKERROR CreateSetProgressiveMeshOptionsProto(CKBehaviorPrototype **pproto)
{
	CKBehaviorPrototype *proto = NULL;
	proto = CreateCKBehaviorPrototype("Set Progressive Mesh Options");
	if (!proto)
		return CKERR_OUTOFMEMORY;

	proto->DeclareInput("In");
	proto->DeclareOutput("Out");

	proto->DeclareInParameter("Active", CKPGUID_BOOL, "TRUE");
	proto->DeclareInParameter("GeoMorph", CKPGUID_BOOL, "FALSE");
	proto->DeclareInParameter("GeoMorph Steps", CKPGUID_INT, "0");

	proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
	proto->SetFunction(SetProgressiveMeshOptions);
	proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

	*pproto = proto;
	return CK_OK;
}

int SetProgressiveMeshOptions(const CKBehaviorContext &behcontext)
{
	CKBehavior *beh = behcontext.Behavior;
	CKMesh *mesh = (CKMesh *)beh->GetTarget();
	if (!mesh)
		return CKBR_OK;

	beh->ActivateInput(0, FALSE);
	beh->ActivateOutput(0, TRUE);

	// we get the angle
	CKBOOL active = TRUE;
	beh->GetInputParameterValue(0, &active);

	CKBOOL geo = FALSE;
	beh->GetInputParameterValue(1, &geo);

	int steps = 0;
	beh->GetInputParameterValue(2, &steps);

	if (active)
		mesh->CreatePM();
	else
		mesh->DestroyPM();

	mesh->EnablePMGeoMorph(geo);

	mesh->SetPMGeoMorphStep(steps);

	return CKBR_OK;
}
