local Point3 = _radiant.csg.Point3
local WorkerPlaceItemAction = class()


WorkerPlaceItemAction.name = 'place item'
WorkerPlaceItemAction.does = 'stonehearth:place_item'
WorkerPlaceItemAction.priority = 5

--TODO we need a scale to  describe relative importance.
--Right now all tasks dispatched by worker scheduler have the same priority, 10
--They override the given priority

function WorkerPlaceItemAction:__init(ai, entity)
   self._ai = ai
   self._entity = entity

   self._task = nil
end

--- Pick up and place the item designated by the caller
-- TODO: if the action is stopped, (ie, goblins attack!) should I drop the thing I'm carrying?
-- @param path The path to the object
-- @ghost_entity The ghost object representing the destination
-- @param rotation The degree at which the object should appear when it's placed
-- @param task The task, if this is fired by a worker scheduler
function WorkerPlaceItemAction:run(ai, entity, path, ghost_entity, rotation, task)
   -- Put in some logging
   local name = entity:get_component('unit_info'):get_display_name()
   local proxy_entity = path:get_destination()
   local object_name = proxy_entity:get_component('unit_info'):get_display_name()

   -- If the task is already stopped, someone else got this action first. Exit.
   if not task:get_running() then
      radiant.log.info('%s (Worker %s): Never mind! You got the %s.', tostring(entity), name, object_name)
      ai:abort()
   end

   -- The task must be running, so claim it by stopping it
   task:stop()

   -- Claim the task, so we know we've started. If we succeed, we'll set the task to nil.
   -- If we fail, and the task is still assigned, then we know we need to restart the task
   -- so that some other worker can pick it up.
   self._task = task

   -- Log successful grab
   radiant.log.info('%s (Worker %s): Hands off, guys! I have totally got this %s', tostring(entity), name, object_name)

   -- Was it a proxy item? If so, put it on the ghost, if not put it on the terrain.
   -- If it's on the ghost, it won't register on the terrain and provoke worker behavior
   self._proxy_component = proxy_entity:get_component('stonehearth:placeable_item_proxy')
   ai:execute('stonehearth:carry_item_on_path_to', path, ghost_entity)
   ai:execute('stonehearth:drop_carrying_in_entity', ghost_entity)
   radiant.entities.turn_to(proxy_entity, rotation)

   if self._proxy_component then
      --TODO: replace this with a particle effect
      ai:execute('stonehearth:run_effect', 'work')

      --Get the full sized entity
      local full_sized_entity = self._proxy_component:get_full_sized_entity()
      local faction = entity:get_component('unit_info'):get_faction()
      full_sized_entity:get_component('unit_info'):set_faction(faction)

      -- Remove the icon
      radiant.entities.destroy_entity(proxy_entity)

      -- Place the item in the world
      radiant.terrain.place_entity(full_sized_entity, radiant.entities.get_world_grid_location(ghost_entity))
      radiant.entities.turn_to(full_sized_entity, rotation)
   end

   --destroy the ghost entity
   radiant.entities.destroy_entity(ghost_entity)

   --If we got here, we succeeded at the action.  We can get rid of this task now.
   self._task:destroy()
   self._task = nil
end

--- When we're done with the action, stop the pathfinders and handle the task status
-- Destroy the task if it was successful. Start it if the task was unsuccessful
function WorkerPlaceItemAction:stop()
   -- If we were interrupted before we could destory the task, start it
   -- up again
   if self._task then
      self._task:start()
      self._task = nil
   end
end

return WorkerPlaceItemAction