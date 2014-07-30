local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local UnitController = require 'services.server.town.unit_controller'
local PetOrchestrator = require 'services.server.town.orchestrators.pet_orchestrator'
local Promote = require 'services.server.town.orchestrators.promote_orchestrator'
local CreateWorkshop = require 'services.server.town.orchestrators.create_workshop_orchestrator'

local Town = class()

function Town:__init(session, saved_variables)
   self.__saved_variables = saved_variables
   self._sv = self.__saved_variables:get_data()
   
   if session then
      self._sv.faction = session.faction
      self._sv.kingdom = session.kingdom
      self._sv.player_id = session.player_id
      self._sv._saved_calls = {}
      self._sv._next_saved_call_id = 1
   end

   self._log = radiant.log.create_logger('town', self._sv.player_id)

   self:_create_task_groups()

   self._unit_controllers = {}
   self._thread_orchestrators = {}
   self._harvest_tasks = {}
   self._blueprints = {}
   self._placement_xs = {}
   
   radiant.events.listen(radiant, 'radiant:game_loaded', function()
         self:_restore_saved_calls()
         return radiant.events.UNLISTEN
      end)
end

function Town:_create_task_groups()
   self._scheduler = stonehearth.tasks:create_scheduler(self._sv.player_id)
                                       :set_counter_name(self._sv.player_id)

   self._task_groups = {}

   -- Create new task groups
   local task_group_data = radiant.resources.load_json(self._sv.kingdom).task_groups
   if task_group_data then
      for task_group_name, activity_dispatcher in pairs(task_group_data) do
         self._task_groups[task_group_name] = self._scheduler:create_task_group(activity_dispatcher, {})
                                                            :set_counter_name(task_group_name)
      end
   end
end

function Town:_remember_user_initiated_task(task, fn_name, ...)
   -- task here may be a Task or an Orchestrator.  Both implement the is_completed
   -- and notify_completed functions
   if not task:is_completed() then
      local id = self._sv._next_saved_call_id
      self._sv._next_saved_call_id = self._sv._next_saved_call_id + 1
      self._sv._saved_calls[id] = {
         fn_name = fn_name,
         args = { ... }
      }
      self.__saved_variables:mark_changed()

      task:notify_completed(function()
            self._sv._saved_calls[id] = nil 
            self.__saved_variables:mark_changed()
         end);
   end
end

function Town:_restore_saved_calls()
   local saved_calls = self._sv._saved_calls
   self._sv._saved_calls = {}    
   for _, saved_call in pairs(saved_calls) do
      self[saved_call.fn_name](self, unpack(saved_call.args))
   end
end

function Town:get_scheduler()
   return self._scheduler
end

function Town:get_faction()
   return self._sv.faction
end

function Town:get_player_id()
   return self._sv.player_id
end

function Town:get_kingdom()
   return self._sv.kingdom
end

function Town:destroy()
end

function Town:set_town_name(town_name)
   self._sv.town_name = town_name
end

function Town:get_town_name()
   if not self._sv.town_name then
       local pop = stonehearth.population:get_population(self._sv.player_id)
       self._sv.town_name = pop:create_town_name()
   end
   return self._sv.town_name
end

-- This is for making task groups that the town doesn't track, ie, individual task groups
function Town:create_task_group(activity_group, args)
   -- xxx: stash it away for when we care to enumerate everything everyone in the town
   -- is doing
   return self._scheduler:create_task_group(activity_group, args)
end

--- Add a person to a town task group.
--  If the task group doesn't exist, make one. 
--  @param entity: the person to add
--  @param name: the name of the task group in question 
function Town:join_task_group(entity, name)
   local task_group = self._task_groups[name]
   assert(task_group, 'task group not yet created')
   task_group:add_worker(entity)
   return self
end

--- Creates a task in a given task group
--  If the tg doesn't exist yet, make one. We'll queue the task
--  even if there are no people to receive it yet. 
--  @param task_group_name - name of the tg
--  @param activity_name - name of the activity
--  @param args - args to pass into the activity 
function Town:create_task_for_group(task_group_name, activity_name, args)
   local task_group = self._task_groups[task_group_name]
   assert(task_group, 'task group not yet created')
   return task_group:create_task(activity_name, args)
end

function Town:leave_task_group(entity, name)
   local task_group = self._task_groups[name]
   assert(task_group, string.format('unknown task group "%s"', name))
   task_group:remove_worker(entity:get_id())
   return self
end

