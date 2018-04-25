// Copyright 2018 The MathWorks, Inc.

void *rc_setupruntimeresources_wrapper(const std::string &connStr);
void rc_outputs_wrapper(void *zm, const double *u_ptr, double *y_ptr,const std::string &fcnname);

void *setupruntimeresources_wrapper(const std::string & host, const std::string &port, const std::string & shlibname);
void start_wrapper(void *zm, double c, double g, double k, double m);
void outputs_wrapper(void *zm, const double *u_ptr, double *y_ptr);
void update_wrapper(void *zm, const double *u_ptr);
void terminate_wrapper(void *zm);
void cleanupruntimeresouces_wrapper(void *zm);

void transmit_outputs_wrapper(void *zm, const double *u_ptr, const int w);

