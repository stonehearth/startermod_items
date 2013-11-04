local NoConstructionZoneComponent = class()

function NoConstructionZoneComponent:__init(entity, data_binding)
   self._entity = entity
   self._rgn = _radiant.client.alloc_region()
   self._data_binding = data_binding
end

return NoConstructionZoneComponent
