#include <memory>

#include <gtest/gtest.h>

#include "bartender.h"
#include "lager_utils.h"

class BartenderTests : public ::testing::Test
{
protected:
    virtual void SetUp()
    {

    }

    virtual void TearDown()
    {

    }
};

TEST_F(BartenderTests, DoesItWork)
{
    Bartender b;
    EXPECT_TRUE(b.init(12345));
    b.start();

    lager_utils::sleepMillis(1000);

    b.stop();
}

TEST_F(BartenderTests, BadPortNumber)
{
    Bartender b;
    EXPECT_FALSE(b.init(-1));
    EXPECT_FALSE(b.init(65536));
}

