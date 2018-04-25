// Copyright 2018 The MathWorks, Inc.

//
//  Simulink to external tool co-simulation gateway example using S-function
//  and ZeroMQ https://github.com/zeromq/cppzmq 
//
#define S_FUNCTION_NAME  statcalsfcngateway
#define S_FUNCTION_LEVEL 2

#include <string>
#include <iostream>
#include <memory>

#include "statcalclient.hpp"
#include "simstruc.h"

#define HOST_NAME_P  0
#define PORT_NUM_P   1
#define BETA_P       2
#define INIT_VALUE_P 3
#define STEP_SIZE_P  4
#define NUM_PRMS     5

#define RTP_BETA     0
#define RTP_INIT_VAL 1

#define MAX_STR_LEN 1024

#define MDL_CHECK_PARAMETERS
#if defined(MDL_CHECK_PARAMETERS) && defined(MATLAB_MEX_FILE)
// mdlCheckParameters
// Validate our parameters to verify they are okay
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

    bool isValid = mxIsDouble(ssGetSFcnParam(S,BETA_P)) &&
        mxGetNumberOfElements(ssGetSFcnParam(S,BETA_P)) == 1 &&
        !mxIsComplex(ssGetSFcnParam(S,BETA_P));

    if (isValid) {
        double *v = reinterpret_cast<double *>(mxGetData(ssGetSFcnParam(S,BETA_P)));
        if (*v < 0) isValid = false;
    }

    if (!isValid) {
        ssSetErrorStatus(S,"Exponential weight parameter must be a positive real scalar of double data type.");
        return;
    }

    if (!mxIsDouble(ssGetSFcnParam(S,INIT_VALUE_P)) ||
        mxGetNumberOfElements(ssGetSFcnParam(S,INIT_VALUE_P)) != 1 ||
        mxIsComplex(ssGetSFcnParam(S,INIT_VALUE_P))) {
        ssSetErrorStatus(S,"Initial value for output should be a real scalar of double data type.");
        return;
    }
    
    isValid = mxIsDouble(ssGetSFcnParam(S,STEP_SIZE_P)) &&
        mxGetNumberOfElements(ssGetSFcnParam(S,STEP_SIZE_P)) == 1 &&
        !mxIsComplex(ssGetSFcnParam(S,STEP_SIZE_P));

    if (isValid) {
        double *v = reinterpret_cast<double *>(mxGetData(ssGetSFcnParam(S,STEP_SIZE_P)));
        if (*v < 0) isValid = false;
    }
    if (!isValid) {
        ssSetErrorStatus(S,"Communication interval parameter must be a positive real scalar of double data type.");
        return;
    }
    return;
}
#endif // MDL_CHECK_PARAMETERS


// mdlInitializeSizes
// Method to check parameters and configure the S-Function block.
static void mdlInitializeSizes(SimStruct *S)
{
    // Register the number of expected parameters
    ssSetNumSFcnParams(S, NUM_PRMS);

#if defined(MATLAB_MEX_FILE)
    if (ssGetNumSFcnParams(S) == ssGetSFcnParamsCount(S)) {
        mdlCheckParameters(S);
        if (ssGetErrorStatus(S) != NULL) {
            return;
        }
    } else {
        return; // Parameter mismatch will be reported by Simulink
    }
#endif

    ssSetSFcnParamTunable(S, HOST_NAME_P, false);
    ssSetSFcnParamTunable(S, PORT_NUM_P, false);
    ssSetSFcnParamTunable(S, BETA_P, true);
    ssSetSFcnParamTunable(S, INIT_VALUE_P, true);
    ssSetSFcnParamTunable(S, STEP_SIZE_P, false);

    if (!ssSetNumInputPorts(S, 1)) return;
    
    ssSetInputPortWidth(S, 0, 1);
    ssSetInputPortDataType(S, 0, SS_DOUBLE);
    ssSetInputPortComplexSignal(S, 0, COMPLEX_NO);
    ssSetInputPortRequiredContiguous(S, 0, 1);

    ssSetInputPortDirectFeedThrough(S, 0, 0);
    
    if (!ssSetNumOutputPorts(S, 1)) return;
    
    ssSetOutputPortWidth(S, 0, 1);
    ssSetOutputPortDataType(S, 0, SS_DOUBLE);
    ssSetOutputPortComplexSignal(S, 0, COMPLEX_NO);   


    ssSetNumSampleTimes(S, 1);

    // Specify the sim state compliance to be same as Simulink built-in block
    ssSetSimStateCompliance(S, USE_DEFAULT_SIM_STATE);
    
    ssSetOptions(S,
                 SS_OPTION_EXCEPTION_FREE_CODE);
    
    ssSetModelReferenceNormalModeSupport(S, MDL_START_AND_MDL_PROCESS_PARAMS_OK);
}

// mdlInitializeSampleTimes
//   This function is used to specify the sample time(s) for your
//   S-function. You must register the same number of sample times as
//   specified in ssSetNumSampleTimes.
static void mdlInitializeSampleTimes(SimStruct *S)
{
    double *stepSizeP = reinterpret_cast<double *>(mxGetData(ssGetSFcnParam(S,STEP_SIZE_P)));
    
    ssSetSampleTime(S, 0, *stepSizeP);
    ssSetOffsetTime(S, 0, 0.0);
    ssSetModelReferenceSampleTimeDefaultInheritance(S);
}

