local ProxyFabrication = require 'services.build.proxy_fabrication'
local ProxyColumn = class(ProxyFabrication)
local constants = require('constants').construction

local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

-- this is the component which manages the fabricator entity.
function ProxyColumn:__init(parent_proxy, arg1)
   self[ProxyFabrication]:__init(self, parent_proxy, arg1)
   self:get_region():modify(function(cursor)
      cursor:copy_region(Region3(Cube3(Point3(0, 0, 0), Point3(1, constants.STOREY_HEIGHT, 1))))
   end)
   
   local data = self:add_construction_data()
   data:modify_data(function(o)
         o.connected_to = {}
      end)
end

function ProxyColumn:connect_to_wall(wall)
   local data = self:add_construction_data()
   data:modify_data(function(o)
         table.insert(o.connected_to, wall:get_entity())   
      end)
end

function ProxyColumn:remove_wall_connection(wall)
   local data = self:add_construction_data()
   local id = wall:get_entity():get_id()

   local remove_index
   for i, wall_entity in ipairs(data:get_data().connected_to) do
      if wall_entity:get_id() == id then
         remove_index = i
         break
      end
   end
   
   if remove_index then
      data:modify_data(function(o)
            table.remove(o.connected_to, remove_index)
         end)
   end
end

return ProxyColumn
