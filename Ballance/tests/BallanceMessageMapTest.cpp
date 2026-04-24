#include "BallanceMessages.h"

#include <cstring>
#include <iostream>

static int ExpectEvent(int index, const char *name, const char *parameterName, int messageId)
{
    const Ballance::BaseEventMessage *event = Ballance::GetBaseEventMessage(index);
    if (!event)
    {
        std::cerr << "Missing event at index " << index << "\n";
        return 1;
    }

    if (std::strcmp(event->name, name) != 0)
    {
        std::cerr << "Event " << index << " name: " << event->name << " != " << name << "\n";
        return 1;
    }

    if (std::strcmp(event->parameterName, parameterName) != 0)
    {
        std::cerr << "Event " << index << " parameter: " << event->parameterName << " != " << parameterName << "\n";
        return 1;
    }

    if (event->messageId != messageId)
    {
        std::cerr << "Event " << index << " message: " << event->messageId << " != " << messageId << "\n";
        return 1;
    }

    return 0;
}

int main()
{
    if (Ballance::GetBaseEventMessageCount() != 11)
    {
        std::cerr << "Unexpected event count\n";
        return 1;
    }

    if (ExpectEvent(0, "Load Menu Level", "Message 2", 13)) return 1;
    if (ExpectEvent(1, "Exit To System", "Message 1", 11)) return 1;
    if (ExpectEvent(2, "Load Level", "Message 0", 6)) return 1;
    if (ExpectEvent(3, "Start Level", "Message 3", 7)) return 1;
    if (ExpectEvent(4, "Reset Level", "Message 4", 12)) return 1;
    if (ExpectEvent(5, "Pause Level", "Message 5", 9)) return 1;
    if (ExpectEvent(6, "Unpause Level", "Message 6", 10)) return 1;
    if (ExpectEvent(7, "Exit Level", "Message 7", 8)) return 1;
    if (ExpectEvent(8, "Next Level", "Message 8", 14)) return 1;
    if (ExpectEvent(9, "Show Cursor", "Message 9", 15)) return 1;
    if (ExpectEvent(10, "Highscore", "Message 10", 16)) return 1;

    return 0;
}
