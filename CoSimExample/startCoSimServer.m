% Copyright 2018 The MathWorks, Inc.

%% Launch the server App
p = simulinkproject;
port = '8099';
system([fullfile(p.RootFolder,'CoSimExample','serverApp','statcalserver.exe') ' ' port ' & '])




