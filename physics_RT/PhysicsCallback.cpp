#include "PhysicsCallback.h"

#include "CKIpionManager.h"

class PhysicsCallbackProcessingScope
{
public:
    explicit PhysicsCallbackProcessingScope(CKIpionManager *manager) : m_Manager(manager)
    {
        if (m_Manager)
            ++m_Manager->m_CallbackProcessingDepth;
    }

    ~PhysicsCallbackProcessingScope()
    {
        if (m_Manager && m_Manager->m_CallbackProcessingDepth > 0)
            --m_Manager->m_CallbackProcessingDepth;
    }

private:
    CKIpionManager *m_Manager;
};

CKBehavior *PhysicsCallback::GetBehavior() const
{
    if (!m_IpionManager || !m_IpionManager->m_Context || m_BehaviorID == 0)
        return NULL;

    CKObject *obj = m_IpionManager->m_Context->GetObject(m_BehaviorID);
    if (!obj || !CKIsChildClassOf(obj, CKCID_BEHAVIOR))
        return NULL;

    return (CKBehavior *)obj;
}

PhysicsCallbackContainer::~PhysicsCallbackContainer()
{
    Clear();
}

// An executing callback is removed from its queue before calling user code.
// Clear/ClearBehavior may cancel it, but only its caller destroys it.
void PhysicsCallbackContainer::Clear()
{
    for (ActiveCall *call = m_Active; call; call = call->previous)
        call->cancelled = true;
    for (int i = 0; i < 3; ++i)
        while (m_Callbacks[i].len())
        {
            PhysicsCallback *callback = m_Callbacks[i].element_at(m_Callbacks[i].len() - 1);
            m_Callbacks[i].remove_at(m_Callbacks[i].len() - 1);
            delete callback;
        }
    for (Batch *batch = m_Batches; batch; batch = batch->previous)
        while (batch->callbacks.len())
        {
            PhysicsCallback *callback = batch->callbacks.element_at(batch->callbacks.len() - 1);
            batch->callbacks.remove_at(batch->callbacks.len() - 1);
            delete callback;
        }
    m_HasCallbacks = FALSE;
}

void PhysicsCallbackContainer::ClearBehavior(CK_ID behaviorID)
{
    if (!behaviorID)
        return;
    for (ActiveCall *call = m_Active; call; call = call->previous)
        if (call->callback->m_BehaviorID == behaviorID)
            call->cancelled = true;
    for (int i = 0; i < 3; ++i)
        for (int j = m_Callbacks[i].len() - 1; j >= 0; --j)
            if (m_Callbacks[i].element_at(j)->m_BehaviorID == behaviorID)
            {
                PhysicsCallback *callback = m_Callbacks[i].element_at(j);
                m_Callbacks[i].remove_at(j);
                delete callback;
            }
    for (Batch *batch = m_Batches; batch; batch = batch->previous)
        for (int j = batch->callbacks.len() - 1; j >= 0; --j)
            if (batch->callbacks.element_at(j)->m_BehaviorID == behaviorID)
            {
                PhysicsCallback *callback = batch->callbacks.element_at(j);
                batch->callbacks.remove_at(j);
                delete callback;
            }
    UpdateHasCallbacks();
}

bool PhysicsCallbackContainer::Execute(PhysicsCallback *callback)
{
    if (!callback || !callback->GetBehavior())
        return false;
    ActiveCall call = {callback, false, m_Active};
    m_Active = &call;
    const int result = callback->Execute();
    m_Active = call.previous;
    return !call.cancelled && result == 0 && callback->GetBehavior();
}

void PhysicsCallbackContainer::UpdateHasCallbacks()
{
    m_HasCallbacks = FALSE;
    for (int i = 0; i < 3; ++i)
        if (m_Callbacks[i].len())
            m_HasCallbacks = TRUE;
}

void PhysicsCallbackContainer::Process()
{
    PhysicsCallbackProcessingScope processing(m_IpionManager);
    for (int i = 0; i < 3; ++i)
    {
        Batch batch;
        batch.previous = m_Batches;
        m_Batches = &batch;
        for (int j = 0; j < m_Callbacks[i].len(); ++j)
            batch.callbacks.add(m_Callbacks[i].element_at(j));
        m_Callbacks[i].clear();
        while (batch.callbacks.len())
        {
            PhysicsCallback *callback = batch.callbacks.element_at(batch.callbacks.len() - 1);
            batch.callbacks.remove_at(batch.callbacks.len() - 1);
            if (Execute(callback))
            {
                m_Callbacks[i].add(callback);
                // Retain the original reverse execution order on every frame.
                for (int j = m_Callbacks[i].len() - 1; j > 0; --j)
                    m_Callbacks[i].swap_elems(j, j - 1);
            }
            else
                delete callback;
        }
        m_Batches = batch.previous;
    }
    UpdateHasCallbacks();
}

void PhysicsCallbackContainer::Process(PhysicsCallback *callback)
{
    if (!callback)
        return;
    PhysicsCallbackProcessingScope processing(m_IpionManager);
    if (Execute(callback) && callback->m_Type >= 0 && callback->m_Type < 3)
        m_Callbacks[callback->m_Type].add(callback);
    else
        delete callback;
    UpdateHasCallbacks();
}
