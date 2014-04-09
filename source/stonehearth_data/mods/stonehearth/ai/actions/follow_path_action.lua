local Path = _radiant.sim.Path
local FollowPathAction = class()

FollowPathAction.name = 'follow path'
FollowPathAction.does = 'stonehearth:follow_path'
FollowPathAction.args = {
   path = Path,          -- the path to follow
   stop_distance = {
      type = 'number',
      default = 0
   }
}
FollowPathAction.version = 2
FollowPathAction.priority = 1

function FollowPathAction:start_thinking(ai, entity, args)
   ai.CURRENT.location = args.path:get_finish_point()
   ai:get_log():debug('setting CURRENT.location %s (path = %s) %s', tostring(ai.CURRENT.location), tostring(args.path), tostring(ai.CURRENT))
   ai:set_think_output()
end

function FollowPathAction:run(ai, entity, args)
   local path = args.path
   local log = ai:get_log()
   self._ai = ai   
   
   log:detail('following path: %s', path)
   if path:is_empty() then
      log:detail('path is empty.  returning')
      return
   end

   radiant.events.listen(entity, 'stonehearth:posture_changed', self, self._on_posture_change)

   local speed = radiant.entities.get_speed(entity)

   -- make sure the event doesn't clean up after itself when the effect finishes.  otherwise,
   -- people will only play through the animation once.
   self._effect = radiant.effects.run_effect(entity, 'run')
                                    :set_cleanup_on_finish(false)
   local arrived_fn = function()
      if self._postures_trace then
         self._postures_trace:destroy()
         self._postures_trace = nil
      end
      ai:resume('mover finished')
   end
   
   self._mover = _radiant.sim.create_follow_path(entity, speed, path, args.stop_distance, arrived_fn)   
   ai:get_log():debug('starting mover %s...', self._mover:get_name());
   ai:suspend('waiting for mover to finish')

   -- manually stop now to terminate the effect and remove the posture
   self:stop(ai, entity, args)
end

function FollowPathAction:_on_posture_change()
   self._ai:abort('posture changed while following path')
end

function FollowPathAction:stop(ai, entity)
   radiant.events.unlisten(entity, 'stonehearth:posture_changed', self, self._on_posture_change)

   if self._mover then
      ai:get_log():debug('stopping mover %s in stop...', self._mover:get_name());
      self._mover:stop()
      self._mover = nil
   end
   if self._effect then
      self._effect:stop()
      self._effect = nil
   end
   if self._postures_trace then
      self._postures_trace:destroy()
      self._postures_trace = nil
   end
end

return FollowPathAction
