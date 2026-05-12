//////////////////////////////////////
//////////////////////////////////////
//
//        TT Switch on Layer
//
//////////////////////////////////////
//////////////////////////////////////
#include "CKAll.h"
#include "ToolboxGuids.h"

CKObjectDeclaration *FillBehaviorSwitchonLayerDecl();
CKERROR CreateSwitchonLayerProto(CKBehaviorPrototype **pproto);
int SwitchonLayer(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorSwitchonLayerDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("TT Switch on Layer");
    od->SetDescription("Gets the value of a square");
    od->SetCategory("TT Toolbox/Grids");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x1a151160, 0x1be26257));
    od->SetAuthorGuid(TERRATOOLS_GUID);
    od->SetAuthorName("Terratools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateSwitchonLayerProto);
    od->NeedManager(GRID_MANAGER_GUID);
    od->SetCompatibleClassId(CKCID_3DENTITY);
    return od;
}

CKERROR CreateSwitchonLayerProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("TT Switch on Layer");
    if (!proto) return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareInput("Off");

    proto->DeclareOutput("In");
    proto->DeclareOutput("Out");
    proto->DeclareOutput("Enter");
    proto->DeclareOutput("Exit");

    proto->DeclareInParameter("Ref", CKPGUID_3DENTITY);
    proto->DeclareInParameter("Layer", CKPGUID_LAYERTYPE, "- default -");
    proto->DeclareInParameter("Value", CKPGUID_INT, "255");

    proto->DeclareOutParameter("Exitvector", CKPGUID_VECTOR);

    proto->DeclareLocalParameter("Last Position", CKPGUID_VECTOR);
    proto->DeclareLocalParameter("Was inside", CKPGUID_BOOL);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(SwitchonLayer);

    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

    *pproto = proto;
    return CK_OK;
}

int SwitchonLayer(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;
    CKContext *ctx = behcontext.Context;

    CK3dEntity *target = (CK3dEntity *)beh->GetTarget();
    if (!target)
        return CKBR_OWNERERROR;

    VxVector position;
    target->GetPosition(&position);

    if (beh->IsInputActive(1))
    {
        beh->ActivateInput(1, FALSE);
        return CKBR_OK;
    }

    if (beh->IsInputActive(0))
    {
        beh->ActivateInput(0, FALSE);
        beh->SetLocalParameterValue(0, &position);
        CKBOOL wasInside = FALSE;
        beh->SetLocalParameterValue(1, &wasInside);
    }

    int checkValue = 0;
    beh->GetInputParameterValue(2, &checkValue);

    CK3dEntity *refEntity = (CK3dEntity *)beh->GetInputParameterObject(0);
    int layer = 1;
    beh->GetInputParameterValue(1, &layer);

    CKGridManager *gridManager = (CKGridManager *)ctx->GetManagerByGuid(GRID_MANAGER_GUID);
    CKGrid *grid = gridManager ? gridManager->GetPreferredGrid(&position, refEntity) : nullptr;
    if (grid)
    {
        CKLayer *layerObj = grid->GetLayer(layer);
        if (layerObj)
        {
            int gridX = 0;
            int gridZ = 0;
            grid->Get2dCoordsFrom3dPos(&position, &gridX, &gridZ);

            int faceValue = 0;
            layerObj->GetValue(gridX, gridZ, &faceValue);

            CKBOOL wasInside = FALSE;
            beh->GetLocalParameterValue(1, &wasInside);

            if (faceValue == checkValue)
            {
                if (wasInside)
                {
                    beh->ActivateOutput(0, TRUE);
                }
                else
                {
                    wasInside = TRUE;
                    beh->SetLocalParameterValue(1, &wasInside);
                    beh->ActivateOutput(2, TRUE);
                }
            }
            else if (wasInside)
            {
                wasInside = FALSE;
                beh->SetLocalParameterValue(1, &wasInside);

                VxVector lastPos;
                beh->GetLocalParameterValue(0, &lastPos);

                VxVector exitVector;
                exitVector.x = position.x - lastPos.x;
                exitVector.y = position.y - lastPos.y;
                exitVector.z = position.z - lastPos.z;

                beh->SetOutputParameterValue(0, &exitVector);
                beh->ActivateOutput(3, TRUE);
            }
            else
            {
                beh->ActivateOutput(1, TRUE);
            }
        }
    }

    beh->SetLocalParameterValue(0, &position);
    return CKBR_ACTIVATENEXTFRAME;
}
