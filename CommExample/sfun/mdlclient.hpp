// Copyright 2018 The MathWorks, Inc.

void *setupruntimeresources_wrapper(const std::string &connStr);

void cleanupruntimeresouces_wrapper(void *zm);

void transmit_outputs_wrapper(void *zm, const double *u_ptr, const int w, const double request_timeout);