-- does not tame the pet, just adds it to the town
function Town:add_pet(entity)
   local unit_info = entity:add_component('unit_info')
   unit_info:set_faction(self._sv.faction)
   unit_info:set_kingdom(self._sv.kingdom)
   unit_info:set_player_id(self._sv.player_id)

   self:create_orchestrator(PetOrchestrator, { entity = entity })

   return self
end

-- does not untame the pet, just removes it from the town
function Town:remove_pet(entity)
   entity:add_component('unit_info'):set_faction('critter')
end

function Town:_add_unit_controller(entity, activity_name, priority)
   local id = entity:get_id()
   local unit_controller = UnitController(self._scheduler, entity, activity_name, priority)
   self._unit_controllers[id] = unit_controller
   return unit_controller
end

function Town:_get_unit_controller(entity)
   local id = entity:get_id()
   local unit_controller = self._unit_controllers[id]
   if not unit_controller then
      unit_controller = self:_add_unit_controller(entity, 'stonehearth:unit_control',
         stonehearth.constants.priorities.top.UNIT_CONTROL)
   end
   return unit_controller
end

function Town:set_banner(entity)
   self._sv.banner = entity
   self.__saved_variables:mark_changed()
end

function Town:get_banner()
   return self._sv.banner
end

--- xxx: deprecated.  get rid of this
function Town:command_unit(entity, activity_name, activity_args)
   local unit_controller = self:_get_unit_controller(entity)
   return unit_controller:create_immediate_task(activity_name, activity_args)
end

--- xxx: deprecated.  get rid of this
function Town:command_unit_scheduled(entity, activity_name, activity_args)
   local unit_controller = self:_get_unit_controller(entity)
   return unit_controller:create_scheduled_task(activity_name, activity_args)
end

