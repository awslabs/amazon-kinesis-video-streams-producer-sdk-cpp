#include <gtest/gtest.h>
#include <Logger.h>

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    LOG_CONFIGURE_STDOUT("DEBUG");
    return RUN_ALL_TESTS();
}
