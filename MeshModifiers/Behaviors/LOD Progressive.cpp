/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            LODProgressive
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKObjectDeclaration *FillBehaviorLODProgressiveDecl();
CKERROR CreateLODProgressiveProto(CKBehaviorPrototype **);
int LODProgressive(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorLODProgressiveDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("LOD Progressive");
    od->SetDescription("Decimate the mesh continously according to the distance of the camera to the object.");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process<BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Min Distance: </SPAN>minimum distance from which the most detailled mesh is displayed.<BR>
    <SPAN CLASS=pin>Max Distance: </SPAN>maximum distance after which the least detailled mesh is displayed.<BR>
    <SPAN CLASS=pin>Min Vertex Count: </SPAN>lowest number of vertices to be displayed.<BR>
    <BR>
    This behavior transform the mesh into a progressive mesh, if it was not already done.<BR>
    Warning: This behavior doesn't work well if the mesh is shared between objects.<BR>
    */
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x50426f49, 0x434e6fef));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateLODProgressiveProto);
    od->SetCompatibleClassId(CKCID_3DOBJECT);
    od->SetCategory("Optimizations/Level Of Detail");
    return od;
}

CKERROR CreateLODProgressiveProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = NULL;
    proto = CreateCKBehaviorPrototype("LOD Progressive");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Distance Min", CKPGUID_FLOAT, "100");
    proto->DeclareInParameter("Distance Max", CKPGUID_FLOAT, "1000");
    proto->DeclareInParameter("Minimum Vertex Count", CKPGUID_INT, "0");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(LODProgressive);
    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

    *pproto = proto;
    return CK_OK;
}

int LODProgressive(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    // SET IO
    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0);

    CK3dEntity *ent = (CK3dEntity *)beh->GetTarget();
    if (!ent)
        return CKBR_OWNERERROR;

    CKMesh *mesh = (CKMesh *)ent->GetCurrentMesh();
    // No patch mesh support for now
    if (CKIsChildClassOf(mesh, CKCID_PATCHMESH))
        return CKBR_PARAMETERERROR;

    // we force the PM creation. Nothing its done if it was already a pm
    mesh->CreatePM();

    float distmin = 100.0f;
    beh->GetInputParameterValue(0, &distmin);

    float distmax = 1000.0f;
    beh->GetInputParameterValue(1, &distmax);

    int minvc = 0;
    beh->GetInputParameterValue(2, &minvc);

    // we get the viewer position
    CK3dEntity *cam = behcontext.CurrentRenderContext->GetViewpoint();

    VxVector campos;
    ent->GetPosition(&campos, cam);

    float distance = Magnitude(campos);

    float index = 1.0f - (distance - distmin) / (distmax - distmin);
    int vtr = (int)(index * mesh->GetVertexCount());
    if (vtr < minvc)
        vtr = minvc;

    mesh->SetVerticesRendered(vtr);

    return CKBR_OK;
}