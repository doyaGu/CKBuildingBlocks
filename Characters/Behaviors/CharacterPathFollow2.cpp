/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//	          Character Path Follow  v2
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKERROR CreateCharacterPathFollow2BehaviorProto(CKBehaviorPrototype **pproto);
int DoCharacterPathFollow2(const CKBehaviorContext &behcontext);

CKObjectDeclaration *FillBehaviorCharacterPathFollow2Decl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Enhanced Character Curve Follow");
    od->SetDescription("Makes a charcter follow all or a specified part of a curve performing a specified animation.");
    /* rem:
    <SPAN CLASS=in>In: </SPAN>triggers the process.<BR>
    <SPAN CLASS=in>Loop In: </SPAN>triggers the next step in a behavior's process loop.<BR>
    <BR>
    <SPAN CLASS=out>Out: </SPAN>is activated when the process is completed.<BR>
    <SPAN CLASS=out>Loop Out: </SPAN>is activated when the behavior's process is not yet completed.<BR>
    <BR>
    <SPAN CLASS=pin>Curve To Follow: </SPAN>name of the curve the character will follow.<BR>
    <SPAN CLASS=pin>Start Percentage: </SPAN>where the character should start on the curve, with 0 (zero) being the beginning, 50% being midway etc.<BR>
    <SPAN CLASS=pin>End Percentage: </SPAN>point on the curve at which the character should stop, with 100% being the end.<BR>
    <SPAN CLASS=pin>Character Direction: </SPAN>defines the character's frontal direction.<BR>
    <SPAN CLASS=pin>Animation: </SPAN>name of animation the character should perform whilst following the curve.<BR>
    <BR>
    <SPAN CLASS=pout>Progression: </SPAN>percentage between 0% and 100% which defines the progression of the process. start=0%, middle time=50%, end=100%.
    Useful if you need to interpolates at the same time (in the same loop) a color vlaue, a vector, a orientation or something like this.<BR>
    <BR>
    See Also: "Character Path Follow", "Character Go To".<BR>
    */
    /* warning:
    - also Remember: PERCENTAGE values are compatible with FLOAT values, and ANGLE values.<BR>
    */
    od->SetCategory("Characters/Movement");
    od->SetType(CKDLL_BEHAVIORPROTOTYPE);
    od->SetGuid(CKGUID(0x785e7760, 0x46ea50b5));
    od->SetAuthorGuid(CKGUID());
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateCharacterPathFollow2BehaviorProto);
    od->SetCompatibleClassId(CKCID_CHARACTER);
    return od;
}

CKERROR CreateCharacterPathFollow2BehaviorProto(CKBehaviorPrototype **pproto)
{
    CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Enhanced Character Curve Follow");
    if (!proto)
        return CKERR_OUTOFMEMORY;
    proto->SetBehaviorFlags((CK_BEHAVIOR_FLAGS)(CKBEHAVIOR_ACTIVE | CKBEHAVIOR_MESSAGESENDER));

    proto->DeclareInput("In");
    proto->DeclareInput("Loop In");

    proto->DeclareOutput("Out");
    proto->DeclareOutput("Loop Out");

    proto->DeclareInParameter("Curve To Follow", CKPGUID_CURVE);
    proto->DeclareInParameter("Start Percentage", CKPGUID_PERCENTAGE, "0");
    proto->DeclareInParameter("End Percentage", CKPGUID_PERCENTAGE, "100");
    proto->DeclareInParameter("Character Direction", CKPGUID_DIRECTION, "X");
    proto->DeclareInParameter("Animation", CKPGUID_ANIMATION, "NULL");
    proto->DeclareInParameter("Loop", CKPGUID_BOOL, "FALSE");

    proto->DeclareOutParameter("Current Percentage", CKPGUID_PERCENTAGE, "0");

    proto->DeclareLocalParameter(NULL, CKPGUID_FLOAT);  // Frame
    proto->DeclareLocalParameter(NULL, CKPGUID_VECTOR); // Old Position
    proto->DeclareLocalParameter(NULL, CKPGUID_FLOAT);  // Current Length
    proto->DeclareLocalParameter(NULL, CKPGUID_FLOAT);  // Total Length
    proto->DeclareLocalParameter(NULL, CKPGUID_BOOL);   // Reverse ?

    proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
    proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);
    proto->SetFunction(DoCharacterPathFollow2);

    *pproto = proto;
    return CK_OK;
}

