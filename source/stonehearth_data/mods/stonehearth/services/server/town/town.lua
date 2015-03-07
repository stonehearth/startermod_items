local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local UnitController = require 'services.server.town.unit_controller'
local PetOrchestrator = require 'services.server.town.orchestrators.pet_orchestrator'
local Promote = require 'services.server.town.orchestrators.promote_orchestrator'
local CreateWorkshop = require 'services.server.town.orchestrators.create_workshop_orchestrator'

local Town = class()

function Town:initialize(player_id)
   self._sv.player_id = player_id
   self._sv._saved_calls = {}
   self._sv._next_saved_call_id = 1
   self._sv.worker_combat_enabled = false
   self._sv.rally_to_battle_standard = false
   self._sv.mining_zones = {}

   self._sv.entity = radiant.entities.create_entity('', { owner = player_id })
   
   self.__saved_variables:mark_changed()

   self:restore()
end

function Town:restore()
   self._log = radiant.log.create_logger('town', self._sv.player_id)

   self:_create_task_groups()

   self._unit_controllers = {}
   self._thread_orchestrators = {}
   self._harvest_tasks = {}
   self._loot_tasks = {}
   self._clear_tasks = {}
   self._blueprints = {}
   self._placement_xs = {}
   self._worker_combat_tasks = {}
   self._rally_tasks = {}
   self._mining_zone_destroy_listeners = {}
   self._temporary_effects = {}

   self:_restore_mining_zone_listeners()

   radiant.events.listen_once(radiant, 'radiant:game_loaded', function()
         self:_on_game_loaded()
      end
   )
end

function Town:_on_game_loaded()
   self:_restore_saved_calls()

   if self._sv.worker_combat_enabled then
      self:enable_worker_combat()
   end

   if self._sv.rally_to_battle_standard then
      self:enable_rally_to_battle_standard()
   end
end

function Town:_create_task_groups()
   self._scheduler = stonehearth.tasks:create_scheduler(self._sv.player_id)
                                          :set_counter_name(self._sv.player_id)

   self._task_groups = {}

   -- Create new task groups
   local task_group_data = radiant.resources.load_json('stonehearth:data:player_task_groups')
   if task_group_data then
      for task_group_name, entry in pairs(task_group_data.task_groups) do

         local group = self._scheduler:create_task_group(entry.dispatcher, {})
                                          :set_counter_name(entry.name)

         self._task_groups[task_group_name] = group
      end
   end
end

function Town:_remember_user_initiated_task(task, fn_name, ...)
   -- task here may be a Task or an Orchestrator.  Both implement the is_completed
   -- and notify_completed functions
   if task:is_active() then
      local id = self._sv._next_saved_call_id
      self._sv._next_saved_call_id = self._sv._next_saved_call_id + 1
      self._sv._saved_calls[id] = {
         fn_name = fn_name,
         args = { ... }
      }
      self.__saved_variables:mark_changed()

      --When the task is destroyed (either b/c of successful completion or active destruction)
      task:notify_destroyed(function()
            self._sv._saved_calls[id] = nil 
            self.__saved_variables:mark_changed()
         end)
   end
end

function Town:_restore_saved_calls()
   local saved_calls = self._sv._saved_calls
   self._sv._saved_calls = {}    
   for _, saved_call in pairs(saved_calls) do
      self[saved_call.fn_name](self, unpack(saved_call.args))
   end
end

function Town:get_task_groups_model()
   -- we can return a table because we know we'll never create new groupings
   -- after the town is initialized.  this saves us a trace and a round trip
   --  on the client
   local model = {}
   for name, group in pairs(self._task_groups) do
      model[name] = group:get_model();
   end
   return model;
end

function Town:get_entity()
   return self._sv.entity
end

function Town:get_scheduler()
   return self._scheduler
end

function Town:get_player_id()
   return self._sv.player_id
end

function Town:get_citizens()
   local population = stonehearth.population:get_population(self._sv.player_id)
   return population:get_citizens()
end

function Town:destroy()
   for _, listener in pairs(self._mining_zone_destroy_listeners) do
      listener:destroy()
   end
   self._mining_zone_destroy_listeners = {}
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

function Town:leave_task_group(entity_id, name)
   local task_group = self._task_groups[name]
   assert(task_group, string.format('unknown task group "%s"', name))
   task_group:remove_worker(entity_id)
   return self
end

-- does not tame the pet, just adds it to the town
function Town:add_pet(entity)
   entity:add_component('unit_info')
            :set_player_id(self._sv.player_id)

   self:create_orchestrator(PetOrchestrator, { entity = entity })

   return self
end

-- does not untame the pet, just removes it from the town
function Town:remove_pet(entity)
   entity:add_component('unit_info')
            :set_player_id('critters')
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

