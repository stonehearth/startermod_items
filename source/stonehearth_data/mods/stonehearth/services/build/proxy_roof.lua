local ProxyFabrication = require 'services.build.proxy_fabrication'
local ProxyRoof = class(ProxyFabrication)
local constants = require('constants').construction

local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

-- this is the component which manages the fabricator entity.
function ProxyRoof:__init(parent_proxy, arg1)
   self[ProxyFabrication]:__init(self, parent_proxy, arg1)
   
   self:get_region():modify(function(cursor)
      self:move_to(Point3(0, constants.STOREY_HEIGHT, 0))
   end)
end

function ProxyRoof:cover_region(region2)
   local data = self:add_construction_data()

   -- add the region before we try to create the brush, otherwise
   -- we'll have no idea where to place things
   data:modify_data(function (o)
         -- o.needs_scaffolding = true
         o.nine_grid_region = region2
      end)

   local brush = self:get_voxel_brush()
   local collsion_shape = brush:paint_once()
   self:get_region():modify(function(cursor)
      cursor:copy_region(collsion_shape)
   end)
end

return ProxyRoof
