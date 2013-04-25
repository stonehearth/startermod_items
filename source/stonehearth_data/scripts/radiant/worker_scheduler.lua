local WorkerScheduler = class()

local md = require 'radiant.core.md'
local om = require 'radiant.core.om'
local log = require 'radiant.core.log'
local check = require 'radiant.core.check'

md:register_msg("radiant.resource_node.harvest", Harvest)

md:register_msg_handler('radiant.msg_handlers.worker_scheduler', WorkerScheduler)


function WorkerScheduler:__tostring()
   return '[WorkerScheduler]'
end

function WorkerScheduler:start()
   self._running = true
   md:listen('radiant.events.gameloop', self)
end

function WorkerScheduler:recommend_activity_for(worker)
   assert(worker)
   if self._worker_activities[worker:get_id()] then
      return 5, self._worker_activities[worker:get_id()]
   end
end

function WorkerScheduler:finish_activity(worker)
   self._worker_activities[worker:get_id()] = nil
   self:add_worker(worker)
end

function WorkerScheduler:_recommend_activity(worker, action, ...)
   check:is_entity(worker)
   check:is_string(action)
   self._worker_activities[worker:get_id()] = { 'radiant.actions.worker_scheduler_slave', self, action, ... }
   self:remove_worker(worker)
end

function WorkerScheduler:stop()
   -- xxx: this doesn't actually stop the expensive stuff: the pathfinders
   -- what we really want is a granular way to say "stop picking stuff up
   -- and harvesting, but continue puttting stuff down" or osmething (??)
   self._running = false
   md:unlisten('radiant.events.gameloop', self)
end

WorkerScheduler['radiant.md.create'] = function(self)
   --log:info('constructing worker scheduler...')
   self._workers = {}
   self._worker_activities = {}
   self._items = {}
   self._stockpiles = {}
   self._stockpiles_dests = {}
   self._build_orders = {}
   
   self.allpf = {}   
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

   md:listen('radiant.resource_node.harvest', self)
   self:start()
end

WorkerScheduler['radiant.md.destroy'] = function(self)
   md:unlisten('radiant.events.gameloop', self)
   md:unlisten('radiant.worker_scheduler', self)
   md:unlisten('radiant.resource_node.harvest', self)
end

WorkerScheduler['radiant.resource_node.harvest'] = function(self, resource_node)
   assert(resource_node)

   log:info('harvesting resource entity %d', resource_node:get_id())

   local node = om:get_component(resource_node, 'resource_node')
   check:verify(node)

   local locations = node:get_harvest_locations()
   check:verify(locations)

   local dst = EntityDestination(resource_node, locations)
   self.pf.chop:add_destination(dst)
end

function WorkerScheduler:_create_pf(name, job)
   job = job and job or name
   local pf = native:create_multi_path_finder(name)
   
   assert(not self.allpf[name])
   self.allpf[name] = {
      job = job,
      pf = pf,
   }
   return pf
end

function WorkerScheduler:add_build_order(entity)
   if entity then
      local build_order = om:get_build_order(entity)
      if build_order then
         local id = entity:get_id()
         self._build_orders[id] = build_order
      end
   end
end

function WorkerScheduler:remove_build_order(id)
   log:info('nuking a build_order');
end

function WorkerScheduler:add_item(entity)
   log:info('tracking new item')

   check:is_entity(entity)
   local id = entity:get_id()
   local mob = om:get_component(entity, 'mob')
   local item = om:get_component(entity, 'item')
   
   assert(mob and item)

   if not self._items[id] then
      local location = mob:get_grid_location()
      local stocked = self:_find_stockpile(location)
      local material = om:get_component(entity, 'item'):get_material()
      
      local points = PointList()
      points:insert(RadiantIPoint3(location.x, location.y, location.z + 1))
      points:insert(RadiantIPoint3(location.x, location.y, location.z - 1))
      points:insert(RadiantIPoint3(location.x + 1, location.y, location.z))
      points:insert(RadiantIPoint3(location.x - 1, location.y, location.z))
      
      local dst = EntityDestination(entity, points);
      self._items[id] = entity      
      if material ~= "" then
         local pf = self:_alloc_pf('pickup', material)
         pf:add_destination(dst)
         pf:set_enabled(false) -- start off disabled.  
      end
      if not stocked then
         self.pf.pickup:add_destination(dst)
      end

      om:on_removed_from_terrain(entity, function()
         self:remove_item(entity:get_id())
      end)
   end
