////////////////////////
////////////////////////
//
// SetLayerVisibility
//
////////////////////////
////////////////////////
#include "CKAll.h"

CKObjectDeclaration *FillBehaviorSetLayerVisibleDecl();
CKERROR CreateSetLayerVisibleProto(CKBehaviorPrototype **);
int SetLayerVisible(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorSetLayerVisibleDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Set Layer Visibility");

    od->SetDescription("Sets the visibility of a specific grid's layer");

    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetCategory("Grids/Basic");
    od->SetGuid(CKGUID(0xb000dd6, 0x2fd66d53));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateSetLayerVisibleProto);
    od->SetCompatibleClassId(CKCID_GRID);

    return (od);
}

CKERROR CreateSetLayerVisibleProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Set Layer Visibility");

    if (!proto)
        return (CKERR_OUTOFMEMORY);

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Layer", CKPGUID_LAYERTYPE);
    proto->DeclareInParameter("Visible ?", CKPGUID_BOOL);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(SetLayerVisible);
    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

    *pproto = proto;
    return (CK_OK);
}

int SetLayerVisible(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    CKGrid *grid = (CKGrid *)beh->GetTarget();
    CKLayer *layer;

    if (!grid)
        return (CKBR_OWNERERROR);

    CKBOOL visible;
    int layerType;

    beh->ActivateInput(0, FALSE);

    beh->GetInputParameterValue(0, &layerType);
    beh->GetInputParameterValue(1, &visible);

    layer = grid->GetLayer(layerType);
    if (layer != NULL)
    {
        layer->SetVisible(visible);
    }

    beh->ActivateOutput(0);
    return CKBR_OK;
}