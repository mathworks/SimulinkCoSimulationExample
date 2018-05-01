// Copyright 2018 The MathWorks, Inc.

/*
 * File : slfcn_gw.c
 * Abstract:
 */

#include <zmq.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <memory>

#define S_FUNCTION_NAME  sfcn_receive
#define S_FUNCTION_LEVEL 2

#include "simstruc.h"
#include "statcal_util.hpp"

/*================*
 * Build checking *
 *================*/

#define HOST_NAME_P  0
#define PORT_NUM_P   1
#define DATA_WIDTH_P 2
#define STEP_SIZE_P  3
#define TIMEOUT_P    4
#define NUM_PRMS     5

static bool isPositiveRealDoubleParam(const mxArray *p)
{
    bool isValid = (mxIsDouble(p) &&
                    mxGetNumberOfElements(p) == 1 &&
                    !mxIsComplex(p));

    if (isValid) {
        double *v = reinterpret_cast<double *>(mxGetData(p));
        if (*v < 0) isValid = false;
    }
    return isValid;
}


class ZmqServer {
  public:
    ZmqServer(const std::string &addr) : context(1), socket_addr(addr)
    {
        socket_ptr.reset(new zmq::socket_t(context, ZMQ_REP));
        socket_ptr->bind(socket_addr.c_str());
    }

    ~ZmqServer() {}

    MsgType receiveRequest(std::vector<double> &u, int request_timeout,
                           int retries_left = 3)
    {
        MsgType type = CONN;
        while (retries_left) {
            zmq::message_t request;
            //  Poll socket for a reply, with timeout
            zmq::pollitem_t items[] = { {(void*)*socket_ptr, 0, ZMQ_POLLIN, 0 } };
            zmq::poll (&items[0], 1, request_timeout);
            
            //  If we got a reply, process it
            if (items[0].revents & ZMQ_POLLIN) {
                
                socket_ptr->recv(&request);
                char *data_str = static_cast<char*>(request.data());
                data_str[request.size()] = '\0';
                
                int ulen;                
                std::tie(type, ulen) = decode_header(data_str);
                decode_data(ulen, data_str, u);
                return type;
            } else if (--retries_left == 0) {
                throw std::runtime_error("Connection timed out. Please ensure that the transmitter side is running and two sides are not in a locked state due to unintended execution orders. If you have a long running algorithm, you can increase timeout parameter value from the block dialog.");
            } else {
                std::cout << "No request received, try again" << std::endl;
            }
        }
        return type;
    }

    void sendReply()
    {
        std::string reply_str;
        encode_data(INP_DATA, {}, reply_str);
        zmq::message_t reply(reply_str.size());
        memcpy(reply.data (), reply_str.c_str(), reply_str.size());
        socket_ptr->send(reply);
    }

    void resetSocketPtr()
    {
        socket_ptr.reset(nullptr);
    }
    
  private:
    zmq::context_t context;
    std::string    socket_addr;
    std::unique_ptr<zmq::socket_t>  socket_ptr;
    
};

#define MDL_CHECK_PARAMETERS
#if defined(MDL_CHECK_PARAMETERS) && defined(MATLAB_MEX_FILE)
/* Function: mdlCheckParameters =============================================
 * Abstract:
 *    Validate our parameters to verify they are okay.
 */
static void mdlCheckParameters(SimStruct *S)
{
    if (!mxIsChar(ssGetSFcnParam(S,HOST_NAME_P))) {
        ssSetErrorStatus(S,"Host name parameter must be a char array.");
        return;
    }
    
    if (!mxIsChar(ssGetSFcnParam(S,PORT_NUM_P))) {
        ssSetErrorStatus(S,"Port number parameter must be a char array.");
        return;
    }

    bool isValid = isPositiveRealDoubleParam(ssGetSFcnParam(S,DATA_WIDTH_P));
    if (!isValid) {
        ssSetErrorStatus(S,"Data width parameter must be a positive scalar.");
        return;
    }

    isValid = isPositiveRealDoubleParam(ssGetSFcnParam(S,STEP_SIZE_P));
    if (!isValid) {
        ssSetErrorStatus(S,"Step size parameter must be a double real scalar.");
        return;
    }

    isValid = isPositiveRealDoubleParam(ssGetSFcnParam(S,TIMEOUT_P));
    if (!isValid) {
        ssSetErrorStatus(S,"Timeout in seconds parameter must be a positive double real scalar.");
        return;
    }

    
    return;
}
#endif /* MDL_CHECK_PARAMETERS */


/* Function: mdlInitializeSizes ===============================================
 * Abstract:
 *   Setup sizes of the various vectors.
 */
