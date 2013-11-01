local ProxyFabrication = require 'services.build.proxy_fabrication'
local ProxyRoof = class(ProxyFabrication)
local constants = require 'services.build.constants'

local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

-- this is the component which manages the fabricator entity.
function ProxyRoof:__init(parent_proxy, arg1)
   self[ProxyFabrication]:__init(self, parent_proxy, arg1)
   
   local cursor = self:get_region():modify()
   self:move_to(Point3(0, constants.STOREY_HEIGHT, 0))
end

function ProxyRoof:cover_region(region2)
   local data = self:add_construction_data()
   -- data.needs_scaffolding = true
   self:update_datastore()   

   -- add the region before we try to create the brush, otherwise
   -- we'll have no idea where to place things
   local data = self:add_construction_data()
   data.nine_grid_region = region2
   self:update_datastore()

   local brush = self:get_voxel_brush()
   local collsion_shape = brush:paint_once()
   self:get_region():modify():copy_region(collsion_shape)
end

return ProxyRoof
