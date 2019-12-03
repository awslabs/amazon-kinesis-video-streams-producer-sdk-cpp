#include "StateTestFixture.h"

StateMachineState TEST_STATE_MACHINE_STATES[] = {
        {TEST_STATE_0, TEST_STATE_0 | TEST_STATE_1, fromTestState, executeTestState, INFINITE_RETRY_COUNT_SENTINEL, STATUS_INVALID_OPERATION},
        {TEST_STATE_1, TEST_STATE_0, fromTestState, executeTestState, SERVICE_CALL_MAX_RETRY_COUNT, STATUS_INVALID_OPERATION},
        {TEST_STATE_2, TEST_STATE_0 | TEST_STATE_1 | TEST_STATE_2 | TEST_STATE_3 | TEST_STATE_4 | TEST_STATE_5, fromTestState, executeTestState, SERVICE_CALL_MAX_RETRY_COUNT, STATUS_INVALID_OPERATION},
        {TEST_STATE_3, TEST_STATE_2 | TEST_STATE_3, fromTestState, executeTestState, SERVICE_CALL_MAX_RETRY_COUNT, STATUS_INVALID_OPERATION},
        {TEST_STATE_4, TEST_STATE_3 | TEST_STATE_4, fromTestState, executeTestState, INFINITE_RETRY_COUNT_SENTINEL, STATUS_INVALID_OPERATION},
        {TEST_STATE_5, TEST_STATE_4 | TEST_STATE_5, fromTestState, executeTestState, SERVICE_CALL_MAX_RETRY_COUNT, STATUS_INVALID_OPERATION},
};

UINT32 TEST_STATE_MACHINE_STATE_COUNT = SIZEOF(TEST_STATE_MACHINE_STATES) / SIZEOF(StateMachineState);

///////////////////////////////////////////////////////////////////////////
// State machine callback functions
///////////////////////////////////////////////////////////////////////////
STATUS fromTestState(UINT64 customData, PUINT64 pState)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    StateTestBase* pTest = (StateTestBase*) customData;

    CHK(pTest != NULL && pState != NULL, STATUS_NULL_ARG);
    *pState = pTest->mTestTransitions[pTest->GetCurrentStateIndex()].nextState;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS executeTestState(UINT64 customData, UINT64 time)
{
    ENTERS();
    UNUSED_PARAM(time);
    STATUS retStatus = STATUS_SUCCESS;
    StateTestBase* pTest = (StateTestBase*) customData;

    CHK(pTest != NULL, STATUS_NULL_ARG);
    pTest->mTestTransitions[pTest->GetCurrentStateIndex()].stateCount++;

CleanUp:

    LEAVES();
    return retStatus;
}