static void mdlInitializeSizes(SimStruct *S)
{
    ssSetNumSFcnParams(S, NUM_PRMS);

#if defined(MATLAB_MEX_FILE)
    if (ssGetNumSFcnParams(S) == ssGetSFcnParamsCount(S)) {
        mdlCheckParameters(S);
        if (ssGetErrorStatus(S) != NULL) {
            return;
        }
    } else {
        return; /* Parameter mismatch will be reported by Simulink */
    }

#endif
    
    ssSetSFcnParamTunable(S, HOST_NAME_P, false);
    ssSetSFcnParamTunable(S, PORT_NUM_P, false);
    ssSetSFcnParamTunable(S, STEP_SIZE_P, false);
    ssSetSFcnParamTunable(S, TIMEOUT_P, false);
    
    if (!ssSetNumInputPorts(S, 0)) return;

    if (!ssSetNumOutputPorts(S, 1)) return;

    double *dataWidthP = reinterpret_cast<double *>(mxGetData(ssGetSFcnParam(S,DATA_WIDTH_P)));
    ssSetOutputPortWidth(S, 0, static_cast<int>(*dataWidthP));
    
    ssSetNumSampleTimes(S, 1);
    
    /* specify the sim state compliance to be same as a built-in block */
    ssSetSimStateCompliance(S, USE_DEFAULT_SIM_STATE);

    ssSetOptions(S,
                 SS_OPTION_WORKS_WITH_CODE_REUSE |
                 SS_OPTION_EXCEPTION_FREE_CODE |
                 SS_OPTION_USE_TLC_WITH_ACCELERATOR);
}
/* Function: mdlInitializeSampleTimes =====================================
 * Abstract:
 *   This function is used to specify the sample time(s) for your
 *   S-function. You must register the same number of sample times as
 *   specified in ssSetNumSampleTimes.
 */
static void mdlInitializeSampleTimes(SimStruct *S)
{
    double *stepSizeP = reinterpret_cast<double *>(mxGetData(ssGetSFcnParam(S,STEP_SIZE_P)));
    
    ssSetSampleTime(S, 0, *stepSizeP);
    ssSetOffsetTime(S, 0, 0.0);
    ssSetModelReferenceSampleTimeDefaultInheritance(S);
}

#define MDL_SET_WORK_WIDTHS
#if defined(MDL_SET_WORK_WIDTHS) && defined(MATLAB_MEX_FILE)
static void mdlSetWorkWidths(SimStruct *S)
{
    ssSetNumPWork(S, 1);
}
#endif // MDL_SET_WORK_WIDTHS

#define GET_ZM_PTR(S) reinterpret_cast<ZmqServer *>(ssGetPWorkValue(S,0))

auto Mx_Deleter = [](char *m) { mxFree(m); };
using mxCharUnqiuePtr = std::unique_ptr<char, decltype(Mx_Deleter)>;

static std::string host_and_port_addr(const SimStruct *S)
{
    mxCharUnqiuePtr hostStr(mxArrayToString(ssGetSFcnParam(S,HOST_NAME_P)), Mx_Deleter);
    std::string serverHostStr = hostStr.get();

    mxCharUnqiuePtr portStr(mxArrayToString(ssGetSFcnParam(S,PORT_NUM_P)), Mx_Deleter);
    std::string serverPortStr = portStr.get();
    
    std::string connStr = "tcp://*";
    // connStr += serverHostStr;
    connStr += ":";
    connStr += serverPortStr;

    return connStr;
}

#define MDL_SETUP_RUNTIME_RESOURCES
void mdlSetupRuntimeResources(SimStruct *S)
{
    std::cout << "Starting connection" << std::endl;

    std::string connStr = host_and_port_addr(S);

    auto zmq = new ZmqServer(connStr);
    
    ssSetPWorkValue(S, 0, zmq);
}

#define MDL_START /* to indicate that the S-function has mdlStart method */

/* mdlStart ==========================================================
 * Abstract:
 *   Allocate and initialize run-time data and cache the pointer in the PWork.
 */
static void mdlStart(SimStruct *S)
{
}

/* Function: mdlOutputs =======================================================
 * Abstract:
 *    Do nothing
 */
static void mdlOutputs(SimStruct *S, int_T tid)
{    
    std::string fcnName;
    std::vector<double> uv;
    std::string fcn;
    double *timeout_ptr = reinterpret_cast<double *>(mxGetData(ssGetSFcnParam(S,TIMEOUT_P)));

    auto zmq = GET_ZM_PTR(S);
    MsgType r;
    
    try {
        r = zmq->receiveRequest(uv, (*timeout_ptr)*1000);
    } catch (std::exception &e) {
        static std::string errstr(e.what());
        ssSetErrorStatus(S, errstr.c_str());
        return;
    }
    
    if (r == SHUTDOWN) {
        ssSetStopRequested(S, 1);
        return;
    } else if (r != INP_DATA) {
        ssSetErrorStatus(S, "Expecting input data request");
        return;
    }

    auto y = ssGetOutputPortRealSignal(S,0);
    
    std::memcpy(y, &uv[0], sizeof(double)*ssGetOutputPortWidth(S,0));
    
    zmq->sendReply();
}

#define MDL_CLEANUP_RUNTIME_RESOURCES
static void mdlCleanupRuntimeResources(SimStruct *S)
{
    std::cout << "Closing connection" << std::endl;
    auto zmq = GET_ZM_PTR(S);
    if (zmq) {
        zmq->resetSocketPtr();
        delete zmq;
    }
}

/* Function: mdlTerminate =====================================================
 * Abstract:
 *    This method is required for Level 2 S-functions.
 */
static void mdlTerminate(SimStruct *S)
{
}

#ifdef  MATLAB_MEX_FILE    /* Is this file being compiled as a MEX-file? */
#include "simulink.c"      /* MEX-file interface mechanism */
#else
#include "cg_sfun.h"       /* Code generation registration function */
#endif
