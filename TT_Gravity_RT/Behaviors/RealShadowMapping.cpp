/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		         TT RealShadowMapping
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"
#include "RealShadowMappingGeometry.h"

#define TERRATOOLS_GUID CKGUID(0x56495254, 0x4f4f4c53)

#define A_MAX_NUMBER_OF_FLOOR_UNDER_OBJECT 16

CKObjectDeclaration *FillBehaviorRealShadowMappingDecl();
CKERROR CreateRealShadowMappingProto(CKBehaviorPrototype **pproto);
int RealShadowMapping(const CKBehaviorContext &behcontext);
CKERROR RealShadowMappingCallBack(const CKBehaviorContext &behcontext);
void RealShadowMappingRenderCallBack(CKRenderContext *dev, void *arg);

////////////////////////
//   Local Structure
////////////////////////
typedef struct
{
    CK_ID id;
    VxVector min;
    VxVector max;
} RealShadowFloor;

typedef struct
{
    CK_ID matID;                                     // material to be projected
    CK_ID texID;                                     // texture to be associated with the projected material
    RealShadowFloor floor[A_MAX_NUMBER_OF_FLOOR_UNDER_OBJECT]; // floors under the object ...
    int nb_floors_under;                             // number of floors being under the object...
} RealShadowStruct;

void A_GetFloors(RealShadowStruct *tss, CK3dEntity *ent, CKBehavior *beh);
void A_Delete_SoftShadow_From_Floors(RealShadowFloor *floor, CKMaterial *cmat, int nb_floors);
static void A_Delete_SoftShadow_From_FloorIds(const CK_ID *ids, CKMaterial *cmat, int nb_floors);

CKObjectDeclaration *FillBehaviorRealShadowMappingDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("TT RealShadowMapping");
    od->SetDescription("Calculate the real postion of shadow");
    od->SetCategory("TT Gravity");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x452816af, 0x681a3a81));
    od->SetAuthorGuid(TERRATOOLS_GUID);
    od->SetAuthorName("Terratools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateRealShadowMappingProto);
    od->SetCompatibleClassId(CKCID_3DOBJECT);
    return od;
}

CKERROR CreateRealShadowMappingProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("TT RealShadowMapping");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("On");
    proto->DeclareInput("Off");

    proto->DeclareOutput("Exit On");
    proto->DeclareOutput("Exit Off");

    proto->DeclareInParameter("Light-Vector", CKPGUID_VECTOR);
    proto->DeclareInParameter("Texture", CKPGUID_TEXTURE);
    proto->DeclareInParameter("Size", CKPGUID_2DVECTOR, "1,1");
    proto->DeclareInParameter("SourceBlend", CKPGUID_BLENDFACTOR, "Zero");
    proto->DeclareInParameter("DestBlend", CKPGUID_BLENDFACTOR, "Source Color");

    proto->DeclareOutParameter("Eckpunkt1", CKPGUID_VECTOR);
    proto->DeclareOutParameter("Eckpunkt2", CKPGUID_VECTOR);
    proto->DeclareOutParameter("Eckpunkt3", CKPGUID_VECTOR);
    proto->DeclareOutParameter("Eckpunkt4", CKPGUID_VECTOR);
    proto->DeclareOutParameter("Mittelpunkt", CKPGUID_VECTOR);

    proto->DeclareLocalParameter("Data", CKPGUID_VOIDBUF);
    proto->DeclareLocalParameter("Active", CKPGUID_BOOL);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(RealShadowMapping);

    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);
    proto->SetBehaviorCallbackFct(RealShadowMappingCallBack);

    *pproto = proto;
    return CK_OK;
}

int RealShadowMapping(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;
    CKRenderContext *dev = behcontext.CurrentRenderContext;
    if (!dev)
        return CKBR_PARAMETERERROR;

    if (!beh->GetTarget())
        return CKBR_OWNERERROR;

    dev->AddPreRenderCallBack(RealShadowMappingRenderCallBack, beh, TRUE);

    CKBOOL active = FALSE;
    beh->GetLocalParameterValue(1, &active);
    if (!active)
        return CKBR_ACTIVATENEXTFRAME;

    dev->RemovePreRenderCallBack(RealShadowMappingRenderCallBack, beh);

    return CKBR_OK;
}

