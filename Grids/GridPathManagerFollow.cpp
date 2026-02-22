#include "CKAll.h"
#include "GridPathManager.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// RegisterFollowProblem
/////////////////////////////////////////////////////////////////////////////////////////////////////////
CKBOOL GridPathManager::RegisterFollowProblem(FollowStruct *followStruct)
{
    int followProblemID;
    Path **path;

    // Get the path.
    path = m_PathID2Path.FindPtr(followStruct->followPathID);
    if (!path)
        return FALSE;
    m_PathID2Path.Remove(followStruct->followPathID);

    if (!(*path)->NbSubPath())
    {
        delete *path;
        return FALSE;
    }

    // Get a free index.
    if (m_StackFreeFollowIndex.Size())
    {
        followProblemID = *(m_StackFreeFollowIndex.Begin());
        m_StackFreeFollowIndex.PopFront();
    }
    else
    {
        // Resize the context.
        followProblemID = m_MaxFollowProblem++;
        ResizeFollowContextArray(m_MaxFollowProblem);
    }

    ++m_NumFollowProblem;
    m_ArrayFollowProblem[followProblemID]->Set(followStruct, *path, m_PFCAttrType);
    delete *path;
    m_ArrayFollowProblem[followProblemID]->FirstOrient();
    m_PathID2FollowContext.Insert(followStruct->followPathID, followProblemID, FALSE);
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// UnregisterFollowProblem
/////////////////////////////////////////////////////////////////////////////////////////////////////////
CKBOOL GridPathManager::UnregisterFollowProblem(int pathID)
{
    int *followContextPtr;
    int followContext;

    if (!(followContextPtr = m_PathID2FollowContext.FindPtr(pathID)))
        return FALSE;
    followContext = *followContextPtr;
    m_PathID2FollowContext.Remove(pathID);

    // Reset the follow problem.
    m_ArrayFollowProblem[followContext]->Reset();

    // Free the index context;
    m_StackFreeFollowIndex.PushBack(followContext);
    --m_NumFollowProblem;
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// UnregisterFollowProblem
/////////////////////////////////////////////////////////////////////////////////////////////////////////
void GridPathManager::ResizeFollowContextArray(int numberContext)
{
    int oldNumberContext = m_ArrayFollowProblem.Size();

    m_ArrayFollowProblem.Resize(numberContext);
    for (int i = oldNumberContext; i < numberContext; i++)
    {
        m_ArrayFollowProblem[i] = new FollowProblem(m_MessageManager, m_AttributManager, m_ArrayNodeGrid);
        m_ArrayFollowProblem[i]->m_ArrayFollowProblem = &m_ArrayFollowProblem;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// ManagePathFollow
/////////////////////////////////////////////////////////////////////////////////////////////////////////
void GridPathManager::ManagePathFollow()
{
    int NumProblem;
    int i;

    // Calcul Direction for all follow problem.
    NumProblem = m_NumFollowProblem;
    for (i = 0; i < m_MaxFollowProblem; ++i)
    {
        if (!NumProblem)
            break;
        FollowProblem *followProblem = m_ArrayFollowProblem[i];
        if (followProblem->m_State != finish)
        {
            --NumProblem;
            followProblem->CalculDirection();
        }
    }

    // Correct Direction for all follow problem.
    NumProblem = m_NumFollowProblem;
    for (i = 0; i < m_MaxFollowProblem; ++i)
    {
        if (!NumProblem)
            break;
        FollowProblem *followProblem = m_ArrayFollowProblem[i];
        if (followProblem->m_State != finish)
        {
            --NumProblem;
            followProblem->CorrectDirection();
        }
    }

    // Move for all follow problem.
    NumProblem = m_NumFollowProblem;
    for (i = 0; i < m_MaxFollowProblem; ++i)
    {
        if (!NumProblem)
            break;
        FollowProblem *followProblem = m_ArrayFollowProblem[i];
        if (followProblem->m_State != finish)
        {
            --NumProblem;
            followProblem->Move();
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// SetStateFollowProblem
/////////////////////////////////////////////////////////////////////////////////////////////////////////
CKBOOL GridPathManager::SetStateFollowProblem(int pathID, FollowState state)
{
    int *followIdPtr;

    if (!(followIdPtr = m_PathID2FollowContext.FindPtr(pathID)))
        return FALSE;
    m_ArrayFollowProblem[*followIdPtr]->m_State = state;
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// GetStateFollowProblem
/////////////////////////////////////////////////////////////////////////////////////////////////////////
FollowState GridPathManager::GetStateFollowProblem(int pathID)
{
    int *followIdPtr;

    if (!(followIdPtr = m_PathID2FollowContext.FindPtr(pathID)))
        return dontexist;
    return m_ArrayFollowProblem[*followIdPtr]->m_State;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// ConstructFollowData
/////////////////////////////////////////////////////////////////////////////////////////////////////////
void GridPathManager::ConstructFollowData()
{
    for (int i = 0; i < DEFAUL_MAX_FOLLOW_CONTEXT; i++)
    {
        m_ArrayFollowProblem[i] = new FollowProblem(m_MessageManager, m_AttributManager, m_ArrayNodeGrid);
        m_ArrayFollowProblem[i]->m_ArrayFollowProblem = &m_ArrayFollowProblem;
        m_StackFreeFollowIndex.PushBack(i);
    }
}
