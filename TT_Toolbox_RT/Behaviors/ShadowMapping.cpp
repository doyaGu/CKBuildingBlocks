////////////////////////////////////
////////////////////////////////////
//
//        TT ShadowMapping
//
////////////////////////////////////
////////////////////////////////////
#include "CKAll.h"
#include "ToolboxGuids.h"

CKObjectDeclaration *FillBehaviorShadowMappingDecl();
CKERROR CreateShadowMappingProto(CKBehaviorPrototype **pproto);
int ShadowMapping(const CKBehaviorContext &behcontext);
CKERROR ShadowMappingCallBack(const CKBehaviorContext &behcontext);

// Structure for shadow data (460 bytes total)
// Each shadow receiver entry is 28 bytes
struct ShadowReceiverEntry
{
    CK_ID objectId;        // Object ID (offset 0)
    VxVector bboxMax;      // Bounding box max (offset 4)
    VxVector bboxMin;      // Bounding box min (offset 16)
};

struct ShadowMappingData
{
    CK_ID materialId;              // Shadow material ID (offset 0)
    CK_ID textureId;               // Current texture ID (offset 4)
    ShadowReceiverEntry entries[16]; // Up to 16 receivers (offset 8)
    int receiverCount;             // Number of active receivers (offset 456)
};

CKObjectDeclaration *FillBehaviorShadowMappingDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("TT ShadowMapping");
    od->SetDescription("Creates shadows via UV mapping");
    od->SetCategory("TT Toolbox/FX");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x334c7dfe, 0x47c81a31));
    od->SetAuthorGuid(TERRATOOLS_GUID);
    od->SetAuthorName("Terratools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateShadowMappingProto);
    od->SetCompatibleClassId(CKCID_3DOBJECT);
    return od;
}

CKERROR CreateShadowMappingProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("TT ShadowMapping");
    if (!proto) return CKERR_OUTOFMEMORY;

    proto->DeclareInput("On");
    proto->DeclareInput("Off");

    proto->DeclareOutput("Exit On");
    proto->DeclareOutput("Exit Off");

    proto->DeclareInParameter("Light", CKPGUID_3DENTITY);
    proto->DeclareInParameter("Texture", CKPGUID_TEXTURE);
    proto->DeclareInParameter("Size", CKPGUID_2DVECTOR, "1,1");
    proto->DeclareInParameter("SourceBlend", CKPGUID_BLENDFACTOR, "Zero");
    proto->DeclareInParameter("DestBlend", CKPGUID_BLENDFACTOR, "Source Color");

    proto->DeclareLocalParameter("Data", CKPGUID_VOIDBUF);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(ShadowMapping);

    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);
    proto->SetBehaviorCallbackFct(ShadowMappingCallBack);

    *pproto = proto;
    return CK_OK;
}

// Helper to remove shadow channel from mesh
static void RemoveShadowChannels(ShadowMappingData *data, CKMaterial *shadowMat, CKContext *ctx)
{
    for (int i = 0; i < data->receiverCount; i++)
    {
        CK3dEntity *obj = (CK3dEntity *)ctx->GetObject(data->entries[i].objectId);
        if (!obj) continue;

        CKMesh *mesh = obj->GetCurrentMesh();
        if (!mesh) continue;

        int channel = mesh->GetChannelByMaterial(shadowMat);
        if (channel >= 0)
            mesh->RemoveChannel(channel);
    }
}

