// Copyright 2018 The MathWorks, Inc.

typedef enum {
    CONN = 1,
    SHUTDOWN,
    PRM_DATA,
    INP_DATA,
    INIT_MTH,
    OUTPUT_MTH,
    UPDATE_MTH,
    TERMINATE_MTH,
    SLFCN_QUERY,
    SLFCN_CALL,
    SLFCN_REPLY
} MsgType;

void convert2double(char *data_str, std::vector<double> & data);
std::string convert2str(const std::vector<double> & data);
void segment_data_str(char *data_str, std::vector<std::string> & data);

std::pair<MsgType,int> decode_header(const char *str);

void encode_data(const MsgType type, const std::vector<double> & data, std::string & ec);
void decode_data(const int len, const char *str, std::vector<double> & data);

void encode_str_data(const MsgType type, const char *data, std::string & ec);

std::tuple<MsgType,int,int> decode_fcn_header(const char *str);

std::tuple<MsgType,int,int> io_decode_header(const char *str);
void encode_fcn_data(const MsgType type, const std::string & fcn, const std::vector<double> & data, std::string & ec);
void decode_fcn_data(const int flen, std::string & fcn, const int len, std::vector<double> & data, const char *str);

void io_decode_data(const char *str, const int ulen, const int ylen, std::vector<double> &udata, std::vector<double> &ydata);

void io_encode_data(const MsgType type, const std::vector<double> & udata, const std::vector<double> & ydata, std::string & ec);