function Town:_handle_orchestrator_thread_exit(thread)
   local orchestrators = self._thread_orchestrators[thread]
   while orchestrators and #orchestrators > 0 do
      local o = table.remove(orchestrators, #orchestrators)
      if o.destroy then
         o:destroy()
      end
   end
   self._thread_orchestrators[thread] = nil
end

function Town:run_orchestrator(orchestrator_ctor, args)
   local orchestrator = orchestrator_ctor()

   local thread = stonehearth.threads:get_current_thread()
   local thread_orchestrators = self._thread_orchestrators[thread]
   assert(thread_orchestrators, 'not currently on an orchestartor thread!')

   table.insert(thread_orchestrators, orchestrator)

   local result = orchestrator:run(self, args)

   local o = table.remove(thread_orchestrators, #thread_orchestrators)
   assert(o == orchestrator)
   return result
end

function Town:create_orchestrator(orchestrator_ctor, args)
   local orchestrator = orchestrator_ctor()
   local thread = stonehearth.threads.create_thread()
                  :add_exit_handler(function (thread, err)
                        self:_handle_orchestrator_thread_exit(thread)
                     end)
                  :set_thread_main(function ()
                        orchestrator:run(self, args)
                     end)
   self._thread_orchestrators[thread] = {}
   table.insert(self._thread_orchestrators[thread], orchestrator)
   thread:start()
   
   return {
      destroy = function()
            if not thread:is_finished() then
               thread:terminate()
            end
         end
   }
end

function Town:promote_citizen(person, talisman)
   return self:create_orchestrator(Promote, {
      person = person,
      talisman = talisman,
   })
end

function Town:place_item_in_world(item_proxy, full_sized_uri, location, rotation)
   local proxy_id = item_proxy:get_id()
   if self._placement_xs[proxy_id] ~= nil then
      self._placement_xs[proxy_id].task:destroy()
      radiant.entities.destroy_entity(self._placement_xs[proxy_id].ghost_entity)
   end

   local ghost_entity = radiant.entities.create_entity()
   local ghost_entity_component = ghost_entity:add_component('stonehearth:ghost_form')
   ghost_entity_component:set_full_sized_mod_uri(full_sized_uri)
   radiant.terrain.place_entity_at_exact_location(ghost_entity, location)
   radiant.entities.turn_to(ghost_entity, rotation)

   local remove_ghost_entity = function(placed_item)
      radiant.entities.destroy_entity(ghost_entity)
      self._placement_xs[proxy_id] = nil
   end

   local task = self:create_task_for_group('stonehearth:task_group:placement', 'stonehearth:place_item', {
         item = item_proxy,
         location = location,
         rotation = rotation,
         finish_fn = remove_ghost_entity
      })
      :set_priority(stonehearth.constants.priorities.worker_task.PLACE_ITEM)                    
      :once()
      :start()

   self._placement_xs[proxy_id] = {
      ghost_entity = ghost_entity,
      task = task
   }

   self:_remember_user_initiated_task(task, 'place_item_in_world', item_proxy, full_sized_uri, location, rotation)

   return task
end

function Town:place_item_type_in_world(entity_uri, full_item_uri, location, rotation)
   local proxy_id = item:get_id()
   if self._placement_xs[proxy_id] ~= nil then
      self._placement_xs[proxy_id].task:destroy()
      radiant.entities.destroy_entity(self._placement_xs[proxy_id].ghost_entity)
   end

   local ghost_entity = radiant.entities.create_entity()
   local ghost_entity_component = ghost_entity:add_component('stonehearth:ghost_form')
   ghost_entity_component:set_full_sized_mod_uri(full_item_uri)
   radiant.terrain.place_entity(ghost_entity, location)
   radiant.entities.turn_to(ghost_entity, rotation)

   local remove_ghost_entity = function(placed_item)
      radiant.entities.destroy_entity(ghost_entity)
   end

   local filter_fn = function(item)
      return item:get_uri() == entity_uri
   end

   local task = self:create_task_for_group('stonehearth:task_group:placement', 'stonehearth:place_item_type', {
         item_uri = uri,
         location = location,
         rotation = rotation,
         finish_fn = remove_ghost_entity
      })
      :set_priority(stonehearth.constants.priorities.worker_task.PLACE_ITEM)                    
      :once()
      :start()

   self:_remember_user_initiated_task(task, 'place_item_type_in_world', entity_uri, full_item_uri, location, rotation)

   return task
end

function Town:harvest_resource_node(node)
   if not radiant.util.is_a(node, Entity) then
      return false
   end

   local id = node:get_id()
   if not self._harvest_tasks[id] then
      local node_component = node:get_component('stonehearth:resource_node')
      if node_component then
         local task_group_name = node_component:get_task_group_name()
         local effect_name = node_component:get_harvest_overlay_effect()
         local task = self:create_task_for_group(task_group_name, 'stonehearth:harvest_resource_node', { node = node })
                                      :set_source(node)
                                      :add_entity_effect(node, effect_name)
                                      :set_priority(stonehearth.constants.priorities.farmer_task.HARVEST)
                                      :once()
                                      :start()
         self._harvest_tasks[id] = task
         self:_remember_user_initiated_task(task, 'harvest_resource_node', node)
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
         local task_group_name = node_component:get_task_group_name()
         local effect_name = node_component:get_harvest_overlay_effect()
         local task = self:create_task_for_group(task_group_name, 'stonehearth:harvest_plant', { plant = plant })
                                :set_source(plant)
                                :add_entity_effect(plant, effect_name)
                                :set_priority(stonehearth.constants.priorities.farmer_task.HARVEST)
                                :once()
                                :start()
         self._harvest_tasks[id] = task
         self:_remember_user_initiated_task(task, 'harvest_renewable_resource_node', plant)

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

function Town:harvest_crop(crop, player_initialized)
   if not radiant.util.is_a(crop, Entity) then
      return false
   end

   local id = crop:get_id()
   if not self._harvest_tasks[id] then
      local task = self:create_task_for_group('stonehearth:task_group:simple_farming', 'stonehearth:harvest_crop', { crop = crop })
                                   :set_source(crop)
                                   :set_priority(stonehearth.constants.priorities.farming.HARVEST)
                                   :once()
      self._harvest_tasks[id] = task

      --Only track the task here if it was player initialized.
      --Only put toasts on player-init actions
      if player_initialized then
         task:add_entity_effect(crop, '/stonehearth/data/effects/chop_overlay_effect')
         self:_remember_user_initiated_task(task, 'harvest_crop', crop, player_initialized)
      end
      task:start()

   end
   return true
end

function Town:add_construction_blueprint(blueprint)
   table.insert(self._blueprints, blueprint)
   radiant.events.trigger_async(self, 'stonehearth:blueprint_added', {
         town = self,
         blueprint = blueprint,
      })
end

function Town:plant_crop(crop)
   local task = self:create_task_for_group(
      'stonehearth:task_group:simple_farming', 
      'stonehearth:plant_crop', { target_crop = crop })
         :set_source(crop)
         :set_name('plant_crop')
         :set_priority(stonehearth.constants.priorities.farming.PLANT)
         :start()
   return task
end

return Town