static void UpdateShadowReceivers(ShadowMappingData *data, CK3dEntity *target, CKBehavior *beh)
{
    CKContext *ctx = beh->GetCKContext();
    CK3dEntity *light = (CK3dEntity *)beh->GetInputParameterObject(0);
    if (!ctx || !light || !target)
    {
        data->receiverCount = 0;
        return;
    }

    VxVector lightPos;
    light->GetPosition(&lightPos, target);

    VxVector targetWorldPos;
    target->GetPosition(&targetWorldPos);

    Vx2DVector size(1.0f, 1.0f);
    beh->GetInputParameterValue(2, &size);

    CKFloorManager *floorMgr = (CKFloorManager *)ctx->GetManagerByGuid(FLOOR_MANAGER_GUID);
    if (!floorMgr)
    {
        data->receiverCount = 0;
        return;
    }

    CKAttributeManager *attrMgr = ctx->GetAttributeManager();
    if (!attrMgr)
    {
        data->receiverCount = 0;
        return;
    }

    const XObjectPointerArray &floorObjects = attrMgr->GetGlobalAttributeListPtr(floorMgr->GetFloorAttribute());

    const VxBbox &bbox = target->GetBoundingBox(TRUE);
    const float minX = bbox.Min.x * size.x;
    const float maxX = bbox.Max.x * size.x;
    const float minZ = bbox.Min.z * size.y;
    const float maxZ = bbox.Max.z * size.y;

    VxVector localCorners[4] = {
        VxVector(minX, 0.0f, minZ),
        VxVector(minX, 0.0f, maxZ),
        VxVector(maxX, 0.0f, minZ),
        VxVector(maxX, 0.0f, maxZ),
    };

    int receiverCount = 0;
    for (CKObject **it = floorObjects.Begin(); it != floorObjects.End(); ++it)
    {
        CK3dEntity *floor = (CK3dEntity *)*it;
        if (!floor || !floor->IsVisible() || floor->IsAllOutsideFrustrum())
            continue;

        VxVector floorPos;
        floor->GetPosition(&floorPos, target);

        const VxBbox &floorBbox = floor->GetBoundingBox(TRUE);
        if (lightPos.y <= 0.0f || floorBbox.Min.y >= targetWorldPos.y)
            continue;

        VxVector bboxMax(-1000.0f, 0.0f, -1000.0f);
        VxVector bboxMin(1000.0f, 0.0f, 1000.0f);
        CKBOOL under = FALSE;

        for (int i = 0; i < 4; ++i)
        {
            const VxVector &corner = localCorners[i];
            const float t = (lightPos.y - floorPos.y) / (lightPos.y - corner.y);
            VxVector projected;
            projected.x = lightPos.x + (corner.x - lightPos.x) * t;
            projected.y = floorPos.y;
            projected.z = lightPos.z + (corner.z - lightPos.z) * t;

            VxVector floorLocalPoint;
            floor->InverseTransform(&floorLocalPoint, &projected, target);

            if (!under &&
                floorLocalPoint.x > floorBbox.Min.x && floorLocalPoint.x < floorBbox.Max.x &&
                floorLocalPoint.z > floorBbox.Min.z && floorLocalPoint.z < floorBbox.Max.z)
                under = TRUE;

            if (projected.x < bboxMin.x) bboxMin.x = projected.x;
            if (projected.x > bboxMax.x) bboxMax.x = projected.x;
            if (projected.z < bboxMin.z) bboxMin.z = projected.z;
            if (projected.z > bboxMax.z) bboxMax.z = projected.z;
        }

        if (!under || receiverCount >= 16)
            continue;

        ShadowReceiverEntry &entry = data->entries[receiverCount++];
        entry.objectId = floor->GetID();
        entry.bboxMax = bboxMax;
        entry.bboxMin = bboxMin;
    }

    data->receiverCount = receiverCount;
}

