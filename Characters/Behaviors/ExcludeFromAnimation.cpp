/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            ExcludeFromAnimation
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKObjectDeclaration *FillBehaviorExcludeFromAnimationDecl();
CKERROR CreateExcludeFromAnimationProto(CKBehaviorPrototype **pproto);
int ExcludeFromAnimation(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorExcludeFromAnimationDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Exclude From Animation");
    od->SetDescription("Excludes the selected 3D Entity from all taking part in any animation that uses it.");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process.<BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Exclude: </SPAN>if TRUE the 3D Entity will be excluded, if FALSE it will be re-included.<BR>
    <SPAN CLASS=pin>Hierarchy: </SPAN>if TRUE, then this behavior will also apply to the 3D Entity's children.<BR>
    <BR>
    An example of this building block would be excluding all the body-parts of an arm during a
    character's walking naimation. The character would still walk, only the arm subject to this behavior
    would not move (it would not swing as arms normally do during walking movements) at all and would
    simply dangle lifelessly.<BR>
    */
    /* warning:
    This Behavior neither hides nor removes the object from the scene, it simply prevents it from taking
    part in animations.<BR>
    */
    od->SetCategory("Characters/Animation");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x4d3f515b, 0x450c20c6));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateExcludeFromAnimationProto);
    od->SetCompatibleClassId(CKCID_3DENTITY);
    return od;
}

CKERROR CreateExcludeFromAnimationProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Exclude From Animation");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Exclude", CKPGUID_BOOL, "TRUE");
    proto->DeclareInParameter("Hierarchy", CKPGUID_BOOL, "TRUE");

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetFunction(ExcludeFromAnimation);
    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

    *pproto = proto;
    return CK_OK;
}

void ExcludeHierarchy(CK3dEntity *ent, CKBOOL b)
{
    ent->IgnoreAnimations(b);
    for (int i = 0; i < ent->GetChildrenCount(); i++)
    {
        CK3dEntity *child = ent->GetChild(i);
        ExcludeHierarchy(child, b);
    }
}

int ExcludeFromAnimation(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    CK3dEntity *ent = (CK3dEntity *)beh->GetTarget();
    if (!ent)
        return CKBR_OWNERERROR;

    // Set IO states
    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0);

    // Get frame number
    CKBOOL b = TRUE;
    beh->GetInputParameterValue(0, &b);
    CKBOOL h = TRUE;
    beh->GetInputParameterValue(1, &h);

    if (h)
    {
        ExcludeHierarchy(ent, b);
    }
    else
    {
        ent->IgnoreAnimations(b);
    }

    return CKBR_OK;
}
