#ifndef CHP_SERVER
#define CHP_SERVER

#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <thread>

#ifdef __linux__
#include <unistd.h>
#endif

#include <zmq.hpp>

#include "lager_utils.h"

class ChpServer
{
public:
    ChpServer(int basePort);

    void init(std::shared_ptr<zmq::context_t> context_in);
    void addOrUpdateKeyValue(const std::string& key, const std::string& value);
    void removeKey(const std::string& key);
    void start();
    void stop();

private:
    void snapshotThread();
    void snapshotMap(const std::string& identity);
    void snapshotBye(const std::string& identity);
    void publisherThread();
    void publishUpdatedKeys();
    void publishHugz();
    void collectorThread();
    void snapshotStopped();
    void initialize(std::shared_ptr<zmq::context_t> context_in);
    std::string getLocalUri(int port) const;
    std::string getRemoteUri(const std::string& remoteUriBase, int remotePort) const;

    std::shared_ptr<zmq::context_t> context;
    std::shared_ptr<zmq::socket_t> snapshot;
    std::shared_ptr<zmq::socket_t> publisher;
    std::shared_ptr<zmq::socket_t> collector;

    std::thread publisherThreadHandle;
    std::thread snapshotThreadHandle;
    std::thread collectorThreadHandle;

    std::map<std::string, std::string> hashMap;
    std::map<std::string, std::string> uuidMap;
    std::vector<std::string> updatedKeys;

    int snapshotPort;
    int publisherPort;
    int collectorPort;

    double sequence;

    bool running;
};

#endif
