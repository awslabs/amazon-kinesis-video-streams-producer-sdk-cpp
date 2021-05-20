#if 0
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif

#include "gtest/gtest.h"

// Header file from jni/dependencies/trace/src/ folder
#include "src/trace/src/Include_i.h"

#define TEST_TRACE_NAME ((PCHAR) "Test trace name")

class TraceTestBase : public ::testing::Test {
  public:
    TraceTestBase(){};

  protected:
};
