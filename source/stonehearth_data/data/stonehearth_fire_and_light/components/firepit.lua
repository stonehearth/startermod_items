local Point3 = _radiant.csg.Point3

radiant.mods.require('stonehearth_calendar')

local Firepit = class()

function Firepit:__init(entity, data_binding)
   radiant.check.is_entity(entity)
   self._entity = entity

   self._is_lit = false
   self._my_wood = nil

   radiant.events.listen('radiant.events.calendar.sunrise', self)
   radiant.events.listen('radiant.events.calendar.sunset', self)
end

--[[ At sunset, tell a worker to fill the fire with wood, and light it.
     At dawn, tell a worker to extingish the fire
     TODO: right now, we automatically populate with wood.
     TODO: what about cooking, for which the firepit would need to stay lit during the day?
     I suppose a cook could check if it's lit, and when it's not, fill it again.
     Note: this could have been modeled as an action, but the fire never
     really decides to have different kinds of behavior. It's pretty static.
]]
Firepit['radiant.events.calendar.sunset'] = function (self)
   self:light_fire()
end

Firepit['radiant.events.calendar.sunrise'] = function (self)
   self:extinguish()
end

function Firepit:light_fire()
   self._my_wood = radiant.entities.create_entity('stonehearth_trees', 'oak_log')
   radiant.entities.add_child(self._entity, self._my_wood, Point3(0, 0, 0))
end

function Firepit:extinguish()
   if self._my_wood then
      radiant.entities.remove_child(self._entity, self._my_wood)
      radiant.entities.destroy_entity(self._my_wood)
      self._my_wood = nil
   end
end

return Firepit