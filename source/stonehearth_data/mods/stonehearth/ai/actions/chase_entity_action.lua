local Path = _radiant.sim.Path
local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
local ChaseEntity = class()

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
      default = 1
   }
}
ChaseEntity.version = 2
ChaseEntity.priority = 1

function ChaseEntity:start_thinking(ai, entity, args)
   local target = args.target
   local path

   self._running = false
   self._ai = ai

   path = self:_get_direct_path(entity, target, ai.CURRENT.location)

   if path then
      self:_set_think_output(path, target)
      return
   end

   -- no direct path, wait for a solution from the pathfinder
   self:_get_astar_path(entity, target, ai.CURRENT.location)
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
      error('destination must be an Entity or a Point3')
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
      self:_set_think_output(path, destination)
   end

   self._path_finder = _radiant.sim.create_astar_path_finder(entity, 'chase entity')
                         :add_destination(destination)
                         :set_solved_cb(on_solved)

   if start_location_override ~= nil then
      self._path_finder:set_source(self._ai.CURRENT.location)
   end

   self._path_finder:start()
end

function ChaseEntity:_set_think_output(path, target)
   self._path = path
   self._target_last_location = target:add_component('mob'):get_world_grid_location()
   self._ai:set_think_output()
end

function ChaseEntity:stop_thinking(ai, entity, args)
   self:_destroy_path_finder()
   self:_clean_up_references()
end

function ChaseEntity:start(ai, entity, args)
   self._running = true
end

function ChaseEntity:run(ai, entity, args)
   local speed = radiant.entities.get_speed(entity)
   local stop_distance = args.stop_distance
   local target = args.target
   local target_mob = target:add_component('mob')
   local target_current_location, new_path
   local target_previous_locations = {}
   local finished = false

   local on_target_moved = function()
      target_current_location = target_mob:get_world_grid_location()

      if target_current_location ~= self._target_last_location then

         new_path = self:_find_run_time_path(entity, target, target_previous_locations)

         self._target_last_location = target_current_location

         if new_path ~= nil then
            self._path = new_path
            target_previous_locations = {}
            self:_destroy_mover()
            ai:resume()
         else
            -- track target's path so we can follow or intercept
            table.insert(target_previous_locations, target_current_location)
         end
      end
   end

   local on_target_destroyed = function()
      -- no guarantee that stop will be called on abort, call if explicitly
      self:stop()
      ai:abort('target destroyed')
   end

   self._trace = radiant.entities.trace_location(target, 'chase entity')
      :on_changed(on_target_moved)
      :on_destroyed(on_target_destroyed)

   -- check if target has moved between start_thinking and run
   on_target_moved()

   self:_start_run_effect(entity)

   while not finished do
      self._mover = _radiant.sim.create_follow_path(entity, speed, self._path, stop_distance,
         function ()
            finished = true
            ai:resume('mover finished')
            -- being finished doesn't mean we were able to catch the target
            -- could follow target's previous locations from here, but right now we prefer 
            -- to bail and let a higher level function decide what to do (e.g. rethink)
         end
      )

      ai:suspend('waiting for mover to finish')
   end
end

-- don't use astar_path_finder here because it could stall for a long time, leaving the entity frozen
function ChaseEntity:_find_run_time_path(entity, target, target_previous_locations)
   local path, location, num_locations

   -- check for a direct path to the target
   path = self:_get_direct_path(entity, target)

   if path == nil then
      -- check for a direct path to the prior locations (most recent first)
      num_locations = #target_previous_locations

      -- if num_locations is too large, we could downsample the locations
      for i = num_locations, 1, -1 do
         location = target_previous_locations[i]
         path = self:_get_direct_path(entity, location)

         if path ~= nil then
            break
         end
      end
   end

   return path
end

function ChaseEntity:_start_run_effect(entity)
   if not self._run_effect then
      -- make sure the event doesn't clean up after itself when the effect finishes.
      -- otherwise, people will only play through the animation once.
      self._run_effect = radiant.effects.run_effect(entity, 'run')
                           :set_cleanup_on_finish(false)
   end
end

function ChaseEntity:stop(ai, entity, args)
   self._running = false
   self:_clean_up_references()
   self:_destroy_trace()
   self:_destroy_mover()
   self:_destroy_run_effect()
   self:_destroy_path_finder()
end

function ChaseEntity:_clean_up_references()
   if not self._running then
      self._ai = nil
      self._path = nil
      self._target_last_location = nil
   end
end

function ChaseEntity:_destroy_trace()
   if self._trace then
      self._trace:destroy()
      self._trace = nil
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
