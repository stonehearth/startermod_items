local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

local Inventory = class()

--Keeps track of the stuff in a player's stockpiles
--TODO: slowly move away from events towards direct calls

function Inventory:__init()
end

function Inventory:initialize(session)   
   self._data = {
      next_stockpile_no = 1,
      player_id = session.player_id,
      kingdom = session.kingdom,
      faction = session.faction,
      storage = {},
      items = {},
   }
   self.__saved_variables = radiant.create_datastore(self._data)
end

function Inventory:restore(saved_variables)
   self.__saved_variables = saved_variables
   self._data = saved_variables:get_data()
end

function Inventory:create_stockpile(location, size)
   local entity = radiant.entities.create_entity('stonehearth:stockpile')   
   radiant.terrain.place_entity(entity, location)
   entity:get_component('stonehearth:stockpile'):set_size(size.x, size.y)

   self:_add_collision_region(entity, size)

   --xxx localize
   entity:get_component('unit_info'):set_display_name('Stockpile No.' .. self._data.next_stockpile_no)
   entity:get_component('unit_info'):set_player_id(self._data.player_id)
   entity:get_component('unit_info'):set_faction(self._data.faction)

   self._data.next_stockpile_no = self._data.next_stockpile_no + 1
   self.__saved_variables:mark_changed()
   return entity
end

function Inventory:get_all_storage()
   return self._data.storage
end

function Inventory:add_storage(storage_entity)
   assert(not self._data.storage[storage_entity:get_id()])
   self._data.storage[storage_entity:get_id()] = storage_entity
   self.__saved_variables:mark_changed()
   radiant.events.trigger(self, 'stonehearth:storage_added', { storage = storage_entity })
   --radiant.events.listen(storage_entity, "stonehearth:item_added",   self, self._on_item_added)
   --radiant.events.listen(storage_entity, "stonehearth:item_removed", self, self._on_item_removed)
end

function Inventory:remove_storage(storage_entity)
   local id = storage_entity:get_id()
   if self._data.storage[id] then
      self._data.storage[id] = nil
      self.__saved_variables:mark_changed()
   end
   radiant.events.trigger(self, 'stonehearth:storage_removed', { storage = storage_entity })
   --radiant.events.unlisten(storage_entity, "stonehearth:item_added",   self, self._on_item_added)
   --radiant.events.unlisten(storage_entity, "stonehearth:item_removed", self, self._on_item_removed)
   --xxx remove the items?
end

function Inventory:_add_collision_region(entity, size)
   local collision_component = entity:add_component('region_collision_shape')
   local collision_region_boxed = _radiant.sim.alloc_region()

   collision_region_boxed:modify(
      function (region3)
         region3:add_unique_cube(
            Cube3(
               -- recall that region_collision_shape is in local coordiantes
               Point3(0, 0, 0),
               Point3(size.x, 1, size.y)
            )
         )
      end
   )

   collision_component:set_region(collision_region_boxed)
   collision_component:set_region_collision_type(_radiant.om.RegionCollisionShape.NONE)
end

--TODO: Phase the event-based system here out
--[[
function Inventory:_on_item_added(e)
   local storage_id = e.storage:get_id()
   local item = e.item
   self:add_item(storage_id, item)
end
]]

--- Call whenever a stockpile wants to tell the inventory that we're adding an item
function Inventory:add_item(storage, item)
   local storage_id = storage:get_id()
   assert(self._data.storage[storage_id], 'tried to add an item to an untracked storage entity ' .. tostring(storage))

   self._data.items[item:get_id()] = item
   self.__saved_variables:mark_changed()
   radiant.events.trigger(self, 'stonehearth:item_added', { item = item })
end

--[[
--TODO: phase out the event-based system
function Inventory:_on_item_removed(e)
   local storage_id = e.storage:get_id()
   local item = e.item
   self:remove_item(storage_id, item)
end
]]

--- Call whenever a stockpile wants to tell the inventory that we're removing an item
function Inventory:remove_item(storage, item, item_id)
   local storage_id = storage:get_id()
   assert(self._data.storage[storage_id], 'tried to remove an item to an untracked storage entity ' .. tostring(storage))

   if self._data.items[item_id] then
      self._data.items[item_id] = nil
      self.__saved_variables:mark_changed()
      radiant.events.trigger(self, 'stonehearth:item_removed', { item = item, item_id = item_id })
   end
end

return Inventory
