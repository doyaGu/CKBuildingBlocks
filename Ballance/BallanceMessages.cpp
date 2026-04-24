#include "BallanceMessages.h"

namespace Ballance
{
    static const BaseEventMessage kBaseEventMessages[] = {
        {"Load Menu Level", "Message 2", 13},
        {"Exit To System", "Message 1", 11},
        {"Load Level", "Message 0", 6},
        {"Start Level", "Message 3", 7},
        {"Reset Level", "Message 4", 12},
        {"Pause Level", "Message 5", 9},
        {"Unpause Level", "Message 6", 10},
        {"Exit Level", "Message 7", 8},
        {"Next Level", "Message 8", 14},
        {"Show Cursor", "Message 9", 15},
        {"Highscore", "Message 10", 16},
    };

    int GetBaseEventMessageCount()
    {
        return (int)(sizeof(kBaseEventMessages) / sizeof(kBaseEventMessages[0]));
    }

    const BaseEventMessage *GetBaseEventMessage(int index)
    {
        if (index < 0 || index >= GetBaseEventMessageCount())
            return 0;
        return &kBaseEventMessages[index];
    }
}
