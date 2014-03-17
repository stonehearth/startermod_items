local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local UnitController = require 'services.town.unit_controller'
local PetOrchestrator = require 'services.town.orchestrators.pet_orchestrator'
local Promote = require 'services.town.orchestrators.promote_orchestrator'
local CreateWorkshop = require 'services.town.orchestrators.create_workshop_orchestrator'

local Town = class()

function Town:__init(session)   
   self._faction = session.faction
   self._player_id = session.player_id

   self._log = radiant.log.create_logger('town', self._player_id)
   self._scheduler = stonehearth.tasks:create_scheduler(self._player_id)
                                       :set_counter_name(self._player_id)

   self._town_entity = radiant.entities.create_entity()
   radiant.entities.add_child(radiant._root_entity, self._town_entity, Point3(0, 0, 0))

   self._task_groups = {
      workers = self._scheduler:create_task_group('stonehearth:work', {})
                               :set_priority(stonehearth.constants.priorities.top.WORK)
                               :set_counter_name('workers'), 
      farmers = self._scheduler:create_task_group('stonehearth:farm', {})
                               :set_priority(stonehearth.constants.priorities.top.WORK)
                               :set_counter_name('farmers'), 
   }

   self._unit_controllers = {}
   self._thread_orchestrators = {}
   self._harvest_tasks = {}
   self._buildings = {}
end

function Town:get_faction()
   return self._faction
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
function Town:create_farmer_task(activity_name, args)
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
   task_group:remove_worker(entity)
   return self
end

-- does not tame the pet, just adds it to the town
function Town:add_pet(entity)
   entity:add_component('unit_info'):set_faction(self._faction)

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

function Town:harvest_resource_node(node)
   if not radiant.util.is_a(node, Entity) then
      return false
   end

   local id = node:get_id()
   if not self._harvest_tasks[id] then
      local node_component = node:get_component('stonehearth:resource_node')
      if node_component then
         local effect_name = node_component:get_harvest_overlay_effect()
         --If the node is of type crop, have farmers harvest it
         --TODO: make create X task, where you pass in the type of work group
         local material_component = node:get_component('stonehearth:material')
         if material_component and material_component:has_tag('crop') then
            self._harvest_tasks[id] = self:create_farmer_task('stonehearth:harvest_resource_node', { node = node })
                                         :set_source(node)
                                         :add_entity_effect(node, effect_name)
                                         :once()
                                         :start()
         else 
            self._harvest_tasks[id] = self:create_worker_task('stonehearth:harvest_resource_node', { node = node })
                                         :set_source(node)
                                         :add_entity_effect(node, effect_name)
                                         :once()
                                         :start()
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
         local effect_name = node_component:get_harvest_overlay_effect()
         local material_component = plant:get_component('stonehearth:material')
         if material_component and material_component:has_tag('crop') then
            self._harvest_tasks[id] = self:create_farmer_task('stonehearth:harvest_plant', { plant = plant })
                                   :set_source(plant)
                                   :add_entity_effect(plant, effect_name)
                                   :once()
                                   :start()
         else
         self._harvest_tasks[id] = self:create_worker_task('stonehearth:harvest_plant', { plant = plant })
                                   :set_source(plant)
                                   :add_entity_effect(plant, effect_name)
                                   :once()
                                   :start()
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
return Town

