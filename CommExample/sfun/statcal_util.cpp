// Copyright 2018 The MathWorks, Inc.

#include <string.h>
#include <vector>
#include <utility>
#include <iostream>
#include <cstring>
#include <tuple>
#include "statcal_util.hpp"

const char *DELIMITERS = " ,/";

// void convert2double(char *data_str, std::vector<double> & data)
// {
//     char *p;    
//     p = strtok(data_str, DELIMITERS);
//     while (p != nullptr) {
//         data.push_back(std::stod(p));
//         p = strtok(nullptr, DELIMITERS);
//     }
// }

// std::string convert2str(const std::vector<double> & data)
// {
//     std::string s;
//     for (auto & it : data) {
//         s += std::to_string(it);
//         s += " ";
//     }
//     return s;
// }

void segment_data_str(char *data_str, std::vector<std::string> & data)
{
    char *p;    
    p = strtok(data_str, DELIMITERS);
    while (p != nullptr) {
        data.push_back(std::string(p));
        p = strtok(nullptr, DELIMITERS);
    }
}

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

std::tuple<MsgType,int,int> decode_fcn_header(const char *str)
{
    MsgType type;
    int flen, ulen;
    std::memcpy(&type, str, sizeof(int));
    std::memcpy(&flen, str+sizeof(int), sizeof(int));
    std::memcpy(&ulen, str+2*sizeof(int)+flen*sizeof(char), sizeof(int));
    return std::make_tuple(type, flen, ulen);
}

void encode_fcn_data(const MsgType type, const std::string & fcn, const std::vector<double> & data, std::string & ec)
{
    size_t flen = fcn.size();
    size_t len  = data.size();
    size_t rlen = sizeof(double)*len;

    ec.resize(3*sizeof(int)+rlen+flen*sizeof(char), 0);

    std::memcpy(&ec[0], &type, sizeof(int));
    size_t offset = sizeof(int);

    std::memcpy(&ec[offset], &flen, sizeof(int));
    offset += sizeof(int);
    
    std::memcpy(&ec[offset], &fcn[0], flen*sizeof(char));
    offset += flen*sizeof(char);
    
    std::memcpy(&ec[offset], &len, sizeof(int));
    if (len > 0) {
        offset += sizeof(int);
        std::memcpy(&ec[offset], &data[0], sizeof(double)*len);
    }
}

void decode_fcn_data(const int flen, std::string & fcn, const int len, std::vector<double> & data, const char *str)
{
    data.resize(len, 0);
    fcn.resize(flen);

    std::memcpy(&fcn[0], str+2*sizeof(int), flen*sizeof(char));
    
    std::memcpy(&data[0], str+3*sizeof(int)+flen*sizeof(char),
                len*sizeof(double)); 
}

// [type][ulen int][u double data][ylen int][y double data]
std::tuple<MsgType,int,int> io_decode_header(const char *str)
{
    MsgType type;
    int ulen;
    int ylen;
    size_t offset = 0;
    std::memcpy(&type, str, sizeof(int));
    offset += sizeof(int);
    std::memcpy(&ulen, str+offset, sizeof(int));
    offset += sizeof(int);
    offset += ulen*sizeof(double);
    std::memcpy(&ylen, str+offset, sizeof(int));
    
    return std::make_tuple(type, ulen, ylen);
}

void io_decode_data(const char *str, const int ulen, const int ylen, std::vector<double> &udata, std::vector<double> &ydata)
{
    size_t offset = sizeof(int)*2;
    std::memcpy(&udata[0], str+offset, ulen*sizeof(double));

    offset += ulen*sizeof(double)+sizeof(int);
    std::memcpy(&ydata[0], str+offset, ylen*sizeof(double));
}

void io_encode_data(const MsgType type, const std::vector<double> & udata, const std::vector<double> & ydata, std::string & ec)
{
    size_t ulen = udata.size();
    size_t ylen = ydata.size();
    size_t rlen = sizeof(double)*(ulen+ylen);

    ec.resize(3*sizeof(int)+rlen, 0);

    size_t offset = 0;
    std::memcpy(&ec[0], &type, sizeof(int));

    offset += sizeof(int);
    std::memcpy(&ec[offset], &ulen, sizeof(int));

    offset += sizeof(int);
    if (ulen > 0) {
        std::memcpy(&ec[offset], &udata[0], sizeof(double)*ulen);
        offset += sizeof(double)*ulen;
    }
    std::memcpy(&ec[offset], &ylen, sizeof(int));
    offset += sizeof(int);
    if (ylen >0) {
        std::memcpy(&ec[offset], &ydata[0], sizeof(double)*ylen);
    }
}
                    
void encode_str_data(const MsgType type, const char *data, std::string & ec)
{
    size_t len = strlen(data);
    size_t rlen = sizeof(char)*len;
    ec.resize(2*sizeof(int)+rlen, 0);

    std::memcpy(&ec[0], &type, sizeof(int));
    std::memcpy(&ec[sizeof(int)], &len, sizeof(int));
        
    if (len > 0) {
        std::memcpy(&ec[2*sizeof(int)], &data[0], sizeof(char)*len);
    }
}
