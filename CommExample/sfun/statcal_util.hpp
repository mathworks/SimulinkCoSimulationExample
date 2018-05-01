// Copyright 2018 The MathWorks, Inc.

typedef enum {
    CONN = 1,
    SHUTDOWN,
    INP_DATA,
} MsgType;

std::pair<MsgType,int> decode_header(const char *str);

void encode_data(const MsgType type, const std::vector<double> & data, std::string & ec);
void decode_data(const int len, const char *str, std::vector<double> & data);
