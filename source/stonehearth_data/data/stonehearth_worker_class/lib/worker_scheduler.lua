local WorkerScheduler = class()

local all_build_orders = {
   'wall',
   'floor',
   'scaffolding',
   'post',
   'peaked_roof',
   'fixture'
}


--radiant.events.register_msg("radiant.resource_node.harvest", Harvest)
--radiant.events.register_msg_handler('radiant.msg_handlers.worker_scheduler', WorkerScheduler)

function WorkerScheduler:__init(faction)
   --radiant.log.info('constructing worker scheduler...')
   self._faction = faction
   self._workers = {}
   self._items = {}
   self._stockpiles = {}
   self._stockpiles_dests = {}
   self._build_orders = {}
   
   self.allpf = {}   
   self.pf = {
      pickup     = self:_create_pf('pickup'), -- generic "put stuff in stockpiles"
      group = {  -- pf's by material (e.g. pickup.wood, construct.wood, etc.)
         pickup = {},
         restock = {},
         construct = {},
         teardown = {},
      }, 
   }
   self._running = true
   radiant.events.listen('radiant.events.gameloop', self)

   self._filter_pickup_items = function(entity)
      return true
   end

   self._dispatch_pickup_task = function(path)
      local worker = path:get_source()
      local item = path:get_destination()
      self:_remove_item(item:get_id())
      self:_recommend_activity(worker, 'stonehearth_worker.actions.pickup', path, item)
   end
   
   self._dispatch_restock_task = function(path)
      local worker = path:get_source()
      local stockpile = path:get_destination()
      local drop_location = path:get_finish_point()
      local s = stockpile:get_component('radiant:stockpile')
      if s:reserve(drop_location) then
         self:_recommend_activity(worker, 'stonehearth_worker.actions.restock', path, stockpile, drop_location)
      else
         radiant.log.warning('failed to reserve location adjacent to stockpile.  aborting.')
      end
   end

   self:_install_traces()
end

