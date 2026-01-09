/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            GetLayerSquare
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKObjectDeclaration *FillBehaviorGetLayerSquareDecl();
CKERROR CreateGetLayerSquareProto(CKBehaviorPrototype **);
int GetLayerSquare(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorGetLayerSquareDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Get Square From 3D Pos");
    od->SetDescription("Gets the value of a square");
    /* rem:
    <SPAN CLASS=in>In : </SPAN>triggers the process.<BR>
    <SPAN CLASS=out>Out : </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Pos3d : </SPAN>3d position to be converted into a 2d position in the affected grid.<BR>
    <SPAN CLASS=pin>Ref : </SPAN>basis in which the 3d position is expressed.<BR>
    <SPAN CLASS=pin>Layer : </SPAN>Layer to be considered.<BR>
    <BR>
    <SPAN CLASS=pout>Value : </SPAN>Value to be retrieved.<BR>
    <BR><BR>
    Remember, if you want to express an object position just set the referential as your object, and let 3dpos to (0,0,0)
    */
    /* warning:
    - by now the only possible values are bound to a 0-255 range.<BR>
    */
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetCategory("Grids/Basic");
    od->SetGuid(CKGUID(0xa376e0, 0x326609e6));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateGetLayerSquareProto);
    od->SetCompatibleClassId(CKCID_BEOBJECT);
    return od;
}

CKERROR CreateGetLayerSquareProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Get Square From 3D Pos");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Pos3d", CKPGUID_VECTOR, "0,0,0");
    proto->DeclareInParameter("Ref", CKPGUID_3DENTITY);
    proto->DeclareInParameter("Layer", CKPGUID_LAYERTYPE, DEFAULT_LAYER_NAME);

    proto->DeclareOutParameter("Value", CKPGUID_INT, "255");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(GetLayerSquare);

    *pproto = proto;
    return CK_OK;
}

int GetLayerSquare(const CKBehaviorContext &behcontext)
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

    //_________________________________/ Get Preferred Grid
    CKGridManager *gm = (CKGridManager *)ctx->GetManagerByGuid(GRID_MANAGER_GUID);
    CKGrid *grid = gm->GetPreferredGrid(&pos, ref);

    int value = 0;

    if (grid)
    {
        CKLayer *layer = grid->GetLayer(layer_type);
        if (layer)
        {

            VxVector tmp;
            grid->InverseTransform(&tmp, &pos, ref);

            int x = (int)tmp.x;
            int y = (int)tmp.z;

            layer->GetValue(x, y, &value);
        }
    }

    beh->SetOutputParameterValue(0, &value);

    return CKBR_OK;
}
