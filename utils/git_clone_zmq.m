% Copyright 2018 The MathWorks, Inc.

p = simulinkproject;
cd(p.RootFolder)

if ~exist('cppzmq','dir')
    status = system('git clone https://github.com/zeromq/cppzmq');
    if status ~=0
        error('Problem calling Git, see MATLAB documentation or download manually from https://github.com/zeromq/cppzmq')
    end
end

if ~exist('libzmq','dir')
    status = system('git clone https://github.com/zeromq/libzmq')
    if status ~=0
        error('Problem calling Git, see MATLAB documentation or download manually from https://github.com/zeromq/libzmq')
    end
end