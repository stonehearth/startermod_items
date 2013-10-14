local ProxyFabrication = require 'services.build.proxy_fabrication'
local ProxyColumn = class(ProxyFabrication)
local Constants = require 'services.build.constants'

local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

-- this is the component which manages the fabricator entity.
function ProxyColumn:__init(arg1)
   self[ProxyFabrication]:__init(arg1, 'stonehearth:column')
   local cursor = self:get_region():modify()
   cursor:copy_region(Region3(Cube3(Point3(0, 0, 0), Point3(1, Constants.STOREY_HEIGHT, 1))))
end

return ProxyColumn
