local TreeTracker = radiant.mods.require('/stonehearth_inventory/lib/tree_tracker.lua')
--local ItemTracker = radiant.mods.require('/stonehearth_inventory/lib/item_tracker.lua')
local RestockTracker = radiant.mods.require('/stonehearth_inventory/lib/restock_tracker.lua')
local StockpileTracker = radiant.mods.require('/stonehearth_inventory/lib/stockpile_tracker.lua')
local Inventory = class()

function Inventory:__init(faction)
   self._faction = faction

   self._tree_tracker = TreeTracker(faction)
   self._all_items_tracker = native:create_multi_path_finder('all items tracker')

   self._all_items_backtracker = native:create_multi_path_finder('all items back tracker')
   self._all_items_backtracker:set_reverse_search(true)

   self._restock_pathfinder = native:create_multi_path_finder('restock pathfinder')
   --self._restock_tracker = RestockTracker()
   self._stockpile_trackers = {}
   self._item_restock_paths = {}

   local ec = radiant.entities.get_root_entity():get_component('entity_container'):get_children()

   -- put a trace on the root entity container to detect when items 
   -- go on and off the terrain.  each item is forwarded to the
   -- appropriate tracker.
   self._trace = ec:trace('tracking items on terrain')
   self._trace:on_added(function (id, entity)
         self:_add_entity_to_terrain(id, entity)
      end)
   self._trace:on_removed(function (id, entity)
         self:_remove_entity_from_terrain(id, entity)
      end)
   -- UGGGG. ITERATE THROUGH EVERY ITEM IN THE WORLD. =..(
   for id, item in ec:items() do
      self:_add_entity_to_terrain(id, item)
   end
end


function Inventory:_add_entity_to_terrain(id, entity)
   if entity then
      local stockpile = entity:get_component('radiant:stockpile')
      if stockpile then
         self:_track_stockpile(entity, stockpile)
      end

      local item = entity:get_component('item')
      if item then
         self:_track_item(entity, item)
      end
   end
end

function Inventory:_track_item(item_entity)
   -- unconditionally add the item to the all item tracker
   self._all_items_tracker:add_destination(item_entity)
   self._all_items_backtracker:add_destination(item_entity)
end

function Inventory:_track_stockpile(stockpile_entity)
   -- simply create a new stockpile tracker.
   local id = stockpile_entity:get_id()
   self._stockpile_trackers[id] = StockpileTracker(self, stockpile_entity)
end

function Inventory:_remove_entity_from_terrain(id, entity)
   for id, tracker in pairs(self._stockpile_trackers) do
      tracker:untrack_item(id)
   end
   self._all_items_tracker:remove_destination(id)
   -- hmmm....
end

function Inventory:create_stockpile(location, size)
   local entity = radiant.entities.create_entity('/stonehearth_inventory/entities/stockpile')
   
   radiant.terrain.place_entity(entity, location)
   entity:get_component('radiant:stockpile'):set_size(size)
   entity:get_component('unit_info'):set_faction(self._faction)
   -- return something?
end 

function Inventory:harvest_tree(tree)
   return self._tree_tracker:harvest_tree(tree)
end

function Inventory:find_path_to_tree(from, cb)
   return self._tree_tracker:find_path_to_tree(from, cb)
end

function Inventory:find_backpath_to_item(source_entity, cb)
   local solved = function (path)
      cb(path)
   end
   self._all_items_backtracker:add_entity(source_entity, solved, nil)
end

function Inventory:find_path_to_item(source_entity, cb, filter)
   local solved = function (path)
      cb(path)
   end
   self._all_items_tracker:add_entity(source_entity, solved, filter)
end

function Inventory:find_item_to_restock(worker_entity, cb)
   local solved = function(path_to_item)
      local worker_entity = path_to_item:get_source()
      local item_entity = path_to_item:get_destination()
      local path_to_stockpile = self._item_restock_paths[item_entity:get_id()]
      if path_to_stockpile then
         self._restock_pathfinder:remove_entity(worker_entity:get_id())
         cb(item_entity, path_to_item, path_to_stockpile)
      else
         -- this shouldn't happen...
         self._restock_pathfinder:remove_destination(item_entity)
      end
   end
   self._restock_pathfinder:add_entity(worker_entity, solved, nil)
end

function Inventory:_register_restock_item(item_entity, path)
   local id = item_entity:get_id()
   self._item_restock_paths[id] = path
   self._restock_pathfinder:add_destination(item_entity)
end

function Inventory:_create_pf(name)
   assert(not self.allpf[name])
   local pf = native:create_multi_path_finder(name)
   self.allpf[name] = pf
   return pf
end

return Inventory