function Town:get_centroid()
   if not self._sv.banner then
      return
   end
   local centroid = radiant.entities.get_world_grid_location(self._sv.banner)
   if not centroid then
      return
   end
   centroid = radiant.terrain.get_point_on_terrain(centroid + Point3.unit_z)
   return centroid
end

function Town:set_battle_standard(entity)
   self._sv.battle_standard = entity
   self.__saved_variables:mark_changed()
end

function Town:get_battle_standard()
   return self._sv.battle_standard
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

function Town:create_promote_orchestrator(person, talisman)
   return self:create_orchestrator(Promote, {
      person = person,
      talisman = talisman,
   })
end

function Town:loot_item(item)
   -- is the item on the ground?
   local location = radiant.entities.get_world_grid_location(item)
   if not location then
      return
   end
   -- is the item actually an item?
   local ic = item:get_component('item')
   if not ic then
      return
   end

   self:remove_previous_task_on_item(item)
   local id = item:get_id()
   if not self._loot_tasks[id] then
      local task = self:create_task_for_group('stonehearth:task_group:restock', 'stonehearth:pickup_item_into_backpack', { item = item })
                           :set_priority(stonehearth.constants.priorities.simple_labor.LOOT_ITEM)
                           :add_entity_effect(item, '/stonehearth/data/effects/loot_effect')
                           :notify_completed(function()
                              self._loot_tasks[id] = nil
                           end)
                           :once()
                           :start()
      self._loot_tasks[id] = task
      self:_remember_user_initiated_task(task, 'loot_item', item)
   end
end

function Town:harvest_resource_node(node)
   if not radiant.util.is_a(node, Entity) then
      return false
   end

   self:remove_previous_task_on_item(node)

   local id = node:get_id()
   if not self._harvest_tasks[id] then
      local node_component = node:get_component('stonehearth:resource_node')
      if node_component and node_component:is_harvestable() then
         local task_group_name = node_component:get_task_group_name()
         local effect_name = node_component:get_harvest_overlay_effect()
         local task = self:create_task_for_group(task_group_name, 'stonehearth:harvest_resource_node', { node = node })
                              :set_source(node)
                              :add_entity_effect(node, effect_name)
                              :set_priority(stonehearth.constants.priorities.farmer_task.HARVEST)
                              :once()
                              :notify_completed(function()
                                 self._harvest_tasks[id] = nil
                              end)
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
   self:remove_previous_task_on_item(plant)

   if not self._harvest_tasks[id] then
      local node_component = plant:get_component('stonehearth:renewable_resource_node')
      if node_component and node_component:is_harvestable() then
         local task_group_name = node_component:get_task_group_name()
         local effect_name = node_component:get_harvest_overlay_effect()
         local task = self:create_task_for_group(task_group_name, 'stonehearth:harvest_plant', { plant = plant })
                              :set_source(plant)
                              :add_entity_effect(plant, effect_name)
                              :set_priority(stonehearth.constants.priorities.farmer_task.HARVEST)
                              :once()
                              :notify_completed(function()
                                 self._harvest_tasks[id] = nil
                              end)
                              :start()
         self._harvest_tasks[id] = task
         self:_remember_user_initiated_task(task, 'harvest_renewable_resource_node', plant)
      end
   end
   return true
end


--Add a temporary effect to this entity. Keep a pointer to it so we can remove it later.
--Assume we won't have more than 1 of a given effect type on an object at a time. 
function Town:add_temporary_effect(entity, effect_name)
   local entity_id = entity:get_id()
   local effect_index = effect_name .. entity_id
   if not self._temporary_effects[effect_index] then
      self._temporary_effects[effect_index] = radiant.effects.run_effect(entity, effect_name)
   end
end

function Town:remove_temporary_effect(entity_id, effect_name)
   local effect_index = effect_name .. entity_id
   if self._temporary_effects[effect_index]  then
      self._temporary_effects[effect_index]:stop()
      self._temporary_effects[effect_index] = nil
   end
end


--Tell the harvesters to remove an item permanently from the world
--If there was already an outstanding task on the object, make sure to cancel it first.
function Town:clear_item(item)
   self:remove_previous_task_on_item(item)
   local id = item:get_id()
   
   --trace the item. If it moves, cancel the task
   local position_trace = radiant.entities.trace_location(item, 'clear item task generator')
                                 :on_changed(function()
                                    if self._clear_tasks[id] then 
                                       self._clear_tasks[id]:destroy()
                                       self._clear_tasks[id] = nil
                                    end
                                 end)

   local task = self:create_task_for_group('stonehearth:task_group:harvest', 'stonehearth:clear_item', {item = item})
                     :set_source(item)
                     :set_priority(stonehearth.constants.priorities.simple_labor.CLEAR)
                     :add_entity_effect(item, "/stonehearth/data/effects/clear_effect")
                     :once()
                     :notify_destroyed(function()
                        self._clear_tasks[id] = nil
                        if position_trace then
                           position_trace:destroy()
                           position_trace = nil
                        end
                     end)
                     :start()
   self._clear_tasks[id] = task
   self:_remember_user_initiated_task(task, 'clear_item', item)