end

function WorkerScheduler:_find_stockpile(location)
   for id, stockpile in pairs(self._stockpiles) do
      local origin = om:get_world_grid_location(stockpile)
      local designation = om:get_component(stockpile, 'stockpile_designation')
      if designation:get_bounds():contains(location - origin) then
         return stockpile
      end
   end
   return nil
end

function WorkerScheduler:remove_item(id)
   check:verify(id and id ~= 0)
   local entity = self._items[id]
   if entity then
      log:info('removing item from pickup pathfinder!')
      self.pf.pickup:remove_destination(id)
      local material = om:get_component(entity, 'item'):get_material()
      if material and material ~= '' then
         self.pf.group.pickup[material]:remove_destination(id);
      end
      self._items[id] = nil
   end
end

function WorkerScheduler:remove_stockpile(id)
   check:verify(id ~= 0)
   if self._stockpiles[id] then
      self._stockpiles[id] = nil
      self._stockpiles_dests[id] = nil
      log:info('removing stockpile from restock pathfinder!')
      self.pf.restock:remove_destination(id)
      for material, pf in pairs(self.pf.groups.restock) do
         pf:remove_destination(id)
      end
   end
end

--[[
function WorkerScheduler:track_stockpile(entity)
   local stockpile = entity:get_component('stockpile_designation')
   self.pf.restock:add_destination(RegionDestination(stockpile.get_available_region()))
end
]]

function WorkerScheduler:_alloc_pf(job, material)
   check:is_string(job)
   check:is_string(material)
   check:is_table(self.pf.group[job])
   
   local pf = self.pf.group[job][material]
   if not pf then
      pf = self:_create_pf(job .. '.' .. material, job)
      self.pf.group[job][material] = pf
  
      for id, worker in pairs(self._workers) do
         local item, material = self:_get_carrying(worker)
         if job == 'construct' and item and material then
            pf:add_entity(worker);
         elseif job == 'restock' and item and material then
            pf:add_entity(worker);
         elseif job == 'pickup' and not item then
            pf:add_entity(worker)
         elseif job == 'teardown' and material and om:worker_can_pickup_material(worker, material, 1) then
            pf:add_entity(worker)
         end
      end
      if job == 'restock' then
         for id, dst in pairs(self._stockpiles_dests) do
            pf:add_destination(dst)
         end
      end
   end

   return pf
end

function WorkerScheduler:_get_carrying(worker)
   local carrying = nil
   local material = ""
   
   local c = om:get_component(worker, 'carry_block')
   if c:is_valid() and c:is_carrying() then
      carrying = c:get_carrying()
      if om:has_component(carrying, 'item') then
         local item = om:get_component(carrying, 'item')
         material = item:get_material();
      end
   end
   return carrying, material
end

function WorkerScheduler:add_worker(worker)
   assert(worker)
   local id = worker:get_id()
   
   log:info('adding worker %d.', id)
   assert(self._worker_activities[id] == nil)

   self._workers[id] = worker

   local item, material = self:_get_carrying(worker)
   if item then
      if material then
         self:_alloc_pf('construct', material):add_entity(worker);
         self:_alloc_pf('restock',   material):add_entity(worker);
         if om:worker_can_pickup_material(worker, material, 1) then
            self:_alloc_pf('teardown', material):add_entity(worker);
         end
      else
         self.pf.restock:add_entity(worker)
      end
   else
      for material, pf in pairs(self.pf.group.pickup) do
         pf:add_entity(worker)
      end
      self.pf.pickup:add_entity(worker)
      self.pf.chop:add_entity(worker)
      for name, pf in pairs(self.pf.group.teardown) do
         pf:add_entity(worker)     
      end
   end
