local CompoundTask = require 'services.tasks.compound_task'
local Town = class()

function Town:__init(name)
   self._log = radiant.log.create_logger('town', name)

   stonehearth.tasks:get_scheduler('stonehearth:workers', faction)
                     :set_activity('stonehearth:work')
end

function Town:destroy()
end

-- xxx: this is a stopgap until we can provide a better interface
function Town:create_worker_task(name, args)
   return stonehearth.tasks.create_task('stonehearth:workers', name, args)
end

function Town:create_orchestrator(name, args, co)
   assert(type(name) == 'string')
   assert(args == nil or type(args) == 'table')

   local activity = {
      name = name,
      args = args or {}
   }
   
   local compound_task_ctor = radiant.mods.load_script(name)
   local ct = CompoundTask(self, compound_task_ctor, activity, co)
   self._compound_tasks[ct] = true
   return ct
end

function Town:join(entity, scheduler)
   --assert(self._activity, 'must call set_activity before adding entities')
   
   self._log:debug('adding %s to task scheduler', entity)
   local id = entity:get_id()
   if not self._entities[id] then      
      local entry = {
         entity = entity,
         tasks = {},
      }
      for id, task in pairs(self._tasks) do
         self:_add_action_to_entity(task, entry)
      end
      self._entities[id] = entry
   end
   return self
end

function Town:leave(entity, scheduler)
   if entity and entity:is_valid() then
      self._log:debug('removing %s from task scheduler', entity)
      local id = entity:get_id()
      local entry = self._entities[id]
      if entry then
         for task, action in ipairs(entry.tasks) do
            self:_remove_action_from_entity(task, entry)
         end
         self._entities[id] = nil
      end
   end
   return self
end


function Town:place_item_in_world(item_proxy, full_sized_uri, location, rotation)
   local ghost_entity = radiant.entities.create_entity()
   local ghost_entity_component = ghost_entity:add_component('stonehearth:ghost_item')
   ghost_entity_component:set_full_sized_mod_uri(full_sized_uri)
   radiant.terrain.place_entity(ghost_entity, location)
   radiant.entities.turn_to(ghost_entity, rotation)

   local remove_ghost_entity = function(placed_item)
      radiant.entities.destroy_entity(ghost_entity)
   end

   local scheduler = stonehearth.tasks:get_scheduler('stonehearth:workers', session.faction)
   local task = scheduler:create_task('stonehearth:place_item', {
         item = item,
         location = location,
         rotation = rotation,
         finish_fn = remove_ghost_entity
      })
      :once()
      :start()

   return task
end

function Town:place_item_type_in_world(entity_uri, full_item_uri, location, rotation)
   local ghost_entity = radiant.entities.create_entity()
   local ghost_entity_component = ghost_entity:add_component('stonehearth:ghost_item')
   ghost_entity_component:set_full_sized_mod_uri(full_item_uri)
   radiant.terrain.place_entity(ghost_entity, location)
   radiant.entities.turn_to(ghost_entity, rotation)

   local remove_ghost_entity = function(placed_item)
      radiant.entities.destroy_entity(ghost_entity)
   end

   local filter_fn = function(item)
      return item:get_uri() == entity_uri
   end

   local scheduler = stonehearth.tasks:get_scheduler('stonehearth:workers', session.faction)
   local task = scheduler:create_task('stonehearth:place_item_type', {
         filter_fn = filter_fn,
         location = location,
         rotation = rotation,
         finish_fn = remove_ghost_entity
      })
      :once()
      :start()

   return task
end

function Town:promote_citizen(person, talisman)
   local scheduler = stonehearth.tasks:create_scheduler()
                        :set_activity('stonehearth:top')
                        :join(person)

   local task = scheduler:create_orchestrator('stonehearth:tasks:promote_with_talisman', {
         person = person,
         talisman = talisman,
      })
      :start()
      
   radiant.events.listen(task, 'completed', function()
          stonehearth.tasks:destroy_scheduler(scheduler)
         return radiant.events.UNLISTEN
      end)

   return true
end

local all_harvest_tasks = {}

function Town:harvest_resource_node(tree)
   if not tree or not tree:is_valid() then
      return false
   end

   local id = tree:get_id()
   if not all_harvest_tasks[id] then
      local node_component = tree:get_component('stonehearth:resource_node')
      if node_component then
         local effect_name = node_component:get_harvest_overlay_effect()
         all_harvest_tasks[id] = stonehearth.tasks:get_scheduler('stonehearth:workers', session.faction)
                                   :create_task('stonehearth:chop_tree', { tree = tree })
                                   :add_entity_effect(tree, effect_name)
                                   :once()
                                   :start()
      end
   end
   return true
end

function Town:harvest_renewable_resource_node(plant)
   if not plant or not plant:is_valid() then
      return false
   end

   local id = plant:get_id()
   if not all_harvest_tasks[id] then
      local node_component = plant:get_component('stonehearth:renewable_resource_node')
      if node_component then
         local effect_name = node_component:get_harvest_overlay_effect()
         all_harvest_tasks[id] = stonehearth.tasks:get_scheduler('stonehearth:workers', session.faction)
                                   :create_task('stonehearth:harvest_plant', { plant = plant })
                                   :add_entity_effect(plant, effect_name)
                                   :once()
                                   :start()

         radiant.events.listen(plant, 'stonehearth:is_harvestable', function(e) 
               local plant = e.entity
               if not plant or not plant:is_valid() then
                  return radiant.events.UNLISTEN
               end
               all_harvest_tasks[plant:get_id()] = nil
            end)
      end
   end
   return true
end

function Town:construct_workshop(ghost_workshop)
   -- todo: stick this in a taskmaster manager somewhere so we can show it (and cancel it!)
   local scheduler = stonehearth.tasks:get_scheduler('stonehearth:workers', session.faction)
   scheduler:create_orchestrator('stonehearth:tasks:create_workshop', {
         faction = session.faction,
         ghost_workshop = ghost_workshop,
         outbox_entity = outbox_entity,
      })
      :start()

   return true
end

function Town:add_workshop_task_group(workshop, crafter)
   self._scheduler = stonehearth.tasks:create_scheduler()
                        :set_activity('stonehearth:top')
                        :join(crafter)
                        
   self._scheduler:create_orchestrator('stonehearth:tasks:work_at_workshop', {
         workshop = workshop:get_entity(),
         craft_order_list = workshop:get_craft_order_list(),
         faction = radiant.entities.get_faction(crafter)
      })
      :start()
end

return Town

