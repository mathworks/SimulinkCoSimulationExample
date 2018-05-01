// Copyright 2018 The MathWorks, Inc.

/*
 * File : sfcn_transmit.c
 */


#define S_FUNCTION_NAME  sfcn_transmit
#define S_FUNCTION_LEVEL 2

#include <string>
#include <iostream>
#include <memory>

#include "simstruc.h"
#include "mdlclient.hpp"

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
        ssSetErrorStatus(S,"Step size parameter must be a positive double real scalar.");
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
{    /* Register the number of expected parameters */
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
    
    if (!ssSetNumInputPorts(S, 1)) return;

    double *dataWidthP = reinterpret_cast<double *>(mxGetData(ssGetSFcnParam(S,DATA_WIDTH_P)));
    
    ssSetInputPortWidth(S, 0, static_cast<int>(*dataWidthP));
    ssSetInputPortDataType(S, 0, SS_DOUBLE);
    ssSetInputPortComplexSignal(S, 0, COMPLEX_NO);
    ssSetInputPortRequiredContiguous(S, 0, 1);

    ssSetInputPortDirectFeedThrough(S, 0, 1);
    
    if (!ssSetNumOutputPorts(S, 0)) return;
    
    ssSetNumSampleTimes(S, 1);

    /* specify the sim state compliance to be same as Simulink built-in block */
    ssSetSimStateCompliance(S, USE_DEFAULT_SIM_STATE);
    
    ssSetOptions(S,
                 SS_OPTION_EXCEPTION_FREE_CODE);
    
    ssSetModelReferenceNormalModeSupport(S, MDL_START_AND_MDL_PROCESS_PARAMS_OK);
}

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

#define GET_ZM_PTR(S) ssGetPWorkValue(S,0)

auto Mx_Deleter = [](char *m) { mxFree(m); };
using mxCharUnqiuePtr = std::unique_ptr<char, decltype(Mx_Deleter)>;

static std::string host_and_port_addr(const SimStruct *S)
{
    mxCharUnqiuePtr hostStr(mxArrayToString(ssGetSFcnParam(S,HOST_NAME_P)), Mx_Deleter);
    std::string serverHostStr = hostStr.get();

    mxCharUnqiuePtr portStr(mxArrayToString(ssGetSFcnParam(S,PORT_NUM_P)), Mx_Deleter);
    std::string serverPortStr = portStr.get();
    
    std::string connStr = "tcp://";
    connStr += serverHostStr;
    connStr += ":";
    connStr += serverPortStr;

    return connStr;
}

#define MDL_SETUP_RUNTIME_RESOURCES
void mdlSetupRuntimeResources(SimStruct *S)
{
    auto connStr = host_and_port_addr(S);
    ssSetPWorkValue(S, 0, setupruntimeresources_wrapper(connStr));
}

/* Function: mdlOutputs =======================================================
 * Abstract:
 *    Do nothing
 */
static void mdlOutputs(SimStruct *S, int_T tid)
{
    const double *u_ptr = reinterpret_cast<const double *>(ssGetInputPortSignal(S,0));
    double *timeout_ptr = reinterpret_cast<double *>(mxGetData(ssGetSFcnParam(S,TIMEOUT_P)));
    
    try {
        transmit_outputs_wrapper(GET_ZM_PTR(S), u_ptr, ssGetInputPortWidth(S,0), *timeout_ptr*1000);
    } catch (std::exception &e) {
        static std::string errstr(e.what());
        ssSetErrorStatus(S, errstr.c_str());
        return;
    }
}

#define MDL_CLEANUP_RUNTIME_RESOURCES
static void mdlCleanupRuntimeResources(SimStruct *S)
{
    std::cout << "Closing connection" << std::endl;
    try {
        cleanupruntimeresouces_wrapper(GET_ZM_PTR(S));
    } catch (std::exception &e) {
        static std::string errstr(e.what());
        ssSetErrorStatus(S, errstr.c_str());
        return;
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
