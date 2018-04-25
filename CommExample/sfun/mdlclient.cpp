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

#define REQUEST_TIMEOUT 2500 //  msecs (> 1000)
#define REQUEST_RETRIES  3 //  Number of tries before we abandon

const char *DELIMITERS = " ,"; // <space> or ","

// class ZmqMgr for managing socket connection with the server
class ZmqMgr {
  public:
    ZmqMgr(const std::string &addr) : context(1), socket_addr(addr)
    {
    }

    ~ZmqMgr() {}

    void sendRequest(const std::string & request_str);

    void retrieveReply(std::vector<double> & yout, int retries_left = REQUEST_RETRIES);
    void plainRetrieveReply(int retries_left =  REQUEST_RETRIES);
    
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

void ZmqMgr::plainRetrieveReply(int retries_left)
{
    assert(socket_ptr);
    
    while (retries_left) {
        zmq::message_t reply;
        
        //  Poll socket for a reply, with timeout
        zmq::pollitem_t items[] = { {(void*)*socket_ptr, 0, ZMQ_POLLIN, 0 } };
        zmq::poll (&items[0], 1, REQUEST_TIMEOUT);
        
        //  If we got a reply, process it
        if (items[0].revents & ZMQ_POLLIN) {
            
            socket_ptr->recv(&reply);
            char *reply_str = static_cast<char*>(reply.data());
            reply_str[reply.size()] = '\0';

            std::cout << "Received: " << reply_str << std::endl;

            break;
        } else if (--retries_left == 0) {
            throw std::runtime_error("Server connection timed out");
        } else {
            std::cout << "No response from server, retrying … " << std::endl;
        }
    }
}

// ZmqMgr class method retrieveReply
void ZmqMgr::retrieveReply(std::vector<double> & yout, int retries_left)
{
    assert(socket_ptr);
    
    while (retries_left) {
        zmq::message_t reply;
        
        //  Poll socket for a reply, with timeout
        zmq::pollitem_t items[] = { {(void*)*socket_ptr, 0, ZMQ_POLLIN, 0 } };
        zmq::poll (&items[0], 1, REQUEST_TIMEOUT);
        
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
            throw std::runtime_error("Server connection timed out");
        } else {
            std::cout << "No response from server, retrying … " << std::endl;
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
    std::cout << "Connecting to stats calculator server…" << std::endl;
        
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

// Wrapper functions
void *setupruntimeresources_wrapper(const std::string & host,
                                    const std::string & port,
                                    const std::string & shlibname)
{

    // start_server(host, port, shlibname);
    std::string connStr = "tcp://"+host+":5555";
    std::unique_ptr<ZmqMgr> dzmp(new ZmqMgr(connStr));

    std::string request_str = "model " + port + " " + shlibname;
    dzmp->sendRequest(request_str);
    dzmp->plainRetrieveReply();
    
    connStr = "tcp://"+host+":"+port;
    auto zmp = new ZmqMgr(connStr);
    
    return reinterpret_cast<void *>(zmp);  
}

void *rc_setupruntimeresources_wrapper(const std::string &connStr)
{
    auto zmp = new ZmqMgr(connStr);
    return reinterpret_cast<void *>(zmp);
}

void start_wrapper(void *zm, double c, double g, double k, double m)
{
    std::vector<double> yout;
    std::string request_str;
    auto type = PRM_DATA;
    encode_data(type, {c, g, k, m}, request_str);
    reinterpret_cast<ZmqMgr *>(zm)->sendRequest(request_str);
    reinterpret_cast<ZmqMgr *>(zm)->retrieveReply(yout);
    
    type = INIT_MTH; // init
    encode_data(type, {}, request_str);
    reinterpret_cast<ZmqMgr *>(zm)->sendRequest(request_str);
    reinterpret_cast<ZmqMgr *>(zm)->retrieveReply(yout);
}

void outputs_wrapper(void *zm, const double *u_ptr, double *y_ptr)
{
    std::vector<double> yout;
    std::string request_str;

    auto type = INP_DATA;
    encode_data(type, {*u_ptr}, request_str);
    reinterpret_cast<ZmqMgr *>(zm)->sendRequest(request_str);
    reinterpret_cast<ZmqMgr *>(zm)->retrieveReply(yout);

    type = OUTPUT_MTH;
    encode_data(type, {}, request_str);
    reinterpret_cast<ZmqMgr *>(zm)->sendRequest(request_str);
    reinterpret_cast<ZmqMgr *>(zm)->retrieveReply(yout);

    *y_ptr = yout[0];
}

void rc_outputs_wrapper(void *zm, const double *u_ptr, double *y_ptr, const std::string &fcnname)
{
    std::vector<double> yout;
    std::string request_str;

    auto type = SLFCN_CALL;
    encode_fcn_data(type, fcnname, {*u_ptr}, request_str);
    reinterpret_cast<ZmqMgr *>(zm)->sendRequest(request_str);
    reinterpret_cast<ZmqMgr *>(zm)->retrieveReply(yout);
    
    *y_ptr = yout[0];
}

void update_wrapper(void *zm, const double *u_ptr)
{
    std::vector<double> yout;
    std::string request_str;

    auto type = UPDATE_MTH;
    encode_data(type, {}, request_str);
    reinterpret_cast<ZmqMgr *>(zm)->sendRequest(request_str);
    reinterpret_cast<ZmqMgr *>(zm)->retrieveReply(yout);
}

void terminate_wrapper(void *zm)
{
    std::vector<double> yout;
    std::string request_str;
    auto type = TERMINATE_MTH;
    encode_data(type, {}, request_str);
    reinterpret_cast<ZmqMgr *>(zm)->sendRequest(request_str);
    reinterpret_cast<ZmqMgr *>(zm)->retrieveReply(yout);

}

void cleanupruntimeresouces_wrapper(void *zm)
{
    auto zmp = reinterpret_cast<ZmqMgr *>(zm);
    if (zmp) {
        shutdown_server(zmp);
        delete zmp;
    }
}

void query_simulink_function(const char *fcnName)
{
    std::string request_str;
    auto type = SLFCN_QUERY;
    encode_data(type, {}, request_str);
}

void transmit_outputs_wrapper(void *zm, const double *u_ptr, const int w)
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
    reinterpret_cast<ZmqMgr *>(zm)->retrieveReply(yout);
}