function WorkerScheduler:_install_traces()
   local ec = radiant.entities.get_root_entity():get_component('entity_container');
   local children = ec:get_children()

   -- put a trace on the root entity container to detect when items 
   -- go on and off the terrain.  each item is forwarded to the
   -- appropriate tracker.
   self._trace = children:trace('tracking items on terrain')
   self._trace:on_added(function (id, entity)
         self:_on_add_entity(id, entity)
      end)
   self._trace:on_removed(function (id)
         self:_on_remove_entity(id)
      end)
      
   -- UGGGG. ITERATE THROUGH EVERY ITEM IN THE WORLD. =..(
   for id, item in children:items() do
      self:_on_add_entity(id, item)
   end
end


function WorkerScheduler:_on_add_entity(id, entity)
   if entity then
      if entity:get_component('item') then
         self:_add_item(entity)
      end
      if entity:get_component('radiant:stockpile') then
         local faction = entity:get_component('unit_info'):get_faction()
         if faction == self._faction then
            self:_add_stockpile(entity)
         end
      end
   end
end

function WorkerScheduler:_on_remove_entity(id)
   self:_remove_item(id)
   --self._worker_scheduler:remove_stockpile(id)
end

function WorkerScheduler:destroy()
   radiant.events.unlisten('radiant.events.gameloop', self)
end


function WorkerScheduler:_recommend_activity(worker, action, ...)
   radiant.check.is_entity(worker)
   radiant.check.is_string(action)

   local entry = self._workers[worker:get_id()]
   if entry then
      radiant.log.warning('dispatching task %s', action)
      entry.dispatch_fn(10, action, ...)
   end
   self:remove_worker(worker)
end

function WorkerScheduler:stop()
   -- xxx: this doesn't actually stop the expensive stuff: the pathfinders
   -- what we really want is a granular way to say "stop picking stuff up
   -- and harvesting, but continue puttting stuff down" or osmething (??)
   self._running = false
   radiant.events.unlisten('radiant.events.gameloop', self)
end

function WorkerScheduler:abort_worker_task(task)
   radiant.log.warning('aborting task in worker scheduler...')
end

function WorkerScheduler:_create_pf(name)
   assert(not self.allpf[name])
  
   local pf = native:create_multi_path_finder(name)
   self.allpf[name] = pf
   return pf
end

-- ug!  this is decidely un-extensible.  fix it!
function WorkerScheduler:get_build_order(entity)
   for _, name in ipairs(all_build_orders) do
      local component = entity:get_component(name)
      if component then
         return component
      end
   end
end

function WorkerScheduler:add_build_order(entity)
   if entity then
      local build_order = self._get_build_order(entity)
      if build_order then
         local id = entity:get_id()
         self._build_orders[id] = build_order
      end
   end
end

function WorkerScheduler:remove_build_order(id)
   radiant.log.info('nuking a build_order');
end

function WorkerScheduler:_add_item(entity)
   radiant.check.is_entity(entity)
   radiant.log.info('tracking new item')
   
   local id = entity:get_id()
   if not self._items[id] then
      self._items[id] = entity      

      local material = entity:get_component('item'):get_material()
      if material ~= "" then
         local pf = self:_alloc_pf('pickup', material)
         pf:add_destination(entity)
         pf:set_enabled(false) -- start off disabled.  
      end
      
      local stocked = self:_find_stockpile(entity)
      if not stocked then
         self.pf.pickup:add_destination(entity)
      end
   end
end

function WorkerScheduler:_find_stockpile(item_entity)
   for id, entity in pairs(self._stockpiles) do
      local stockpile = entity:get_component('radiant:stockpile')
      if stockpile:contains(item_entity) then
         return stockpile
      end
   end
   return nil
end

function WorkerScheduler:_remove_item(id)
   radiant.check.is_number(id)
   
   local entity = self._items[id]
   if entity then
      radiant.log.info('removing item from pickup pathfinder!')
      self.pf.pickup:remove_destination(id)
      local material = entity:get_component('item'):get_material()
      if material and material ~= '' then
         self.pf.group.pickup[material]:remove_destination(id);
      end
      self._items[id] = nil
   end
end

function WorkerScheduler:remove_stockpile(id)
   radiant.check.verify(id ~= 0)
   if self._stockpiles[id] then
      self._stockpiles[id] = nil
      self._stockpiles_dests[id] = nil
      radiant.log.info('removing stockpile from restock pathfinder!')
      for material, pf in pairs(self.pf.groups.restock) do
         pf:remove_destination(id)
      end
   end
end


function WorkerScheduler:_can_grab_more_of(worker, material, count)
   radiant.check.is_entity(worker)
   radiant.check.is_string(material)
   radiant.check.is_number(count)
   
   local carry_block = worker:get_component('carry_block') 
   if not carry_block then
      return false
   end
   
   if not carry_block:is_carrying() then
      -- Not carrying anything.  Good to go!
      return true
   end
   
   local item = carry_block:get_carrying():get_component('item')
   if not item then
      -- Not carrying an item.  Couldn't possibly be a resource, then.
      return false -- this shouldn't be possible, right?
   end
   
   if material ~= item:get_material() then
      -- Wrong item...
      return false
   end
   
   local stacks = item:get_stacks()
   if stacks + count >= item:get_max_stacks() then
      -- Item stacks are full.  Can't grab anymore!
      return false
   end
   return true
end

function WorkerScheduler:_find_job_for_worker(worker)
   local item, material = self:_get_carrying(worker)   

   if item then   
      self:_add_worker_to_pf('construct', material, worker)
      self:_add_worker_to_pf('restock', material, worker)
      self:_add_worker_to_pf('teardown', material, worker)
      self:_add_worker_to_pf('pickup', material, worker)
   else
      self.pf.pickup:add_entity(worker, self._dispatch_pickup_task, self._filter_pickup_item)
      for material, pf in pairs(self.pf.group.pickup) do
         pf:add_entity(worker, self._dispatch_pickup_task, self._filter_pickup_item)
      end
      for name, pf in pairs(self.pf.group.teardown) do
         pf:add_entity(worker, self._dispatch_teardown_task, nil)
      end
   end
end

function WorkerScheduler:_add_worker_to_pf(job, material, worker)
   --local pf = self.pf.group[job][material]
   local pf = self:_alloc_pf(job, material)
   if pf then
      local item, material = self:_get_carrying(worker)
      if job == 'construct' and item and material then
         pf:add_entity(worker, self._dispatch_construct_task, nil);
      elseif job == 'restock' and item and material then
         pf:add_entity(worker, self._dispatch_restock_task, nil);
      elseif job == 'pickup' and not item then
         pf:add_entity(worker, self._dispatch_pickup_task, nil)
      elseif job == 'teardown' and material and self:_can_grab_more_of(worker, material, 1) then
         pf:add_entity(worker, self._dispatch_teardown_task, nil)
      end
   end
end

function WorkerScheduler:_alloc_pf(job, material)
   radiant.check.is_string(job)
   radiant.check.is_string(material)
   radiant.check.is_table(self.pf.group[job])
   
   local pf = self.pf.group[job][material]
   if not pf then
      pf = self:_create_pf(job .. '.' .. material)
      self.pf.group[job][material] = pf
  
      for id, entry in pairs(self._workers) do
         self:_add_worker_to_pf(job, material, entry.worker)
      end      
      if job == 'restock' then
         for id, entity in pairs(self._stockpiles) do            
            pf:add_destination(entity)
         end
      end
   end

   return pf
end

function WorkerScheduler:_get_carrying(worker)
   local carrying = nil
   local material = ""
   
   local c = worker:get_component('carry_block')
   if c and c:is_carrying() then
      carrying = c:get_carrying()
      local item = carrying:get_component('item')
      if item then
         material = item:get_material();
      end
   end
   return carrying, material
end

function WorkerScheduler:add_worker(worker, dispatch_fn)
   assert(worker)
   local id = worker:get_id()
   
   radiant.log.info('adding worker %d.', id)
   assert(self._workers[id] == nil)

   self._workers[id] = {
      worker = worker,
      dispatch_fn = dispatch_fn
   }
   
   self:_find_job_for_worker(worker)
end

function WorkerScheduler:remove_worker(worker)
   assert(worker)

   local id = worker:get_id()
   radiant.log.info('removing worker %d.', id)
   self._workers[id] = nil

   for name, pf in pairs(self.allpf) do
      pf:remove_entity(id)
   end
end

WorkerScheduler['radiant.events.gameloop'] = function(self)
   --self:_check_build_orders()
   self:_enable_pathfinders()
   --self:_dispatch_jobs()
end

function WorkerScheduler:_stock_space_available()
   for id, entity in pairs(self._stockpiles) do
      local stockpile = entity:get_component('radiant:stockpile')
      if not stockpile:is_full() then
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
      if radiant.components.has_component(entity, 'build_order_dependencies') then
         local dependencies = entity:get_component('build_order_dependencies')
         for dep_entity in dependencies:get_dependencies() do
            local dep_bo = self._get_build_order(dep_entity)
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
   
   --radiant.log.info('--- check build orders --------------------------------------------------')
   for _, build_order in ipairs(self:_walk_build_orders()) do
      local id = build_order:get_id()
      local reason = "doesn't need work"
      local entity = build_order:get_entity()
      local entity_id = entity:get_id();
      local should_queue = build_order:needs_more_work()
      
      if should_queue then
         if radiant.components.has_component(entity, 'build_order_dependencies') then
            local dependencies = radiant.components.get_component(build_order:get_entity(), 'build_order_dependencies')
            for e in dependencies:get_dependencies() do
               if ancestor_is_busy[e:get_id()] then
                  reason = string.format('dependency %s in progress', tostring(self._get_build_order(e)))
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
         --radiant.log.info('enabling %s to pathfinder (teardown? %s).', tostring(build_order), teardown and "yes" or "no")
      else
         --radiant.log.info('disabling %s to pathfinder (reason: %s).', tostring(build_order), reason)
         destination:set_enabled(false)
         teardown_dst:set_enabled(false);
      end
   end
   --radiant.log.info('----------------')
end

function WorkerScheduler:_enable_pathfinders()   
   local space_available = self:_stock_space_available()
   -- why do we have a generic pickup finder for restocking!?
   self.pf.pickup:set_enabled(space_available)
   --self.pf.pickup:set_enabled(false)
   for material, pf in pairs(self.pf.group.construct) do
      local needs_work = not pf:is_idle()
      self:_alloc_pf('pickup', material):set_enabled(needs_work)
      self:_alloc_pf('restock', material):set_enabled(not needs_work and space_available)
   end
end

--[[
function WorkerScheduler:_dispatch_jobs()
   for name, pf in pairs(self.allpf) do
      local path = pf:get_solution()
      if path then
         local dst = path:get_destination()
         local worker = path:get_entity()

         if self._workers[worker:get_id()] then
            radiant.log.info('dispatching job %s to %d.', entry.job, worker:get_id())
            if worker and self:_dispatch_job(entry.job, worker, path, dst) then
               self:remove_worker(worker)
            else
               radiant.log.warning('!!!!!!!!!!!!!!! restarting pathfinder due to failed dispatch')
               --entry.pf:restart()
               --entry.pf:remove_destination(dst)
               --entry.pf:add_destination(dst)
            end
         end
      end
   end
end
]]

function WorkerScheduler:_dispatch_job(job, worker, path, dst)
   --local job = name:gmatch('(%w+)')()
   if job == 'pickup' then
      local node = dst:get_entity()
      if node then
         self:_recommend_activity(worker, job, path, node)
         return true
      end
   elseif job == 'restock' then
      local stockpile = dst:get_entity()
      if stockpile then
         local entity = dst:get_entity();
         local entity_id = entity:get_id();
         local location = path:get_points():last()
         local stockpile = entity:get_component('stockpile_designation')    
         local success, location = stockpile:reserve_adjacent(location)
         if success then
            self:_recommend_activity(worker, 'restock', path, stockpile, location)
         else
            radiant.log.warning('failed to reserve location adjacent to stockpile.  aborting.')
         end
         return success         
      end
      return true
   elseif job == 'construct' or job == 'teardown' then
      local entity_id = dst:get_entity():get_id();
      local build_order = self._build_orders[entity_id]
      local location = path:get_points():last()
      if not build_order then
         radiant.log.info('no build order?  wtf.')
      end
      local success, location = build_order:reserve_adjacent(location)
      if success then
         self:_recommend_activity(worker, job, path, build_order, location)
      else
         radiant.log.warning('failed to reserve location adjacent to build_order.  aborting.')
      end
      return success
   else
      assert(false)
   end
   radiant.log.warning('unknown job "%s" in worker scheduler dispatch', job);
   return false
end

function WorkerScheduler:_add_stockpile(entity)
   radiant.log.info('tracing stockpile available region')
   assert(entity:get_component('radiant:stockpile') ~= nil)
   
   local id = entity:get_id()
   self._stockpiles[id] = entity
   
   for material, pf in pairs(self.pf.group.restock) do
      pf:add_destination(entity)
   end
   -- xxx!
   -- run through the list of items and stick them in stockpiles!
end

return WorkerScheduler
