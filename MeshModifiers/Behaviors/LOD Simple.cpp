/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            LODSimple
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKObjectDeclaration *FillBehaviorLODSimpleDecl();
CKERROR CreateLODSimpleProto(CKBehaviorPrototype **);
int LODSimple(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorLODSimpleDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("LOD Simple");
    od->SetDescription("Chooses the mesh with the appropriate level of detail according to the distance of the camera to the object.");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process<BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Min Distance: </SPAN>minimum distance from which the most detailled mesh is displayed.<BR>
    <SPAN CLASS=pin>Max Distance: </SPAN>maximum distance after which the least detailled mesh is displayed.<BR>
    <BR>
    The mesh with the highest level of detail will be shown when the nearest the object is. When moving away from the object, transitional meshes are swapped so that as distance increases, the level of detail decreases in order to save computational power.<BR>
    Typically, you'll have to use the 'Add Mesh' behavior to add meshes to the object.<BR>
    Therefore you'd be able to use the 'Level Of Detail' building block, which will choose between them all, to render the appropriate one, according to its remotness.
    */
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x68676637, 0x52d439de));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateLODSimpleProto);
    od->SetCompatibleClassId(CKCID_3DOBJECT);
    od->SetCategory("Optimizations/Level Of Detail");
    return od;
}

CKERROR CreateLODSimpleProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = NULL;
    proto = CreateCKBehaviorPrototype("LOD Simple");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Distance Min", CKPGUID_FLOAT, "100");
    proto->DeclareInParameter("Distance Max", CKPGUID_FLOAT, "1000");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(LODSimple);
    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

    *pproto = proto;
    return CK_OK;
}

int LODSimple(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    // SET IO
    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0);

    CK3dEntity *ent = (CK3dEntity *)beh->GetTarget();
    if (!ent)
        return CKBR_OWNERERROR;

    float distmin = 100.0f;
    beh->GetInputParameterValue(0, &distmin);

    float distmax = 1000.0f;
    beh->GetInputParameterValue(1, &distmax);

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
    int mindex = (int)(index * (ent->GetMeshCount() - 1));

    ent->SetCurrentMesh(ent->GetMesh(mindex), FALSE);

    return CKBR_ACTIVATENEXTFRAME;
}