int ShadowMapping(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;
    CKContext *ctx = behcontext.Context;

    CK3dEntity *target = (CK3dEntity *)beh->GetTarget();
    if (!target)
        return CKBR_OWNERERROR;

    ShadowMappingData *data = (ShadowMappingData *)beh->GetLocalParameterWriteDataPtr(0);
    const int oldReceiverCount = data->receiverCount;
    CK_ID oldReceiverIds[16] = {};
    for (int i = 0; i < oldReceiverCount && i < 16; ++i)
        oldReceiverIds[i] = data->entries[i].objectId;

    CKMaterial *shadowMat = (CKMaterial *)ctx->GetObject(data->materialId);

    if (beh->IsInputActive(1))
    {
        beh->ActivateInput(1, FALSE);
        int outputCount = beh->GetOutputCount();
        beh->ActivateOutput(outputCount >= 2 ? 1 : 0, TRUE);

        RemoveShadowChannels(data, shadowMat, ctx);
        return CKBR_OK;
    }

    if (beh->IsInputActive(0))
    {
        beh->ActivateInput(0, FALSE);
        beh->ActivateOutput(0, TRUE);
    }

    CK_ID newTextureId = 0;
    beh->GetInputParameterValue(1, &newTextureId);
    if (newTextureId != data->textureId)
    {
        data->textureId = newTextureId;
        CKTexture *texture = (CKTexture *)ctx->GetObject(newTextureId);
        if (shadowMat)
            shadowMat->SetTexture0(texture);
    }

    VXBLEND_MODE srcBlend = VXBLEND_ZERO;
    VXBLEND_MODE destBlend = VXBLEND_SRCCOLOR;
    beh->GetInputParameterValue(3, &srcBlend);
    beh->GetInputParameterValue(4, &destBlend);

    UpdateShadowReceivers(data, target, beh);

    for (int i = 0; i < oldReceiverCount; ++i)
    {
        CKBOOL stillPresent = FALSE;
        for (int j = 0; j < data->receiverCount; ++j)
        {
            if (oldReceiverIds[i] == data->entries[j].objectId)
            {
                stillPresent = FALSE;
                stillPresent = TRUE;
                break;
            }
        }

        if (!stillPresent)
        {
            CK3dEntity *oldReceiver = (CK3dEntity *)ctx->GetObject(oldReceiverIds[i]);
            if (!oldReceiver)
                continue;

            CKMesh *oldMesh = oldReceiver->GetCurrentMesh();
            if (!oldMesh)
                continue;

            int oldChannel = oldMesh->GetChannelByMaterial(shadowMat);
            if (oldChannel >= 0)
                oldMesh->RemoveChannel(oldChannel);
        }
    }

    for (int r = 0; r < data->receiverCount; r++)
    {
        CK3dEntity *receiver = (CK3dEntity *)ctx->GetObject(data->entries[r].objectId);
        if (!receiver) continue;

        CKMesh *mesh = receiver->GetCurrentMesh();
        if (!mesh) continue;

        // Get or create shadow channel
        int channel = mesh->GetChannelByMaterial(shadowMat);
        if (channel < 0)
            channel = mesh->AddChannel(shadowMat, FALSE);
        mesh->SetChannelSourceBlend(channel, srcBlend);
        mesh->SetChannelDestBlend(channel, destBlend);

        CKDWORD uvStride = 0;
        Vx2DVector *uvCoords = (Vx2DVector *)mesh->GetTextureCoordinatesPtr(&uvStride, channel);

        CKDWORD vertStride = 0;
        VxVector *vertices = (VxVector *)mesh->GetPositionsPtr(&vertStride);

        const int vertexCount = mesh->GetVertexCount();
        VxVector *bboxMax = &data->entries[r].bboxMax;
        VxVector *bboxMin = &data->entries[r].bboxMin;
        float invSizeX = 1.0f / (bboxMax->x - bboxMin->x);
        float invSizeZ = 1.0f / (bboxMax->z - bboxMin->z);

        for (int v = 0; v < vertexCount; v++)
        {
            VxVector localPos;
            target->InverseTransform(&localPos, vertices, receiver);

            uvCoords->x = (localPos.x - bboxMin->x) * invSizeX;
            uvCoords->y = (localPos.z - bboxMin->z) * invSizeZ;

            uvCoords = (Vx2DVector *)((CKBYTE *)uvCoords + uvStride);
            vertices = (VxVector *)((CKBYTE *)vertices + vertStride);
        }

        mesh->UVChanged();
    }

    return CKBR_ACTIVATENEXTFRAME;
}

CKERROR ShadowMappingCallBack(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;
    CKContext *ctx = behcontext.Context;

    switch (behcontext.CallbackMessage)
    {
    case CKM_BEHAVIORATTACH:
    case CKM_BEHAVIORLOAD:
        {
            CK_OBJECTCREATION_OPTIONS options = beh->IsDynamic() ? CK_OBJECTCREATION_DYNAMIC : CK_OBJECTCREATION_NONAMECHECK;
            CKMaterial *shadowMat = (CKMaterial *)ctx->CreateObject(CKCID_MATERIAL, "SimpleShadow Material", options);

            VxColor white(1.0f, 1.0f, 1.0f, 1.0f);
            shadowMat->SetDiffuse(white);
            shadowMat->SetAmbient(white);
            VxColor black(0.0f, 0.0f, 0.0f, 1.0f);
            shadowMat->SetSpecular(black);
            shadowMat->SetEmissive(black);
            shadowMat->SetTextureAddressMode(VXTEXTURE_ADDRESSCLAMP);
            shadowMat->SetTextureBlendMode(VXTEXTUREBLEND_COPY);

            ShadowMappingData data;
            memset(&data, 0, sizeof(data));
            data.materialId = shadowMat ? shadowMat->GetID() : 0;
            data.textureId = (CK_ID)-1;

            beh->SetLocalParameterValue(0, &data, sizeof(data));
        }
        break;

    case CKM_BEHAVIORDETACH:
        {
            ShadowMappingData *data = (ShadowMappingData *)beh->GetLocalParameterWriteDataPtr(0);
            CKMaterial *shadowMat = (CKMaterial *)ctx->GetObject(data->materialId);
            if (!shadowMat)
                break;

            RemoveShadowChannels(data, shadowMat, ctx);
            ctx->DestroyObject(shadowMat);
        }
        break;

    case CKM_BEHAVIORPAUSE:
    case CKM_BEHAVIORRESET:
    case CKM_BEHAVIORNEWSCENE:
    case CKM_BEHAVIORDEACTIVATESCRIPT:
        {
            ShadowMappingData *data = (ShadowMappingData *)beh->GetLocalParameterWriteDataPtr(0);
            CKMaterial *shadowMat = (CKMaterial *)ctx->GetObject(data->materialId);
            if (shadowMat)
                RemoveShadowChannels(data, shadowMat, ctx);
        }
        break;
    }

    return CKBR_OK;
}
