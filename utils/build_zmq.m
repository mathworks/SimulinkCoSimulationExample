% Copyright 2018 The MathWorks, Inc.


msg = sprintf(['\n\nBuilding ZMQ is different on different platform.\n\n',...
    'For Windows OS and Microsoft Visual Studio 2015, see ' sprintf('<a href="https://blogs.mathworks.com/simulink/">''This blog post''</a>')],...
    'For Other platforms and compilers, see installation instructions in cppzmq');

disp(msg);