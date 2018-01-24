#include <memory>

#include <gtest/gtest.h>

#include <zmq.hpp>

#include "chp_server.h"
#include "chp_client.h"
#include "lager_utils.h"

class ChpTests : public ::testing::Test
{
protected:
    virtual void SetUp()
    {

    }

    virtual void TearDown()
    {

    }

    std::shared_ptr<zmq::context_t> context;
};

TEST_F(ChpTests, UninitializedStart)
{
    ClusteredHashmapServer s(12345);
    ClusteredHashmapClient c("localhost", 12345, 1000);

    EXPECT_ANY_THROW(s.start());
    EXPECT_ANY_THROW(c.start());
}

TEST_F(ChpTests, ServerAddRemoveKeys)
{
    context.reset(new zmq::context_t(1));
    ClusteredHashmapServer c(12345);
    c.init(context);
    c.start();
    EXPECT_EQ(c.getHashMap().size(), 0);
    c.addOrUpdateKeyValue("testkey", "testvalue");
    lager_utils::sleep(100);
    EXPECT_STREQ(c.getHashMap()["testkey"].c_str(), "testvalue");
    c.removeKey("testkey");
    lager_utils::sleep(100);
    EXPECT_EQ(c.getHashMap().size(), 0);
    context->close();
    c.stop();
}

TEST_F(ChpTests, BothAddRemoveKeys)
{
    context.reset(new zmq::context_t(1));
    ClusteredHashmapServer s(12345);
    ClusteredHashmapClient c("localhost", 12345, 1000);

    s.init(context);
    c.init(context, lager_utils::getUuid());

    s.start();
    c.start();
    EXPECT_EQ(s.getHashMap().size(), 0);
    c.addOrUpdateKeyValue("testkey", "testvalue");
    lager_utils::sleep(100);
    EXPECT_STREQ(s.getHashMap()["testkey"].c_str(), "testvalue");
    c.removeKey("testkey");
    lager_utils::sleep(100);
    EXPECT_EQ(s.getHashMap().size(), 0);
    context->close();
    c.stop();
    s.stop();
}

TEST_F(ChpTests, AddMultipleKeys)
{
    context.reset(new zmq::context_t(1));
    ClusteredHashmapServer s(12345);
    ClusteredHashmapClient c("localhost", 12345, 1000);

    s.init(context);
    c.init(context, lager_utils::getUuid());

    s.start();
    c.start();
    EXPECT_EQ(s.getHashMap().size(), 0);
    c.addOrUpdateKeyValue("testkey1", "testvalue1");
    c.addOrUpdateKeyValue("testkey2", "testvalue2");
    lager_utils::sleep(100);
    EXPECT_STREQ(s.getHashMap()["testkey1"].c_str(), "testvalue1");
    c.removeKey("testkey1");
    lager_utils::sleep(100);
    EXPECT_EQ(s.getHashMap().size(), 1);
    context->close();
    c.stop();
    s.stop();
}

TEST_F(ChpTests, ClientMapReceive)
{
    context.reset(new zmq::context_t(1));
    ClusteredHashmapServer s(12345);
    ClusteredHashmapClient c("localhost", 12345, 1000);

    s.init(context);
    c.init(context, lager_utils::getUuid());

    s.start();
    EXPECT_EQ(s.getHashMap().size(), 0);
    s.addOrUpdateKeyValue("testkey1", "testvalue1");
    lager_utils::sleep(100);
    EXPECT_STREQ(s.getHashMap()["testkey1"].c_str(), "testvalue1");
    c.start();
    lager_utils::sleep(1000);
    EXPECT_EQ(c.getHashMap().size(), 1);
    context->close();
    c.stop();
    s.stop();
}

TEST_F(ChpTests, ServerDuplicateKeys)
{
    context.reset(new zmq::context_t(1));
    ClusteredHashmapServer s(12345);
    ClusteredHashmapClient c("localhost", 12345, 1000);
    ClusteredHashmapClient cDupe("localhost", 12345, 1000);

    s.init(context);
    c.init(context, lager_utils::getUuid());
    cDupe.init(context, lager_utils::getUuid());

    s.start();
    c.start();
    cDupe.start();
    EXPECT_EQ(s.getHashMap().size(), 0);
    c.addOrUpdateKeyValue("testkey1", "testvalue1");
    lager_utils::sleep(1000);
    cDupe.addOrUpdateKeyValue("testkey1", "testvalue1");
    lager_utils::sleep(1000);
    EXPECT_EQ(c.getHashMap().size(), 1);
    context->close();
    c.stop();
    s.stop();
}

TEST_F(ChpTests, ClientNoHugz)
{
    context.reset(new zmq::context_t(1));

    ClusteredHashmapClient c("localhost", 12345, 1000);

    c.init(context, lager_utils::getUuid());

    c.start();

    lager_utils::sleep(2500);

    EXPECT_TRUE(c.isTimedOut());

    context->close();
    c.stop();
}
