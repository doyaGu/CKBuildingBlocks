/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            SetLayerSquare
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKObjectDeclaration *FillBehaviorSetLayerSquareDecl();
CKERROR CreateSetLayerSquareProto(CKBehaviorPrototype **);
int SetLayerSquare(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorSetLayerSquareDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Set Square From 3D Pos");
    od->SetDescription("Sets the value of a square");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process.<BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Pos3D: </SPAN>3d position to be converted into a 2d position in the affected grid.<BR>
    <SPAN CLASS=pin>Ref: </SPAN>basis in which the 3d position is expressed.<BR>
    <SPAN CLASS=pin>Layer: </SPAN>Layer to be conidered.<BR>
    <SPAN CLASS=pin>Value: </SPAN>Value to be put.<BR>
    <BR><BR>
    Remember, if you want to express an object position just set the referential as your object, and let 3dpos to (0,0,0)
    */
    /* warning:
    - by now the only possible values are bound to a 0-255 range.<BR>
    */
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetCategory("Grids/Basic");
    od->SetGuid(CKGUID(0x73a67252, 0x1d5b39c1));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateSetLayerSquareProto);
    od->SetCompatibleClassId(CKCID_BEOBJECT);
    return od;
}

CKERROR CreateSetLayerSquareProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Set Square From 3D Pos");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Pos 3D", CKPGUID_VECTOR, "0,0,0");
    proto->DeclareInParameter("Ref", CKPGUID_3DENTITY);
    proto->DeclareInParameter("Layer", CKPGUID_LAYERTYPE, DEFAULT_LAYER_NAME);
    proto->DeclareInParameter("Value", CKPGUID_INT, "255");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(SetLayerSquare);

    *pproto = proto;
    return CK_OK;
}

int SetLayerSquare(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;
    CKContext *ctx = behcontext.Context;

    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0);

    //_________________________________/ Retrieve Inputs
    VxVector pos(0, 0, 0);
    beh->GetInputParameterValue(0, &pos);

    CK3dEntity *ref = (CK3dEntity *)beh->GetInputParameterObject(1);

    int layer_type = 1;
    beh->GetInputParameterValue(2, &layer_type);

    int value = 255;
    beh->GetInputParameterValue(3, &value);

    //_________________________________/ Get Preferred Grid
    CKGridManager *gm = (CKGridManager *)ctx->GetManagerByGuid(GRID_MANAGER_GUID);
    CKGrid *grid = gm->GetPreferredGrid(&pos, ref);

    if (grid)
    {
        CKLayer *layer = grid->GetLayer(layer_type);
        if (layer)
        {

            if (value > 255)
                value = 255;
            else if (value < 0)
                value = 0;

            VxVector tmp;
            grid->InverseTransform(&tmp, &pos, ref);

            int x = (int)tmp.x;
            int y = (int)tmp.z;

            layer->SetValue(x, y, &value);
        }
    }

    return CKBR_OK;
}
