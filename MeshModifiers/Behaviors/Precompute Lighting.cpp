/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            PrecomputeLighting
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKERROR CreatePrecomputeLightingBehaviorProto(CKBehaviorPrototype **);
int PrecomputeLighting(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorPrecomputeLightingDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Precompute Lighting");
    od->SetDescription("Precompute the lighting of an object.");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process.<BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Specular: </SPAN>if TRUE, then specular effect is computed by vertex.<BR>
    <BR>
    This building block precompute the lighting of an object when activated, setting its mesh
    as prelit, thus saving the lighting computation at each render frame.
    */
    od->SetCategory("Mesh Modifications/Basic");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x53a5be9, 0x78cc3e15));
    od->SetAuthorGuid(CKGUID());
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreatePrecomputeLightingBehaviorProto);
    od->SetCompatibleClassId(CKCID_3DOBJECT);
    return od;
}

CKERROR CreatePrecomputeLightingBehaviorProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Precompute Lighting");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Specular", CKPGUID_BOOL, "FALSE");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);

    proto->SetFunction(PrecomputeLighting);
    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

    *pproto = proto;
    return CK_OK;
}

struct VxPreparedLight
{
    VxVector WPosition;                       // 0
    VxVector LPosition;                       // 12
    VxVector WDirection;                      // 24
    VxVector LDirection;                      // 36
    VxVector Halfway;                         // 48
    float Range, InvRange;                    // 60/64
    float R, G, B;                            // 68/72/76
    float att0, att1, att2;                   // 80/84/88
    float falloff;                            // 92
    float SqrRange, Active;                   // 96/100
    float cosphi, costheta, invthetaminusphi; // 104/108/112
    CKBOOL Specular;                            // 116
    CKBOOL Attenuation;                         // 120
    VXLIGHT_TYPE Type;                        // 124
                                              // 32 float= 128 bytes
};

class LightingEngine
{
public:
    void LitVertex(VxVector *pos, VxVector *norm);
    void SetCurrentMaterial(CKMaterial *mat, CKScene *Scene);
    void ComputeColors(CKWORD *indices, int indicescount, int count, CKBYTE *varray, int StridePos, CKBYTE *narray, int StrideNormal, CKBYTE *carray, int StrideColors);
    ///
    // Members

    XArray<VxPreparedLight> m_Lights;
    XArray<int> m_ColorSet;

    // Material
    VxColor m_MaterialAmbient;
    VxColor m_MaterialDiffuse;
    VxColor m_MaterialSpecular;
    VxColor m_MaterialEmissive;

    float m_Power;

    // Vertices
    VxColor m_VertexDiffuse;
    VxColor m_VertexSpecular;
};

void LightingEngine::SetCurrentMaterial(CKMaterial *mat, CKScene *scene)
{
    VxColor ambient(scene->GetAmbientLight());

    m_MaterialAmbient = mat->GetAmbient();
    m_MaterialAmbient *= ambient;

    m_MaterialEmissive = mat->GetEmissive();
    m_MaterialAmbient += m_MaterialEmissive;

    m_MaterialDiffuse = mat->GetDiffuse();

    m_MaterialSpecular = mat->GetSpecular();

    m_Power = mat->GetPower();
}