CKERROR RealShadowMappingCallBack(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;
    CKContext *ctx = behcontext.Context;

    switch (behcontext.CallbackMessage)
    {
    case CKM_BEHAVIORATTACH:
    case CKM_BEHAVIORLOAD:
    {
        // creation of a CkMaterial
        CKMaterial *mat = (CKMaterial *)ctx->CreateObject(CKCID_MATERIAL, "SimpleShadow Material",
                                                          beh->IsDynamic() ? CK_OBJECTCREATION_DYNAMIC : CK_OBJECTCREATION_NONAMECHECK);
        if (!mat)
            return CKERR_OUTOFMEMORY;
        mat->SetEmissive(VxColor(255, 255, 255));
        mat->SetDiffuse(VxColor(255, 255, 255, 255));
        mat->SetSpecular(VxColor(0, 0, 0));
        mat->SetAmbient(VxColor(0, 0, 0));
        mat->SetTextureAddressMode(VXTEXTURE_ADDRESSCLAMP);
        mat->SetTextureBlendMode(VXTEXTUREBLEND_COPY);

        // save of the IDs
        RealShadowStruct tss;

        tss.matID = CKOBJID(mat);
        tss.texID = -1;
        tss.nb_floors_under = 0;
        memset(tss.floor, 0, sizeof(tss.floor));

        beh->SetLocalParameterValue(0, &tss, sizeof(RealShadowStruct));
    }
    break;
    case CKM_BEHAVIORDETACH:
    {
        RealShadowStruct *tss = (RealShadowStruct *)beh->GetLocalParameterWriteDataPtr(0);
        if (!tss)
            return CK_OK;

        CKMaterial *mat = (CKMaterial *)ctx->GetObject(tss->matID);
        if (!mat)
            return CK_OK;

        A_Delete_SoftShadow_From_Floors(tss->floor, mat, tss->nb_floors_under);

        ctx->DestroyObject(mat); // we destroy the soft_shadow material
    }
    break;
    case CKM_BEHAVIORPAUSE:
    case CKM_BEHAVIORRESET:
    case CKM_BEHAVIORNEWSCENE:
    case CKM_BEHAVIORDEACTIVATESCRIPT:
    {
        RealShadowStruct *tss = (RealShadowStruct *)beh->GetLocalParameterWriteDataPtr(0);
        if (!tss)
            return CK_OK;

        CKMaterial *mat = (CKMaterial *)ctx->GetObject(tss->matID);
        if (mat)
            A_Delete_SoftShadow_From_Floors(tss->floor, mat, tss->nb_floors_under);
    }
    break;
    }

    return CKBR_OK;
}

