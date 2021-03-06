% Copyright 2018 The MathWorks, Inc.


p = simulinkproject;

%% Build the server App
cd(fullfile(p.RootFolder,'CoSimExample','serverApp'));

mex('-client', 'engine',...
    ['-I' fullfile(p.RootFolder,'CoSimExample','util')],...
    ['-I' fullfile(p.RootFolder,'libzmq','include')],...
    ['-I' fullfile(p.RootFolder,'cppzmq')],...
    ['-L' fullfile(p.RootFolder,'libzmq','bin','x64','Release','v140','dynamic')],...
    '-llibzmq',...
    'statcalserver.cpp',...
    fullfile(p.RootFolder,'CoSimExample','util','statcal_util.cpp'));

%% Build the S-function
cd(fullfile(p.RootFolder,'CoSimExample','sfun'));

mex('-I..\util',...
    ['-I' p.RootFolder '\libzmq\include'],...
    ['-I' fullfile(p.RootFolder,'libzmq','include')],...
    ['-I' fullfile(p.RootFolder,'cppzmq')],...
    ['-L' fullfile(p.RootFolder,'libzmq','bin','x64','Release','v140','dynamic')],...
    '-llibzmq',...
    'statcalsfcngateway.cpp',...
    'statcalclient.cpp',...
    fullfile(p.RootFolder,'CoSimExample','util','statcal_util.cpp'));

cd(p.RootFolder)

