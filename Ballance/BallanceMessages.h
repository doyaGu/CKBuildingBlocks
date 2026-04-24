#pragma once

namespace Ballance
{
    struct BaseEventMessage
    {
        const char *name;
        const char *parameterName;
        int messageId;
    };

    int GetBaseEventMessageCount();
    const BaseEventMessage *GetBaseEventMessage(int index);
}
