#include "ViewTestFixture.h"

VOID ViewTestBase::removeNotificationCallback(PContentView pContentView, UINT64 customData, PViewItem pViewItem, BOOL frameNotTransferred)
{
    ViewTestBase* pTestBase = (ViewTestBase *) customData;
    pTestBase->gContentView = pContentView;
    pTestBase->gCustomData = customData;
    pTestBase->gFrameDropped = frameNotTransferred;
    pTestBase->gViewItem = *pViewItem;
    pTestBase->gCallCount++;
}