local Path = _radiant.sim.Path
local FollowPathAction = class()

FollowPathAction.name = 'follow path'
FollowPathAction.does = 'stonehearth:follow_path'
FollowPathAction.args = {
   path = Path,          -- the path to follow
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
   
   log:detail('following path: %s', path)
   if path:is_empty() then
      log:detail('path is empty.  returning')
      return
   end
   
   local postures = entity:get_component('stonehearth:posture')
   if postures then
      self._postures_trace = postures:trace('follow path')
         :on_changed(function()
            ai:abort('posture changed while following path')
         end)
   end

   local speed = radiant.entities.get_attribute(entity, 'speed')
   if speed == nil then
      speed = 1.0
   end

   --TODO: may need to reevaluate as we tweak attribute display
   if speed > 0 then
      speed = math.floor(50 + (50 * speed / 60)) / 100
   end

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
   
   self._mover = _radiant.sim.create_follow_path(entity, speed, path, 0, arrived_fn)   
   ai:get_log():debug('starting mover %s...', self._mover:get_name());
   ai:suspend('waiting for mover to finish')

   -- manually stop now to terminate the effect and remove the posture
   self:stop(ai, entity, args)
end

function FollowPathAction:stop(ai, entity)
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
