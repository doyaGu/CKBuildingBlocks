/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            LODAlpha
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKObjectDeclaration *FillBehaviorLODAlphaDecl();
CKERROR CreateLODAlphaProto(CKBehaviorPrototype **);
int LODAlpha(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorLODAlphaDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("LOD Alpha");
    od->SetDescription("Chooses the mesh with the appropriate level of detail according to the distance of the camera to the object.");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process<BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Min Distance: </SPAN>minimum distance from which the most detailled mesh is displayed.<BR>
    <SPAN CLASS=pin>Max Distance: </SPAN>maximum distance after which the least detailled mesh is displayed.<BR>
    <BR>
    */
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x737e5f8e, 0x29570686));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateLODAlphaProto);
    od->SetCompatibleClassId(CKCID_3DOBJECT);
    od->SetCategory("Optimizations/Level Of Detail");
    return od;
}

CKERROR CreateLODAlphaProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = NULL;
    proto = CreateCKBehaviorPrototype("LOD Alpha");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Distance Min", CKPGUID_FLOAT, "100");
    proto->DeclareInParameter("Distance Max", CKPGUID_FLOAT, "1000");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(LODAlpha);
    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

    *pproto = proto;
    return CK_OK;
}

void MakeTransparentFunc(CK3dEntity *ent, int smode, int dmode, float alpha)
{
    VxColor col;
    CKDWORD dcol;
    int i;
    for (i = 0; i < ent->GetMeshCount(); i++)
    {
        CKMesh *mesh = ent->GetMesh(i);
        mesh->SetTransparent(TRUE);
        if (mesh->GetLitMode() == VX_PRELITMESH)
        {
            for (int j = 0; j < mesh->GetVertexCount(); j++)
            {
                dcol = mesh->GetVertexColor(j);
                dcol = ColorSetAlpha(dcol, (unsigned char)(255 * alpha));
                mesh->SetVertexColor(j, dcol);
            }
        }

        for (int j = 0; j < mesh->GetMaterialCount(); ++j)
        {
            CKMaterial *mat = mesh->GetMaterial(j);
            mat->SetSourceBlend((VXBLEND_MODE)smode);
            mat->SetDestBlend((VXBLEND_MODE)dmode);
            mat->EnableAlphaBlend(TRUE);
            mat->EnableZWrite(FALSE);
            col = mat->GetDiffuse();
            col.a = alpha;
            mat->SetDiffuse(col);
        }
    }
}

void MakeOpaqueFunc(CK3dEntity *ent)
{
    VxColor col;
    CKDWORD dcol;
    int i;
    for (i = 0; i < ent->GetMeshCount(); i++)
    {
        CKMesh *mesh = ent->GetMesh(i);
        mesh->SetTransparent(TRUE);
        if (mesh->GetLitMode() == VX_PRELITMESH)
        {
            for (int j = 0; j < mesh->GetVertexCount(); j++)
            {
                dcol = mesh->GetVertexColor(j);
                dcol = ColorSetAlpha(dcol, 255);
                mesh->SetVertexColor(j, dcol);
            }
        }

        for (int j = 0; j < mesh->GetMaterialCount(); ++j)
        {
            CKMaterial *mat = mesh->GetMaterial(j);
            mat->EnableAlphaBlend(FALSE);
            mat->EnableZWrite(TRUE);
            col = mat->GetDiffuse();
            col.a = 1.0f;
            mat->SetDiffuse(col);
        }
    }
}

int LODAlpha(const CKBehaviorContext &behcontext)
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

    if (index == 0)
    {
        MakeOpaqueFunc(ent);
    }
    else if (index == 1.0f)
    {
        ent->Show(CKHIDE);
    }
    else
    {
        ent->Show(CKSHOW);
        MakeTransparentFunc(ent, VXBLEND_SRCALPHA, VXBLEND_INVSRCALPHA, 1.0f - index);
    }

    return CKBR_ACTIVATENEXTFRAME;
}
