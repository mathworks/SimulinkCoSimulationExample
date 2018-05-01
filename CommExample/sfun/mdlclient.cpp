// Copyright 2018 The MathWorks, Inc.

#include <zmq.hpp>
#include <vector>
#include <utility>
#include <iostream>
#include <memory>
#include <zmq.hpp>
#include <cstdlib>
#include <future>
//#include <unistd.h>

// #include "mdlclient.hpp"
#include "statcal_util.hpp"

namespace {

#define REQUEST_RETRIES  3 //  Number of tries before we abandon

// class ZmqMgr for managing socket connection with the server
class ZmqMgr {
  public:
    ZmqMgr(const std::string &addr) : context(1), socket_addr(addr)
    {
    }

    ~ZmqMgr() {}

    void sendRequest(const std::string & request_str);

    void retrieveReply(std::vector<double> & yout, int request_timeout, int retries_left = REQUEST_RETRIES);
    
    void resetSocketPtr()
    {
        socket_ptr.reset(nullptr);
    }

  private:
    std::string socket_addr;
    zmq::context_t context;
    std::unique_ptr<zmq::socket_t>  socket_ptr;
    
    std::unique_ptr<zmq::socket_t> createSocket();
};

// ZmqMgr class method sendRequest
void ZmqMgr::sendRequest(const std::string & request_str)
{
    if (!socket_ptr) {
        socket_ptr = createSocket();
    }
    zmq::message_t request;
    request.rebuild(request_str.size());
    memset(request.data(), 0, request_str.size());    
    memcpy(request.data (), request_str.c_str(), request_str.size());
    
    //std::cout << "Sending " << request_str << std::endl;
    socket_ptr->send(request);
}

// ZmqMgr class method retrieveReply
    void ZmqMgr::retrieveReply(std::vector<double> & yout, int request_timeout, int retries_left)
{
    assert(socket_ptr);
    
    while (retries_left) {
        zmq::message_t reply;
        
        //  Poll socket for a reply, with timeout
        zmq::pollitem_t items[] = { {(void*)*socket_ptr, 0, ZMQ_POLLIN, 0 } };
        zmq::poll (&items[0], 1, request_timeout);
        
        //  If we got a reply, process it
        if (items[0].revents & ZMQ_POLLIN) {
            
            socket_ptr->recv(&reply);
            char *reply_str = static_cast<char*>(reply.data());
            reply_str[reply.size()] = '\0';

            int len;
            MsgType type;
            std::tie(type, len) = decode_header(reply_str);
            if (len > 0) {
                decode_data(len, reply_str, yout);
                for (auto & it : yout) {
                    std::cout << " " << it;
                }
                std::cout << std::endl;
            }
            break;
        } else if (--retries_left == 0) {
            throw std::runtime_error("Connection timed out. Please ensure that the transmitter side is running and two sides are not in a locked state due to unintended execution orders. If you have a long running algorithm, you can increase timeout parameter value from the block dialog.");
        } else {
            std::cout << "No response, try again" << std::endl;
        }
    }
}

// ZmqMgr class method createSocket
std::unique_ptr<zmq::socket_t> ZmqMgr::createSocket()
{
    std::unique_ptr<zmq::socket_t> s_ptr(new zmq::socket_t(context, ZMQ_REQ));

    s_ptr->connect(socket_addr.c_str());
    int linger = 0;
    s_ptr->setsockopt (ZMQ_LINGER, &linger, sizeof (linger));
    std::cout << "Starting connection" << std::endl;
        
    return s_ptr;
}

} // anonymous namespace

void shutdown_server(ZmqMgr *zmp)
{
    std::string request_str;
    auto type = SHUTDOWN;
    encode_data(type, {}, request_str);
    zmp->sendRequest(request_str);
}

void *setupruntimeresources_wrapper(const std::string &connStr)
{
    auto zmp = new ZmqMgr(connStr);
    return reinterpret_cast<void *>(zmp);
}

void cleanupruntimeresouces_wrapper(void *zm)
{
    auto zmp = reinterpret_cast<ZmqMgr *>(zm);
    if (zmp) {
        shutdown_server(zmp);
        delete zmp;
    }
}

void transmit_outputs_wrapper(void *zm, const double *u_ptr, const int w, const double request_timeout)
{
    std::vector<double> yout;
    std::string request_str;
    std::vector<double> uv(w,0);

    for (int k=0; k<w; k++) {
        uv[k] = *(u_ptr++);
    }
    
    auto type = INP_DATA;
    encode_data(type, uv, request_str);
    reinterpret_cast<ZmqMgr *>(zm)->sendRequest(request_str);
    reinterpret_cast<ZmqMgr *>(zm)->retrieveReply(yout, request_timeout);
}