end

function WorkerScheduler:remove_worker(worker)
   assert(worker)

   local id = worker:get_id()
   log:info('removing worker %d.', id)
   self._workers[id] = nil

   for name, entry in pairs(self.allpf) do
      entry.pf:remove_entity(id)
   end
end

WorkerScheduler['radiant.events.gameloop'] = function(self)
   self:_check_build_orders()
   self:_enable_pathfinders()
   self:_dispatch_jobs()
end

function WorkerScheduler:_stock_space_available()
   for id, stockpile in pairs(self._stockpiles) do
      local designation = stockpile:get_stockpile_designation_component()
      if not designation:is_full() then
         return true
      end
   end
   return false
end

function WorkerScheduler:_collect_build_order_deps(build_order, checked, check_order)
   local id = build_order:get_id()
   if not checked[id] then
      local entity = build_order:get_entity()
      checked[id] = true
      if om:has_component(entity, 'build_order_dependencies') then
         local dependencies = om:get_component(entity, 'build_order_dependencies')
         for dep_entity in dependencies:get_dependencies() do
            local dep_bo = om:get_build_order(dep_entity)
            if dep_bo then
               self:_collect_build_order_deps(dep_bo, checked, check_order)
            end
         end
      end
      table.insert(check_order, build_order)
   end
end

function WorkerScheduler:_walk_build_orders()
   local checked = {}
   local check_order = {}
   for _, build_order in pairs(self._build_orders) do
      self:_collect_build_order_deps(build_order, checked, check_order)
   end
   return check_order;
end

function WorkerScheduler:_get_destination(pf, build_order)
   local entity = build_order:get_entity()
   local destination = pf:get_destination(entity:get_id())
   if not destination then
      local rgn = build_order:get_construction_region()
      destination = RegionDestination(entity, rgn)
      pf:add_destination(destination)
   end
   return destination
end

function WorkerScheduler:_check_build_orders()
   local ancestor_is_busy = {}
   
   --log:info('--- check build orders --------------------------------------------------')
   for _, build_order in ipairs(self:_walk_build_orders()) do
      local id = build_order:get_id()
      local reason = "doesn't need work"
      local entity = build_order:get_entity()
      local entity_id = entity:get_id();
      local should_queue = build_order:needs_more_work()
      
      if should_queue then
         if om:has_component(entity, 'build_order_dependencies') then
            local dependencies = om:get_component(build_order:get_entity(), 'build_order_dependencies')
            for e in dependencies:get_dependencies() do
               if ancestor_is_busy[e:get_id()] then
                  reason = string.format('dependency %s in progress', tostring(om:get_build_order(e)))
                  should_queue = false
                  ancestor_is_busy[entity_id] = true
                  break
               end
            end
         end
      end
      if ancestor_is_busy[entity_id] == nil then
         ancestor_is_busy[entity_id] = should_queue
      end
      
      local material = build_order:get_material()
      local teardown = build_order:should_tear_down()
      
      local destination = self:_get_destination(self:_alloc_pf('construct', material), build_order)
      local teardown_dst = self:_get_destination(self:_alloc_pf('teardown', material), build_order)

      if should_queue then
         destination:set_enabled(not teardown)
         teardown_dst:set_enabled(teardown);
         --log:info('enabling %s to pathfinder (teardown? %s).', tostring(build_order), teardown and "yes" or "no")
      else
         --log:info('disabling %s to pathfinder (reason: %s).', tostring(build_order), reason)
         destination:set_enabled(false)
         teardown_dst:set_enabled(false);
      end
   end
   --log:info('----------------')
end

