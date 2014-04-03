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
      self._sv.player_id = session.player_id
      self._sv.town_entity = radiant.entities.create_entity()
      self._sv._saved_calls = {}
      self._sv._next_saved_call_id = 1
      radiant.entities.add_child(radiant._root_entity, self._sv.town_entity, Point3(0, 0, 0))
   end

   self._log = radiant.log.create_logger('town', self._sv.player_id)
   self:_create_task_groups()

   self._unit_controllers = {}
   self._thread_orchestrators = {}
   self._harvest_tasks = {}
   self._buildings = {}
   
   radiant.events.listen(radiant, 'radiant:game_loaded', function()
         self:_restore_saved_calls()
         return radiant.events.UNLISTEN
      end)
end

function Town:_create_task_groups()
   self._scheduler = stonehearth.tasks:create_scheduler(self._sv.player_id)
                                       :set_counter_name(self._sv.player_id)

   self._task_groups = {
      workers = self._scheduler:create_task_group('stonehearth:work', {})
                               :set_priority(stonehearth.constants.priorities.top.WORK)
                               :set_counter_name('workers'), 
      farmers = self._scheduler:create_task_group('stonehearth:farm', {})
                               :set_priority(stonehearth.constants.priorities.top.WORK)
                               :set_counter_name('farmers'), 
   }      

   -- a map from a profession_id to a task group for that profession
   self._profession_to_taskgroup = {
      worker = self._task_groups.workers,
      farmer = self._task_groups.farmers,
   }
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

function Town:get_faction()
   return self._sv.faction
end

function Town:get_player_id()
   return self._sv.player_id
end

function Town:destroy()
end

-- xxx: this is a stopgap until we can provide a better interface
function Town:create_worker_task(activity_name, args)
   return self._task_groups.workers:create_task(activity_name, args)
end

function Town:create_task_group(name, args)
   -- xxx: stash it away for when we care to enumerate everything everyone in the town
   -- is doing
   return self._scheduler:create_task_group(name, args)
end

-- xxx: this is a stopgap until we can provide a better interface
-- Yes, since I'm duplicating it for farmers
-- TODO: fix and generalize
function Town:create_farmer_task(activity_name, args, player_init)
   return self._task_groups.farmers:create_task(activity_name, args)
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
   task_group:remove_worker(entity:get_id())
   return self
end

-- does not tame the pet, just adds it to the town
function Town:add_pet(entity)
   entity:add_component('unit_info'):set_faction(self._sv.faction)

   self:_add_unit_controller(entity, 'stonehearth:ambient_pet_behavior',
      stonehearth.constants.priorities.top.AMBIENT_PET_BEHAVIOR)

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
   self._banner = entity
end

function Town:get_banner()
   return self._banner
end

function Town:command_unit(entity, activity_name, activity_args)
   local unit_controller = self:_get_unit_controller(entity)
   return unit_controller:create_immediate_task(activity_name, activity_args)
end

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
      :set_priority(stonehearth.constants.priorities.worker_task.PLACE_ITEM)                    
      :once()
      :start()

   self:_remember_user_initiated_task(task, 'place_item_in_world', item_proxy, full_sized_uri, location, rotation)

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
         local harvest_profession = node_component:get_harvest_profession()
         local task_group = self._profession_to_taskgroup[harvest_profession]
         if task_group then
            local effect_name = node_component:get_harvest_overlay_effect()
            local task = task_group:create_task('stonehearth:harvest_resource_node', { node = node })
                                         :set_source(node)
                                         :add_entity_effect(node, effect_name)
                                         :set_priority(stonehearth.constants.priorities.farmer_task.HARVEST)
                                         :once()
                                         :start()
            self._harvest_tasks[id] = task
            self:_remember_user_initiated_task(task, 'harvest_resource_node', node)
         end
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
      local harvest_profession = node_component:get_harvest_profession()
         local task_group = self._profession_to_taskgroup[harvest_profession]
         if task_group then
            local effect_name = node_component:get_harvest_overlay_effect()
            local task = task_group:create_task('stonehearth:harvest_plant', { plant = plant })
                                   :set_source(plant)
                                   :add_entity_effect(plant, effect_name)
                                   :set_priority(stonehearth.constants.priorities.farmer_task.HARVEST)
                                   :once()
                                   :start()
            self._harvest_tasks[id] = task
            self:_remember_user_initiated_task(task, 'harvest_renewable_resource_node', plant)
         end

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

function Town:add_construction_project(building)
   table.insert(self._buildings, building)
   radiant.events.trigger(self, 'stonehearth:building_added', {
         town = self,
         building = building,
      })
end

--- Tell farmers to plan the crop_type in the designated locations
-- @param faction: the group that should be planting the crop
-- @param soil_plots: array of entities on top of which to plant the crop
-- @param crop_type: the name of the thing to plant (ie, stonehearth:corn, etc)
-- REVIEW QUESTION: this function seems like it would belong in FarmingService, but we need to save the
-- task somewhere, and the best infrastructure is here. Long term solution? Factor out the save task ability?
function Town:plant_crop(player_id, soil_plots, crop_type, player_speficied, auto_plant, auto_harvest, player_initialized)
   if not soil_plots[1] or not crop_type then
      return false
   end
   for i, plot in ipairs(soil_plots) do
      --Tell the dirt plot whether the player wanted to autoreplant/autoharvest this plot
      --TODO: right now these are always false for manual plant commands. Base this on the UI.
      --TODO: maybe change this from the UI, instead of from the plant command
      local dirt_plot_component = plot:get_component('stonehearth:dirt_plot')
      dirt_plot_component:set_player_override(player_speficied, auto_plant, auto_harvest)

      --TODO: store these tasks, so they can be cancelled
      local overlay_effect = stonehearth.farming:get_overlay_for_crop(crop_type)
      local task = self:create_farmer_task('stonehearth:plant_crop', {target_plot = plot, 
                                                         dirt_plot_component = dirt_plot_component,
                                                         crop_type = crop_type})
                              :set_source(plot)
                              :set_name('plant_crop')
                              :set_priority(stonehearth.constants.priorities.farmer_task.PLANT)
                              :once()
                              
      --TODO: track plant tasks so the most *recent* one is always executed
      --Only track the task here if it was player initialized. Otherwise, the farm tracks it
      --Only put toasts on player-init actions
      if player_initialized then
         task:add_entity_effect(plot, overlay_effect)
         self:_remember_user_initiated_task(task, 'plant_crop', player_id, soil_plots, crop_type, player_speficied, auto_plant, auto_harvest, player_initialized)
      end
      task:start()
   end
   return true
end

return Town

