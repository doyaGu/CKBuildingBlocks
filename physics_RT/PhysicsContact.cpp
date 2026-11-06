#include "PhysicsContact.h"

#include "CK3dEntity.h"
#include "CKBehavior.h"
#include "CKParameterOut.h"
#include "CKAttributeManager.h"
#include "CKIpionManager.h"

PhysicsContactManager::~PhysicsContactManager()
{
    while (m_Records.len())
        RemoveRecord(m_Records.len() - 1);
}

void PhysicsContactManager::Process(IVP_Time time)
{
    const int len = m_Records.len();
    for (int i = len - 1; i >= 0; --i)
    {
        PhysicsContactRecord *record = m_Records.element_at(i);
        PhysicsObject *po = record->m_PhysicsObject;
        PhysicsContactData *data = po->m_ContactData;
        CKBehavior *beh = data ? data->GetBehavior() : NULL;
        if (!beh || record->m_Index < 0 || record->m_Index >= data->m_GroupOutputCount ||
            2 * record->m_Index + 1 >= beh->GetOutputCount())
        {
            m_Records.remove_at(i);
            delete record;
            continue;
        }

        double delta = time - record->m_Time;
        int index = record->m_Index;
        PhysicsContactData::GroupOutput *output = data->m_GroupOutputs;

        if (output[index].active)
        {
            if (output[index].number != 0)
            {
                m_Records.remove_at(i);
                delete record;
            }
            else if (delta > data->m_TimeDelayEnd)
            {
                output[index].active = FALSE;
                beh->ActivateOutput(2 * index + 1, TRUE);
                m_Records.remove_at(i);
                delete record;
            }
        }
        else if (output[index].number <= 0)
        {
            if (delta * 0.5 > data->m_TimeDelayStart)
            {
                m_Records.remove_at(i);
                delete record;
            }
        }
        else if (delta > data->m_TimeDelayStart)
        {
            output[index].active = TRUE;
            beh->ActivateOutput(2 * index, TRUE);
            m_Records.remove_at(i);
            delete record;
        }
    }
}

int PhysicsContactManager::GetRecordCount() const
{
    return m_Records.len();
}

PhysicsContactRecord *PhysicsContactManager::GetRecord(int index)
{
    return m_Records.element_at(index);
}

void PhysicsContactManager::AddRecord(PhysicsObject *obj, int index, IVP_Time time)
{
    if (!obj || !obj->m_ContactData || index < 0 || index >= obj->m_ContactData->m_GroupOutputCount)
        return;
    bool found = false;
    const int len = m_Records.len();
    for (int i = 0; i < len; ++i)
    {
        PhysicsContactRecord *record = m_Records.element_at(i);
        if (record->m_PhysicsObject == obj && record->m_Index == index)
        {
            found = true;
            break;
        }
    }

    if (!found)
    {
        PhysicsContactRecord *record = new PhysicsContactRecord(time, index, obj);
        m_Records.add(record);
    }
}

void PhysicsContactManager::RemoveRecord(int index)
{
    PhysicsContactRecord *record = m_Records.element_at(index);
    m_Records.remove_at(index);
    delete record;
}

void PhysicsContactManager::RemoveRecord(PhysicsObject *obj)
{
    if (!obj)
        return;

    const int len = m_Records.len();
    for (int i = len - 1; i >= 0; --i)
    {
        PhysicsContactRecord *record = m_Records.element_at(i);
        if (record->m_PhysicsObject == obj)
        {
            m_Records.remove_at(i);
            delete record;
        }
    }
}

void PhysicsContactManager::RemoveRecord(PhysicsObject *obj, int index)
{
    if (!obj)
        return;

    const int len = m_Records.len();
    for (int i = len - 1; i >= 0; --i)
    {
        PhysicsContactRecord *record = m_Records.element_at(i);
        if (record->m_PhysicsObject == obj && record->m_Index == index)
        {
            m_Records.remove_at(i);
            delete record;
        }
    }
}

void PhysicsContactManager::SetupContactID()
{
    CKAttributeManager *am = m_IpionManager->m_Context->GetAttributeManager();
    m_ContactIDAttribType = am->GetAttributeTypeByName("Continuous Contact ID");
}

int PhysicsContactManager::GetContactID(CK3dEntity *entity) const
{
    int contactID = -1;
    if (m_ContactIDAttribType != -1 && entity)
    {
        CKParameterOut *pa = entity->GetAttributeParameter(m_ContactIDAttribType);
        if (pa)
            pa->GetValue(&contactID);
    }
    return contactID;
}

PhysicsContactData::PhysicsContactData(float timeDelayStart, float timeDelayEnd, int groupOutputCount,
                                       PhysicsContactManager *man, CKBehavior *beh, PhysicsObject *object)
    : m_TimeDelayStart(timeDelayStart), m_TimeDelayEnd(timeDelayEnd),
      m_GroupOutputCount(groupOutputCount > 0 ? groupOutputCount : 0), m_Manager(man),
      m_BehaviorID(beh ? beh->GetID() : 0), m_PhysicsObject(object)
{
    m_GroupOutputs = m_GroupOutputCount > 0 ? new GroupOutput[m_GroupOutputCount] : NULL;
    m_Listener = NULL;
}

CKBehavior *PhysicsContactData::GetBehavior() const
{
    if (!m_Manager || !m_Manager->m_IpionManager || !m_Manager->m_IpionManager->m_Context || m_BehaviorID == 0)
        return NULL;

    CKObject *obj = m_Manager->m_IpionManager->m_Context->GetObject(m_BehaviorID);
    if (!obj || !CKIsChildClassOf(obj, CKCID_BEHAVIOR))
        return NULL;

    return (CKBehavior *)obj;
}

PhysicsContactData::~PhysicsContactData()
{
    if (m_Manager)
        m_Manager->RemoveRecord(m_PhysicsObject);
    if (m_PhysicsObject && m_PhysicsObject->m_ContactData == this)
        m_PhysicsObject->m_ContactData = NULL;
    delete m_Listener;

    CKBehavior *beh = GetBehavior();
    if (beh)
    {
        PhysicsContactData *empty = NULL;
        beh->SetLocalParameterValue(1, &empty);
        for (int i = 0; i < m_GroupOutputCount; ++i)
            if (m_GroupOutputs[i].active && 2 * i + 1 < beh->GetOutputCount())
                beh->ActivateOutput(2 * i + 1, TRUE);
    }
    delete[] m_GroupOutputs;
}
