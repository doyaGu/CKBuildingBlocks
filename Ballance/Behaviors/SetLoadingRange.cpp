#include "CKAll.h"
#include "BallanceHelpers.h"

CKObjectDeclaration *FillBehaviorBallanceSetLoadingRangeDecl();
CKERROR CreateBallanceSetLoadingRangeProto(CKBehaviorPrototype **pproto);
int BallanceSetLoadingRange(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorBallanceSetLoadingRangeDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Ballance Set Loading Range");
    od->SetDescription("Writes the Ballance loading parameter row used to select an NMO table range.");
    od->SetCategory("Ballance/Loading");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x42414c01, 0x10000001));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Ballance");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateBallanceSetLoadingRangeProto);
    od->SetCompatibleClassId(CKCID_BEOBJECT);
    return od;
}

CKERROR CreateBallanceSetLoadingRangeProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Ballance Set Loading Range");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");
    proto->DeclareOutput("Failed");

    proto->DeclareInParameter("Loading Parameters", CKPGUID_DATAARRAY);
    proto->DeclareInParameter("Row Index", CKPGUID_INT, "0");
    proto->DeclareInParameter("NMO Table", CKPGUID_DATAARRAY);
    proto->DeclareInParameter("Start Row", CKPGUID_INT, "0");
    proto->DeclareInParameter("End Row", CKPGUID_INT, "0");
    proto->DeclareInParameter("Load ID", CKPGUID_INT, "0");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(BallanceSetLoadingRange);

    *pproto = proto;
    return CK_OK;
}

int BallanceSetLoadingRange(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;
    beh->ActivateInput(0, FALSE);

    CKDataArray *loading = (CKDataArray *)beh->GetInputParameterObject(0);
    int row = 0;
    int start = 0;
    int end = 0;
    int loadId = 0;
    beh->GetInputParameterValue(1, &row);
    CKDataArray *table = (CKDataArray *)beh->GetInputParameterObject(2);
    beh->GetInputParameterValue(3, &start);
    beh->GetInputParameterValue(4, &end);
    beh->GetInputParameterValue(5, &loadId);

    if (!loading || !table || loading->GetColumnCount() < 4 || row < 0)
    {
        beh->ActivateOutput(1);
        return CKBR_PARAMETERERROR;
    }

    while (loading->GetRowCount() <= row)
        loading->InsertRow();

    Ballance::SetObjectCell(loading, row, 0, table);
    Ballance::SetIntCell(loading, row, 1, start);
    Ballance::SetIntCell(loading, row, 2, end);
    Ballance::SetIntCell(loading, row, 3, loadId);

    beh->ActivateOutput(0);
    return CKBR_OK;
}
