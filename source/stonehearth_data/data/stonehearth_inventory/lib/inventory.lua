local Inventory = class()

function Inventory:__init(faction)
   self._faction = faction
   self._stockpiles = {}

   self.allpf = {}
   self.cb = {
      chop = {}
   }
   self.pf = {
      chop       = self:_create_pf('chop'),
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
end

Inventory['radiant.events.gameloop'] = function(self)
   self:_enable_pathfinders()
   self:_dispatch_jobs()
end

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
   for id, stockpile in pairs(self._stockpiles) do
      local designation = stockpile:get_component('stockpile_designation')
      if not designation:is_full() then
         return true
      end
   end
   return false
end


function Inventory:create_stockpile(bounds)
   local stockpile = radiant.entities.create_entity('mod://stonehearth_inventory/entities/stockpile')
   
   local origin = RadiantIPoint3(bounds.min)
   bounds.max = bounds.max - bounds.min
   bounds.min = RadiantIPoint3(0, 0, 0)   
   radiant.terrain.place_entity(stockpile, origin)
   
   local designation = stockpile:add_component('stockpile_designation') 
   designation:set_bounds(bounds)

   --designation:set_container(om:get_component(om:get_root_entity(), 'entity_container')) -- xxx do this automatically as part of becoming a child? 
   
   local unitinfo = stockpile:add_component('unit_info')
   unitinfo:set_faction(self._faction)
   
   table.insert(self._stockpiles, stockpile)
   return { entity_id = stockpile:get_id() }
end

function Inventory:harvest_tree(tree)
   assert(tree)

   radiant.log.info('harvesting resource entity %d', tree:get_id())

   local node = tree:get_component('resource_node')
   radiant.check.verify(node)

   local locations = node:get_harvest_locations()
   radiant.check.verify(locations)

   local dst = EntityDestination(tree, locations)
   self.pf.chop:add_destination(dst)
end

function Inventory:find_path_to_tree(from, cb)
   self.pf.chop:add_entity(from)
   self.cb.chop[from:get_id()] = cb
end

function Inventory:_create_pf(name)
   assert(not self.allpf[name])
   local pf = native:create_multi_path_finder(name)
   self.allpf[name] = pf
   return pf
end

return Inventory
