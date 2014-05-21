local Point3 = _radiant.csg.Point3

local GetPatrolPoint = class()
GetPatrolPoint.name = 'get patrol point'
GetPatrolPoint.does = 'stonehearth:get_patrol_point'
GetPatrolPoint.args = {}
GetPatrolPoint.think_output = {
   destination = Point3
}
GetPatrolPoint.version = 2
GetPatrolPoint.priority = 1

function GetPatrolPoint:start_thinking(ai, entity, args)
   self._entity = entity
   self._ai = ai

   self:_check_for_patrol_point()

   self._listening = true
   -- CHECKCHECK - trigger this
   radiant.events.listen(stonehearth.town_defense, 'stonehearth:patrol_point_available', self, self._check_for_patrol_point)
end

function GetPatrolPoint:stop_thinking(ai, entity, args)
   if self._listening then
      radiant.events.unlisten(stonehearth.town_defense, 'stonehearth:patrol_point_available', self, self._check_for_patrol_point)
      self._listening = false
   end

   self._entity = nil
   self._ai = nil
end

function GetPatrolPoint:_check_for_patrol_point()
   local destination = stonehearth.town_defense:get_patrol_point(self._entity)
   if destination then
      self._ai:set_think_output({ destination = destination })
      self._listening = false
      return radiant.events.UNLISTEN
   end
end

return GetPatrolPoint
