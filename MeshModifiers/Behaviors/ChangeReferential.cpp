/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            ChangeReferential
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKERROR CreateChangeReferentialBehaviorProto(CKBehaviorPrototype **);
int ChangeReferential(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorChangeReferentialDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Change Referential");
    od->SetDescription("Change the referential of a specified object, with another object referential.");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process.<BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Referential to match: </SPAN>the referential (3D Entity) on which our object axis should be match.<BR>
    <BR>
    With this building block, you can change the pivot of rotation of an object.<BR>
    */
    od->SetCategory("Mesh Modifications/Local Deformation");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x5f6428f2, 0x4e55c7a));
    od->SetAuthorGuid(CKGUID());
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateChangeReferentialBehaviorProto);
    od->SetCompatibleClassId(CKCID_3DOBJECT);
    return od;
}

CKERROR CreateChangeReferentialBehaviorProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Change Referential");
    if (!proto)
        return CKERR_OUTOFMEMORY;

    proto->DeclareInput("In");
    proto->DeclareOutput("Out");

    proto->DeclareInParameter("Referential to match", CKPGUID_3DENTITY);

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);

    proto->SetFunction(ChangeReferential);
    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);

    *pproto = proto;
    return CK_OK;
}

int ChangeReferential(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    beh->ActivateInput(0, FALSE);
    beh->ActivateOutput(0);

    CK3dEntity *object = (CK3dEntity *)beh->GetTarget();
    if (!object)
        return CKBR_OWNERERROR;

    CK3dEntity *ref = (CK3dEntity *)beh->GetInputParameterObject(0);

    object->ChangeReferential(ref);

    return CKBR_OK;
}
