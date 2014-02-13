local Entity = _radiant.om.Entity
local CompoundTask = require 'services.tasks.compound_task'
local UnitController = require 'services.town.unit_controller'
local Town = class()

function Town:__init(name)
   self._log = radiant.log.create_logger('town', name)
   self._scheduler = stonehearth.tasks:create_scheduler(name)
                                       :set_counter_name(name)

   self._task_groups = {
      workers = self._scheduler:create_task_group('stonehearth:work', {})
                               :set_counter_name'workers')
   }
   self._unit_controllers = {}
   self._harvest_tasks = {}
end

function Town:destroy()
end

-- xxx: this is a stopgap until we can provide a better interface
function Town:create_worker_task(activity_name, args)
   return self._task_groups.workers:create_task(activity_name, args)
end

function Town:join_task_group(entity, name)
   local task_group = self._task_groups[name]
   assert(task_group, string.format('unknown task group "%s"', name))
   task_group:add_worker(entity)
   return self
end

function Town:leave_task_group(entity, name)
   local task_group = self._task_groups[name]
   assert(task_group, string.format('unknown task group "%s"', name))
   task_group:remove_worker(entity)
   return self
end

function Town:get_unit_controller(entity, activity_name, activity_args)
   local id = entity:get_id()
   local unit_controller = self._unit_controllers[id]
   if not unit_controller then
      unit_controller = UnitController(self._scheduler, entity)
   end
   return unit_controller
end

function Town:promote_citizen(person, talisman)
   local args = {
      person = person,
      talisman = talisman,
   }

   local controller = self:get_unit_controller(person)
   local task = controller:create_task('stonehearth:tasks:promote_with_talisman', args)
                          :set_priority(constants.priorities.top.GRAB_PROMOTION_TALISMAN)


   local scheduler = stonehearth.tasks:create_scheduler()
                        :set_activity('stonehearth:top')
                        :join(person)

   local task = scheduler:create_orchestrator('stonehearth:tasks:promote_with_talisman', )
      :set_priority(constants.priorities.top.GRAB_PROMOTION_TALISMAN)
      :start()
      
   radiant.events.listen(task, 'completed', function()
          stonehearth.tasks:destroy_scheduler(scheduler)
         return radiant.events.UNLISTEN
      end)

   return true
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

   local task = self:create_worker_task('stonehearth:place_item', {
         item = item_proxy,
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

   local task = self:create_worker_task('stonehearth:place_item_type', {
         filter_fn = filter_fn,
         location = location,
         rotation = rotation,
         finish_fn = remove_ghost_entity
      })
      :once()
      :start()

   return task
end

function Town:harvest_resource_node(tree)
   if not radiant.util.is_a(tree, Entity) then
      return false
   end

   local id = tree:get_id()
   if not self._harvest_tasks[id] then
      local node_component = tree:get_component('stonehearth:resource_node')
      if node_component then
         local effect_name = node_component:get_harvest_overlay_effect()
         self._harvest_tasks[id] = self:create_worker_task('stonehearth:chop_tree', { tree = tree })
                                      :set_source(tree)
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
   if not self._harvest_tasks[id] then
      local node_component = plant:get_component('stonehearth:renewable_resource_node')
      if node_component then
         local effect_name = node_component:get_harvest_overlay_effect()
         self._harvest_tasks[id] = self:create_worker_task('stonehearth:harvest_plant', { plant = plant })
                                   :set_source(plant)
                                   :add_entity_effect(plant, effect_name)
                                   :once()
                                   :start()

         radiant.events.listen(plant, 'stonehearth:is_harvestable', function(e) 
               local plant = e.entity
               if not plant or not plant:is_valid() then
                  return radiant.events.UNLISTEN
               end
               self._harvest_tasks[plant:get_id()] = nil
            end)
      end
   end
   return true
end

function Town:create_workshop(crafter, ghost_workshop)
   --REVIEW Q: can I reuse a scheduler that exists already for the crafter?
   local scheduler = stonehearth.tasks:create_scheduler()
                        :set_activity('stonehearth:top')
                        :join(crafter)
   scheduler:create_orchestrator('stonehearth:tasks:create_workshop', {
      faction = session.faction,
      ghost_workshop = ghost_workshop,
      outbox_entity = outbox_entity,
      crafter = crafter
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

function Town:create_orchestrator(name, args, co)
   assert(false, 'get rid of this abominiation')
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


return Town

