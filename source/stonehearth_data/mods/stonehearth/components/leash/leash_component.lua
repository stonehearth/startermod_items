local LeashComponent = class()

function LeashComponent:initialize(entity, json)
   radiant.check.is_entity(entity)
   self._entity = entity
   self._sv = {}
   self.__savestate = radiant.create_datastore(self._sv)
end

function LeashComponent:restore(entity, saved_variables)
   self.__savestate = saved_variables
   self._sv = saved_variables:get_data()
end

function LeashComponent:set_to_entity_location(entity)
   self:set_location(radiant.entities.get_location_aligned(entity))
end

function LeashComponent:get_location()
   return self._sv.position
end

function LeashComponent:set_location(point)
   self._sv.position = point
   self.__savestate:mark_changed()
end

function LeashComponent:get_radius()
   return self._sv.radius
end

function LeashComponent:set_radius(number)
   self._sv.radius = number
   self.__savestate:mark_changed()
end


return LeashComponent
