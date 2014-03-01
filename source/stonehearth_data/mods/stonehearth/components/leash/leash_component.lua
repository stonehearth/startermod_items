local LeashComponent = class()

function LeashComponent:__create(entity, json)
   radiant.check.is_entity(entity)
   self._entity = entity
   self._info = json
   self.__savestate = radiant.create_datastore(self._info)
end


function LeashComponent:set_to_entity_location(entity)
   self:set_location(radiant.entities.get_location_aligned(entity))
end

function LeashComponent:get_location()
   return self._info.position
end

function LeashComponent:set_location(point)
   self._info.position = point
   self.__savestate:mark_changed()
end

function LeashComponent:get_radius()
   return self._info.radius
end

function LeashComponent:set_radius(number)
   self._info.radius = number
   self.__savestate:mark_changed()
end


return LeashComponent
