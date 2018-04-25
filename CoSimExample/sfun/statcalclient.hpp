// Copyright 2018 The MathWorks, Inc.


#ifndef STATCAL_CLIENT_HPP
#define STATCAL_CLIENT_HPP

void *setupruntimeresources_wrapper(const std::string & connStr);

void start_wrapper(double *prev_ptr, unsigned int *iter_ptr);

void outputs_wrapper(void *zm, unsigned int *iter_ptr, double *y_ptr, double *init_ptr, double *prev_ptr);

void update_wrapper(void *zm, unsigned int *iter_ptr, const double *u_ptr, const double *beta_ptr, const double *prev_ptr);

void terminate_wrapper(void *zm, unsigned int *iter_ptr);

void cleanupruntimeresouces_wrapper(void *zm);

#endif // STATCAL_CLIENT_HPP
