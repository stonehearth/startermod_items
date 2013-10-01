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
   self._success = nil    --True only if the action completes before it is stopped
end

--- Pick up and place the item designated by the player
-- TODO: if the action is stopped, (ie, goblins attack!) should I drop the thing I'm carrying?
-- @param path The path to the object
-- @param drop_location The place to drop the entity
-- @param rotation The degree at which the object should appear when it's placed
-- @param task The task, if this is fired by a worker scheduler
function WorkerPlaceItemAction:run(ai, entity, path, drop_location, rotation, task)
   self._task = task

   -- Put in some logging
   local name = entity:get_component('unit_info'):get_display_name()
   local target_item = path:get_destination()
   local object_name = target_item:get_component('unit_info'):get_display_name()

   -- If the task is already stopped, someone else got this action first. Exit.
   if not task:get_running() then
      radiant.log.info('Worker %s: Never mind! You got the %s.', name, object_name)
      return
   end

   -- The task must be running, so claim it by stopping it
   task:stop()

   -- Set success to false, because we're starting but haven't finished yet
   -- Make sure to set this only after we check to see if the task is running;
   -- If we fail at the task, that way we know it was the person who had claimed the
   -- task that failed.
   self._success = false

   -- Log successful grab
   radiant.log.info('Worker %s: Hands off, guys! I have totally got this %s', name, object_name)

   -- We already have a path to the object, so set up a pathfinder
   -- between the object and its final destination, to use later.
   local temp_target, temp_dest_target = self:_get_intermediary_path(target_item, drop_location)
   ai:execute('stonehearth.pickup_item_on_path', path)

   -- If the carried item doesn't exist, it moved before we could pick it up.
   -- TODO: Look confused.
   local proxy_entity = radiant.entities.get_carrying(entity)
   if not proxy_entity then
      return
   end
   -- If we're here, pickup succeeded, so we're now carrying the item.
   -- Wait until the PF we started earlier returns
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
      radiant.terrain.remove_entity(proxy_entity)
      radiant.entities.destroy_entity(proxy_entity)

      -- Place the item in the world
      radiant.terrain.place_entity(full_sized_entity, drop_location)
      radiant.entities.turn_to(full_sized_entity, rotation)
   end

   -- destroy the other entities
   self:_destroy_temp_entities(temp_target, temp_dest_target)

   --If we got here, we succeeded at the action
   self._success = true
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
   local temp_entity = radiant.entities.create_entity()
   radiant.terrain.place_entity(temp_entity, target_item_loc)

   local temp_dest_entity = radiant.entities.create_entity()
   radiant.terrain.place_entity(temp_dest_entity, final_location)

   --When the path is solved, save the path so we can get it elsewhere
   local solved_cb = function(path)
      self._path_to_destination = path
   end

   local desc = string.format('finding a path from %s to player-designated location', tostring(target_item))
   self._pathfinder = radiant.pathfinder.find_path_to_entity(desc, temp_entity, temp_dest_entity, solved_cb)

   return temp_entity, temp_dest_entity
end

function WorkerPlaceItemAction:_destroy_temp_entities(temp_intermediate, temp_destination)
   -- Remove the intermediate entity
   radiant.terrain.remove_entity(temp_intermediate)
   radiant.entities.destroy_entity(temp_intermediate)

   -- Remove the fake destination entity (pf only works between entities)
   radiant.terrain.remove_entity(temp_destination)
   radiant.entities.destroy_entity(temp_destination)

end

--- When we're done with the action, stop the pathfinders and handle the task status
-- Destroy the task if it was successful. Start it if the task was unsuccessful
function WorkerPlaceItemAction:stop()
   if self._pathfinder then
      self._pathfinder:stop()
      self._pathfinder = nil
   end
   self._path_to_destination = nil

   -- If we were successful, then destroy the task (we know it's a one-time task)
   if self._success == true then
      self._task:destroy()
   else
      -- If we're stopping but the task wasn't successful, then start it again so some other worker can get it
      if self._success == false then
         self._task:start()
      end
   end
   self._success = nil
   return self
end

return WorkerPlaceItemAction