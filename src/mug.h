#ifndef MUG
#define MUG

#include <future>
#include <memory>
#include <thread>

#include "chp_client.h"

class Mug
{
public:
    Mug();
    virtual ~Mug();

    bool init(const std::string& serverHost_in, int basePort);
    void start();
    void stop();
    void log(int data);

private:
    void subscriberThread();

    // std::shared_ptr<ChpClient> chpClient;
    std::shared_ptr<zmq::context_t> context;
    std::shared_ptr<zmq::socket_t> subscriber;

    std::thread subscriberThreadHandle;
    std::mutex mutex;
    std::condition_variable subscriberCv;

    std::string serverHost;
    std::string uuid;

    int subscriberPort;

    bool running;
    bool subscriberRunning;
};

#endif
