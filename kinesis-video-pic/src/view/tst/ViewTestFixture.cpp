#include "ViewTestFixture.h"

VOID removeNotificationCallback(PContentView pContentView, UINT64 customData, PViewItem pViewItem, BOOL current)
{
    gContentView = pContentView;
    gCustomData = customData;
    gCurrent = current;
    gViewItem = *pViewItem;
    gCallCount++;
}