end

--Remove only town-related tasks, like harvest/clear
function Town:remove_town_tasks_on_item(item)
   local id = item:get_id()
   if self._clear_tasks[id] then
      self._clear_tasks[id]:destroy()
      self._clear_tasks[id] = nil
   end
   if self._harvest_tasks[id] then
      self._harvest_tasks[id]:destroy()
      self._harvest_tasks[id] = nil
   end
   if self._loot_tasks[id] then
      self._loot_tasks[id]:destroy()
      self._loot_tasks[id] = nil
   end
end

--Generally, remove as many kinds of tasks as we know about
function Town:remove_previous_task_on_item(item)
   self:remove_town_tasks_on_item(item)
   local entity_forms_component = item:get_component('stonehearth:entity_forms')
   if entity_forms_component then
      entity_forms_component:cancel_placement_tasks()
   end
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
         :notify_completed(function()
            self._harvest_tasks[id] = nil
         end)

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

--Classes are enrolled in worker defense training by default, 
--but some classes, like the shepherd, can choose to opt out. 
--This is defined in classname_description.lua. We get the data
--by asking for the job controller and queriying it for its
--worker defense status.
-- @returns true if the citizen participates in worker defense, false otherwise
function Town:_get_citizen_participation_in_defense(entity)
   local job_component = entity:get_component('stonehearth:job')
   if not job_component then
      return false
   end

   local job = job_component:get_curr_job_controller()
   if not job then
      return false
   end
   return job:get_worker_defense_participation()
end

function Town:worker_combat_enabled()
   return self._sv.worker_combat_enabled
end

-- Cause all defense-capable classes to go into red alert mode
function Town:enable_worker_combat()
   local citizens = self:get_citizens()
   
   self._town_defense_thoughts = {}
   for _, citizen in pairs(citizens) do
      if self:_get_citizen_participation_in_defense(citizen) then
         stonehearth.combat:set_panicking_from(citizen, nil)
         stonehearth.combat:set_stance(citizen, 'aggressive')
         local thought = radiant.entities.think(citizen, '/stonehearth/data/effects/thoughts/alert', stonehearth.constants.think_priorities.ALERT)
         table.insert(self._town_defense_thoughts, thought)
         radiant.entities.add_buff(citizen, 'stonehearth:buffs:defender');

         -- xxx: let's do this by given them an item with a buff.  tasks are expensive.
         local task = citizen:add_component('stonehearth:ai')
            :get_task_group('stonehearth:urgent')
            :create_task('stonehearth:town_defense:dispatcher')
            :set_priority(stonehearth.constants.priorities.urgent.TOWN_DEFENSE)
            :start()

         self._worker_combat_tasks[citizen:get_id()] = task
      end
   end
   self._sv.worker_combat_enabled = true
   self.__saved_variables:mark_changed()

   radiant.events.trigger_async(self, 'stonehearth:town_defense_mode_changed', { enabled = true })
end

-- Tell all defense-capable classes to come out of red alert mode
function Town:disable_worker_combat()
   for _, thought in pairs(self._town_defense_thoughts) do
      thought:destroy()
   end
   self._town_defense_thoughts = {}

   for id, task in pairs(self._worker_combat_tasks) do
      task:destroy()

      local citizen = radiant.entities.get_entity(id)
      if citizen then
         local job_component = citizen:get_component('stonehearth:job')
         job_component:reset_to_default_comabat_stance()
         radiant.entities.remove_buff(citizen, 'stonehearth:buffs:defender');
      end
   end
   self._worker_combat_tasks = {}
   self._sv.worker_combat_enabled = false
   self.__saved_variables:mark_changed()

   radiant.events.trigger_async(self, 'stonehearth:town_defense_mode_changed', { enabled = false })
end

----- Zones -----

function Town:get_mining_zones()
   return self._sv.mining_zones
end

function Town:add_mining_zone(mining_zone)
   local id = mining_zone:get_id()
   self._sv.mining_zones[id] = mining_zone
   self.__saved_variables:mark_changed()

   self:_listen_for_mining_zone_destroyed(mining_zone)
end

function Town:_listen_for_mining_zone_destroyed(mining_zone)
   local id = mining_zone:get_id()
   local listener -- forward declaration
   
   listener = radiant.events.listen(mining_zone, 'radiant:entity:pre_destroy', function()
         self._sv.mining_zones[id] = nil
         self.__saved_variables:mark_changed()

         listener:destroy()
         self._mining_zone_destroy_listeners[id] = nil
      end)

   self._mining_zone_destroy_listeners[id] = listener
end

function Town:_restore_mining_zone_listeners()
   for id, mining_zone in pairs(self._sv.mining_zones) do
      self:_listen_for_mining_zone_destroyed(mining_zone)
   end
end

return Town
