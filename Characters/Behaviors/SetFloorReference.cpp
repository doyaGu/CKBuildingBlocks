/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		      Set Floor Reference Object
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKERROR CreateCharacterSetFloorReferenceBehaviorProto(CKBehaviorPrototype **pproto);
int DoCharacterSetFloorReference(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorSetFloorReferenceDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Set Floor Reference Object");
    od->SetDescription("Sets the bodypart used to represent floor reference for the Character");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process<BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Floor Reference: </SPAN>entity used to represent floor reference.<BR>
    <BR>
    The floor reference is an object which represents where the floor should be
    according to character position. This floor reference must be a child of
    the RootBodyPart. Typically for a walk animation the floor reference object position is the
    same as the feet. On the other hand, a jump animation will have a fixed floor reference
    while the RootBodyPart will have an up-down movement.<BR>
  */
    od->SetCategory("Characters/Basic");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0xad75ccb, 0xcd1369a));
    od->SetAuthorGuid(CKGUID());
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateCharacterSetFloorReferenceBehaviorProto);
    od->SetCompatibleClassId(CKCID_CHARACTER);
    return od;
}

CKERROR CreateCharacterSetFloorReferenceBehaviorProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Set Floor Reference Object");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Floor Reference", CKPGUID_3DENTITY);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

    proto->SetFunction(DoCharacterSetFloorReference);
    *pproto = proto;
    return CK_OK;
}

int DoCharacterSetFloorReference(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    CKCharacter *carac = (CKCharacter *)beh->GetTarget();
    if (!carac)
        return CKBR_OWNERERROR;

    CK3dEntity *flooref = (CK3dEntity *)beh->GetInputParameterObject(0);

    if (flooref)
    {
        carac->SetFloorReferenceObject(flooref);
    }

    // Set IO states
    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0);

    return CKBR_OK;
}
