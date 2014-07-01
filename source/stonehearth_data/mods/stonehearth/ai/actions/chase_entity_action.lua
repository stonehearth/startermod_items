local Path = _radiant.sim.Path
local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
local ChaseEntity = class()
local log = radiant.log.create_logger('chase_entity')

-- ChaseEntity attempts to catch a moving target by following an initial (nonlinear) path
-- to the target and recomputing direct (line of sight) paths as the target moves.
-- ChaseEntity gives up when reaching the end of the inital path or a subsequent direct path
-- that was found

ChaseEntity.name = 'chase entity'
ChaseEntity.does = 'stonehearth:chase_entity'
ChaseEntity.args = {
   target = Entity,
   stop_distance = {
      type = 'number',
      default = 1,
   },
   move_effect = {
      type = 'string',
      default = 'run',
   },
   grid_location_changed_cb = {  -- triggered as the chasing entity changes grid locations
      type = 'function',
      default = stonehearth.ai.NIL,
   },
}
ChaseEntity.version = 2
ChaseEntity.priority = 1

function ChaseEntity:start_thinking(ai, entity, args)
   if not args.target:is_valid() then
      ai:abort('Invalid target')
   end

   self._selected_to_run = false
   self._moving = false
   self._ai = ai
   self._entity = entity
   self._target = args.target
   self._target_location_history = {}

   self:_trace_target_location()

   local path = self:_get_direct_path(self._entity, self._target, ai.CURRENT.location)

   if path then
      -- only an approximate location as the target may move or we may exit early based on stop_distance
      ai.CURRENT.location = path:get_finish_point()
      self:_set_think_output(path)
      return
   end

   -- no direct path, wait for a solution from the pathfinder
   self:_get_astar_path(self._entity, self._target, ai.CURRENT.location)
end

function ChaseEntity:_trace_target_location()
   local target_mob = self._target:add_component('mob')
   local target_last_location = self._target:add_component('mob'):get_world_grid_location()

   local on_target_moved = function()
      local target_current_location = target_mob:get_world_grid_location()

      if target_current_location ~= target_last_location then
         -- include target_current_location in target_location_history before recalculating path
         table.insert(self._target_location_history, target_current_location)
         target_last_location = target_current_location

         -- don't waste cycles recalculating the path if we haven't started running yet,
         -- especially since a future solution may make the current solution obsolete
         if self._moving then
            local found_path = self:_try_recalculate_path()
            if found_path then
               -- restart the mover using the new path
               self:_destroy_mover()
               self._ai:resume()
            end
         end
      end
   end

   local on_target_destroyed = function()
      -- no guarantee that stop will be called on abort, call if explicitly
      local ai = self._ai
      self:stop()
      ai:abort('Target destroyed')
   end

   self._trace_target = radiant.entities.trace_location(self._target, 'chase entity')
      :on_changed(on_target_moved)
      :on_destroyed(on_target_destroyed)
end

function ChaseEntity:_try_recalculate_path()
   local unused_points = {}
   local path = self:_search_for_new_path(self._entity, self._target, self._target_location_history, unused_points)

   if path ~= nil then
      log:info('New path to target %s found %s -> %s', self._target, path:get_start_point(), path:get_finish_point())
      self._path = path
      self._target_location_history = unused_points
      return true
   else
      log:info('No direct path to target %s at found.', self._target)
      return false
   end
end

-- don't use astar_path_finder here because it could stall for a long time, leaving the entity frozen
function ChaseEntity:_search_for_new_path(entity, target, target_location_history, unused_points)
   local path, location, num_locations, i, n

   -- check for a direct path to the target
   path = self:_get_direct_path(entity, target)

   if path == nil then
      -- check for a direct path to the prior locations (most recent first)
      num_locations = #target_location_history

      -- if num_locations is too large, we could downsample the locations
      for i = num_locations, 1, -1 do
         location = target_location_history[i]
         path = self:_get_direct_path(entity, location)

         if path ~= nil then
            -- why doesn't a for loop work here?
            n = i + 1
            while n <= num_locations do
               table.insert(unused_points, target_location_history[n])
               n = n + 1
            end
            break
         end
      end
   end

   return path
end

-- destination may be an Entity or a Point3
function ChaseEntity:_get_direct_path(entity, destination, start_location_override)
   local direct_path_finder = _radiant.sim.create_direct_path_finder(entity)
                                 :set_reversible_path(true)

   if radiant.util.is_a(destination, Entity) then
      direct_path_finder:set_destination_entity(destination)
   elseif radiant.util.is_a(destination, Point3) then
      direct_path_finder:set_end_location(destination)
   else
      error('Destination must be an Entity or a Point3')
   end

   if start_location_override ~= nil then
      direct_path_finder:set_start_location(start_location_override)
   end

   return direct_path_finder:get_path()
