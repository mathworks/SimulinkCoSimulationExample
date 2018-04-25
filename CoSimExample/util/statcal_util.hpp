// Copyright 2018 The MathWorks, Inc.


#ifndef STATCAL_UTIL_HPP
#define STATCAL_UTIL_HPP

void convert2double(char *data_str, std::vector<double> & data);
std::string convert2str(const std::vector<double> & data);

int decode_header(const char *str);
void encode_double_data(const std::vector<double> & data, std::string & ec);
void encode_str_data(const char *data, std::string & ec);
void decode_double_data(const int len, const char *str, std::vector<double> & data);
void decode_str_data(const char *str, std::string & s);

#endif
