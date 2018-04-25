% Copyright 2018 The MathWorks, Inc.

%% Add the ZMQ DLL to the system path:
p = simulinkproject;
if strcmp(mexext,'mexw64')
    if exist(fullfile(p.RootFolder,'libzmq','bin','x64','Release','v140','dynamic','libzmq.dll'),'file')
        pathEnvVar = getenv('PATH');
        setenv('PATH',[pathEnvVar ';' fullfile(p.RootFolder,'libzmq','bin','x64','Release','v140','dynamic')]);
        disp('libzmq.dll added to system PATH')
    else
        disp('libzmq.dll not found, build libzmq.dll before adding to the system path. See readme.mlx for more details.')
    end
else
    error('Non-Windows OS, please add the ZMQ library to your PATH environment variable manually');
end