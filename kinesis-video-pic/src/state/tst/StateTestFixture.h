#include "gtest/gtest.h"
#include <com/amazonaws/kinesis/video/client/Include.h>
#include <com/amazonaws/kinesis/video/utils/Include.h>
#include <com/amazonaws/kinesis/video/state/Include.h>

#include "src/state/src/Include_i.h"

/**
 * Kinesis Video client states definitions
 */
#define TEST_STATE_0 ((UINT64) 0)
#define TEST_STATE_1 ((UINT64)(1 << 0))
#define TEST_STATE_2 ((UINT64)(1 << 1))
#define TEST_STATE_3 ((UINT64)(1 << 2))
#define TEST_STATE_4 ((UINT64)(1 << 3))
#define TEST_STATE_5 ((UINT64)(1 << 4))

#define TEST_STATE_COUNT 6

STATUS fromTestState(UINT64, PUINT64);
STATUS executeTestState(UINT64, UINT64);

extern StateMachineState TEST_STATE_MACHINE_STATES[];
extern UINT32 TEST_STATE_MACHINE_STATE_COUNT;

typedef struct {
    UINT32 stateCount;
    UINT64 nextState;
} TestTransitions, *PTestTransitions;

class StateTestBase : public ::testing::Test {
  public:
    StateTestBase() : mStateMachine(NULL)
    {
        MEMSET(mTestTransitions, 0x00, SIZEOF(mTestTransitions));
        mTestTransitions[0].nextState = TEST_STATE_1;
        mTestTransitions[1].nextState = TEST_STATE_2;
        mTestTransitions[2].nextState = TEST_STATE_3;
        mTestTransitions[3].nextState = TEST_STATE_4;
        mTestTransitions[4].nextState = TEST_STATE_5;
        mTestTransitions[5].nextState = TEST_STATE_0;
    }

    UINT32 GetCurrentStateIndex()
    {
        switch (((PStateMachineImpl) mStateMachine)->context.pCurrentState->state) {
            case TEST_STATE_0:
                return 0;
            case TEST_STATE_1:
                return 1;
            case TEST_STATE_2:
                return 2;
            case TEST_STATE_3:
                return 3;
            case TEST_STATE_4:
                return 4;
            case TEST_STATE_5:
                return 5;
        }

        return 0;
    }

    TestTransitions mTestTransitions[TEST_STATE_COUNT];
    PStateMachine mStateMachine;

  protected:
    STATUS CreateStateMachine()
    {
        STATUS retStatus;

        // Create the state machine
        EXPECT_TRUE(STATUS_SUCCEEDED(retStatus = createStateMachine(TEST_STATE_MACHINE_STATES, TEST_STATE_MACHINE_STATE_COUNT, (UINT64) this,
                                                                    kinesisVideoStreamDefaultGetCurrentTime, (UINT64) this, &mStateMachine)));

        return retStatus;
    };

    virtual void SetUp()
    {
        UINT32 logLevel = 0;
        auto logLevelStr = GETENV("AWS_KVS_LOG_LEVEL");
        if (logLevelStr != NULL) {
            assert(STRTOUI32(logLevelStr, NULL, 10, &logLevel) == STATUS_SUCCESS);
            SET_LOGGER_LOG_LEVEL(logLevel);
        }

        DLOGI("\nSetting up test: %s\n", GetTestName());
        CreateStateMachine();
    };

    virtual void TearDown()
    {
        DLOGI("\nTearing down test: %s\n", GetTestName());

        // Proactively free the allocation
        if (mStateMachine != NULL) {
            freeStateMachine(mStateMachine);
            mStateMachine = NULL;
        }
    };

    PCHAR GetTestName()
    {
        return (PCHAR)::testing::UnitTest::GetInstance()->current_test_info()->test_case_name();
    };

  private:
};
