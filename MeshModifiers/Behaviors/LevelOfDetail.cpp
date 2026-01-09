/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            LevelOfDetail
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKObjectDeclaration *FillBehaviorLevelOfDetailDecl();
CKERROR CreateLevelOfDetailProto(CKBehaviorPrototype **);
int LevelOfDetail(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorLevelOfDetailDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Level Of Detail");
    od->SetDescription("Chooses the mesh with the appropriate level of detail according to the distance of the camera to the object.");
    od->SetCategory("Optimizations/Level Of Detail");
    /* rem:
    <SPAN CLASS=in>On: </SPAN>activates the process.<BR>
    <SPAN CLASS=in>Off: </SPAN>deactivates the process.<BR>
    <BR>
    <SPAN CLASS=out>Exit On: </SPAN>is activated if the building block is activated.<BR>
    <SPAN CLASS=out>Exit Off: </SPAN>is activated if the building block is deactivated.<BR>
    <BR>
    <SPAN CLASS=pin>Min Distance: </SPAN>minimum distance from which the most detailled mesh is displayed.<BR>
    <SPAN CLASS=pin>Max Distance: </SPAN>maximum distance after which the least detailled mesh is displayed.<BR>
    <BR>
    The mesh with the highest level of detail will be shown when the nearest the object is. When moving away from the object, transitional meshes are swapped so that as distance increases, the level of detail decreases in order to save computational power.<BR>
    Typically, you'll have to use the 'Add Mesh' behavior to add meshes to the object.<BR>
    Therefore you'd be able to use the 'Level Of Detail' building block, which will choose between them all, to render the appropriate one, according to its remotness.
    */
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x21465f42, 0x194c71a1));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateLevelOfDetailProto);
    od->SetCompatibleClassId(CKCID_3DOBJECT);
    return od;
}

CKERROR CreateLevelOfDetailProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = NULL;
    proto = CreateCKBehaviorPrototype("Level Of Detail");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("On");
    proto->DeclareInput("Off");
    proto->DeclareOutput("Exit On");
    proto->DeclareOutput("Exit Off");

    proto->DeclareInParameter("Distance Min", CKPGUID_FLOAT, "100");
    proto->DeclareInParameter("Distance Max", CKPGUID_FLOAT, "1000");

    proto->SetFlags((CK_BEHAVIORPROTOTYPE_FLAGS)(CK_BEHAVIORPROTOTYPE_NORMAL | CK_BEHAVIORPROTOTYPE_OBSOLETE));
    proto->SetFunction(LevelOfDetail);

    *pproto = proto;
    return CK_OK;
}

int LevelOfDetail(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    if (beh->IsInputActive(1)) // we get by the off input
    {
        beh->ActivateInput(1, FALSE);
        beh->ActivateOutput(1);
        return CKBR_OK;
    }
    else
    {
        if (beh->IsInputActive(0)) // we get by the On input
        {
            beh->ActivateInput(0, FALSE);
            beh->ActivateOutput(0);
        }
    }
    // SET IO
    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0);

    CK3dEntity *ent = (CK3dEntity *)beh->GetTarget();
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
