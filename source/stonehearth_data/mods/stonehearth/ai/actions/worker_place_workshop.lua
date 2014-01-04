--Action to help workers create and move workshops
--TODO: depending on the complexity of the workshop,
--we may need to abstract this for multiple types of workshops, 
--with arbitrary steps for moving/creating.

local Point3 = _radiant.csg.Point3
local analytics = stonehearth.analytics

local WorkerPlaceWorkshop = class()
local log = radiant.log.create_logger('actions.place_workshop')

WorkerPlaceWorkshop.name = 'place workshop'
WorkerPlaceWorkshop.does = 'stonehearth:place_workshop'
WorkerPlaceWorkshop.version = 1
WorkerPlaceWorkshop.priority = 5

function WorkerPlaceWorkshop:__init(ai, entity)
   self._ai = ai
   self._entity = entity
   self._task = nil
end

function WorkerPlaceWorkshop:run(ai, entity, path, ghost_entity, outbox_entity, task)
   --Run to get the target item that we can use to build the workshop
   --TODO: what if we need multiple items to build?
   local target_ingredient = path:get_destination()
   if not target_ingredient then
      ai:abort()
   end

   -- If the task is already stopped, someone else got this action first. Exit.
   if not task:is_running() then
      ai:abort()
   end

   --The task must be running, so claim it by stopping it
   task:stop()
   self._task = task

   -- Log successful grab
   log:info('%s (Worker %s): I am totally building this workshop!', tostring(entity))

   ai:execute('stonehearth:carry_item_on_path_to', path, ghost_entity)

   --What are we carrying?
   local carrying = radiant.entities.get_carrying(entity)
   if not carrying then
      --if we're not carrying something here, something is terribly wrong
      ai:abort()
   end

   -- Run over to the workshop's designated location and use up the wood
   --TODO: Handle case where workshop requires multiple ingredients
   local target_location = radiant.entities.get_world_grid_location(ghost_entity)
   ai:execute('stonehearth:drop_carrying', target_location)
   ai:execute('stonehearth:run_effect', 'work')
   radiant.entities.destroy_entity(carrying)

   --Create the workbench
   local ghost_object_data = ghost_entity:get_component('stonehearth:ghost_item'):get_object_data()
   local real_item_uri = ghost_object_data.full_sized_mod_url
   local real_item_rotation = ghost_object_data.rotation
   local workbench_entity = radiant.entities.create_entity(real_item_uri)
   local workshop_component = workbench_entity:get_component('stonehearth:workshop')

   -- Place the bench in the world
   radiant.terrain.place_entity(workbench_entity, target_location)
   radiant.entities.turn_to(workbench_entity, real_item_rotation)

   --TODO: Ask Doug if a different type of sound attenuates with distance?
   local json = radiant.resources.load_json(real_item_uri)
   if json and json.components then
      local workshop_sound = json.components['stonehearth:workshop'].build_sound_effect
      radiant.effects.run_effect(entity, workshop_sound)
   end



   -- Place the promotion talisman on the workbench, if there is one
   local promotion_talisman_entity = workshop_component:init_from_scratch()

   -- set the faction of the bench and talisman
   local faction = radiant.entities.get_faction(entity)
   workbench_entity:get_component('unit_info'):set_faction(faction)
   promotion_talisman_entity:get_component('unit_info'):set_faction(faction)

   --associate the outbox
   workshop_component:associate_outbox(outbox_entity)

   analytics:send_design_event('game:place_workshop', entity, carrying)

   --destroy the ghost entity
   radiant.entities.destroy_entity(ghost_entity)

   --If we got here, we succeeded at the action.  We can get rid of this task now.
   self._task:destroy()
   self._task = nil
end

function WorkerPlaceWorkshop:stop()
   -- If we were interrupted before we could destory the task, start it
   -- up again
   if self._task then
      self._task:start()
      self._task = nil
   end
end

return WorkerPlaceWorkshop