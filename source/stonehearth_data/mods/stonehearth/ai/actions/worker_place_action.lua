local Point3 = _radiant.csg.Point3
local WorkerPlaceItemAction = class()

WorkerPlaceItemAction.name = 'stonehearth.place_item'
WorkerPlaceItemAction.does = 'stonehearth.place_item'
WorkerPlaceItemAction.priority = 10
--TODO we need a scale to  describe relative importance.
--Right now all tasks dispatched by worker scheduler have the same priority, 10
--They override the given priority

function WorkerPlaceItemAction:__init(ai, entity)
   self._ai = ai
   self._entity = entity
   self._path_to_destination = nil
   self._temp_entity = nil
   self._temp_dest_entity = nil
end

--- Pick up and place the item designated by the player
-- TODO: if the action is stopped, (ie, goblins attack!) should I drop the thing I'm carrying?
-- @param path The path to the object
-- @param drop_location The place to drop the entity
-- @param rotation The degree at which the object should appear when it's placed
-- @param task The task, if this is fired by a worker scheduler
function WorkerPlaceItemAction:run(ai, entity, path, drop_location, rotation, task)
   -- Put in some logging
   local name = entity:get_component('unit_info'):get_display_name()
   local target_item = path:get_destination()
   local object_name = target_item:get_component('unit_info'):get_display_name()

   -- If the task is already stopped, someone else got this action first. Exit.
   if not task:get_running() then
      radiant.log.info('Worker %s: Never mind! You got the %s.', name, object_name)
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
   radiant.log.info('Worker %s: Hands off, guys! I have totally got this %s', name, object_name)

   -- We already have a path to the object, so set up a pathfinder
   -- between the object and its final destination, to use later.
   self:_get_intermediary_path(target_item, drop_location)
   ai:execute('stonehearth.pickup_item_on_path', path)

   -- If we're here, pickup succeeded, so we're now carrying the item.
   -- Wait until the PF we started earlier returns
   local proxy_entity = radiant.entities.get_carrying(entity)
   ai:wait_until(function()
      return self._path_to_destination ~= nil
   end)
   ai:execute('stonehearth.follow_path', self._path_to_destination)
   ai:execute('stonehearth.drop_carrying', drop_location)
   radiant.entities.turn_to(proxy_entity, rotation)

   -- if we placed a proxy entity then replace the proxy we dropped with a real thing
   local proxy_component = proxy_entity:get_component('stonehearth:placeable_item_proxy')
   if proxy_component then
      --TODO: replace this with a particle effect
      ai:execute('stonehearth.run_effect', 'work')

      --Get the full sized entity
      local full_sized_entity = proxy_component:get_full_sized_entity()
      local faction = entity:get_component('unit_info'):get_faction()
      full_sized_entity:get_component('unit_info'):set_faction(faction)

      -- Remove the icon
      radiant.entities.destroy_entity(proxy_entity)

      -- Place the item in the world
      radiant.terrain.place_entity(full_sized_entity, drop_location)
      radiant.entities.turn_to(full_sized_entity, rotation)
   end

   --If we got here, we succeeded at the action.  We can get rid of this task now.
   self._task:destroy()
   self._task = nil
end

--- Make a pathfinder between the target item and the final destination
-- Since the pf can only path between 2 entities, and since the target_item may be
-- picked up by the worker before the pf returns, create a temporary entity to serve
-- as the starting destiantion and the final destination
-- @param target_item the item that the worker is planning to pick up
-- @param final_location The location the player wanted to put the item
-- @return temporary destination entity that will serve as the starting poitn of the path
function WorkerPlaceItemAction:_get_intermediary_path(target_item, final_location)
   local target_item_loc = target_item:get_component('mob'):get_world_grid_location()
   self._temp_entity = radiant.entities.create_entity()
   radiant.terrain.place_entity(self._temp_entity, target_item_loc)

   self._temp_dest_entity = radiant.entities.create_entity()
   radiant.terrain.place_entity(self._temp_dest_entity, final_location)

   --When the path is solved, save the path so we can get it elsewhere
   local solved_cb = function(path)
      self._path_to_destination = path
   end

   local desc = string.format('finding a path from %s to player-designated location', tostring(target_item))
   self._pathfinder = radiant.pathfinder.create_path_finder(desc)
                        :set_source(self._temp_entity)
                        :add_destination(self._temp_dest_entity)
                        :set_solved_cb(solved_cb)
   --self._pathfinder = radiant.pathfinder.find_path_to_entity(desc, self._temp_entity, self._temp_dest_entity, solved_cb)
end

function WorkerPlaceItemAction:_destroy_temp_entities()
   if self._pathfinder then
      self._pathfinder:stop()
      self._pathfinder = nil
   end
   self._path_to_destination = nil

   if self._temp_entity then
      -- Remove the intermediate entity
      radiant.entities.destroy_entity(self._temp_entity)
   end

   if self._temp_dest_entity then
      -- Remove the fake destination entity (pf only works between entities)
      radiant.entities.destroy_entity(self._temp_dest_entity)
   end

   self._temp_intermediate = nil
   self._temp_destination = nil
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
   self:_destroy_temp_entities()
end

return WorkerPlaceItemAction