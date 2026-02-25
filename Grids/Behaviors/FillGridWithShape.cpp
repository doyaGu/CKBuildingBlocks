////////////////////////
////////////////////////
//
// FillGridWithObjectShape
//
////////////////////////
////////////////////////
#include "CKAll.h"
#include "GridManager.h"

CKObjectDeclaration *FillBehaviorFillGridWithShapeDecl();
CKERROR CreateFillGridWithShapeProto(CKBehaviorPrototype **);
int FillGridWithShape(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorFillGridWithShapeDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("3D Entity Fill");
    od->SetDescription("Fill a grid's layer using the bounding box of a 3D Entity");
    /* Note:
    <SPAN CLASS=in>In: </SPAN>triggers the process<BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Object: </SPAN>to get shape from.<BR>
    <SPAN CLASS=pin>Solid Layer: </SPAN>layer to be filled with object inner surface.<BR>
    <SPAN CLASS=pin>Shape Layer: </SPAN>layer to be filled with object outlines.<BR>
    <SPAN CLASS=pin>Value: </SPAN>value to be written.<BR>
    <BR>
    The "3D Entity Fill" building block fills grid squares with an object shape & surface.<BR>
    */
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetCategory("Grids/Basic");
    od->SetGuid(CKGUID(0x672f0997, 0x563644cc));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateFillGridWithShapeProto);
    od->SetCompatibleClassId(CKCID_BEOBJECT);
    return od;
}

CKERROR CreateFillGridWithShapeProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("3D Entity Fill");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Object", CKPGUID_3DENTITY);
    proto->DeclareInParameter("Solid Layer", CKPGUID_LAYERTYPE);
    proto->DeclareInParameter("Shape Layer", CKPGUID_LAYERTYPE);
    proto->DeclareInParameter("Value", CKPGUID_INT, "255"); /// TODO: Type of the input parameter is variable

    proto->DeclareSetting("Only Bounding Box", CKPGUID_BOOL, "TRUE");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(FillGridWithShape);

    *pproto = proto;
    return CK_OK;
}

int FillGridWithShape(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0);

    CKGridManager *gridManager = (CKGridManager *)behcontext.Context->GetManagerByGuid(GRID_MANAGER_GUID);
    if (!gridManager)
        return CKBR_BEHAVIORERROR;

    // entity
    CK3dEntity *ent = (CK3dEntity *)beh->GetInputParameterObject(0);
    if (!ent)
        return CKBR_OK;

    // solide type
    int solidType = -1;
    beh->GetInputParameterValue(1, &solidType);

    // shape type
    int shapeType = -1;
    beh->GetInputParameterValue(2, &shapeType);

    // value
    int value = 255;
    beh->GetInputParameterValue(3, &value);

    // Bbox only ?
    CKBOOL BoundingBoxOnly = FALSE;
    beh->GetLocalParameterValue(0, &BoundingBoxOnly);

    if (BoundingBoxOnly)
    {
        ((GridManager *)gridManager)->FillBoundingBox(ent->GetBoundingBox(), solidType, &value);
    }
    else
        gridManager->FillGridWithObjectShape(ent, solidType, shapeType, &value);

    return CKBR_OK;
}
