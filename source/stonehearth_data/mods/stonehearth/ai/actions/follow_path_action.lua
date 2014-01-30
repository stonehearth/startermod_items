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

   if path:is_empty() then
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
      speed = 100
   end
   speed = speed / 100.0

   self._effect = radiant.effects.run_effect(entity, 'run')
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

function FollowPathAction:destroy(ai, entity)
   if self._mover then
      ai:get_log():debug('stopping mover %s in destroy...', self._mover:get_name());
      self._mover:stop()
      self._mover = nil
   end
end

return FollowPathAction
