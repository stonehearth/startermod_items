local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

local PortalComponent = class()
local log = radiant.log.create_logger('build')

function PortalComponent:__init(entity, data_binding)
end

function PortalComponent:extend(json)
   log:warning('extend on PortalComponent is not yet implemented')
end

return PortalComponent
