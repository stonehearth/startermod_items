local NoConstructionZoneComponent = class()

function NoConstructionZoneComponent:__init(entity, json)
   self._entity = entity
   self._rgn = _radiant.client.alloc_region()
end

return NoConstructionZoneComponent