void LightingEngine::LitVertex(VxVector *pos, VxVector *norm)
{
    m_VertexDiffuse.Set(0.0f, 0.0f, 0.0f, 0.0f);
    m_VertexSpecular.Set(0.0f, 0.0f, 0.0f, 0.0f);

    // Iterate on all lights
    for (VxPreparedLight *light = m_Lights.Begin(); light != m_Lights.End(); ++light)
    {
        float spot = 1.0f;
        float attspot = 1.0f;

        switch (light->Type)
        {
        case VX_LIGHTPOINT:
        {
            ///#pragma todo("this needs a unit matrix = crappy!")
            VxVector direction = light->LPosition - (*pos);
            float att = Magnitude(direction);
            if (att < light->Range)
            {
                direction /= att;
                att = (light->Range - att) * light->InvRange;
                att = light->att0 + light->att1 * att + light->att2 * att * att;

                float factor = att * DotProduct(*norm, direction);
                if (factor > 0)
                {
                    m_VertexDiffuse.r += light->R * factor;
                    m_VertexDiffuse.g += light->G * factor;
                    m_VertexDiffuse.b += light->B * factor;
                }
            }
        }
        break;
        case VX_LIGHTSPOT:
        {
            //#pragma todo("this needs a unit matrix = crappy!")
            VxVector direction = light->LPosition - (*pos);
            float att = Magnitude(direction);
            if (att < light->Range)
            {
                direction /= att;
                float rho = DotProduct(direction, light->LDirection);
                if (rho <= light->cosphi)
                {
                    attspot = 0; // Out of cone No Influence
                }
                else
                {
                    if (rho > light->costheta)
                    {
                        spot = 1.0f;
                    }
                    else
                    {
                        spot = (rho - light->cosphi) * (light->invthetaminusphi);
                        if (light->falloff != 1.0f)
                            pow(spot, light->falloff); // TODO is falloff THIS important ?
                    }
                    attspot = light->att0;
                    att = (light->Range - att) * light->InvRange;
                    attspot += light->att1 * att + light->att2 * att * att;
                    attspot *= spot;
                    float factor = attspot * DotProduct(*norm, direction);
                    if (factor > 0)
                    {
                        m_VertexDiffuse.r += light->R * factor;
                        m_VertexDiffuse.g += light->G * factor;
                        m_VertexDiffuse.b += light->B * factor;
                    }
                }
            }
        }
        break;
        case VX_LIGHTDIREC:
        {
            float factor = DotProduct(*norm, light->LDirection);
            if (factor > 0)
            {
                m_VertexDiffuse.r += light->R * factor;
                m_VertexDiffuse.g += light->G * factor;
                m_VertexDiffuse.b += light->B * factor;
            }
        }
        break;
        case VX_LIGHTPARA:
        {
            float factor = DotProduct(*norm, light->LPosition);
            if (factor > 0)
            {
                m_VertexDiffuse.r += light->R * factor;
                m_VertexDiffuse.g += light->G * factor;
                m_VertexDiffuse.b += light->B * factor;
            }
        }
        break;
        }

        //----------------------------------------------------------------
        // Specular Component calculation
        if (light->Specular && m_Power)
        {
            float factor = DotProduct(*norm, light->Halfway);
            if (factor > 0 && attspot > 0)
            {
                if (m_Power != 1.0f)
                    factor = (float)pow(factor, m_Power);
                factor *= attspot;
                m_VertexSpecular.r += light->R * factor;
                m_VertexSpecular.g += light->G * factor;
                m_VertexSpecular.b += light->B * factor;
            }
        }
    }
}

#define CLAMPF(a)      \
    if (a < 0.0f)      \
        a = 0.0f;      \
    else if (a > 1.0f) \
        a = 1.0f;

void LightingEngine::ComputeColors(CKWORD *indices, int indicescount, int vcount, CKBYTE *varray, int StridePos, CKBYTE *narray, int StrideNormal, CKBYTE *carray, int StrideColors)
{
    VxColor col;
    int zero = 0;

    m_ColorSet.Resize(vcount);
    m_ColorSet.Fill(zero);

    // we iterate on all faces
    for (int f = 0; f < indicescount; ++f)
    {
        int a = indices[f];
        if (!m_ColorSet[a])
        {
            CKDWORD *c = (CKDWORD *)(carray + a * StrideColors);
            VxVector *v = (VxVector *)(varray + a * StridePos);
            VxVector *n = (VxVector *)(narray + a * StrideNormal);

            // we calulate the lights participation
            LitVertex(v, n);

            // we finalize the color computation
            col.r = m_MaterialAmbient.r + m_MaterialDiffuse.r * m_VertexDiffuse.r + m_MaterialSpecular.r * m_VertexSpecular.r;
            col.g = m_MaterialAmbient.g + m_MaterialDiffuse.g * m_VertexDiffuse.g + m_MaterialSpecular.g * m_VertexSpecular.g;
            col.b = m_MaterialAmbient.b + m_MaterialDiffuse.b * m_VertexDiffuse.b + m_MaterialSpecular.b * m_VertexSpecular.b;
            col.a = m_MaterialDiffuse.a;

            CLAMPF(col.r);
            CLAMPF(col.g);
            CLAMPF(col.b);
            CLAMPF(col.a);
            *c = RGBAFTOCOLOR(&col);
            *(c + 1) = 0;

            // the vertices has been calculated
            m_ColorSet[a] = 1;
        }
    }
}