// TT_Gravity_RT.dll 0x10003620 (RealShadowMappingPreRenderCallBack).
void RealShadowMappingRenderCallBack(CKRenderContext *dev, void *arg)
{
    CKBehavior *beh = (CKBehavior *)arg;
    if (!beh)
        return;
    CKContext *context = dev->GetCKContext();

    CKBOOL active = FALSE;
    beh->GetLocalParameterValue(1, &active);
    CK3dEntity *ent = (CK3dEntity *)beh->GetTarget();
    if (!ent)
        active = TRUE;

    RealShadowStruct *tss = (RealShadowStruct *)beh->GetLocalParameterWriteDataPtr(0);
    if (!tss)
        return;

    //////////////////////////////////////////////////
    //  We get the Floors that WERE under the object
    //////////////////////////////////////////////////
    int old_nb_floors_under = tss->nb_floors_under;
    if (old_nb_floors_under < 0)
        old_nb_floors_under = 0;
    if (old_nb_floors_under > A_MAX_NUMBER_OF_FLOOR_UNDER_OBJECT)
        old_nb_floors_under = A_MAX_NUMBER_OF_FLOOR_UNDER_OBJECT;

    // DLL quirk: the snapshot copies only 4 bytes per floor but is read back
    // with the 28-byte floor stride, so only the slots that fall inside the
    // copied bytes (in practice the first old floor) hold a real id. The rest
    // are uninitialised in the DLL; we use 0 so those floors keep the channel.
    CK_ID old_floor[A_MAX_NUMBER_OF_FLOOR_UNDER_OBJECT];
    for (int i = 0; i < old_nb_floors_under; ++i)
        old_floor[i] = RealShadowOldFloorSlotCopied(i, old_nb_floors_under) ? tss->floor[i].id : 0;

    CKMaterial *mat = (CKMaterial *)context->GetObject(tss->matID);

    // Off: strip the (quirky) old floor list and mark the block as stopped.
    // The callback still re-places the shadow below for this frame, so the
    // shadow stays on the floors after Off; the behavior function then removes
    // this callback on its next run.
    if (beh->IsInputActive(1))
    {
        beh->ActivateInput(1, FALSE);
        beh->ActivateOutput(1, TRUE);
        if (mat)
            A_Delete_SoftShadow_From_FloorIds(old_floor, mat, old_nb_floors_under);
        active = TRUE;
    }

    if (beh->IsInputActive(0))
    {
        beh->ActivateInput(0, FALSE);
        beh->ActivateOutput(0, TRUE);
        active = FALSE;
    }

    if (!mat || !ent)
    {
        // The DLL dereferences both unconditionally.
        beh->SetLocalParameterValue(1, &active);
        return;
    }

    // we get the input texture id
    CK_ID newtexID = 0;
    beh->GetInputParameterValue(1, &newtexID);

    // if the texture change ( or at start )
    if (newtexID != tss->texID)
    {
        tss->texID = newtexID;
        CKTexture *tex = (CKTexture *)context->GetObject(tss->texID);
        mat->SetTexture0(tex);
    }

    VXBLEND_MODE srcBlend = VXBLEND_ZERO;
    beh->GetInputParameterValue(3, &srcBlend);

    VXBLEND_MODE destBlend = VXBLEND_SRCCOLOR;
    beh->GetInputParameterValue(4, &destBlend);

    A_GetFloors(tss, ent, beh);
    RealShadowFloor *floor = tss->floor;

    //////////////////////////////////////////////////
    //  We REMOVE the material from floors
    //  that are no longer under the object
    //////////////////////////////////////////////////
    CK3dEntity *o;
    CKMesh *dMesh;
    int matChannel;

    for (int a = 0; a < old_nb_floors_under; a++)
    {
        // list of OLD floors
        CKBOOL out = TRUE;
        for (int b = 0; b < tss->nb_floors_under; b++)
        {
            // list of NEW floors
            if (old_floor[a] == floor[b].id)
            {
                out = FALSE;
                break;
            }
        }
        if (out)
        {
            o = (CK3dEntity *)context->GetObject(old_floor[a]);
            if (!CKIsChildClassOf(o, CKCID_3DENTITY))
                continue;
            dMesh = (CKMesh *)o->GetCurrentMesh();
            if (dMesh)
            {
                matChannel = dMesh->GetChannelByMaterial(mat);
                if (matChannel >= 0)
                    dMesh->RemoveChannel(matChannel);
            }
        }
    }

    //////////////////////////////////////////////////
    //  We ADD the material to the floors
    //  that are under the object
    //////////////////////////////////////////////////

    Vx2DVector *uvArray;
    CKDWORD stride, cStride;
    VxVector *vertexArray;

    for (int a = 0; a < tss->nb_floors_under; a++)
    {
        o = (CK3dEntity *)context->GetObject(floor[a].id);
        if (!CKIsChildClassOf(o, CKCID_3DENTITY))
            continue;
        dMesh = (CKMesh *)o->GetCurrentMesh();
        if (!dMesh)
            continue;

        matChannel = dMesh->GetChannelByMaterial(mat);

        // There were no shadow channel applied to the mesh: the blend modes are
        // only set when the channel is first added.
        if (matChannel < 0)
        {
            matChannel = dMesh->AddChannel(mat, FALSE);
            dMesh->SetChannelSourceBlend(matChannel, srcBlend);
            dMesh->SetChannelDestBlend(matChannel, destBlend);
        }

        ////////////////////////////////////////////////////////
        //  We now change the U,V coords of the shadow channel
        ////////////////////////////////////////////////////////
        uvArray = (Vx2DVector *)dMesh->GetTextureCoordinatesPtr(&cStride, matChannel);
        vertexArray = (VxVector *)dMesh->GetPositionsPtr(&stride);

        int vertexCount = dMesh->GetVertexCount();

        for (int i = 0; i < vertexCount; i++,
                vertexArray = (VxVector *) ((CKBYTE *) vertexArray + stride),
                uvArray = (Vx2DVector *) ((CKBYTE *) uvArray + cStride))
        {
            // Transform Vertex Position from mesh coordinates system to projector coordinate system
            uvArray->x = (vertexArray->x - floor[a].max.x) / (floor[a].min.x - floor[a].max.x);
            uvArray->y = (vertexArray->z - floor[a].max.z) / (floor[a].min.z - floor[a].max.z);
        }

        dMesh->UVChanged();
    }
    beh->SetLocalParameterValue(1, &active);
}

