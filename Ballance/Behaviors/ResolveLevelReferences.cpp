#include "CKAll.h"
#include "BallanceHelpers.h"

CKObjectDeclaration *FillBehaviorBallanceResolveLevelReferencesDecl();
CKERROR CreateBallanceResolveLevelReferencesProto(CKBehaviorPrototype **pproto);
int BallanceResolveLevelReferences(const CKBehaviorContext &behcontext);

struct BallanceReferenceDecl
{
    const char *outputName;
    const char *objectName;
    CKGUID guid;
    CK_CLASSID cid;
};

static const BallanceReferenceDecl kReferences[] = {
    {"Main Menu Script", "Menu_Main", CKPGUID_SCRIPT, CKCID_BEHAVIOR},
    {"Levelinit_build Script", "Levelinit_build", CKPGUID_SCRIPT, CKCID_BEHAVIOR},
    {"Gameplay_Ingame Script", "Gameplay_Ingame", CKPGUID_SCRIPT, CKCID_BEHAVIOR},
    {"Music_Manager Script", "Music_Manager", CKPGUID_SCRIPT, CKCID_BEHAVIOR},
    {"Cam_MF Entity", "Cam_MF", CKPGUID_3DENTITY, CKCID_3DENTITY},
    {"Balls_MF Entity", "Balls_MF", CKPGUID_3DENTITY, CKCID_3DENTITY},
    {"AnimTrafo_MF Entity", "AnimTrafo_MF", CKPGUID_3DENTITY, CKCID_3DENTITY},
    {"SkyLayer Entity", "SkyLayer", CKPGUID_3DENTITY, CKCID_3DENTITY},
    {"Menu_Pause Script", "Menu_Pause", CKPGUID_SCRIPT, CKCID_BEHAVIOR},
    {"Ball_Paper Entity", "Ball_Paper", CKPGUID_3DENTITY, CKCID_3DENTITY},
    {"Ball_Wood Entity", "Ball_Wood", CKPGUID_3DENTITY, CKCID_3DENTITY},
    {"Ball_Stone Entity", "Ball_Stone", CKPGUID_3DENTITY, CKCID_3DENTITY},
    {"IngameParameter Array", "IngameParameter", CKPGUID_DATAARRAY, CKCID_DATAARRAY},
    {"Gameplay_SectorManager Script", "Gameplay_SectorManager", CKPGUID_SCRIPT, CKCID_BEHAVIOR},
    {"CurrentLevel Array", "CurrentLevel", CKPGUID_DATAARRAY, CKCID_DATAARRAY},
    {"PH Array", "PH", CKPGUID_DATAARRAY, CKCID_DATAARRAY},
    {"Gameplay_Events Script", "Gameplay_Events", CKPGUID_SCRIPT, CKCID_BEHAVIOR},
    {"Menu_Dead Script", "Menu_Dead", CKPGUID_SCRIPT, CKCID_BEHAVIOR},
    {"Menu_Score Script", "Menu_Score", CKPGUID_SCRIPT, CKCID_BEHAVIOR},
    {"Menu_End Script", "Menu_End", CKPGUID_SCRIPT, CKCID_BEHAVIOR},
    {"Menu_HighscoreEntry Script", "Menu_HighscoreEntry", CKPGUID_SCRIPT, CKCID_BEHAVIOR},
    {"Menu_Highscore Script", "Menu_Highscore", CKPGUID_SCRIPT, CKCID_BEHAVIOR},
    {"Menu_ScriptParameters Array", "Menu_ScriptParameters", CKPGUID_DATAARRAY, CKCID_DATAARRAY},
    {"Gameplay_Tutorial Script", "Gameplay_Tutorial", CKPGUID_SCRIPT, CKCID_BEHAVIOR},
    {"Gameplay_Refresh Script", "Gameplay_Refresh", CKPGUID_SCRIPT, CKCID_BEHAVIOR},
    {"All_Gameplay Group", "All_Gameplay", CKPGUID_GROUP, CKCID_GROUP},
    {"Interface_Life_End 2dEntity", "Interface_Life_End", CKPGUID_2DENTITY, CKCID_2DENTITY},
    {"Interface_Life_Kugel 2dEntity", "Interface_Life_Kugel", CKPGUID_2DENTITY, CKCID_2DENTITY},
    {"Interface_Life_Startbogen 2dEntity", "Interface_Life_Startbogen", CKPGUID_2DENTITY, CKCID_2DENTITY},
    {"Checkpoints Array", "Checkpoints", CKPGUID_DATAARRAY, CKCID_DATAARRAY},
    {"Sound_Refresh Script", "Sound_Refresh", CKPGUID_SCRIPT, CKCID_BEHAVIOR},
    {"All_Sound Group", "All_Sound", CKPGUID_GROUP, CKCID_GROUP},
    {"HUD_sprites Group", "HUD_sprites", CKPGUID_GROUP, CKCID_GROUP},
    {"All_Balls Group", "All_Balls", CKPGUID_GROUP, CKCID_GROUP},
};

CKObjectDeclaration *FillBehaviorBallanceResolveLevelReferencesDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Ballance Resolve Level References");
    od->SetDescription("Resolves the named scripts, arrays, groups, and entities used by Ballance level logic.");
    od->SetCategory("Ballance/Runtime");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x42414c06, 0x10000006));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Ballance");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateBallanceResolveLevelReferencesProto);
    od->SetCompatibleClassId(CKCID_BEOBJECT);
    return od;
}

CKERROR CreateBallanceResolveLevelReferencesProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Ballance Resolve Level References");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Resolved");
    proto->DeclareOutput("Missing");

    for (int i = 0; i < (int)(sizeof(kReferences) / sizeof(kReferences[0])); ++i)
        proto->DeclareOutParameter(const_cast<CKSTRING>(kReferences[i].outputName), kReferences[i].guid);
    proto->DeclareOutParameter("Missing Count", CKPGUID_INT);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(BallanceResolveLevelReferences);

    *pproto = proto;
    return CK_OK;
}

int BallanceResolveLevelReferences(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;
    CKContext *ctx = behcontext.Context;
    beh->ActivateInput(0, FALSE);

    int missing = 0;
    const int refCount = (int)(sizeof(kReferences) / sizeof(kReferences[0]));
    for (int i = 0; i < refCount; ++i)
    {
        CKObject *object = Ballance::FindObject(ctx, kReferences[i].objectName, kReferences[i].cid);
        if (!object)
            ++missing;
        beh->SetOutputParameterObject(i, object);
    }

    beh->SetOutputParameterValue(refCount, &missing);
    beh->ActivateOutput(missing == 0 ? 0 : 1);
    return CKBR_OK;
}
