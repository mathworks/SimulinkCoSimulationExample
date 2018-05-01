// Copyright 2018 The MathWorks, Inc.

#include <string.h>
#include <vector>
#include <utility>
#include <iostream>
#include <cstring>
#include <tuple>
#include "statcal_util.hpp"

// [int type][int len][double][double]...[double]

std::pair<MsgType,int> decode_header(const char *str)
{
    MsgType type;
    int len;
    std::memcpy(&type, str, sizeof(int));
    std::memcpy(&len, str+sizeof(int), sizeof(int));
    return std::make_pair(type, len);
}

void encode_data(const MsgType type, const std::vector<double> & data, std::string & ec)
{
    size_t len  = data.size();
    size_t rlen = sizeof(double)*len;

    ec.resize(2*sizeof(int)+rlen, 0);
 
    std::memcpy(&ec[0], &type, sizeof(int));
    std::memcpy(&ec[sizeof(int)], &len, sizeof(int));
        
    if (len > 0) {
        std::memcpy(&ec[2*sizeof(int)], &data[0], sizeof(double)*len);
    }
}

void decode_data(const int len, const char *str, std::vector<double> & data)
{
    data.resize(len, 0);
    std::memcpy(&data[0], str+2*sizeof(int), len*sizeof(double)); 
}
