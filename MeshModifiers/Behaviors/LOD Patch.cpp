/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            LODPatch
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKObjectDeclaration *FillBehaviorLODPatchDecl();
CKERROR CreateLODPatchProto(CKBehaviorPrototype **);
int LODPatch(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorLODPatchDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("LOD Patch");
    od->SetDescription("Chooses the mesh with the appropriate level of detail according to the distance of the camera to the object.");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process<BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Min Distance: </SPAN>minimum distance from which the most detailled mesh is displayed.<BR>
    <SPAN CLASS=pin>Max Distance: </SPAN>maximum distance after which the least detailled mesh is displayed.<BR>
    <SPAN CLASS=pin>Max Tesselation Level: </SPAN>maximum tesselation of the patch mesh, used within the minimu range.<BR>
    <BR>
    */
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x5ba9568e, 0x36420409));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateLODPatchProto);
    od->SetCompatibleClassId(CKCID_3DOBJECT);
    od->SetCategory("Optimizations/Level Of Detail");
    return od;
}

CKERROR CreateLODPatchProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = NULL;
    proto = CreateCKBehaviorPrototype("LOD Patch");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Distance Min", CKPGUID_FLOAT, "100");
    proto->DeclareInParameter("Distance Max", CKPGUID_FLOAT, "1000");
    proto->DeclareInParameter("Max Tesselation Level", CKPGUID_INT, "5");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(LODPatch);
    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

    *pproto = proto;
    return CK_OK;
}

int LODPatch(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    // SET IO
    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0);

    CK3dEntity *ent = (CK3dEntity *)beh->GetTarget();
    if (!ent)
        return CKBR_OWNERERROR;

    CKPatchMesh *pmesh = (CKPatchMesh *)ent->GetCurrentMesh();
    if (!CKIsChildClassOf(pmesh, CKCID_PATCHMESH))
        return CKBR_OK;

    float distmin = 100.0f;
    beh->GetInputParameterValue(0, &distmin);

    float distmax = 1000.0f;
    beh->GetInputParameterValue(1, &distmax);

    int tesselate = 5;
    beh->GetInputParameterValue(2, &tesselate);

    // we get the viewer position
    CK3dEntity *cam = behcontext.CurrentRenderContext->GetViewpoint();

    VxVector campos;
    ent->GetPosition(&campos, cam);

    float distance = Magnitude(campos);

    float index = (distance - distmin) / (distmax - distmin);
    if (index < 0)
        index = 0;
    if (index > 1)
        index = 1;
    int mindex = (int)(tesselate * (1 - index));

    pmesh->SetIterationCount(mindex);

    return CKBR_OK;
}
