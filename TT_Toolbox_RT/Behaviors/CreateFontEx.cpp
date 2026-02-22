///////////////////////////////////
///////////////////////////////////
//
//        TT_CreateFontEx
//
///////////////////////////////////
///////////////////////////////////
#include "CKAll.h"
#include "ToolboxGuids.h"
#include "CKFontManager.h"
#include "CKTextureFont.h"

CKObjectDeclaration *FillBehaviorCreateFontExDecl();
CKERROR CreateCreateFontExProto(CKBehaviorPrototype **pproto);
int CreateFontEx(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorCreateFontExDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("TT_CreateFontEx");
    od->SetDescription("Creates a font from a provided texture.");
    od->SetCategory("TT Toolbox/Font");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x260e4eb0, 0xe256b90));
    od->SetAuthorGuid(TERRATOOLS_GUID);
    od->SetAuthorName("Terratools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateCreateFontExProto);
    od->SetCompatibleClassId(CKCID_BEOBJECT);
    return od;
}

CKERROR CreateCreateFontExProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("TT CreateFontEx");
    if (!proto) return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");

    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Font Name", CKPGUID_STRING, "MyFont");
    proto->DeclareInParameter("Font Texture", CKPGUID_TEXTURE);
    proto->DeclareInParameter("Horizontal/Vertical Character Number", CKPGUID_2DVECTOR, "16,8");
    proto->DeclareInParameter("Proportionnal", CKPGUID_BOOL, "TRUE");
    proto->DeclareInParameter("Font Bounds", CKPGUID_RECT, "(0,0),(0,0)");
    proto->DeclareInParameter("First Character", CKPGUID_INT, "0");
    proto->DeclareInParameter("FontCoordinatesData", CKPGUID_DATAARRAY);

    proto->DeclareOutParameter("Font", CKPGUID_FONT);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(CreateFontEx);

    proto->SetBehaviorFlags(CKBEHAVIOR_INTERNALLYCREATEDINPUTPARAMS);

    *pproto = proto;
    return CK_OK;
}

int CreateFontEx(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;
    CKContext *context = behcontext.Context;

    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0, TRUE);

    CKSTRING fontName = (CKSTRING)beh->GetInputParameterReadDataPtr(0);
    if (!fontName)
        return CKBR_OK;

    CKTexture *texture = (CKTexture *)beh->GetInputParameterObject(1);
    if (!texture)
        return CKBR_PARAMETERERROR;

    CKFontManager *fm = (CKFontManager *)context->GetManagerByGuid(FONT_MANAGER_GUID);
    if (!fm)
        return CKBR_PARAMETERERROR;

    CKDataArray *fontCoordData = (CKDataArray *)beh->GetInputParameterObject(6);

    Vx2DVector characterNumber(16.0f, 8.0f);
    beh->GetInputParameterValue(2, &characterNumber);

    CKBOOL proportional = TRUE;
    beh->GetInputParameterValue(3, &proportional);

    VxRect fontBounds;
    fontBounds.right = (float)texture->GetWidth();
    fontBounds.bottom = (float)texture->GetHeight();
    beh->GetInputParameterValue(4, &fontBounds);

    int firstCharacter = 0;
    beh->GetInputParameterValue(5, &firstCharacter);
    int font = fm->CreateTextureFont(fontName, texture, fontBounds, characterNumber, !proportional, firstCharacter);
    beh->SetOutputParameterValue(0, &font);

    CKTextureFont *texFont = fm->GetFont(font);
    if (!texFont)
        return CKBR_PARAMETERERROR;

    if (!fontCoordData)
        return CKBR_OK;

    const int columnCount = fontCoordData->GetColumnCount();
    const int rowCount = fontCoordData->GetRowCount();
    if (columnCount < 6 || rowCount <= 0)
        return CKBR_OK;

    CharacterTextureCoordinates *fontCoords = texFont->m_FontCoordinates;
    const int limit = (rowCount < 256) ? rowCount : 256;
    for (int i = 0; i < limit; ++i)
    {
        CharacterTextureCoordinates coord = fontCoords[i];
        float v = 0.0f;

        if (fontCoordData->GetElementValue(i, 0, &v)) coord.ustart = v;
        if (fontCoordData->GetElementValue(i, 1, &v)) coord.vstart = v;
        if (fontCoordData->GetElementValue(i, 2, &v)) coord.uwidth = v;
        if (fontCoordData->GetElementValue(i, 3, &v)) coord.uprewidth = v;
        if (fontCoordData->GetElementValue(i, 4, &v)) coord.upostwidth = v;
        if (fontCoordData->GetElementValue(i, 5, &v)) coord.vwidth = v;

        fontCoords[i] = coord;
    }

    return CKBR_OK;
}