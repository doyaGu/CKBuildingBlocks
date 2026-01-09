#ifndef _LINKERGRAPH_
#define _LINKERGRAPH_

#include "CKAll.h"

#define DEPARTURE_LIST 0
#define ARRIVAL_LIST 1
#define BOTH_LIST 2

class LinkerGraphIterator;

//--- Linker's Graph Structure --------------------------------------
class LinkerGraph
{
    // Grid that hosts linker points
    CKGrid *m_Host;

    int m_Count[3];
    CKPOINT *m_List[3];

public:
    friend class LinkerGraphIterator;

    // Constructor / Destructor
    LinkerGraph(CKGrid *host = NULL) : m_Host(host)
    {
        for (int i = 0; i < 3; i++)
        {
            m_Count[i] = 0;
            m_List[i] = NULL;
        }
    }

    ~LinkerGraph()
    {
        m_Host = NULL;
        Clear();
    }

    // Clears all datas
    void Clear()
    {
        for (int i = 0; i < 3; i++)
        {
            delete[] m_List[i];
            m_Count[i] = 0;
        }
    }

    // Returns the hosting grid
    CKGrid *GetGrid() { return (m_Host); }
    // Returns the number of 'Departure' points
    int GetDepartureCount() { return (m_Count[0]); }
    // Returns the number of 'Arrival' points
    int GetArrivalCount() { return (m_Count[1]); }
    // Returns the number of 'Departure & Arrival' points
    int GetBothCount() { return (m_Count[2]); }

    // Retrieves the specified 'Departure' point
    void GetDeparture(int index, CKPOINT &vect) { vect = m_List[0][index]; }
    void GetDeparture(int index, int x, int y)
    {
        x = m_List[0][index].x;
        y = m_List[0][index].y;
    }

    // Retrieves the specified 'Arrival' point
    void GetArrival(int index, CKPOINT &vect) { vect = m_List[1][index]; }
    void GetArrival(int index, int x, int y)
    {
        x = m_List[1][index].x;
        y = m_List[1][index].y;
    }

    // Retrieves the specified 'Departure & Arrival' point
    void GetBoth(int index, CKPOINT &vect) { vect = m_List[2][index]; }
    void GetBoth(int index, int x, int y)
    {
        x = m_List[2][index].x;
        y = m_List[2][index].y;
    }

    // Adds a point
    void AddDeparture(int x, int y) { AddPoint(x, y, 0); }
    void RemoveDeparture(int x, int y) { RemovePoint(x, y, 0); }

    void AddArrival(int x, int y) { AddPoint(x, y, 1); }
    void RemoveArrival(int x, int y) { RemovePoint(x, y, 1); }

    void AddBoth(int x, int y) { AddPoint(x, y, 2); }
    void RemoveBoth(int x, int y) { RemovePoint(x, y, 2); }

    // Moves points coordinates
    void MovePoint(int dx, int dy);

    // Build a point list using a precomputed buffer
    void BuildDepartureList(CKPOINT *list, int count) { BuildList(list, count, 0); }
    void BuildArrivalList(CKPOINT *list, int count) { BuildList(list, count, 1); }
    void BuildBothList(CKPOINT *list, int count) { BuildList(list, count, 2); }

private:
    void AddPoint(int x, int y, int listNum);
    void RemovePoint(int x, int y, int listNum);
    void BuildList(CKPOINT *list, int count, int listNum);
};

//-------------------------------------------------------------------

// --- LinkerGraph Iterator -----------------------------------------

class LinkerGraphIterator
{
    // LinkerGraph's List
    LinkerGraph **m_GraphGroup;
    // Number of element in the list
    int m_GraphCount;
    // Current Enumerated Graph
    int m_CurrentGraph;

    // First List to explore
    int m_StartList;
    int m_CurrentPoint;
    int m_CurrentList;

public:
    // Constructor / Destructor
    LinkerGraphIterator() : m_GraphGroup(NULL), m_GraphCount(0) {}
    LinkerGraphIterator(LinkerGraph **graph, int count, unsigned char flags)
    {
        Init(graph, count, flags);
    }

    ~LinkerGraphIterator()
    {
        m_GraphGroup = NULL;
    }

    // Members
    void Init(LinkerGraph **graph, int count, unsigned char flags)
    {
        m_GraphGroup = graph;
        m_GraphCount = count;
        m_CurrentGraph = 0;
        m_StartList = (flags & 32) ? 1 : 0;
        m_CurrentPoint = 0;
        m_CurrentList = m_StartList;
        if ((graph != NULL) && (m_GraphGroup[m_CurrentGraph]->m_Count[m_CurrentList] == 0))
            Next();
    }

    CKBOOL HasMoreElements()
    {
        return ((m_CurrentGraph < m_GraphCount) ? TRUE : FALSE); // Test if m_GraphGroup == NULL
    }

    void Next()
    {
        do
        {
            m_CurrentPoint++;
            if (m_CurrentPoint >= m_GraphGroup[m_CurrentGraph]->m_Count[m_CurrentList])
            {
                m_CurrentPoint = 0;
                if (m_CurrentList != 2)
                {
                    m_CurrentList = 2;
                }
                else
                {
                    m_CurrentList = m_StartList;
                    m_CurrentGraph++;
                }
            }
        } while ((m_CurrentGraph < m_GraphCount) && (m_GraphGroup[m_CurrentGraph]->m_Count[m_CurrentList] == 0));
    }

    void GetCurrentPoint(CKPOINT &point)
    {
        point = m_GraphGroup[m_CurrentGraph]->m_List[m_CurrentList][m_CurrentPoint];
    }

    CKGrid *GetCurrentGrid()
    {
        if (!HasMoreElements())
            return (NULL);
        return (m_GraphGroup[m_CurrentGraph]->m_Host);
    }
};

// ------------------------------------------------------------------

#endif