local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local Entity = _radiant.om.Entity
local RunTowardLocation = class()

RunTowardLocation.name = 'run to location'
RunTowardLocation.does = 'stonehearth:go_toward_location'
RunTowardLocation.args = {
   location = Point3,     -- location to go to
}
RunTowardLocation.version = 2
RunTowardLocation.priority = 1

function RunTowardLocation:run(ai, entity, args)
   local location = args.location
   local speed = radiant.entities.get_attribute(entity, 'speed')
   if speed == nil then
      speed = 1.0
   end

   --TODO: may need to reevaluate as we tweak attribute display
   if speed > 0 then
      speed = math.floor(50 + (50 * speed / 60)) / 100
   end

   self._effect = radiant.effects.run_effect(entity, 'run')
                                       :set_cleanup_on_finish(false)
   local arrived_fn = function()
      ai:resume('mover finished')
   end
   
   self._mover = _radiant.sim.create_goto_location(entity, speed, location, 0, arrived_fn)
   ai:suspend('waiting for mover to finish')

   -- manually stop now to terminate the effect and remove the posture
   self:stop(ai, entity, args)
end

function RunTowardLocation:stop(ai, entity)
   if self._mover then
      self._mover:stop()
      self._mover = nil
   end
   if self._effect then
      self._effect:stop()
      self._effect = nil
   end
end

return RunTowardLocation