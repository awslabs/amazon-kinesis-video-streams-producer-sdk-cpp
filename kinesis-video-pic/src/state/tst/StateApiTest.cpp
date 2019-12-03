#include "StateTestFixture.h"

extern StateMachineState TEST_STATE_MACHINE_STATES[];
extern UINT32 TEST_STATE_MACHINE_STATE_COUNT;

class StateApiTest : public StateTestBase {
};

TEST_F(StateApiTest, createStateMachine_InvalidInput)
{
    PStateMachine pStateMachine;

    EXPECT_NE(STATUS_SUCCESS, createStateMachine(NULL, TEST_STATE_MACHINE_STATE_COUNT, (UINT64) this,
                                                 kinesisVideoStreamDefaultGetCurrentTime, (UINT64) this,
                                                 &pStateMachine));

    EXPECT_NE(STATUS_SUCCESS, createStateMachine(TEST_STATE_MACHINE_STATES, 0, (UINT64) this,
                                                 kinesisVideoStreamDefaultGetCurrentTime, (UINT64) this,
                                                 &pStateMachine));

    EXPECT_NE(STATUS_SUCCESS, createStateMachine(TEST_STATE_MACHINE_STATES, TEST_STATE_MACHINE_STATE_COUNT,
                                                 (UINT64) this, kinesisVideoStreamDefaultGetCurrentTime,
                                                 (UINT64) this, NULL));

    EXPECT_NE(STATUS_SUCCESS, createStateMachine(TEST_STATE_MACHINE_STATES, TEST_STATE_MACHINE_STATE_COUNT,
                                                 (UINT64) this, NULL,
                                                 (UINT64) this, &pStateMachine));

    EXPECT_EQ(STATUS_SUCCESS, createStateMachine(TEST_STATE_MACHINE_STATES, TEST_STATE_MACHINE_STATE_COUNT,
                                                 (UINT64) this, kinesisVideoStreamDefaultGetCurrentTime,
                                                 (UINT64) this, &pStateMachine));

    EXPECT_EQ(STATUS_SUCCESS, freeStateMachine(pStateMachine));
    EXPECT_EQ(STATUS_SUCCESS, freeStateMachine(NULL));
}

TEST_F(StateApiTest, stepStateMachine_InvalidInput)
{
    EXPECT_NE(STATUS_SUCCESS, stepStateMachine(NULL));
}

TEST_F(StateApiTest, acceptStateMachineState_InvalidInput)
{
    EXPECT_NE(STATUS_SUCCESS, acceptStateMachineState(NULL, TEST_STATE_0));

    // There is no check for invalid state
    EXPECT_EQ(STATUS_SUCCESS, acceptStateMachineState(mStateMachine, TEST_STATE_5 + 1));
}

TEST_F(StateApiTest, getStateMachineState_InvalidInput)
{
    PStateMachineState pStateMachineState;
    EXPECT_NE(STATUS_SUCCESS, getStateMachineState(NULL, TEST_STATE_0, &pStateMachineState));
    EXPECT_NE(STATUS_SUCCESS, getStateMachineState(mStateMachine, TEST_STATE_5 + 1, &pStateMachineState));
    EXPECT_NE(STATUS_SUCCESS, getStateMachineState(mStateMachine, TEST_STATE_0, NULL));

    EXPECT_EQ(STATUS_SUCCESS, getStateMachineState(mStateMachine, TEST_STATE_0, &pStateMachineState));
}

TEST_F(StateApiTest, setStateMachineCurrentState_InvalidInput)
{
    EXPECT_NE(STATUS_SUCCESS, setStateMachineCurrentState(NULL, TEST_STATE_0));
    EXPECT_NE(STATUS_SUCCESS, setStateMachineCurrentState(mStateMachine, 3));

    EXPECT_EQ(STATUS_SUCCESS, setStateMachineCurrentState(mStateMachine, TEST_STATE_2));
}

TEST_F(StateApiTest, resetStateMachineRetryCount_InvalidInput)
{
    EXPECT_NE(STATUS_SUCCESS, resetStateMachineRetryCount(NULL));

    EXPECT_EQ(STATUS_SUCCESS, resetStateMachineRetryCount(mStateMachine));
}
