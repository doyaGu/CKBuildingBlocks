/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		            SetGridPriority
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"


CKObjectDeclaration	*FillBehaviorSetGridPriorityDecl();
CKERROR CreateSetGridPriorityProto(CKBehaviorPrototype **);
int SetGridPriority(const CKBehaviorContext& behcontext);

CKObjectDeclaration	*FillBehaviorSetGridPriorityDecl()
{
    CKObjectDeclaration *od = CreateCKObjectDeclaration("Set Grid Priority");
    od->SetDescription("Sets the grid's priority");
    /* rem:
    <SPAN CLASS=in>In : </SPAN>triggers the process.<BR>
    <SPAN CLASS=out>Out : </SPAN>is activated when the process is completed.<BR>
    <BR>
    <SPAN CLASS=pin>Priority : </SPAN>Layer to be considered.<BR>
    <BR><BR>
    See Also : 'Activate Object' and 'Deactivate Object' (for grids activation or deactivation)
    */
    od->SetType( CKDLL_BEHAVIORPROTOTYPE );
    od->SetCategory("Grids/Basic");
    od->SetGuid(CKGUID(0x74412551,0x6146504c));
    od->SetAuthorGuid(VIRTOOLS_GUID);
    od->SetAuthorName("Virtools");
    od->SetVersion(0x00010000);
    od->SetCreationFunction(CreateSetGridPriorityProto);
    od->SetCompatibleClassId(CKCID_GRID);
    return od;
}


/************************************************/
/*                Proto                         */
/************************************************/
CKERROR CreateSetGridPriorityProto(CKBehaviorPrototype **pproto)
{
  CKBehaviorPrototype *proto = CreateCKBehaviorPrototype("Set Grid Priority");
  if(!proto) return CKERR_OUTOFMEMORY;
  
  proto->DeclareInput("In");
  
  proto->DeclareOutput("Out");

  proto->DeclareInParameter("Priority", CKPGUID_INT, "0" );

  proto->SetFlags(CK_BEHAVIORPROTOTYPE_NORMAL);
  proto->SetFunction(SetGridPriority);
  proto->SetBehaviorFlags(CKBEHAVIOR_TARGETABLE);
  
  *pproto = proto;
  return CK_OK;
}



/************************************************/
/*            Behavior Function                 */
/************************************************/
int SetGridPriority(const CKBehaviorContext& behcontext)
{
    CKBehavior* beh = behcontext.Behavior;

    CKGrid *grid = (CKGrid*) beh->GetTarget();
  if( !grid ) return CKBR_OWNERERROR;

  int priority = 0;
  beh->GetInputParameterValue(0, &priority);

  grid->SetGridPriority( priority );
  beh->ActivateOutput(0,TRUE);

  return CKBR_OK;
}
