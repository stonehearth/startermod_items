local StockpileComponent = class()

local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3

function StockpileComponent:__init(entity, data_binding)
   self._entity = entity
   self._destination = entity:add_component('destination')
   self._data = {
      items = {},
      size  = { 0, 0 }
   }   
   self._data_binding = data_binding
   self._data_binding:update(self._data)
   
   self._destination:set_region(_radiant.sim.alloc_region())
   radiant.events.listen('radiant.events.gameloop', self)
end

-- xxx: the 'fire one when i'm constructed' pattern again...
StockpileComponent['radiant.events.gameloop'] = function(self)
   radiant.events.unlisten('radiant.events.gameloop', self)
   
   local root = radiant.entities.get_root_entity()
   local ec = radiant.entities.get_root_entity():get_component('entity_container')
   local children = ec:get_children()
   
   self._ec_trace = children:trace('tracking stockpile')
   self._ec_trace:on_added(function (id, entity)
         self:_add_item(entity)
      end)
   self._ec_trace:on_removed(function (id, entity)
         self:_remove_item(id)
      end)
   
   local mob = self._entity:add_component('mob')
   self._mob_trace = mob:trace_transform('stockpile tracking self position')
                        :on_changed(function()
                              self:_rebuild_item_data()
                           end)
      
   self:_rebuild_item_data()      
end

function StockpileComponent:extend(json)
   if json.size then
      self:set_size(json.size)
   end
end

function StockpileComponent:get_items()
   return self._data.items;
end

function StockpileComponent:_get_bounds()
   local size = self:get_size()
   local bounds = Cube3(Point3(0, 0, 0), Point3(size[1], 1, size[2]))
   return bounds
end

function StockpileComponent:_get_world_bounds()
   local size = self:get_size()
   local origin = Point3(radiant.entities.get_world_grid_location(self._entity))
   local bounds = Cube3(origin, Point3(origin.x + size[1], origin.y + 1, origin.z + size[2]))
   return bounds
end

function StockpileComponent:is_full()
   return false
end

function StockpileComponent:contains(item_entity)
   local location = Point3(radiant.entities.get_world_grid_location(item_entity))
   local world_bounds = self:_get_world_bounds()
   return world_bounds:contains(location)
end

function StockpileComponent:get_size()
   return { self._data.size[1], self._data.size[2] }
end

function StockpileComponent:set_size(size)
   self._data.size = { size[1], size[2] }  
   self:_rebuild_item_data()
end

--[[
function StockpileComponent:reserve_adjacent(location)
   local origin = Point3(radiant.entities.get_world_grid_location(self._entity))
   local pt = Point3(location) - origin
   local points = {
      Point3(pt.x + 1, pt.y, pt.z),
      Point3(pt.x - 1, pt.y, pt.z),
      Point3(pt.x, pt.y, pt.z + 1),
      Point3(pt.x, pt.y, pt.z - 1),
   }
   local region = self._destination:get_region()
   for _, p in ipairs(points) do
      if region:contains(p) then
         self:_remove_world_point_from_region(p)
         return p
      end
   end   
end
]]--

function StockpileComponent:reserve(location)
   local origin = Point3(radiant.entities.get_world_grid_location(self._entity))
   local pt = Point3(location) - origin
   local region = self._destination:get_region()
   if region:get():contains(pt) then
      region:modify():remove_point(pt)
      return true
   end   
end

function StockpileComponent:_remove_world_point_from_region(location)
   local origin = Point3(radiant.entities.get_world_grid_location(self._entity))
   local offset = location - origin
   local region = self._destination:get_region()
   region:modify():remove_point(offset)
end

function StockpileComponent:_add_item(entity)
   local location = Point3(radiant.entities.get_world_grid_location(entity))
   local world_bounds = self:_get_world_bounds()
   
   if world_bounds:contains(location) then
      local item = entity:get_component('item')
      if item then
         -- hold onto the item...
         self._data.items[entity:get_id()] = entity

         -- update our destination component to *remove* the space
         -- where the item is, since we can't drop anything there
         self:_remove_world_point_from_region(location)
         self._data_binding:mark_changed()
      end
   end
end

function StockpileComponent:_remove_item(id)
   local entity = self._data.items[id]
   if entity then
      self._data.items[id] = nil
      self._data_binding:mark_changed()

      -- add this point to our destination region
      local region = self._destination:get_region()
      local origin = radiant.entities.get_world_grid_location(entity)
      local offset = origin - radiant.entities.get_world_grid_location(self._entity)
      region:modify():add_point(Point3(offset))      
   end
end

function StockpileComponent:_rebuild_item_data()
   local region = self._destination:get_region()
   local cursor = region:modify()
   cursor:add_cube(self:_get_bounds())

   self._data.items = {}
   self._data_binding:mark_changed()
   
   local ec = radiant.entities.get_root_entity()
                  :get_component('entity_container')
                  :get_children()

   for id, child in ec:items() do
      self:_add_item(child)
   end
end


return StockpileComponent
