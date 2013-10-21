--[[
   Task that represents a worker bringing a piece of wood to a firepit and setting it on fire.
]]
local Point3 = _radiant.csg.Point3
local WorkerLightFireAction = class()

WorkerLightFireAction.name = 'light fire'
WorkerLightFireAction.does = 'stonehearth:light_fire'
WorkerLightFireAction.priority = 5

function WorkerLightFireAction:__init(ai, entity)
   self._ai = ai
   self._entity = entity
end

--- Grab the log, bring it to the fire, and light it.
-- Make sure only one person is doing this at a time.
function WorkerLightFireAction:run(ai, entity, path_to_log, firepit, task)
   --Put in some logging
   local name = entity:get_component('unit_info'):get_display_name()
   local log = path_to_log:get_destination()

   -- If the task is already stopped, someone else got this action first. Exit.
   if not task:is_running() then
      radiant.log.info('%s (Worker %s): Ok, someone else is taking care of the fire.', tostring(entity), name)
      ai:abort()
      --return
   end

   -- The task must be running, so claim it by stopping it
   task:stop()

   -- Claim the task, so we know we've started. If we succeed, we'll set the task to nil.
   -- If we fail, and the task is still assigned, then we know we need to restart the task
   -- so that some other worker can pick it up.
   self._task = task

   -- Log successful grab
   radiant.log.info('%s (Worker %s): My turn to light the fiiayyyh!!!', tostring(entity), name)

   -- Pick up the log and bring it over
   ai:execute('stonehearth:carry_item_on_path_to', path_to_log, firepit)

   -- Drop the log into the fire
   ai:execute('stonehearth:drop_carrying_in_entity', firepit)

   -- Light the fire
   ai:execute('stonehearth:run_effect', 'light_fire')

   -- Put the log IN the firepit and light it
   local firepit_component = firepit:get_component('stonehearth:firepit')
   firepit_component:light(log)

   -- If we got here, we succeeded at the action.
   -- Note that we succeeded by setting the task to nil.
   self._task = nil
end

--- Destroy the task if we were successful. Start it again if not
function WorkerLightFireAction:stop()
   if self._task then
      self._task:start()
      self._task = nil
   end
end

return WorkerLightFireAction