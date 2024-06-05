#pragma once
#include <list>
#include <thread>
#include <mutex>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#define SOCKET		int
#define closesocket close
#endif

#include "TCP.h"

class CliData {
public:
    /* std::vector<float> spectrum_points;
    std::vector<float> spectrum_hold_points;
    double fft_ceiling, fft_floor;
    long long centerFreq;
    int bandwidth; */
    int type;
    int status;
    std::string msg;
    virtual ~CliData() = default;;
};

typedef std::shared_ptr<CliData> CliDataPtr;
typedef ThreadBlockingQueue<CliDataPtr> CliDataQueue;
typedef std::shared_ptr<CliDataQueue> CliDataQueuePtr;

class CUBServer : public IOThread, public TCP::Server
{
public:
    void onBindOutput(std::string name, ThreadQueueBasePtr threadQueue)
    {
        if (name == "cliDataOutput")
        {
            // protects because it may be changed at runtime
            // std::lock_guard < SpinMutex > lock(m_mutexCliOutputQueue);
            cliDataOutQueue = std::static_pointer_cast<CliDataQueue>(threadQueue);
        }
    }
    void run()
    {
        start(5010);
    }

    void terminate()
    {
        IOThread::terminate();
        if (cliDataOutQueue)
            cliDataOutQueue->flush();
    }

private:
    void processClients()
    {
        timeout = 0;
        for (auto &c : client)
        {
            if (c.isConnected())
            {
                if (!c.msg.empty())
                {
                    CliDataPtr cli = std::make_shared<CliData>();
                    cli->msg = c.msg;

                    std::cout << " receive data:" <<  cli->msg << std::endl;
                    cliDataOutQueue->push(cli);
                }

                c.msg.clear();
            }
        }
    }

    private:
    CliDataQueuePtr cliDataOutQueue = nullptr;
    // SpinMutex m_mutexCliOutputQueue;
};