function WorkerScheduler:_enable_pathfinders()   
   local space_available = self:_stock_space_available()
   self.pf.pickup:set_enabled(space_available)
   self.pf.restock:set_enabled(space_available)
   for material, pf in pairs(self.pf.group.construct) do
      local needs_work = pf:get_active_destination_count() > 0
      self:_alloc_pf('pickup', material):set_enabled(needs_work)
      self:_alloc_pf('restock', material):set_enabled(not needs_work and space_available)
   end
end

function WorkerScheduler:_dispatch_jobs()
   for name, entry in pairs(self.allpf) do
      local path = entry.pf:get_solution()
      if path then
         local dst = path:get_destination()
         local worker = path:get_entity()

         if self._workers[worker:get_id()] then         
            log:info('dispatching job %s to %d.', entry.job, worker:get_id())
            if worker and self:_dispatch_job(entry.job, worker, path, dst) then
               self:remove_worker(worker)
            else
               log:warning('!!!!!!!!!!!!!!! restarting pathfinder due to failed dispatch')
               --entry.pf:restart()
               --entry.pf:remove_destination(dst)
               --entry.pf:add_destination(dst)
            end
         end
      end
   end
end

function WorkerScheduler:_dispatch_job(job, worker, path, dst)
   --local job = name:gmatch('(%w+)')()
   if job == 'chop' or job == 'pickup' then
      local node = dst:get_entity()
      if node then
         self:_recommend_activity(worker, job, path, node)
         return true
      end
   elseif job == 'restock' then
      local stockpile = dst:get_entity()
      if stockpile then
         --[[ the old version...
         local origin = om:get_world_grid_location(stockpile)
         local location = path:get_points():last()
         local c = stockpile:get_stockpile_designation_component()
         local rgn = c:get_available_region()
         
         if rgn:empty() then
            -- xxx: how'd we get here?
            log:warning("stockpile region empty while dispatching restock job.  how?")
            return false
         end
         --rgn:translate(origin)
         local drop_location = rgn:get_closest_point(Point3(location - origin)):to_ipoint3()
         drop_location = drop_location + origin
         log:info('restock location:%s  drop_location:%s', tostring(location), tostring(drop_location))
         -- TODO: prevent dropping across diagonals.  Unfortuntely, the region
         -- generation code appears to be buggy and is path finding to diagonal
         -- destinations!
         check:verify(location:is_adjacent_to(drop_location))
         if location:is_adjacent_to(drop_location) then
            self:_recommend_activity(worker, 'restock', path, drop_location)
         end
         ]]
         local entity = dst:get_entity();
         local entity_id = entity:get_id();
         local location = path:get_points():last()
         local stockpile = om:get_component(entity, 'stockpile_designation')    
         local success, location = stockpile:reserve_adjacent(location)
         if success then
            self:_recommend_activity(worker, 'restock', path, stockpile, location)
         else
            log:warning('failed to reserve location adjacent to stockpile.  aborting.')
         end
         return success         
      end
      return true
   elseif job == 'construct' or job == 'teardown' then
      local entity_id = dst:get_entity():get_id();
      local build_order = self._build_orders[entity_id]
      local location = path:get_points():last()
      if not build_order then
         log:info('no build order?  wtf.')
      end
      local success, location = build_order:reserve_adjacent(location)
      if success then
         self:_recommend_activity(worker, job, path, build_order, location)
      else
         log:warning('failed to reserve location adjacent to build_order.  aborting.')
      end
      return success
   else
      assert(false)
   end
   log:warning('unknown job "%s" in worker scheduler dispatch', job);
   return false
end

function WorkerScheduler:add_stockpile(stockpile)
   log:info('tracing stockpile available region')

   if om:has_component(stockpile, 'stockpile_designation') then
      local designation = om:get_component(stockpile, 'stockpile_designation')
      local rgn = designation:get_standing_region()
      local dst = RegionDestination(stockpile, rgn);

      local id = stockpile:get_id()
      self._stockpiles[id] = stockpile
      self._stockpiles_dests[id] = dst
      
      self.pf.restock:add_destination(dst)
      for material, pf in pairs(self.pf.group.restock) do
         pf:add_destination(dst)
      end
   end
end
