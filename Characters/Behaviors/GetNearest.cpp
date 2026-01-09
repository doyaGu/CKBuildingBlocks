/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            GetNearestObject
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKERROR CreateCharacterGetNearestObjectBehaviorProto(CKBehaviorPrototype **pproto);
int DoCharacterGetNearestObject(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorGetNearestObjectDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Get Nearest Object");
    od->SetDescription("Finds out which object in a group is closest to a character.");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process<BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Group of Objects: </SPAN>name of group of objects to test.<BR>
    <BR>
    <SPAN CLASS=pout>Nearest Object: </SPAN>object in the group which is in fact nearest to the character.<BR>
    <BR>
    */
    od->SetCategory("Characters/Basic");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x4321abcd, 0xeeaabdd));
    od->SetAuthorGuid(CKGUID());
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateCharacterGetNearestObjectBehaviorProto);
    od->SetCompatibleClassId(CKCID_CHARACTER);
    return od;
}

CKERROR CreateCharacterGetNearestObjectBehaviorProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Get Nearest Object");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Group of Objects", CKPGUID_GROUP);
    proto->DeclareOutParameter("Nearest Object", CKPGUID_3DENTITY);

    proto->SetBehaviorFlags(CKBEHAVIOR_NONE);
    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

    proto->SetFunction(DoCharacterGetNearestObject);
    *pproto = proto;
    return CK_OK;
}

int DoCharacterGetNearestObject(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    // Set IO states
    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0);

    CKCharacter *carac = (CKCharacter *)beh->GetTarget();
    if (!carac)
        return CKBR_OWNERERROR;
    CK3dEntity *root = (CK3dEntity *)carac->GetRootBodyPart();

    VxVector poscarac(0, 0, 0);
    carac->GetPosition(&poscarac);

    float distmin = 100000000.0f;
    CK3dEntity *entmin = NULL;

    // Get group
    CKGroup *group = (CKGroup *)beh->GetInputParameterObject(0);

    if (group)
    {
        CKContext *ctx = behcontext.Context;

        int count = group->GetObjectCount();
        for (int i = 0; i < count; i++)
        {
            CKObject *obj = group->GetObject(i);
            if (obj)
                if (CKIsChildClassOf(obj, CKCID_3DENTITY))
                {
                    CK3dEntity *ent = (CK3dEntity *)obj;
                    VxVector pos;
                    ent->GetPosition(&pos);
                    float rdist = Magnitude(pos - poscarac);
                    if (rdist < distmin)
                    {
                        distmin = rdist;
                        entmin = ent;
                    }
                }
        }
    }

    beh->SetOutputParameterObject(0, entmin);

    return CKBR_OK;
}
