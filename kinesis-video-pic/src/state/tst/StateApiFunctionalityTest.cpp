#include "StateTestFixture.h"

class StateApiFunctionalityTest : public StateTestBase {
public:
    StateApiFunctionalityTest() : mTestStateCount(0) {}

    UINT32 mTestStateCount;
};

TEST_F(StateApiFunctionalityTest, checkStateIterationFailure)
{
    UINT32 i;
    PStateMachineState pStateMachineState;
    PStateMachineImpl pStateMachineImpl = (PStateMachineImpl) mStateMachine;

    // State 0 has infinite retry
    // Set to not progress
    mTestTransitions[0].nextState = TEST_STATE_0;
    for (i = 0; i < SERVICE_CALL_MAX_RETRY_COUNT + 10; i++) {
        EXPECT_EQ(STATUS_SUCCESS, stepStateMachine(mStateMachine));
    }

    // Validate the retry
    EXPECT_EQ(SERVICE_CALL_MAX_RETRY_COUNT + 10, pStateMachineImpl->context.retryCount);
    EXPECT_EQ(STATUS_SUCCESS, resetStateMachineRetryCount(mStateMachine));
    EXPECT_EQ(0, pStateMachineImpl->context.retryCount);

    // Step to next state
    mTestTransitions[0].nextState = TEST_STATE_1;
    EXPECT_EQ(STATUS_SUCCESS, stepStateMachine(mStateMachine));

    // Iterating state 1 should fail as the only transition is from state 0
    mTestTransitions[1].nextState = TEST_STATE_1;
    EXPECT_NE(STATUS_SUCCESS, stepStateMachine(mStateMachine));

    // Iterate to state 2 and then state 3
    mTestTransitions[1].nextState = TEST_STATE_2;
    EXPECT_EQ(STATUS_SUCCESS, stepStateMachine(mStateMachine));
    EXPECT_EQ(STATUS_SUCCESS, getStateMachineCurrentState(mStateMachine, &pStateMachineState));
    EXPECT_EQ(TEST_STATE_2, pStateMachineState->state);

    EXPECT_EQ(STATUS_SUCCESS, stepStateMachine(mStateMachine));
    EXPECT_EQ(STATUS_SUCCESS, getStateMachineCurrentState(mStateMachine, &pStateMachineState));
    EXPECT_EQ(TEST_STATE_3, pStateMachineState->state);

    // Iterate max retry count
    mTestTransitions[3].nextState = TEST_STATE_3;
    for (i = 0; i < SERVICE_CALL_MAX_RETRY_COUNT; i++) {
        EXPECT_EQ(STATUS_SUCCESS, stepStateMachine(mStateMachine));
    }
    EXPECT_EQ(SERVICE_CALL_MAX_RETRY_COUNT + 1, mTestTransitions[3].stateCount);

    EXPECT_NE(STATUS_SUCCESS, stepStateMachine(mStateMachine));

    mTestTransitions[3].nextState = TEST_STATE_4;

    EXPECT_EQ(STATUS_SUCCESS, stepStateMachine(mStateMachine));
    EXPECT_EQ(STATUS_SUCCESS, getStateMachineCurrentState(mStateMachine, &pStateMachineState));
    EXPECT_EQ(TEST_STATE_4, pStateMachineState->state);

    // Go back to state 2
    mTestTransitions[4].nextState = TEST_STATE_2;
    EXPECT_EQ(STATUS_SUCCESS, stepStateMachine(mStateMachine));
    EXPECT_EQ(STATUS_SUCCESS, getStateMachineCurrentState(mStateMachine, &pStateMachineState));
    EXPECT_EQ(TEST_STATE_2, pStateMachineState->state);

    // Set the state to 4
    EXPECT_EQ(STATUS_SUCCESS, setStateMachineCurrentState(mStateMachine, TEST_STATE_4));
    EXPECT_EQ(STATUS_SUCCESS, getStateMachineCurrentState(mStateMachine, &pStateMachineState));
    EXPECT_EQ(TEST_STATE_4, pStateMachineState->state);
}

STATUS executeFunctionalityTestState(UINT64 customData, UINT64 time)
{
    ENTERS();
    UNUSED_PARAM(time);
    STATUS retStatus = STATUS_SUCCESS;
    StateApiFunctionalityTest* pTest = (StateApiFunctionalityTest*) customData;

    CHK(pTest != NULL, STATUS_NULL_ARG);
    pTest->mTestStateCount++;

CleanUp:

    LEAVES();
    return retStatus;
}


TEST_F(StateApiFunctionalityTest, nullExecuteFn)
{
    PStateMachine pStateMachine;
    UINT32 i;
    StateMachineState states[TEST_STATE_COUNT];

    // Copy the states
    MEMCPY(states, TEST_STATE_MACHINE_STATES, SIZEOF(StateMachineState) * TEST_STATE_COUNT);

    // Set the execute functions
    for (i = 0; i < TEST_STATE_COUNT; i++) {
        states[i].executeStateFn = executeFunctionalityTestState;
    }

    // Set the state 1 execute function to null
    states[1].executeStateFn = NULL;

    EXPECT_EQ(STATUS_SUCCESS, createStateMachine(states,
                                                 TEST_STATE_MACHINE_STATE_COUNT,
                                                 (UINT64) this,
                                                 kinesisVideoStreamDefaultGetCurrentTime,
                                                 (UINT64) this,
                                                 &pStateMachine));

    EXPECT_EQ(STATUS_SUCCESS, stepStateMachine(pStateMachine));

    // Validate we didn't get called
    EXPECT_EQ(0, mTestStateCount);

    EXPECT_EQ(STATUS_SUCCESS, freeStateMachine(pStateMachine));
}

TEST_F(StateApiFunctionalityTest, nullGetNextStateFn)
{
    PStateMachine pStateMachine;
    StateMachineState states[TEST_STATE_COUNT];

    // Copy the states
    MEMCPY(states, TEST_STATE_MACHINE_STATES, SIZEOF(StateMachineState) * TEST_STATE_COUNT);

    // Set the state 0 execute function to null
    states[0].getNextStateFn = NULL;

    EXPECT_EQ(STATUS_SUCCESS, createStateMachine(states,
                                                 TEST_STATE_MACHINE_STATE_COUNT,
                                                 (UINT64) this,
                                                 kinesisVideoStreamDefaultGetCurrentTime,
                                                 (UINT64) this,
                                                 &pStateMachine));

    EXPECT_NE(STATUS_SUCCESS, stepStateMachine(pStateMachine));

    // Validate we didn't get called
    EXPECT_EQ(0, mTestStateCount);

    EXPECT_EQ(STATUS_SUCCESS, freeStateMachine(pStateMachine));
}