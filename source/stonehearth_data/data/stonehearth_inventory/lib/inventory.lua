local TreeTracker = radiant.mods.require('/stonehearth_inventory/lib/tree_tracker.lua')
--local ItemTracker = radiant.mods.require('/stonehearth_inventory/lib/item_tracker.lua')
--local RestockTracker = radiant.mods.require('/stonehearth_inventory/lib/restock_tracker.lua')
--local StockpileTracker = radiant.mods.require('/stonehearth_inventory/lib/stockpile_tracker.lua')
local Inventory = class()

-- xxx: this should be called the pathfinding system... leaving the "count stocked items" and
-- stuff functionliaty here for the 'Inventory' system
function Inventory:__init(faction)
   self._faction = faction
   self._tree_tracker = TreeTracker(faction)
   self._pathfinders = {}
   
   --[[
   self._all_items_tracker = native:create_multi_path_finder('all items tracker')

   self._all_items_backtracker = native:create_multi_path_finder('all items back tracker')
   self._all_items_backtracker:set_reverse_search(true)

   self._restock_pathfinder = native:create_multi_path_finder('restock pathfinder')
   --self._restock_tracker = RestockTracker()
   self._stockpile_trackers = {}
   self._item_restock_paths = {}

   self._stocked_items = {}
   ]]

   local ec = radiant.entities.get_root_entity():get_component('entity_container');
   local children = ec:get_children()

   -- put a trace on the root entity container to detect when items 
   -- go on and off the terrain.  each item is forwarded to the
   -- appropriate tracker.
   self._trace = children:trace('tracking items on terrain')
   self._trace:on_added(function (id, entity)
         self:_add_entity_to_terrain(id, entity)
      end)
   self._trace:on_removed(function (id, entity)
         self:_remove_entity_from_terrain(id, entity)
      end)
end

function Inventory:find_path_to_entity(name, source, destination, solved_fn)
   local pathfinder = native:create_path_finder(name, source, solved_fn, nil)
   pathfinder:add_destination(destination)
   return pathfinder
end

function Inventory:find_path_to_closest_entity(name, source, solved_fn, filter_fn)
   local pathfinder = native:create_path_finder(name, source, solved_fn, filter_fn)
   self._pathfinders[pathfinder:get_id()] = pathfinder:to_weak_ref()

   -- xxx: iterate through every item in a range provided by the client
   local ec = radiant.entities.get_root_entity():get_component('entity_container');
   local children = ec:get_children()
   for id, entity in children:items() do
      pathfinder:add_destination(entity)
   end

   return pathfinder
end

function Inventory:_add_entity_to_terrain(id, entity)
   for id, pf in pairs(self._pathfinders) do
      local pathfinder = pf:lock()
      if pathfinder then
         pathfinder:add_destination(entity)
      else
         self._pathfinders[id] = nil
      end
   end
end

function Inventory:_remove_entity_from_terrain(id)
   for id, pf in pairs(self._pathfinders) do
      local pathfinder = pf:lock()
      if pathfinder then
         pathfinder:remove_destination(id)
      else
         self._pathfinders[id] = nil
      end
   end
end

-- xxx: move this out of the inventory module
function Inventory:create_stockpile(location, size)
   local entity = radiant.entities.create_entity('/stonehearth_inventory/entities/stockpile')
   
   radiant.terrain.place_entity(entity, location)
   entity:get_component('radiant:stockpile'):set_size(size)
   entity:get_component('unit_info'):set_faction(self._faction)
   -- return something?
end 

-- xxx: move this out of the inventory module
function Inventory:harvest_tree(tree)
   return self._tree_tracker:harvest_tree(tree)
end

-- xxx: move this out of the inventory module
function Inventory:find_path_to_tree(from, cb)
   return self._tree_tracker:find_path_to_tree(from, cb)
end

--[[
function Inventory:find_backpath_to_item(source_entity, solved_cb, filter_fn)
   self._all_items_backtracker:add_entity(source_entity, solved_cb, filter_fn)
end

function Inventory:find_path_to_item(source_entity, solved_cb, filter_fn)
   self._all_items_tracker:add_entity(source_entity, solved_cb, filter_fn)
end

function Inventory:find_item_to_restock(worker_entity, cb)
   local solved = function(path_to_item)
      local worker_entity = path_to_item:get_source()

      local item_entity = path_to_item:get_destination()
      self._restock_pathfinder:remove_destination(item_entity:get_id())

      local path_to_stockpile = self._item_restock_paths[item_entity:get_id()]      
      if path_to_stockpile then
         local drop_location = path_to_stockpile:get_finish_point()
         self._restock_pathfinder:remove_entity(worker_entity:get_id())
         cb(item_entity, path_to_item, path_to_stockpile, drop_location)
      end
   end
   self._restock_pathfinder:add_entity(worker_entity, solved, nil)
end

function Inventory:is_item_stocked(entity)
   return self._stocked_items[entity:get_id()] ~= nil
end

function Inventory:_register_restock_item(item_entity, path)
   local id = item_entity:get_id()
   if self._item_restock_paths[id] then
      -- hm.  two different stockpile trackers found a way to this item...
      -- perhaps it would be better if we compared the lengths of the paths
      -- and picked the shorter one.
      radiant.log.warning('ignoring duplicate restock item.')
      return
   end
   self._item_restock_paths[id] = path
   self._restock_pathfinder:add_destination(item_entity)
end

function Inventory:_create_pf(name)
   assert(not self.allpf[name])
   local pf = native:create_multi_path_finder(name)
   self.allpf[name] = pf
   return pf
end
]]

return Inventory
