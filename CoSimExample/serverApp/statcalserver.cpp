//Copyright 2018 The MathWorks, Inc.

//
//  Stats calculator server in C++
//  Receives (prev_value, current_value, beta, current_iteration_number)
//  Computes EWMA (Exponentially Weighted Moving Average) and returns the raw both
//  bias-corrected and non-bias-corrected EWMA values to the client.
//
#include <zmq.hpp>
#include <string>
#include <iostream>
#include <vector>
#include <utility>
#include <cmath>

#include "statcal_util.hpp"

const char *TERMINATE  = "terminate";
const char *SHUTTINGDOWN = "shutting down";

// Compute Exponentially Weighted Moving Average
std::pair<double, double> compute_ewma(const double prev, const double cv, const double beta, const int t)
{
    // EWMA
    double ma = beta*prev+(1-beta)*cv;

    // Bias corrected EWMA
    double bc = ma/(1-pow(beta,t));

    return std::make_pair(ma, bc);
}

void send_reply(zmq::socket_t & socket, const std::string &reply_str)
{
    zmq::message_t reply(reply_str.size());
    memcpy(reply.data (), reply_str.c_str(), reply_str.size());
    socket.send(reply);
}

// statcalserver <port_number>
int main (int argc, char *argv[]) {

    if (argc != 2) {
        std::cerr << "Error: stats calculator should be launched using statcalserver <port_number>" << std::endl;
        return 1;
    }

    // std::string host = argv[1];
    std::string port = argv[1];    
    std::string socket_addr = "tcp://*:"+port;
    
    //  Prepare our context and socket
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REP);
    
    socket.bind(socket_addr.c_str());

    while (true) {
        zmq::message_t request;
        
        // Wait for next request from client
        socket.recv (&request);
        
        char *data_str = static_cast<char*>(request.data());
        data_str[request.size()] = '\0';

        int len = decode_header(data_str);
        
        // Check received data header to determine if it's a string or array of double values
        if (len >= 0) {
            std::vector<double> data;
            decode_double_data(len, data_str, data);
            if (data.size() > 4) {
                std::cerr << "Data passed to statcalserver must be: prev_data, current_data, beta and current iteration number" << std::endl;
                return 1;
            }

            std::vector<double> md(2,0);
            std::tie(md[0], md[1]) = compute_ewma(data[0], data[1], data[2], static_cast<int>(data[3]));
            
            std::string reply_str;
            encode_double_data(md, reply_str);
            
            // Send reply back to client
            send_reply(socket, reply_str);
        } else {
            std::string d_str;
            decode_str_data(data_str, d_str);

            // Terminate server if client sends "terminate"
            if (strcmp(d_str.c_str(), TERMINATE) == 0) {
                std::string reply_str;
                encode_str_data(SHUTTINGDOWN, reply_str);
                send_reply(socket, reply_str);
                break;
            }
        }
    }
     
    return 0;
}