int PrecomputeLighting(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;
    CKContext *ctx = behcontext.Context;

    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0);

    CK3dEntity *entity = (CK3dEntity *)beh->GetTarget();
    if (!entity)
        return CKBR_OWNERERROR;

    CKBOOL specular = FALSE;
    beh->GetInputParameterValue(0, &specular);

    CKDWORD stride = 0;
    CKMesh *mesh = entity->GetCurrentMesh();
    if (!mesh)
        return CKBR_OWNERERROR;

    VxVector CameraPos;
    CK3dEntity *viewer = behcontext.CurrentRenderContext->GetViewpoint();
    viewer->GetPosition(&CameraPos);

    // We create the lighting engine
    LightingEngine le;

    // TODO : do it with the CKCID_LIGHT too
    int tlcount = ctx->GetObjectsCountByClassID(CKCID_TARGETLIGHT);
    CK_ID *tls = ctx->GetObjectsListByClassID(CKCID_TARGETLIGHT);

    int i = 0;
    for (i = 0; i < tlcount; ++i)
    {
        CKTargetLight *light = (CKTargetLight *)ctx->GetObject(tls[i]);
        if (!light->IsActiveInCurrentScene() || !light->IsVisible())
            continue;

        VXLIGHT_TYPE type = light->GetType();
        float range = light->GetRange();

        // We search to bypass lights too far away
        if ((type == VX_LIGHTSPOT) || (type == VX_LIGHTPOINT))
        {
            VxVector epos;
            entity->GetPosition(&epos);
            VxVector lpos;
            light->GetPosition(&lpos);
            if (range < (Magnitude(epos - lpos) - entity->GetRadius()))
                continue;
        }

        // we add the light to the array of active lights
        le.m_Lights.Expand();

        ///
        // We fill light information
        VxPreparedLight &pl = le.m_Lights.Back();

        // Light World Position
        light->GetPosition(&pl.WPosition);

        // Light Local Position in the entity referential
        light->GetPosition(&pl.LPosition, entity);
        if (type == VX_LIGHTPARA)
            pl.LPosition.Normalize(); // Patch for Parallel lights

        // Light World Direction
        light->GetOrientation(&pl.WDirection, NULL);

        // Light Local Direction in the entity referential
        if (type == VX_LIGHTPOINT)
        {
            entity->InverseTransform(&pl.LDirection, &VxVector::axis0(), light);
        }
        else
        {
            light->GetOrientation(&pl.LDirection, NULL, NULL, entity);
            pl.LDirection = -pl.LDirection;
        }

        // Attenuations
        pl.att0 = light->GetConstantAttenuation();
        pl.att1 = light->GetLinearAttenuation();
        pl.att2 = light->GetQuadraticAttenuation();

        // Ranges
        pl.Range = range;
        pl.InvRange = 1.0f / range;

        // Color
        VxColor col(light->GetColor());
        pl.R = col.r;
        pl.G = col.g;
        pl.B = col.b;

        // light Type
        pl.Type = type;

        // Spot Attributes
        if (type == VX_LIGHTSPOT)
        {
            pl.falloff = light->GetFallOffShape();
            pl.cosphi = cosf(light->GetFallOff() * 0.5f);
            pl.costheta = cosf(light->GetHotSpot() * 0.5f);
            pl.invthetaminusphi = 1.0f / (pl.costheta - pl.cosphi);
        }

        // Specular Data
        if (specular)
        {
            pl.Specular = light->GetSpecularFlag();
            if (pl.Specular)
            {
                pl.Halfway = Normalize(Normalize(CameraPos) + pl.LDirection);
            }
        }
        else
            pl.Specular = FALSE;
    }

    // Vertices
    CKDWORD pStride = 0;
    CKBYTE *varray = (CKBYTE *)mesh->GetPositionsPtr(&pStride);
    // Normals
    CKDWORD nStride = 0;
    CKBYTE *narray = (CKBYTE *)mesh->GetNormalsPtr(&nStride);
    // Colors
    CKDWORD cStride = 0;
    CKBYTE *carray = (CKBYTE *)mesh->GetColorsPtr(&cStride);

    int vcount = mesh->GetVertexCount();

    for (int m = 0; m < mesh->GetMaterialCount(); ++m)
    {
        // For now, only one material is supported
        CKMaterial *material = mesh->GetMaterial(m);
        le.SetCurrentMaterial(material, behcontext.CurrentScene);

        // we must create an index list of faces using this material
        XArray<CKWORD> indices;
        int a, b, c;
        for (int f = 0; f < mesh->GetFaceCount(); ++f)
        {
            if (material == mesh->GetFaceMaterial(f))
            { // good face
                mesh->GetFaceVertexIndex(f, a, b, c);
                indices.PushBack(a);
                indices.PushBack(b);
                indices.PushBack(c);
            }
        }

        // We compute the colors
        le.ComputeColors(indices.Begin(), indices.Size(), vcount, varray, pStride, narray, nStride, carray, cStride);
    }

    // We change the mesh to prelit, if not already
    if (mesh->GetLitMode() != VX_PRELITMESH)
    {
        mesh->SetLitMode(VX_PRELITMESH);
    }

    // we tell the renderer the color have changed
    mesh->ColorChanged();

    return CKBR_OK;
}
