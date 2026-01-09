#ifndef NODEGRID
#define NODEGRID
//----------------------------------------------------------------------------------------------------------
#include "Node.h"
#include "ContextInfo.h"
//----------------------------------------------------------------------------------------------------------
class NodeLinker;
//----------------------------------------------------------------------------------------------------------
class NodeGrid : public Node
{
public:
    int m_CaseIndex;
    int m_LayerValue;
    XArray<GridContext *> m_ArrayContextInfo;
    NodeLinker **m_LinkerObs;

    NodeGrid();
    ~NodeGrid();
    void Set(int, int, int);
    void ResizeParallelContext(int);
};
//----------------------------------------------------------------------------------------------------------
#endif
