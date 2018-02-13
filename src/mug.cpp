#include "mug.h"

/**
* @brief Constructor, sets an invalid port to ensure the user initializes properly
*/
Mug::Mug(): running(false), subscriberPort(-1), subscriberRunning(false)
{
}

Mug::~Mug()
{
}

/**
* @brief Initializes the mug by creating the zmq context and starting up the CHP client
* @param serverHost_in is a string containing the IP or hostname of the bartender to connect to
* @param basePort is an integer containing a the port of the bartender to connect to
* @param kegDir is a string containing a path to an accessible directory to store the keg files
* @returns true on successful initialization, false on failure
*/
bool Mug::init(const std::string& serverHost_in, int basePort, const std::string& kegDir)
{
    subscriberPort = basePort + FORWARDER_BACKEND_OFFSET;

    if (subscriberPort < 0 || subscriberPort > 65535)
    {
        // TODO provide stream output of errors?
        return false;
    }

    serverHost = serverHost_in;

    context.reset(new zmq::context_t(1));

    // TODO make this not magic 2000 default timeout for testing
    chpClient.reset(new ClusteredHashmapClient(serverHost_in, basePort, 2000));
    chpClient->init(context, uuid);
    hashMapUpdatedHandle = std::bind(&Mug::hashMapUpdated, this);
    chpClient->setCallback(hashMapUpdatedHandle);

    formatParser.reset(new DataFormatParser);

    keg.reset(new Keg(kegDir));

    return true;
}

/**
* @brief Starts the mug subscriber thread
*/
void Mug::start()
{
    running = true;

    keg->start();

    subscriberThreadHandle = std::thread(&Mug::subscriberThread, this);
    subscriberThreadHandle.detach();

    chpClient->start();
}

/**
* @brief Closes the zmq context and stops the subscriber and chp threads
*/
void Mug::stop()
{
    running = false;

    context->close();

    std::unique_lock<std::mutex> lock(mutex);

    while (subscriberRunning)
    {
        subscriberCv.wait(lock);
    }

    chpClient->stop();
    keg->stop();
}

/**
* @brief Callback function to update the hashmap and format maps whenever the chp client hashmap is updated
*/
void Mug::hashMapUpdated()
{
    std::map<std::string, std::string> tmpUuidMap; // <uuid, topic name>
    std::shared_ptr<DataFormat> tmpDataFormat;

    mutex.lock();
    hashMap = chpClient->getHashMap();
    tmpUuidMap = chpClient->getUuidMap();

    // TODO later only parse the subscribed topic names
    for (auto i = hashMap.begin(); i != hashMap.end(); ++i)
    {
        // parse the xml data format into a DataFormat object
        tmpDataFormat = formatParser->parseFromString(i->second);

        // look for our DataFormat object's topic name in the uuid map
        for (auto j = tmpUuidMap.begin(); j != tmpUuidMap.end(); ++j)
        {
            if (j->second == i->first)
            {
                // index the formats by uuid for ease of use as the data comes in
                formatMap[j->first] = tmpDataFormat;
                keg->addFormat(j->first, i->second);
                break;
            }
        }
    }

    mutex.unlock();
}

/**
* @brief The main data subscriber thread
*/
void Mug::subscriberThread()
{
    subscriberRunning = true;

    try
    {
        subscriber.reset(new zmq::socket_t(*context.get(), ZMQ_SUB));
        subscriber->connect(lager_utils::getRemoteUri(serverHost.c_str(), subscriberPort).c_str());

        // subscribe to everything (for now)
        // TODO set up filter by uuid of subscribed topics
        subscriber->setsockopt(ZMQ_SUBSCRIBE, "", 0);

        uint64_t timestamp;
        std::vector<uint8_t> data; // the main buffer we're keeping

        // all data that comes from a tap will be one of these sizes
        uint8_t tmp8;
        uint16_t tmp16;
        uint32_t tmp32;
        uint64_t tmp64;

        off_t offset; // keeps track of the current offset of the buffer while recieving more frames

        uint32_t rcvMore = 0;
        size_t moreSize = sizeof(rcvMore);

        // set up a poller for the subscriber socket
        zmq::pollitem_t items[] = {{static_cast<void*>(*subscriber.get()), 0, ZMQ_POLLIN, 0}};

        while (running)
        {
            // TODO check if this timeout should be different or setable by user
            zmq::poll(&items[0], 1, 1000);

            if (items[0].revents & ZMQ_POLLIN)
            {
                // clear the buffer each time
                data.clear();
                offset = 0;

                zmq::message_t msg;

                subscriber->recv(&msg);
                std::string uuid(static_cast<char*>(msg.data()), msg.size());

                // Version frame, string, currently unused
                subscriber->recv(&msg);

                // Compression frame, uint16_t, currently unused
                subscriber->recv(&msg);

                subscriber->recv(&msg);
                timestamp = *(uint64_t*)msg.data();

                // uuid is first in the buffer
                for (size_t i = 0; i < uuid.size(); ++i)
                {
                    data.push_back(uuid[i]);
                }

                offset += UUID_SIZE_BYTES;

                // timestamp is next in the buffer
                data.resize(data.size() + TIMESTAMP_SIZE_BYTES);
                *(reinterpret_cast<uint64_t*>(data.data() + offset)) = timestamp;

                offset += TIMESTAMP_SIZE_BYTES;

                // make sure we have more of the multipart zmq message waiting
                subscriber->getsockopt(ZMQ_RCVMORE, &rcvMore, &moreSize);

                while (rcvMore != 0)
                {
                    subscriber->recv(&msg);

                    // TODO eventually move to a size + blob architecture, if necessary,
                    // since these reallocs may be expensive
                    switch (msg.size())
                    {
                        case 1:
                            data.resize(data.size() + 1);
                            tmp8 = *(uint8_t*)msg.data();
                            data.push_back(tmp8);
                            offset += 1;
                            break;

                        case 2:
                            data.resize(data.size() + 2);
                            tmp16 = *(uint16_t*)msg.data();
                            *(reinterpret_cast<uint16_t*>(data.data() + offset)) = tmp16;
                            offset += 2;
                            break;

                        case 4:
                            data.resize(data.size() + 4);
                            tmp32 = *(uint32_t*)msg.data();
                            *(reinterpret_cast<uint32_t*>(data.data() + offset)) = tmp32;
                            offset += 4;
                            break;

                        case 8:
                            data.resize(data.size() + 8);
                            tmp64 = *(uint64_t*)msg.data();
                            *(reinterpret_cast<uint64_t*>(data.data() + offset)) = tmp64;
                            offset += 8;
                            break;

                        default:
                            throw std::runtime_error("received unsupported zmq message size");
                            break;
                    }

                    // write out the data
                    // TODO this should probably be some kind of callback function
                    keg->write(data, data.size());

                    // check for more multipart messages
                    subscriber->getsockopt(ZMQ_RCVMORE, &rcvMore, &moreSize);
                }
            }
        }
    }
    catch (const zmq::error_t& e)
    {
        // This is the proper way of shutting down multithreaded ZMQ sockets.
        // The creator of the zmq context basically pulls the rug out from
        // under the socket.
        if (e.num() == ETERM)
        {
            subscriber->close();
            subscriberRunning = false;
            subscriberCv.notify_one();
        }
    }
}
