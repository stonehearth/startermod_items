local TreeTracker = radiant.mods.require('mod://stonehearth_inventory/lib/tree_tracker.lua')
local StockpileTracker = radiant.mods.require('mod://stonehearth_inventory/lib/stockpile_tracker.lua')
local Inventory = class()

function Inventory:__init(faction)
   self._tree_tracker = TreeTracker(faction)
   self._faction = faction
   self._stockpiles = {}
   self._items = {}

   self.allpf = {}
   self.pf = {
      pickup     = self:_create_pf('pickup'), -- generic "put stuff in stockpiles"
      restock    = self:_create_pf('restock'), -- tearing down existing structures
      group = {  -- pf's by material (e.g. pickup.wood, construct.wood, etc.)
         pickup = {},
         restock = {},
         construct = {},
         teardown = {},
      }, 
   }
   radiant.events.listen('radiant.events.gameloop', self)

   local ec = radiant.entities.get_root_entity():get_component('entity_container')

   self._trace = ec:trace_children('tracking items on terrain')
   self._trace:on_added(function (id, entity)
         self:_add_entity_to_terrain(id, entity)
      end)
   self._trace:on_removed(function (id, entity)
      self:_remove_entity_from_terrain(id, entity)
      end)
end

Inventory['radiant.events.gameloop'] = function(self)
   --self:_enable_pathfinders()
   self:_dispatch_jobs()
end

function Inventory:_add_entity_to_terrain(id, entity)
   if entity then
      local stockpile = entity:get_component('stockpile')
      if stockpile then
         table.insert(self._stockpiles, entity)
         return { entity_id = entity:get_id() }   
      end
      local item = entity:get_component('item')
      if item then
         radiant.log.info('got a new item!!')
      end
   end
end

function Inventory:_remove_entity_from_terrain(id, entity)
   -- hmmm....
end

--[[
function Inventory:_enable_pathfinders()   
   local space_available = self:_stock_space_available()
   self.pf.pickup:set_enabled(space_available)
   self.pf.restock:set_enabled(space_available)
   for material, pf in pairs(self.pf.group.construct) do
      local needs_work = pf:get_active_destination_count() > 0
      self:_alloc_pf('pickup', material):set_enabled(needs_work)
      self:_alloc_pf('restock', material):set_enabled(not needs_work and space_available)
   end
end
]]

function Inventory:_dispatch_jobs()
   for name, pf in pairs(self.allpf) do
      local path = pf:get_solution()
      if path then
         local entity_id = path:get_entity():get_id()
         local cb = self.cb[name][entity_id]

         if cb then
            pf:remove_entity(entity_id)
            cb(path)
         end
      end
   end
end

function Inventory:_stock_space_available()
   for id, entity in pairs(self._stockpiles) do
      local stockpile = entity:get_component('stockpile')
      if not stockpile:is_full() then
         return true
      end
   end
   return false
end


function Inventory:create_stockpile(location, size)
   local entity = radiant.entities.create_entity('mod://stonehearth_inventory/entities/stockpile')
   
   radiant.terrain.place_entity(entity, location)
   if size then
      entity:get_component('stockpile'):set_size(size)
   end
   entity:get_component('unit_info'):set_faction(self._faction)   
end

function Inventory:harvest_tree(tree)
   return self._tree_tracker:harvest_tree(tree)
end

function Inventory:find_path_to_tree(from, cb)
   return self._tree_tracker:find_path_to_tree(from, cb)
end

function Inventory:_create_pf(name)
   assert(not self.allpf[name])
   local pf = native:create_multi_path_finder(name)
   self.allpf[name] = pf
   return pf
end

return Inventory
