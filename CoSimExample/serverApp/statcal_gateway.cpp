//Copyright 2018 The MathWorks, Inc.


//  stats calculator client in C++
//  Connects REQ socket to tcp://localhost:5555
//
#include <zmq.hpp>
#include <string>
#include <iostream>
#include <memory>
#include <chrono>
#include <cstring>

#include "statcal_util.hpp"

using namespace std::chrono;

#define REQUEST_TIMEOUT     2500    //  msecs, (> 1000!)
#define REQUEST_RETRIES     3       //  Before we abandon

// [int len][double][double]...[double]
// [int -len][char][char]...[char]

std::unique_ptr<zmq::socket_t> CreateSocket(zmq::context_t &context, const std::string & socket_addr)
{
    std::unique_ptr<zmq::socket_t> socket_ptr(new zmq::socket_t(context, ZMQ_REQ));

    socket_ptr->connect(socket_addr.c_str());
    int linger = 0;
    socket_ptr->setsockopt (ZMQ_LINGER, &linger, sizeof (linger));
    std::cout << "Connecting to stats calculator server…" << std::endl;
    
    return socket_ptr;
}

void SendRequest(std::unique_ptr<zmq::socket_t> & socket_ptr,
                 zmq::context_t &context,
                 const std::string & socket_addr,
                 const std::string request_str,
                 zmq::message_t & reply,
                 int retries_left = REQUEST_RETRIES)
{
    zmq::message_t request;
    request.rebuild(request_str.size());
    memset(request.data(), 0, request_str.size());    
    memcpy(request.data (), request_str.c_str(), request_str.size());

    // Try REQUEST_RETRIES times. and each try has a time-out of
    // REQUEST_TIMEOUT milliseconds
    
    while (retries_left) {
        // std::cout << "Sending " << request_str << std::endl;
        socket_ptr->send(request);
  
        //  Poll socket for a reply, with timeout
        zmq::pollitem_t items[] = { {*socket_ptr, 0, ZMQ_POLLIN, 0 } };
        zmq::poll (&items[0], 1, REQUEST_TIMEOUT);
        
        //  If we got a reply, process it
        if (items[0].revents & ZMQ_POLLIN) {
            
            socket_ptr->recv(&reply);
            char *reply_str = static_cast<char*>(reply.data());
            reply_str[reply.size()] = '\0';

            int len = decode_header(reply_str);
            if (len >= 0) {
                std::vector<double> data;
                decode_double_data(len, reply_str, data);
                std::cout << "Received: (len) " << len << "(data)";
                for (auto & it : data) {
                    std::cout << " " << it;
                }
                std::cout << std::endl;
            } else {
                std::string str;
                decode_str_data(reply_str, str);
                std::cout << "Received: " << str << std::endl;
            }
            break;
            
        } else if (--retries_left == 0) {
            throw std::runtime_error("Server seems to be offline, and request has been abandoned");
        } else {
            std::cout << "No response from server, retrying … " << std::endl;
            // Old socket will be confused; close it and open a new one
            // Resend
            socket_ptr = CreateSocket(context, socket_addr);
        }
    }
}

int main (int argc, char *argv[])
{
    if (argc != 3) {
         std::cerr << "Error: stats client should be launched using statcal_gateway <host> <port_number>" << std::endl;
        return 1;
    }

    std::string host = argv[1];
    std::string port = argv[2];
    std::string socket_addr = "tcp://"+host+":"+port;

    //  Prepare our context and socket
    zmq::context_t context (1);

    
    std::string request_str;
    zmq::message_t reply;

    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    
    try {
        auto socket_ptr = CreateSocket(context, socket_addr);

        std::vector<double> data{0, 4.32, 0.99, 1};
        encode_double_data(data, request_str);
        SendRequest(socket_ptr, context, socket_addr, request_str, reply);
        encode_str_data("terminate", request_str);
        SendRequest(socket_ptr, context, socket_addr, request_str, reply);
    } catch (std::exception &e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
    
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    double dif = duration_cast<nanoseconds>( t2 - t1 ).count()/1e9;
    printf ("Elasped time is %lf seconds.\n", dif );
    
    return 0;
}