end

-- destination must be an Entity
function ChaseEntity:_get_astar_path(entity, destination, start_location_override)
   radiant.check.is_entity(destination)

   local on_solved = function (path)
      self:_set_think_output(path)
   end

   self._path_finder = _radiant.sim.create_astar_path_finder(entity, 'chase entity')
                         :add_destination(destination)
                         :set_solved_cb(on_solved)

   if start_location_override ~= nil then
      self._path_finder:set_source(self._ai.CURRENT.location)
   end

   self._path_finder:start()
end

function ChaseEntity:_set_think_output(path)
   self._path = path
   self._ai:set_think_output()
end

function ChaseEntity:stop_thinking(ai, entity, args)
   self:_destroy_path_finder()

   if not self._selected_to_run then
      self:_destroy_trace_target()
      self:_clean_up_references()
   end
end

function ChaseEntity:start(ai, entity, args)
   self._selected_to_run = true
end

function ChaseEntity:run(ai, entity, args)
   local speed = radiant.entities.get_world_speed(entity)
   local finished = false

   ai:set_status_text('chasing ' .. radiant.entities.get_name(args.target))

   -- trace entity location before checking early exit, so that we fire the callback at least once
   self:_trace_entity_location(args.grid_location_changed_cb)

   -- FollowPath stops on grid positions, so don't move if we're within a half cell
   -- otherwise this will cause bouncing when we call BumpAgainstEntity
   local distance = radiant.entities.distance_between(entity, args.target)
   if distance < args.stop_distance + 0.5 then
      return
   end

   self:_start_run_effect(entity, args.move_effect)
   self._moving = true

   while not finished do
      local stop_distance
      local is_partial_path = next(self._target_location_history) ~= nil

      if is_partial_path then
         -- path doesn't reach destination, so stop distance isn't well defined
         stop_distance = 0
      else
         stop_distance = args.stop_distance
      end

      -- we want to ai:execute('stonehearth:follow_path'), but need the ability to terminate the move early
      self._mover = _radiant.sim.create_follow_path(entity, speed, self._path, stop_distance,
         function ()
            if is_partial_path then
               -- we only recalculate the path if the target moves.
               -- if it stopped when there was no direct path, try one last time to reach it.
               local found_path = self:_try_recalculate_path()
               -- we're done if no path could be found. let start_thinking find an a_star path or new target.
               finished = not found_path
            else
               -- path was complete and mover finished. yay!
               finished = true
            end

            ai:resume('Mover finished')
         end
      )

      ai:suspend('Waiting for mover to finish')
   end
end

function ChaseEntity:_trace_entity_location(callback)
   local mob = self._entity:add_component('mob')
   local last_location = self._entity:add_component('mob'):get_world_grid_location()

   local on_moved = function()
      local current_location = mob:get_world_grid_location()
      if current_location ~= last_location then
         callback()
      end
      last_location = current_location
   end

   self._trace_entity = radiant.entities.trace_location(self._entity, 'chase entity')
      :on_changed(on_moved)
      :push_object_state()
end

function ChaseEntity:_start_run_effect(entity, move_effect)
   if not self._run_effect then
      -- make sure the event doesn't clean up after itself when the effect finishes.
      -- otherwise, people will only play through the animation once.
      self._run_effect = radiant.effects.run_effect(entity, move_effect)
         :set_cleanup_on_finish(false)
   end
end

function ChaseEntity:stop(ai, entity, args)
   self._selected_to_run = false
   self._moving = false
   self:_clean_up_references()
   self:_destroy_trace_entity()
   self:_destroy_trace_target()
   self:_destroy_mover()
   self:_destroy_run_effect()
   self:_destroy_path_finder()
end

function ChaseEntity:_clean_up_references()
   self._ai = nil
   self._entity = nil
   self._target = nil
   self._target_location_history = nil
   self._path = nil
end

function ChaseEntity:_destroy_trace_entity()
   if self._trace_entity then
      self._trace_entity:destroy()
      self._trace_entity = nil
   end
end

function ChaseEntity:_destroy_trace_target()
   if self._trace_target then
      self._trace_target:destroy()
      self._trace_target = nil
   end
end

function ChaseEntity:_destroy_mover()
   if self._mover then
      self._mover:stop()
      self._mover = nil
   end
end

function ChaseEntity:_destroy_run_effect()
   if self._run_effect then
      self._run_effect:stop()
      self._run_effect = nil
   end
end

function ChaseEntity:_destroy_path_finder()
   if self._path_finder then
      self._path_finder:stop()
      self._path_finder = nil
   end
end

return ChaseEntity
