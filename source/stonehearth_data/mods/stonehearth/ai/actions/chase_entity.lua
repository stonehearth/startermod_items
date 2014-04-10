local Path = _radiant.sim.Path
local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
local ChaseEntity = class()

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

function ChaseEntity:run(ai, entity, args)
   local speed = radiant.entities.get_speed(entity)
   local stop_distance = args.stop_distance
   local target = args.target
   local target_mob = target:add_component('mob')
   local arrived = false
   local direct_path_finder, path, restart_pathfinder
   local target_start_location, target_current_location, distance

   local on_target_moved = function()
      target_current_location = target_mob:get_world_grid_location()
      distance = radiant.entities.distance_between(target_current_location, target_start_location)
      if distance > 2 then
         restart_pathfinder = true
         self:_destroy_pathfinder()
         self:_destroy_mover()
         ai:resume()
      end
   end

   local on_target_destroyed = function()
      -- currently no guarantee that stop will be called
      self:stop()
      ai:abort('target destroyed')
   end

   self._trace = radiant.entities.trace_location(target, 'chase entity')
      :on_changed(on_target_moved)
      :on_destroyed(on_target_destroyed)

   while not arrived do
      repeat
         restart_pathfinder = false
         target_start_location = target_mob:get_world_grid_location()
         path = self:_find_path(ai, entity, target)
      until not restart_pathfinder

      if path == nil then
         -- currently no guarantee that stop will be called
         self:stop()
         ai:abort('No path to entity')
         return
      end

      self:_start_run_effect(entity)

      self._mover = _radiant.sim.create_follow_path(entity, speed, path, stop_distance,
         function ()
            arrived = true
            ai:resume('mover finished')
         end
      )

      ai:suspend('waiting for mover to finish')

      self:_destroy_mover()
   end
end

function ChaseEntity:_find_path(ai, entity, target)
   local path, direct_path_finder

   -- if we have an unobstructed path, return it immediately
   direct_path_finder = _radiant.sim.create_direct_path_finder(entity, target)
                           :set_reversible_path(true)

   path = direct_path_finder:get_path()

   if path then
      return path
   end

   -- no direct path, wait for a solution from the pathfinder
   local on_solved = function (solved_path)
      path = solved_path
      ai:resume()
   end

   local on_failed = function ()
      ai:resume()
   end

   self._pathfinder = _radiant.sim.create_path_finder(entity, 'chase entity')
                         :add_destination(target)
                         :set_solved_cb(on_solved)
                         :set_search_exhausted_cb(on_failed)

   ai:suspend('waiting for pathfinder to finish')

   self:_destroy_pathfinder()

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
   self:_destroy_trace()
   self:_destroy_mover()
   self:_destroy_run_effect()
   self:_destroy_pathfinder()
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

function ChaseEntity:_destroy_pathfinder()
   if self._pathfinder then
      self._pathfinder:stop()
      self._pathfinder = nil
   end
end

return ChaseEntity
