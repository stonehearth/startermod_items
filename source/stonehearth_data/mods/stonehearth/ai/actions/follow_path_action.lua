local FollowPath = require 'ai.lib.follow_path'
local Path = _radiant.sim.Path
local log = radiant.log.create_logger('follow_path')

local FollowPathAction = class()

FollowPathAction.name = 'follow path'
FollowPathAction.does = 'stonehearth:follow_path'
FollowPathAction.args = {
   path = Path,
   stop_distance = {
      type = 'number', -- stop when closer than this distance to the point of interest
      default = 0,     -- may be (and typically is) a floating point value
   },
}
FollowPathAction.version = 2
FollowPathAction.priority = 1

function FollowPathAction:start_thinking(ai, entity, args)
   local start_location = ai.CURRENT.location
   local distance_to_path_start = start_location:distance_to(args.path:get_start_point())
   local path_length = args.path:get_distance()
   local cost = distance_to_path_start + path_length   -- include the distance to the first point
   
   log:spam('cost of traversing path is distance from ai.CURRENT to start (%s -> %s = %.3f) and path distance (%.3f) = %.3f',
                      ai.CURRENT.location, args.path:get_start_point(), distance_to_path_start, path_length, cost)
   
   -- now update the entity position, set the cost, and notify the ai that we're ready to go!
   ai.CURRENT.location = args.path:get_finish_point()
   log:debug('setting CURRENT.location %s (path = %s) %s', tostring(ai.CURRENT.location), tostring(args.path), tostring(ai.CURRENT))
   
   ai:set_cost(cost)
   ai:set_think_output()
end

function FollowPathAction:run(ai, entity, args)
   if entity:add_component('mob'):get_parent() ~= radiant.entities.get_root_entity() then
      log:warning('cannot follow path because %s is not a child of the root entity', entity)
      ai:abort('cannot follow path because entity is not a child of the root entity')
   end

   self._ai = ai
   self._entity = entity
   local path = args.path

   log:detail('following path: %s', path)
   if path:is_empty() then
      log:detail('path is empty. returning')
      return
   end

   local speed = radiant.entities.get_world_speed(entity)
   local arrived = false
   local suspended = false

   local arrived_fn = function()
      arrived = true
      if suspended then 
         suspended = false
         ai:resume('mover finished')
      end
   end

   local aborted_fn = function()
      log:detail('mover aborted (path may no longer be traversable)')
      ai:abort('mover aborted')
   end
  
   self._mover = FollowPath(entity, speed, path)
      :set_stop_distance(args.stop_distance)
      :set_arrived_cb(arrived_fn)
      :set_aborted_cb(aborted_fn)
      :start()

   if arrived then
      log:detail('mover finished synchronously because entity is already at destination')
   else
      self._posture = radiant.entities.get_posture(entity)
      self._posture_listener = radiant.events.listen(entity, 'stonehearth:posture_changed', self, self._on_posture_changed)
      self._speed_listener = radiant.events.listen(entity, 'stonehearth:attribute_changed:speed', self, self._on_speed_changed)
      self:_start_move_effect()
      
      suspended = true
      ai:suspend('waiting for mover to finish')
   end

   -- stop isn't called until all actions in a compound action finish, so stop the effect here now
   self:stop(ai, entity, args)
end

function FollowPathAction:_on_speed_changed()
   if not self._entity:is_valid() then
      log:error('speed_changed listener should have been destroyed in stop')
   end
   
   assert(self._mover)
   local speed = radiant.entities.get_world_speed(self._entity)
   self._mover:set_speed(speed)
end

function FollowPathAction:_on_posture_changed()
   if not self._entity:is_valid() then
      log:error('posture_changed listener should have been destroyed in stop')
   end

   local new_posture = radiant.entities.get_posture(self._entity)
   if new_posture ~= self._posture then
      self._posture = new_posture
      self:_start_move_effect()
   end
end

function FollowPathAction:_start_move_effect()
   if self._move_effect then
      self._move_effect:stop()
   end

   -- make sure the event doesn't clean up after itself when the effect finishes.  otherwise,
   -- people will only play through the animation once.
   self._move_effect = radiant.effects.run_effect(self._entity, 'run')
      :set_cleanup_on_finish(false)
end

function FollowPathAction:stop(ai, entity)
   if self._speed_listener then
      self._speed_listener:destroy()
      self._speed_listener = nil
   end

   if self._posture_listener then
      self._posture_listener:destroy()
      self._posture_listener = nil
   end

   if self._mover then
      self._mover:stop()
      self._mover = nil
   end

   if self._move_effect then
      self._move_effect:stop()
      self._move_effect = nil
   end

   self._ai = nil
   self._entity = nil
   self._posture = nil
end

return FollowPathAction