int DoCharacterPathFollow2(const CKBehaviorContext &behcontext)
{
    CKBehavior *beh = behcontext.Behavior;

    CKCharacter *Character = (CKCharacter *)beh->GetTarget();
    if (!Character)
        return CKBR_OWNERERROR;
    VxVector pos;
    Character->GetPosition(&pos);
    float caracheight = pos.y;

    // Get curve to follow
    CKCurve *Curve = (CKCurve *)beh->GetInputParameterObject(0);
    float Length = Curve->GetLength();

    // Get character's frontal direction;
    CKBOOL DownToUp;
    int CharacterDirection = 1;
    beh->GetInputParameterValue(3, &CharacterDirection);

    // Get WALK animation;
    CKAnimation *WalkAnimation = (CKAnimation *)beh->GetInputParameterObject(4);
    //	Character->SetActiveAnimation(WalkAnimation);
    if (WalkAnimation)
        WalkAnimation->SetCanBeInterrupt(TRUE);
    VxVector CurvePosition, CurveDirection, Up(0, 1, 0);
    CKBOOL Loop = FALSE;
    beh->GetInputParameterValue(5, &Loop);

    //___________________________ In
    if (beh->IsInputActive(0))
    {
        // Initialization
        beh->ActivateInput(0, FALSE);

        // Get start percentage
        float Start = 0;
        beh->GetInputParameterValue(1, &Start);
        float CurrentLength = Length * Start;
        beh->SetLocalParameterValue(2, &CurrentLength);

        // Get end percentage
        float End = 1;
        beh->GetInputParameterValue(2, &End);
        float TotalLength = Length * End;
        beh->SetLocalParameterValue(3, &TotalLength);

        // Store the direction's course in DownToUp
        if (Start <= End)
            DownToUp = TRUE;
        else
            DownToUp = FALSE;
        beh->SetLocalParameterValue(4, &DownToUp);

        // Get position and direction on curve at the starting point
        Curve->GetPos(Start, &CurvePosition);
        Curve->GetPos(Start + 0.01f, &CurveDirection);
        CurveDirection -= CurvePosition;
        CurveDirection.y = 0;
        CurveDirection.Normalize();
        beh->SetLocalParameterValue(1, &CurvePosition);

        // Align character on curve
        Character->AlignCharacterWithRootPosition();
        CurvePosition.y = caracheight;
        Character->SetPosition(&CurvePosition);

        switch (CharacterDirection)
        {
        case 1:
        {
            VxVector r = CrossProduct(CurveDirection, Up);
            Character->SetOrientation(&r, &Up);
        }
        break;
        case 2:
        {
            VxVector r = CrossProduct(Up, CurveDirection);
            Character->SetOrientation(&r, &Up);
        }
        break;
        case 3:
            Character->SetOrientation(&Up, &CurveDirection);
            break;
        case 4:
        {
            Up *= -1;
            Character->SetOrientation(&Up, &CurveDirection);
        }
        break;
        case 5:
            Character->SetOrientation(&CurveDirection, &Up);
            break;
        case 6:
        {
            CurveDirection *= -1;
            Character->SetOrientation(&CurveDirection, &Up);
        }
        break;
        }
        Character->SetNextActiveAnimation(WalkAnimation, CK_TRANSITION_WARPBEST | CK_TRANSITION_LOOPIFEQUAL | CK_TRANSITION_USEVELOCITY, 5);

        beh->ActivateOutput(1, TRUE);
        return CKBR_OK;
    }

    //___________________________ Loop In
    if (beh->IsInputActive(1))
    {
        beh->ActivateInput(1, FALSE);
        Character->SetAutomaticProcess(TRUE);

        // Get current character's position
        VxVector Position;
        Character->GetPosition(&Position);

        // Get old character's position
        VxVector OldPosition;
        beh->GetLocalParameterValue(1, &OldPosition);

        float CurrentLength;
        beh->GetLocalParameterValue(2, &CurrentLength);
        float TotalLength;
        beh->GetLocalParameterValue(3, &TotalLength);

        VxVector Displacement = Position - OldPosition;
        Displacement.y = 0;

        beh->GetLocalParameterValue(4, &DownToUp);

        if (DownToUp)
        {
            // The character is walking in the positive sens
            CurrentLength += Magnitude(Displacement);
            beh->SetLocalParameterValue(2, &CurrentLength);

            if (CurrentLength >= TotalLength)
            {
                // The character reached the end point
                float CurveStep = 1.0f;
                beh->ActivateOutput(0, TRUE);
                beh->SetOutputParameterValue(0, &CurveStep);
                if (!Loop)
                    Character->SetNextActiveAnimation(NULL, CK_TRANSITION_WARPBEST | CK_TRANSITION_LOOPIFEQUAL | CK_TRANSITION_USEVELOCITY, 5);
                return CKBR_OK;
            }
            else
            {
                // The character continues its progression
                float CurveStep = CurrentLength / Length;
                beh->SetOutputParameterValue(0, &CurveStep);
                Curve->GetPos(CurveStep, &CurvePosition);
                Curve->GetPos(CurveStep + 0.01f, &CurveDirection);
            }
        }
        else
        {
            // The character is walking in the negative sens
            CurrentLength -= Magnitude(Displacement);
            beh->SetLocalParameterValue(2, &CurrentLength);

            if (CurrentLength < TotalLength)
            {
                // The character reached the end point
                beh->ActivateOutput(0, TRUE);
                float CurveStep = 0.0f;
                beh->SetOutputParameterValue(0, &CurveStep);
                if (!Loop)
                    Character->SetNextActiveAnimation(NULL, CK_TRANSITION_WARPBEST | CK_TRANSITION_LOOPIFEQUAL | CK_TRANSITION_USEVELOCITY, 5);
                return CKBR_OK;
            }
            else
            {
                // The character continues its progression
                float CurveStep = CurrentLength / Length;
                Curve->GetPos(CurveStep, &CurvePosition);
                Curve->GetPos(CurveStep + 0.01f, &CurveDirection);
                beh->SetOutputParameterValue(0, &CurveStep);
            }
        }

        beh->SetLocalParameterValue(1, &CurvePosition);
        CurveDirection -= CurvePosition;
        CurveDirection.y = 0;
        CurveDirection.Normalize();

        // Align character on curve
        CurvePosition.y = caracheight;
        Character->SetPosition(&CurvePosition);

        switch (CharacterDirection)
        {
        case 1:
        {
            VxVector r = CrossProduct(CurveDirection, Up);
            Character->SetOrientation(&r, &Up);
        }
        break;
        case 2:
        {
            VxVector r = CrossProduct(Up, CurveDirection);
            Character->SetOrientation(&r, &Up);
        }
        break;
        case 3:
            Character->SetOrientation(&Up, &CurveDirection);
            break;
        case 4:
        {
            Up *= -1;
            Character->SetOrientation(&Up, &CurveDirection);
        }
        break;
        case 5:
            Character->SetOrientation(&CurveDirection, &Up);
            break;
        case 6:
        {
            CurveDirection *= -1;
            Character->SetOrientation(&CurveDirection, &Up);
        }
        break;
        }

        Character->SetNextActiveAnimation(WalkAnimation, CK_TRANSITION_WARPBEST | CK_TRANSITION_LOOPIFEQUAL | CK_TRANSITION_USEVELOCITY, 5);

        beh->ActivateOutput(1, TRUE);
        return CKBR_OK;
    }
    return CKBR_OK;
}
