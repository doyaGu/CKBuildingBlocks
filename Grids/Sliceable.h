// ## Classe de base pour les processus sliceable

#ifndef SLICEABLE_PROCESS_H
#define SLICEABLE_PROCESS_H

/*
Truc � revoir :
    -- Typage des fonctions appel�es (marche po avec Visual, comprend po)
    -- Gestion du Slicing
*/

#include "CKAll.h"

#define SLICE_TIMEOUT -1
#define SLICE_NOTHING -2

class Sliceable;

// Ya un prob de typage !
typedef int (*SliceableProcessFunction)(void); // Interference avec le typage des fonctions membres.

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
        m_ProcessPhaseNumber = 0; // par defaut ...
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