// TT_Gravity_RT.dll 0x100039A0 (A_GetFloors_0).
void A_GetFloors(RealShadowStruct *tss, CK3dEntity *ent, CKBehavior *beh)
{
    if (!tss || !ent || !beh)
        return;

    CKContext *context = beh->GetCKContext();

    VxVector lightVector(0.0f, 0.0f, 0.0f);
    beh->GetInputParameterValue(0, &lightVector);

    Vx2DVector size(0.0f, 0.0f);
    beh->GetInputParameterValue(2, &size);

    CKFloorManager *FloorManager = (CKFloorManager *)context->GetManagerByGuid(FLOOR_MANAGER_GUID);
    if (!FloorManager)
        return;
    int floorAttribute = FloorManager->GetFloorAttribute();

    CKAttributeManager *attman = context->GetAttributeManager();
    if (!attman)
        return;
    const XObjectPointerArray &floor_objects = attman->GetGlobalAttributeListPtr(floorAttribute);

    VxVector objPos;
    ent->GetPosition(&objPos);
    const VxVector light = RealShadowLightPosition(objPos, lightVector);
    const float radius = ent->GetRadius();

    int nbf_under = 0; // number of floors under
    int floor_count = floor_objects.Size();

    for (int n = 0; n < floor_count; ++n)
    {
        CK3dEntity *floor = (CK3dEntity *)floor_objects[n];
        if (!floor || !floor->IsVisible() || floor->IsAllOutsideFrustrum())
            continue;

        CKBOOL under = FALSE;

        VxVector vMax(1000.0f, 0, 1000.0f);
        VxVector vMin(-1000.0f, 0, -1000.0f);

        VxVector floorPos;
        floor->GetPosition(&floorPos);

        const VxBbox &Bbox_floorWorld = floor->GetBoundingBox();

        if (!(light.y > 0.0f && objPos.y > Bbox_floorWorld.Min.y))
            continue;

        // Centre: ray from the light through the object onto this floor;
        // falls back to the floor's height when the ray misses it.
        VxVector centre;
        VxIntersectionDesc desc;
        VxVector pos = objPos;
        floor->RayIntersection(&light, &pos, &desc, NULL);
        if (desc.Object == floor)
            floor->Transform(&centre, &desc.IntersectionPoint);
        else
            centre = RealShadowProject(light, objPos, 0.0f, 0.0f, floorPos.y);
        const float height = centre.y;

        beh->SetOutputParameterValue(0, &centre);
        if (centre.x > Bbox_floorWorld.Min.x && centre.x < Bbox_floorWorld.Max.x &&
            centre.z > Bbox_floorWorld.Min.z && centre.z < Bbox_floorWorld.Max.z)
            under = TRUE;

        // Corners: position +- (radius*Size.x, radius*Size.y) thrown from the
        // light onto the centre hit's height.
        VxVector corners[4];
        RealShadowCorners(light, objPos, radius, size, height, corners);
        for (int i = 0; i < 4; ++i)
        {
            const VxVector &c = corners[i];
            beh->SetOutputParameterValue(i + 1, &c);

            if (!under &&
                c.x > Bbox_floorWorld.Min.x && c.x < Bbox_floorWorld.Max.x &&
                c.z > Bbox_floorWorld.Min.z && c.z < Bbox_floorWorld.Max.z)
                under = TRUE;

            if (c.x < vMax.x)
                vMax.x = c.x;
            if (c.x > vMin.x)
                vMin.x = c.x;
            if (c.z < vMax.z)
                vMax.z = c.z;
            if (c.z > vMin.z)
                vMin.z = c.z;
        }

        if (under && nbf_under < A_MAX_NUMBER_OF_FLOOR_UNDER_OBJECT)
        {
            floor->InverseTransform(&vMax, &vMax);
            floor->InverseTransform(&vMin, &vMin);
            RealShadowFloor &f = tss->floor[nbf_under];
            f.max = vMax;
            f.min = vMin;
            f.id = floor->GetID();
            nbf_under++;
        }
    }

    tss->nb_floors_under = nbf_under;
}

static void A_Delete_SoftShadow_From_FloorIds(const CK_ID *ids, CKMaterial *cmat, int nb_floors)
{
    CKContext *context = cmat->GetCKContext();
    for (int a = 0; a < nb_floors; a++)
    {
        CK3dEntity *o = (CK3dEntity *)context->GetObject(ids[a]);
        if (!CKIsChildClassOf(o, CKCID_3DENTITY))
            continue;
        CKMesh *dMesh = o->GetCurrentMesh();
        if (dMesh)
        {
            int matChannel = dMesh->GetChannelByMaterial(cmat);
            if (matChannel >= 0)
                dMesh->RemoveChannel(matChannel);
        }
    }
}

void A_Delete_SoftShadow_From_Floors(RealShadowFloor *floor, CKMaterial *cmat, int nb_floors)
{
    CKContext *context = cmat->GetCKContext();

    CK3dEntity *o;
    CKMesh *dMesh;
    int matChannel;

    for (int a = 0; a < nb_floors; a++)
    {
        o = (CK3dEntity *)context->GetObject(floor->id);
        if (CKIsChildClassOf(o, CKCID_3DENTITY))
        {
            dMesh = (CKMesh *)o->GetCurrentMesh();
            if (dMesh)
            {
                matChannel = dMesh->GetChannelByMaterial(cmat);
                if (matChannel >= 0)
                    dMesh->RemoveChannel(matChannel);
            }
        }
        ++floor;
    }
}
