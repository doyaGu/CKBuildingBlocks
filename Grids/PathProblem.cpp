#include "CKAll.h"
#include "PathProblem.h"
#include "float.h"
#include "GridInfo.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// PathProblem
/////////////////////////////////////////////////////////////////////////////////////////////////////////
PathProblem::PathProblem()
    : m_Target(0),
      m_GoalPos(0, 0, 0),
      m_ObstacleLayer(-1),
      m_ObstacleThreshold(0),
      m_SlowingFactor(0),
      m_Linker(0),
      m_HeuristicCoef(0),
      m_TimeFrame(0),
      m_OpenListLinker(0),
      m_OpenListGrid(0),
      m_State(1),
      m_GridStart(0),
      m_GridEnd(0),
      m_GridSearch(0),
      m_NodeGrideEnd(0),
      m_NodeLinkerEnd(0),
      m_StartPartSearch(FALSE),
      m_NodeLinkerDynStart(0),
      m_NodeLinkerDynEnd(0),
      m_HypSubPathStart2End(0),
      m_HypSubPathStart2EndCoast(FLT_MAX),
      m_SubPathStart(0),
      m_SubPathEnd(0),
      m_FollowPathID(-1)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// ~PathProblem
/////////////////////////////////////////////////////////////////////////////////////////////////////////
PathProblem::~PathProblem()
{
    int i;
    int size;

    size = m_HypSubPathStart.Size();
    for (i = 0; i < size; ++i)
        delete m_HypSubPathStart[i];
    m_HypSubPathStart.Clear();

    size = m_HypSubPathEnd.Size();
    for (i = 0; i < size; ++i)
        delete m_HypSubPathEnd[i];
    m_HypSubPathEnd.Clear();

    if (m_OpenListLinker)
        delete m_OpenListLinker;
    if (m_OpenListGrid)
        delete m_OpenListGrid;
    if (m_NodeLinkerDynStart)
        delete m_NodeLinkerDynStart;
    if (m_NodeLinkerDynEnd)
        delete m_NodeLinkerDynEnd;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Set
/////////////////////////////////////////////////////////////////////////////////////////////////////////
void PathProblem::Set(CK3dEntity *target, VxVector *targetPos, CK3dEntity *goalRef, VxVector *goalPos, int obstacleLayer, int obstacleThreshold, float slowingFactor, CKBOOL linker, CKBOOL linkerObs, float heuristicCoef, float timeFrame, int heuristic, CKBOOL diagonal, CKBOOL optimize, StatePathFunction stateFunction)
{
    m_Target = target;
    m_Target->Transform(&m_TargetPos, targetPos);

    if (goalRef)
        goalRef->Transform(&m_GoalPos, goalPos);
    else
        m_GoalPos = *goalPos;

    m_ObstacleLayer = obstacleLayer;
    if (obstacleThreshold > 255)
        m_ObstacleThreshold = 255;
    else if (obstacleThreshold < 0)
        m_ObstacleThreshold = 0;
    else
        m_ObstacleThreshold = obstacleThreshold;
    m_SlowingFactor = (float)fabs(slowingFactor);
    m_SlowingDivTreshold = m_SlowingFactor / (float)(m_ObstacleThreshold + 1);
    m_Linker = linker;
    m_LinkerObs = linkerObs;
    if (heuristicCoef < 0)
        m_HeuristicCoef = 0;
    else
        m_HeuristicCoef = heuristicCoef;
    m_TimeFrame = timeFrame;

    m_Heuristic = heuristic;
    m_Diagonal = diagonal;
    m_Optimize = optimize;

    // Problem is waiting for solution.
    m_StateFunction = stateFunction;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// SetOpenList
/////////////////////////////////////////////////////////////////////////////////////////////////////////
void PathProblem::SetOpenList(int numContext, int buffLinker, int buffGrid)
{
    m_OpenListLinker = new OpenListLinker(numContext, buffLinker);
    m_OpenListGrid = new OpenListGrid(numContext, buffGrid);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Reset
/////////////////////////////////////////////////////////////////////////////////////////////////////////
void PathProblem::Reset()
{
    int i;
    int size;

    // Init the solution data.
    m_State = 1;
    m_GridStart = 0;
    m_GridEnd = 0;

    // We clear the open list just in case if the search was stopped.
    if (m_OpenListGrid)
        m_OpenListGrid->Clear();
    if (m_OpenListLinker)
        m_OpenListLinker->Clear();
    m_StartPartSearch = FALSE;

    // Clear the dyn nodelinker
    m_NodeLinkerDynStart->ClearConnected();
    m_NodeLinkerDynStart->ClearDynConnected();
    m_NodeLinkerDynEnd->ClearConnected();
    m_NodeLinkerDynEnd->ClearDynConnected();

    // Clear the path
    size = m_Path.m_ArraySubPath.Size();
    for (i = 0; i < size; i++)
        delete m_Path.m_ArraySubPath[i];
    m_Path.m_ArraySubPath.Clear();
    m_Path.m_Coast = 0;

    size = m_HypSubPathStart.Size();
    for (i = 0; i < size; ++i)
        delete m_HypSubPathStart[i];
    m_HypSubPathStart.Clear();

    size = m_HypSubPathEnd.Size();
    for (i = 0; i < size; ++i)
        delete m_HypSubPathEnd[i];
    m_HypSubPathEnd.Clear();

    m_SubPathStart = 0;
    m_SubPathEnd = 0;

    if (m_HypSubPathStart2End)
    {
        delete m_HypSubPathStart2End;
        m_HypSubPathStart2End = 0;
    }
    m_HypSubPathStart2EndCoast = FLT_MAX;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// ResetOpenList
/////////////////////////////////////////////////////////////////////////////////////////////////////////
void PathProblem::ResetOpenList(CKBOOL linker)
{
    if (linker)
    {
        if (m_OpenListLinker)
            m_OpenListLinker->Clear();
    }
    else
    {
        if (m_OpenListGrid)
            m_OpenListGrid->Clear();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// BuildHypPath
/////////////////////////////////////////////////////////////////////////////////////////////////////////
void PathProblem::BuildSubPath(int context, SubPath *subPath, CKBOOL back) const
{
    NodeGrid *node;
    NodeGrid *prevNode;
    int width = subPath->m_Grid->GetWidth();
    int oldCaseIndex;
    int lastCase = m_NodeGrideEnd->m_CaseIndex;
    int oldDir;
    int dir = 0;
    int diffCase;

    subPath->SetLinker(m_Linker1, m_Linker2);

    if (!m_NodeGrideEnd->m_ArrayContextInfo[context]->m_Parent)
    {
        // The SubPath is just one case. (but a path is one goal and one end...)
        subPath->PushBack(m_NodeGrideEnd->m_CaseIndex);
        subPath->PushFront(m_NodeGrideEnd->m_CaseIndex);
        return;
    }

    // Push the first index;
    subPath->PushBack(m_NodeGrideEnd->m_CaseIndex);

    node = m_NodeGrideEnd->m_ArrayContextInfo[context]->m_Parent;
    diffCase = node->m_CaseIndex - m_NodeGrideEnd->m_CaseIndex;
    if (diffCase == 1)
        oldDir = 1;
    else if (diffCase == -1)
        oldDir = 2;
    else if (diffCase == width)
        oldDir = 3;
    else if (diffCase == -width)
        oldDir = 4;
    else if (diffCase == width + 1)
        oldDir = 5;
    else if (diffCase == width - 1)
        oldDir = 6;
    else if (diffCase == -width + 1)
        oldDir = 7;
    else if (diffCase == -width - 1)
        oldDir = 8;
    oldCaseIndex = node->m_CaseIndex;
    node = node->m_ArrayContextInfo[context]->m_Parent;

    if (!node)
    {
        // There is only 2 nodes.
        if (back)
            subPath->PushBack(oldCaseIndex);
        else
            subPath->PushFront(oldCaseIndex);
        return;
    }

    while (node)
    {
        diffCase = node->m_CaseIndex - oldCaseIndex;

        // Determine the dir(ection) of the node.
        if (diffCase == 1)
            dir = 1;
        else if (diffCase == -1)
            dir = 2;
        else if (diffCase == width)
            dir = 3;
        else if (diffCase == -width)
            dir = 4;
        else if (diffCase == width + 1)
            dir = 5;
        else if (diffCase == width - 1)
            dir = 6;
        else if (diffCase == -width + 1)
            dir = 7;
        else if (diffCase == -width - 1)
            dir = 8;
        if (dir != oldDir || !m_Optimize)
        {
            if (back)
                subPath->PushBack(oldCaseIndex);
            else
                subPath->PushFront(oldCaseIndex);
            oldDir = dir;
            lastCase = oldCaseIndex;
        }
        oldCaseIndex = node->m_CaseIndex;
        prevNode = node;
        node = node->m_ArrayContextInfo[context]->m_Parent;
    }

    if (lastCase != prevNode->m_CaseIndex)
    {
        // Push the last index;
        if (back)
            subPath->PushBack(prevNode->m_CaseIndex);
        else
            subPath->PushFront(prevNode->m_CaseIndex);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// ResetTime
/////////////////////////////////////////////////////////////////////////////////////////////////////////
void PathProblem::ResetTime()
{
    m_Time.Reset();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TimeNotOut
/////////////////////////////////////////////////////////////////////////////////////////////////////////
CKBOOL PathProblem::TimeNotOut()
{
    if (m_Time.Current() < m_TimeFrame)
        return TRUE;
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// ItLinkerStartIsGood
/////////////////////////////////////////////////////////////////////////////////////////////////////////
CKBOOL PathProblem::Linker2StartIsGood()
{
    int dx = m_NodeLinkerDynStart->m_X - (*m_ItLinkerStart)->m_X;
    int dy = m_NodeLinkerDynStart->m_Y - (*m_ItLinkerStart)->m_Y;

    if (sqrt((float)(dx * dx + dy * dy)) > m_HypSubPathStart2EndCoast)
        return FALSE;
    if (!m_Target->HasAttribute((*m_ItLinkerStart)->m_Attribut))
        return FALSE;
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// ItLinkerEndIsGood
/////////////////////////////////////////////////////////////////////////////////////////////////////////
CKBOOL PathProblem::Linker2EndIsGood()
{
    int dx = m_NodeLinkerDynEnd->m_X - (*m_ItLinkerStart)->m_X;
    int dy = m_NodeLinkerDynEnd->m_Y - (*m_ItLinkerStart)->m_Y;

    if (sqrt((float)(dx * dx + dy * dy)) > m_HypSubPathStart2EndCoast)
        return FALSE;
    if (!m_Target->HasAttribute((*m_ItLinkerStart)->m_Attribut))
        return FALSE;
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// SetLinker
/////////////////////////////////////////////////////////////////////////////////////////////////////////
void PathProblem::SetLinker(NodeLinker *linker1, NodeLinker *linker2)
{
    m_Linker1 = linker1;
    m_Linker2 = linker2;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// InitCase2Case
/////////////////////////////////////////////////////////////////////////////////////////////////////////
void PathProblem::InitBeforeCase2Case(CKGrid *grid, XSHashTable<GridInfo *, uintptr_t> *listGridInfo)
{
    ResetOpenList(FALSE);
    m_GridSearch = grid;
    m_NumGrid = (*listGridInfo->FindPtr((uintptr_t)m_GridSearch))->m_Num;
    m_GridW = m_GridSearch->GetWidth();
    m_GridL = m_GridSearch->GetLength();

    CKLayer *layer = m_GridSearch->GetLayer(m_ObstacleLayer);
    if (layer)
        m_Square = layer->GetSquareArray();
    else
        m_Square = 0;

    if (m_Diagonal)
    {
        m_SuccesorIndex[0] = m_GridW - 1;
        m_SuccesorIndex[1] = m_GridW;
        m_SuccesorIndex[2] = m_GridW + 1;
        m_SuccesorIndex[3] = 1;
        m_SuccesorIndex[4] = -m_GridW + 1;
        m_SuccesorIndex[5] = -m_GridW;
        m_SuccesorIndex[6] = -m_GridW - 1;
        m_SuccesorIndex[7] = -1;
        m_SuccesorIndex[8] = m_GridW - 1;
        m_SuccesorIndex[9] = m_GridW;
        m_SuccesorIndex[10] = m_GridW + 1;
        m_SuccesorIndex[11] = 1;
    }
    else
    {
        m_SuccesorIndex[0] = m_GridW;
        m_SuccesorIndex[1] = 1;
        m_SuccesorIndex[2] = -m_GridW;
        m_SuccesorIndex[3] = -1;
        m_SuccesorIndex[4] = m_GridW;
        m_SuccesorIndex[5] = 1;
    }
}
