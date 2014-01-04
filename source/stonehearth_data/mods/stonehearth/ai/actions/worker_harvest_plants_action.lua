--[[
   Tell a worker to collect food and resources from a plant. 
]]

local event_service = stonehearth.events
local Point3 = _radiant.csg.Point3

local HarvestPlantsAction = class()
local log = radiant.log.create_logger('actions.harvest')

HarvestPlantsAction.name = 'harvest plants'
HarvestPlantsAction.does = 'stonehearth:harvest_plant'
HarvestPlantsAction.version = 1
HarvestPlantsAction.priority = 9

function HarvestPlantsAction:__init(ai, entity)
   self._ai = ai
   self._entity = entity

end

function HarvestPlantsAction:run(ai, entity, path, task)
   local plant = path:get_destination()

   --is the plant the berries or the bush? 
   local is_harvestable_item =  radiant.entities.get_entity_data(plant, 'stonehearth:havestable_item')
   if is_harvestable_item then
      --we actually want its parent, the plant that contains the harvestable item
      plant = plant:get_component('mob'):get_parent()
   end

   if not plant then
      ai:abort('plant does not exist anymore in stonehearth:harvest_plant')
   end

   local worker_name = radiant.entities.get_display_name(entity)
   local plant_name = radiant.entities.get_display_name(plant)

   -- If the task is already stopped, someone else got this action first. Exit.
   if not task:is_running() then
      log:info('%s (Worker %s): Never mind! You got the %s.', tostring(entity), worker_name, plant_name)
      ai:abort()
   end

   -- The task must be running, so claim it by stopping it
   task:stop()

   -- Claim the task, so we know we've started. If we succeed, we'll set the task to nil.
   -- If we fail, and the task is still assigned, then we know we need to restart the task
   -- so that some other worker can pick it up.
   self._task = task

   -- Log successful grab
   -- TODO: localize, and put in log!
   --event_service:add_entry(worker_name .. ' is collecting stuff from a ' .. plant_name)

   ai:execute('stonehearth:follow_path', path)
   radiant.entities.turn_to_face(entity, plant)

   --Fiddle with the bush and pop the basket
   local factory = plant:get_component('stonehearth:renewable_resource_node')
   if factory then
      ai:execute('stonehearth:run_effect','fiddle')
      local front_point = self._entity:get_component('mob'):get_location_in_front()
      factory:spawn_resource(Point3(front_point.x, front_point.y, front_point.z))
   end   
   
   --If we got here, we succeeded at the action.  We can get rid of this task now.
   self._task:destroy()
   self._task = nil

end

function HarvestPlantsAction:stop()
   -- If we were interrupted before we could destory the task, start it
   -- up again
   if self._task then
      self._task:start()
      self._task = nil
   end
end

return HarvestPlantsAction