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
      ai:resume()
   end
   
   self._mover = _radiant.sim.create_follow_path(entity, speed, path, 0, arrived_fn)   
   ai:suspend()
end

function FollowPathAction:stop(ai, entity)
   if self._mover then
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
