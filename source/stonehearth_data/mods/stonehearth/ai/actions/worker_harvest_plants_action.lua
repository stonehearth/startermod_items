local event_service = require 'services.event.event_service'
local HarvestPlantsAction = class()

HarvestPlantsAction.name = 'harvest plants'
HarvestPlantsAction.does = 'stonehearth:harvest_plants'
HarvestPlantsAction.priority = 9

function HarvestPlantsAction:__init(ai, entity)
   self._ai = ai
   self._entity = entity

end

function HarvestPlantsAction:run(ai, entity, path, task)
   local plant = path:get_destination()
   if not plant then
      ai:abort('plant does not exist anymore in stonehearth:harvest_plants')
   end

   local worker_name = radiant.entities.get_display_name(entity)
   local plant_name = radiant.entities.get_display_name(plant)

   -- If the task is already stopped, someone else got this action first. Exit.
   if not task:is_running() then
      radiant.log.info('%s (Worker %s): Never mind! You got the %s.', tostring(entity), worker_name, plant_name)
      ai:abort()
   end

   -- The task must be running, so claim it by stopping it
   task:stop()

   -- Claim the task, so we know we've started. If we succeed, we'll set the task to nil.
   -- If we fail, and the task is still assigned, then we know we need to restart the task
   -- so that some other worker can pick it up.
   self._task = task

   -- Log successful grab
   event_service:add_entry('<' .. worker_name .. '>:' .. 'Mmm! A ' .. plant_name)

   ai:execute('stonehearth:follow_path', path)
   radiant.entities.turn_to_face(entity, plant)
   ai:execute('stonehearth:run_effect','fiddle')

   --Pop the basket
   --TODO: get the basket from a component attached to the plant
   local harvestable_component = plant:get_component('stonehearth:harvestable')
   if harvestable_component then
      harvestable_component:harvest()
   end   
   --ai:execute('stonehearth:pickup_item_on_ground', basket)
   --radiant.entities.pickup_item(entity, basket)
   --ai:execute('stonehearth:run_effect', 'carry_pickup')

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