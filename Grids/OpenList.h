#ifndef OPENLIST
#define OPENLIST
//----------------------------------------------------------------------------------------------------------
#include <float.h>
#include "NodeLinker.h"
#include "NodeGrid.h"
//----------------------------------------------------------------------------------------------------------
class OpenListLinker : public XArray<NodeLinker *>
{
    int m_ContextID;

public:
    OpenListLinker(int, int);
    ~OpenListLinker();

    void Push(NodeLinker *node)
    {
        // first one
        if (!Size())
        {
            PushBack(node);
            return;
        }

        float f = node->m_ArrayContextInfo[m_ContextID]->m_F;

        // last one
        if (Back()->m_ArrayContextInfo[m_ContextID]->m_F > f)
        {
            PushBack(node);
            return;
        }

        // search the good insertion point
        NodeLinker **it = m_Begin;
        while (f < (*it)->m_ArrayContextInfo[m_ContextID]->m_F)
            ++it;

        Insert(it, node);
    }

    void Update()
    {
        float oldf = FLT_MAX;
        float f;
        NodeLinker **it;
        for (it = m_Begin; it != m_End; ++it)
        {
            f = (*it)->m_ArrayContextInfo[m_ContextID]->m_F;
            if (f > oldf)
                break;
            oldf = f;
        }

        for (; it != m_End; ++it)
        {
            f = (*it)->m_ArrayContextInfo[m_ContextID]->m_F;
            if (oldf > f)
                break;
            XSwap(*it, *(it - 1));
        }
    }
};
//----------------------------------------------------------------------------------------------------------
class OpenListGrid : public XArray<NodeGrid *>
{
    int m_ContextID;

public:
    OpenListGrid(int, int);
    ~OpenListGrid();

    void Push(NodeGrid *node)
    {
        // first one
        if (!Size())
        {
            PushBack(node);
            return;
        }

        float f = node->m_ArrayContextInfo[m_ContextID]->m_F;

        // last one
        if (Back()->m_ArrayContextInfo[m_ContextID]->m_F > f)
        {
            PushBack(node);
            return;
        }

        // search the good insertion point
        NodeGrid **it = m_Begin;
        while (f < (*it)->m_ArrayContextInfo[m_ContextID]->m_F)
            ++it;

        Insert(it, node);
    }

    void Update()
    {
        float oldf = FLT_MAX;
        float f;
        NodeGrid **it;
        for (it = m_Begin; it != m_End; ++it)
        {
            f = (*it)->m_ArrayContextInfo[m_ContextID]->m_F;
            if (f > oldf)
                break;
            oldf = f;
        }

        for (; it != m_End; ++it)
        {
            f = (*it)->m_ArrayContextInfo[m_ContextID]->m_F;
            if (oldf > f)
                break;
            XSwap(*it, *(it - 1));
        }
    }
};
//----------------------------------------------------------------------------------------------------------
#endif
