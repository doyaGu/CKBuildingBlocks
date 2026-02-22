/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            AddAnimation
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKObjectDeclaration *FillBehaviorAddAnimationDecl();
CKERROR CreateAddAnimationProto(CKBehaviorPrototype **);
int AddAnimation(const CKBehaviorContext &context);

CKObjectDeclaration *FillBehaviorAddAnimationDecl()
{
	CKObjectDeclaration *od = CreateCKObjectDeclaration("Add Animation");
	od->SetDescription("Add an animation to a character.");
	/* rem:
	<SPAN CLASS=in>In : </SPAN>triggers the process.<BR>
	<SPAN CLASS=out>Out : </SPAN>is activated when the process is completed.<BR>
	<BR>
	<SPAN CLASS=pin>Animation : </SPAN>animation to add to the character.<BR>
	<BR>
	Adding an animation to a character remove it from its previous owner. If you want the previous
	owner to keep its animation, copy it before to add it. And to gain some memory, copy it with the
	"Character\Share Animations" flag checked.
	*/
	od->SetCategory("Characters/Animation");
	od->SetType(CKDLL_BEHAVIORPROTOTYPE);
	od->SetGuid(CKGUID(0x6a794228, 0x77197f5e));
	od->SetAuthorGuid(VIRTOOLS_GUID);
	od->SetAuthorName("Virtools");
	od->SetVersion(0x00010000);
	od->SetCreationFunction(CreateAddAnimationProto);
	od->SetCompatibleClassId(CKCID_CHARACTER);
	return od;
}

CKERROR CreateAddAnimationProto(CKBehaviorPrototype **pproto)
{
	CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Add Animation");
	if (!proto)
		return CKERR_OUTOFMEMORY;

	proto->DeclareInput("In");
	proto->DeclareOutput("Out");

	proto->DeclareInParameter("Animation", CKPGUID_ANIMATION);

	proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);
	proto->SetFunction(AddAnimation);

	*pproto = proto;
	return CK_OK;
}

int AddAnimation(const CKBehaviorContext &behcontext)
{
	CKBehavior *beh = behcontext.Behavior;

	beh->ActivateInput(0, FALSE);
	beh->ActivateOutput(0);

	// Get the character
	CKCharacter *car = (CKCharacter *)beh->GetTarget();
	if (!car)
		return CKBR_OK;

	// Get the animation
	CKKeyedAnimation *anim = (CKKeyedAnimation *)beh->GetInputParameterObject(0);
	if (!CKIsChildClassOf(anim, CKCID_ANIMATION))
		return CKBR_OK;

	// Add the animation to the character
	car->AddAnimation(anim);

	//-- For each sub-object animation in the animation
	//-- we try to find the bodypart it applis to...
	int i, NbObjectAnimation = anim->GetAnimationCount();

	// If Load Animation on a character : Find bodyparts on which animations should apply
	int AttribCount = 0;
	for (i = 0; i < NbObjectAnimation; ++i)
	{
		CKObjectAnimation *oanim = (CKObjectAnimation *)anim->GetAnimation(i);
		CKSTRING Name = oanim->GetName();
		int Length = (int)strlen(Name);
		CKBodyPart *OldPart = (CKBodyPart *)oanim->Get3dEntity(); // Previous target of object animation

		if (Name)
		{
			int count = car->GetBodyPartCount();
			for (int k = 0; k < count; k++)
			{
				CKBodyPart *part = car->GetBodyPart(k);
				if (part && part->GetName() && OldPart != part)
				{
					int BpartLength = (int)strlen(part->GetName());
					if (Length == BpartLength)
					{
						// Does name exactly match ?
						if (!strcmp(part->GetName(), Name))
						{
							//-- Need to remove animation from previous bodypart
							if (OldPart)
							{
								OldPart->RemoveObjectAnimation(oanim);
							}
							// Mark animation as attributed
							// to skip it in next loop
							++AttribCount;
							oanim->ModifyObjectFlags(CK_OBJECT_TEMPMARKER, 0);
							oanim->Set3dEntity((CK3dEntity *)part);
							part->AddObjectAnimation(oanim);
						}
					}
				}
			}
		}
	}

	//-- for animation that were not applied , retry with a filter on name (eg : we accept to
	// apply animation "tete" on "tete000" for characters that would have been copied
	if (AttribCount != NbObjectAnimation)
	{
		for (i = 0; i < NbObjectAnimation; ++i)
		{
			CKObjectAnimation *oanim = (CKObjectAnimation *)anim->GetAnimation(i);

			if (!(oanim->GetObjectFlags() & CK_OBJECT_TEMPMARKER))
			{
				CKSTRING Name = oanim->GetName();
				int Length = (int)strlen(Name);
				CKBodyPart *OldPart = (CKBodyPart *)oanim->Get3dEntity(); // Previous target of object animation

				if (Name)
				{
					int count = car->GetBodyPartCount();
					for (int k = 0; k < count; k++)
					{
						CKBodyPart *part = car->GetBodyPart(k);
						if (part && part->GetName() && OldPart != part)
						{
							int BpartLength = (int)strlen(part->GetName());
							// Does name exactly match ?
							if ((Length + 3) == BpartLength)
							{
								const char *NameEnd = part->GetName() + BpartLength - 3;
								if ((NameEnd[0] >= '0') && (NameEnd[0] <= '9') &&
									(NameEnd[1] >= '0') && (NameEnd[1] <= '9') &&
									(NameEnd[1] >= '0') && (NameEnd[1] <= '9'))
								{
									if (!strncmp(part->GetName(), Name, Length))
									{
										//-- Need to remove animation from previous bodypart
										if (OldPart)
										{
											OldPart->RemoveObjectAnimation(oanim);
										}
										oanim->Set3dEntity((CK3dEntity *)part);
										part->AddObjectAnimation(oanim);
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// Remove temporary marker...
	for (i = 0; i < NbObjectAnimation; ++i)
	{
		CKObjectAnimation *oanim = (CKObjectAnimation *)anim->GetAnimation(i);
		oanim->ModifyObjectFlags(0, CK_OBJECT_TEMPMARKER);
	}

	return CKBR_OK;
}