#define MDL_SET_WORK_WIDTHS
#if defined(MDL_SET_WORK_WIDTHS) && defined(MATLAB_MEX_FILE)
// mdlSetWorkWidths
//
static void mdlSetWorkWidths(SimStruct *S)
{
    // Declare the DWorks vectors
    ssSetNumPWork(S, 1);
   
    ssSetNumDWork(S, 2);

    // Previous EWMA value
    ssSetDWorkWidth(S, 0, 1);
    ssSetDWorkDataType(S, 0,SS_DOUBLE );

    // Current number of iterations
    ssSetDWorkWidth(S, 1, 1);
    ssSetDWorkDataType(S, 1, SS_UINT32);

    const char_T    *rtParamNames[] = {"beta", "initval"};
    ssRegAllTunableParamsAsRunTimeParams(S, rtParamNames);

}
#endif // MDL_SET_WORK_WIDTHS

#define MDL_PROCESS_PARAMETERS
#if defined(MDL_PROCESS_PARAMETERS) && defined(MATLAB_MEX_FILE)
// mdlProcessParameters
// Update run-time parameters.
static void mdlProcessParameters(SimStruct *S)
{
    // Update Run-Time parameters
    ssUpdateAllTunableParamsAsRunTimeParams(S);
}
#endif // MDL_PROCESS_PARAMETERS

#define GET_ZM_PTR(S) ssGetPWorkValue(S,0)

auto Mx_Deleter = [](char *m) { mxFree(m); };
using mxCharUnqiuePtr = std::unique_ptr<char, decltype(Mx_Deleter)>;

static std::string host_and_port_addr(const SimStruct *S)
{
    mxCharUnqiuePtr hostStr(mxArrayToString(ssGetSFcnParam(S,HOST_NAME_P)), Mx_Deleter);
    mxCharUnqiuePtr portStr(mxArrayToString(ssGetSFcnParam(S,PORT_NUM_P)), Mx_Deleter);
    
    std::string connStr = "tcp://";
    connStr += hostStr.get();
    connStr += ":";
    connStr += portStr.get();

    return connStr;
}

#define MDL_SETUP_RUNTIME_RESOURCES
// Called at the beginning of one or multiple simulations.
void mdlSetupRuntimeResources(SimStruct *S)
{
    std::cout << "Opening connection with server" << std::endl;
    std::string connStr = host_and_port_addr(S);
    ssSetPWorkValue(S, 0, setupruntimeresources_wrapper(connStr));
}


#define MDL_START // to indicate that the S-function has mdlStart method

// mdlStart
// Called at the beginning of every simulation. Called at every Fast Restart.
static void mdlStart(SimStruct *S)
{
    double *prev = reinterpret_cast<double *>(ssGetDWork(S,0));
    unsigned int *iter = reinterpret_cast<unsigned int *>(ssGetDWork(S,1));
    // Initialize previous states and iteration counter
    start_wrapper(prev, iter);
}

// mdlOutputs
// 
static void mdlOutputs(SimStruct *S, int_T tid)
{
    double   *y_ptr = reinterpret_cast<double *>(ssGetOutputPortSignal(S,0));
    unsigned int *iter_ptr = reinterpret_cast<unsigned int *>(ssGetDWork(S,1));
    double *prev_ptr   = reinterpret_cast<double *>(ssGetDWork(S,0));

    double *init_ptr   = reinterpret_cast<double *>((ssGetRunTimeParamInfo(S,RTP_INIT_VAL))->data);
    try {
        outputs_wrapper(GET_ZM_PTR(S), iter_ptr, y_ptr, init_ptr, prev_ptr);
    } catch (std::exception &e) {
        static std::string errstr(e.what());
        ssSetErrorStatus(S, errstr.c_str());
        return;
    }
}

#define MDL_UPDATE
// mdlUpdate
//
static void mdlUpdate(SimStruct *S, int_T tid)
{
    const double   *prev_ptr = reinterpret_cast<double *>(ssGetDWork(S,0));
    const double   *u_ptr    = reinterpret_cast<const double *>(ssGetInputPortSignal(S,0));
    unsigned int   *iter_ptr = reinterpret_cast<unsigned int *>(ssGetDWork(S,1));
    const double   *beta_ptr = reinterpret_cast<double *>((ssGetRunTimeParamInfo(S,RTP_BETA))->data);
    
    update_wrapper(GET_ZM_PTR(S), iter_ptr, u_ptr, beta_ptr, prev_ptr);                  
}

#define MDL_CLEANUP_RUNTIME_RESOURCES
// Called at the end of one or multiple simulations. This method is NOT called
// at the end of each subsequent Fast Restart.
static void mdlCleanupRuntimeResources(SimStruct *S)
{
    std::cout << "Closing connection with server" << std::endl;
    cleanupruntimeresouces_wrapper(GET_ZM_PTR(S));
}



// mdlTerminate
// Called at the end of every simulation. Called at the end of every Fast Restart.
static void mdlTerminate(SimStruct *S)
{
    unsigned int *iter_ptr = reinterpret_cast<unsigned int *>(ssGetDWork(S,1));
    try {
        terminate_wrapper(GET_ZM_PTR(S), iter_ptr);
    } catch (std::exception &e) {
        static std::string errstr(e.what());
        ssSetErrorStatus(S, errstr.c_str());
        return;
    }
}

#ifdef  MATLAB_MEX_FILE    // Is this file being compiled as a MEX-file?
#include "simulink.c"      // MEX-file interface mechanism
#else
#include "cg_sfun.h"       // Code generation registration function
#endif
