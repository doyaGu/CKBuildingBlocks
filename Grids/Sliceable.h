// ## Base class for sliceable processes

/*
Things to review:
    -- Typing of called functions (doesn't work with Visual, doesn't understand)
    -- Slicing management
*/

#include "CKAll.h"

#define SLICE_TIMEOUT -1
#define SLICE_NOTHING -2

class Sliceable;

// There's a typing problem!
typedef int (*SliceableProcessFunction)(void); // Interference with the typing of member functions.

class Sliceable
{
protected:
    CKTimeManager *m_TimeManager;
    float m_Timeout;
    float m_StartTime, m_NextTime;
    unsigned short int m_ProcessPhaseCount;
    unsigned short int m_ProcessPhaseNumber;
    SliceableProcessFunction *m_ProcessPhaseFunction;

public:
    Sliceable() : m_TimeManager(NULL), m_ProcessPhaseCount(0), m_ProcessPhaseFunction(NULL) {}

    Sliceable(const CKBehaviorContext &bContext)
    {
        m_TimeManager = bContext.TimeManager;
        m_ProcessPhaseCount = 0;
        m_ProcessPhaseFunction = NULL;
    }

    int Launch()
    {
        if ((m_ProcessPhaseNumber >= m_ProcessPhaseCount) ||
            (m_ProcessPhaseFunction[m_ProcessPhaseNumber] == NULL))
            return (SLICE_NOTHING);

        m_StartTime = m_TimeManager->GetTime();
        m_NextTime = m_StartTime + m_Timeout;
        return ((*m_ProcessPhaseFunction[m_ProcessPhaseNumber])());
    }

    void SetPhaseCount(unsigned short int count)
    {
        if (m_ProcessPhaseFunction != NULL)
            delete[] m_ProcessPhaseFunction;
        m_ProcessPhaseCount = count;
        m_ProcessPhaseFunction = new SliceableProcessFunction[count];
        m_ProcessPhaseNumber = 0; // by default ...
        for (int i = 0; i < count; i++)
            m_ProcessPhaseFunction[i] = NULL;
    }

    unsigned short int GetPhaseCount()
    {
        return (m_ProcessPhaseCount);
    }

    void SetPhase(unsigned short int phaseNum)
    {
        m_ProcessPhaseNumber = phaseNum;
    }

    unsigned short int GetPhase()
    {
        return (m_ProcessPhaseNumber);
    }

    void SetPhaseFunction(unsigned short int pos, SliceableProcessFunction function)
    {
        if (pos >= m_ProcessPhaseCount)
            return;
        m_ProcessPhaseFunction[pos] = function;
    }
};

#endif