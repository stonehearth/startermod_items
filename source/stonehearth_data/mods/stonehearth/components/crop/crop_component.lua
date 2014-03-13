--[[
   Every time the crop grows, update its resource node resource. More mature crops 
   yield better resources.
]]
local CropComponent = class()

function CropComponent:initialize(entity, json)
   self._entity = entity
   self._resource_pairings = json.resource_pairings
   radiant.events.listen(radiant.events, 'stonehearth:entity:post_create', self, self._on_create_complete)
end

--- When all components are created, double-check on whether this is a growing crop
function CropComponent:_on_create_complete()
   --If this is the sort of crop that will change as it grows
   if self._entity:get_component('stonehearth:growing') then
      radiant.events.listen(self._entity, 'stonehearth:growth_complete', self, self._on_growth_complete)
      radiant.events.listen(self._entity, 'stonehearth:growing', self, self._on_grow_period)
   end
   return radiant.events.UNLISTEN
end

--- As we grow, change the resources we yield
function CropComponent:_on_grow_period(e)
   local stage = e.stage
   if self._resource_pairings[stage] then
      local resource_component = self._entity:get_component('stonehearth:resource_node')
      resource_component:set_resource(self._resource_pairings[stage])
   end
end

--TODO: is growth ever really complete? Design the difference between "can't continue" and "growth complete"
function CropComponent:_on_growth_complete()
   radiant.events.unlisten(self._entity, 'stonehearth:growing', self, self._on_grow_period)
   return radiant.events.UNLISTEN
end

return CropComponent