function StartUDPModelSim(mdlname)
% Copyright 2018 The MathWorks, Inc.

if ~iscell(mdlname)
    mdlname = {mdlname};
end

numsim = numel(mdlname);

% Change this to minimum of nummodels and default
p = parpool(numsim);

for idx = 1:numsim
    F = parfeval(p, @SingleSim, 0, mdlname{idx});
end

wait(F)


end

function SingleSim(mdl)
    % NOTE: Replace with load_system to avoid opening windows
    open_system(mdl);
    set_param(mdl,'SimulationCommand','Start');    
end