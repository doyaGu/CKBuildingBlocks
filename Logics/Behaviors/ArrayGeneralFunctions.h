/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//
//		              Array General Functions
//
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
#include "CKAll.h"

CKERROR ArrayLineOutputCallBack(const CKBehaviorContext &behcontext);

void ReadInputParameters(CKDataArray *array, CKDataRow *it, CKBehavior *beh, int firstinputindex);

int GetInputValue(CKDataArray *array, CKBehavior *beh, int column, int pindex, CKUINTPTR &key);

void WriteOutputParameters(CKDataArray *array, CKDataRow *it, CKBehavior *beh, int firstoutputindex);

void CreateInputParameter(CKDataArray *array, CKBehavior *beh, int c, int pos, const char *pname = NULL);
void CreateOutputParameter(CKDataArray *array, CKBehavior *beh, int c, int pos, const char *pname = NULL);

void CreateInputParameters(CKDataArray *array, CKBehavior *beh, int firstoutputindex);
void CreateOutputParameters(CKDataArray *array, CKBehavior *beh, int firstoutputindex);
