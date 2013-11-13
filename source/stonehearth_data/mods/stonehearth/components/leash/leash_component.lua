radiant.mods.load('stonehearth')

local LeashComponent = class()

function LeashComponent:__init(entity, data_store)
   radiant.check.is_entity(entity)
   self._entity = entity
   self._info = {}
   self._data_store = data_store
end

-- local promise = leash:get_data_store():trace('reason why tracking')
-- promise:on_changed(function() end)
--   later...
-- promise:destroy()

function LeashComponent:get_data_store(json)
   return self._data_store
end

function LeashComponent:extend(json)
   self:set_info(json)
end

function LeashComponent:set_info(info)
   self._info = info
   self._data_store:update(self._info)
end

function LeashComponent:set_to_entity_location(entity)
   self:set_location(radiant.entities.get_location_aligned(entity))
end

function LeashComponent:get_location()
   return self._info.position
end

function LeashComponent:set_location(point)
   self._info.position = point
   self._data_store:mark_changed()
end

function LeashComponent:get_radius()
   return self._info.radius
end

function LeashComponent:set_radius(number)
   self._info.radius = number
   self._data_store:mark_changed()
end


return LeashComponent
