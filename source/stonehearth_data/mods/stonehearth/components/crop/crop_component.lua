--[[
   Every time the crop grows, update its resource node resource. More mature crops 
   yield better resources.

   When we hear that the growth of this item is complete, 
   remove the old entity, put a new entity into the world in its place
   and destroy the old entity
]]
local CropComponent = class()

function CropComponent:__create(entity, json)
   self._entity = entity

   if json then
      if json.resource_pairings then
         self._resource_pairings = json.resource_pairings
      end
      if json.final_entity then
         self._final_entity = json.final_entity
         radiant.events.listen(self._entity, 'stonehearth:growth_complete', self, self._on_growth_complete)
         radiant.events.listen(self._entity, 'stonehearth:growing', self, self._on_grow_period)
      end
   end
end

--If we're 
function CropComponent:_on_grow_period(e)
   local stage = e.stage
   if self._resource_pairings[stage] then
      local resource_component = self._entity:get_component('stonehearth:resource_node')
      resource_component:set_resource(self._resource_pairings[stage])
   end
end

function CropComponent:_on_growth_complete()
   radiant.events.unlisten(self._entity, 'stonehearth:growing', self, self._on_grow_period)
   radiant.events.unlisten(self._entity, 'stonehearth:growth_complete', self, self._on_growth_complete)
   --local new_entity = radiant.entities.create_entity(self._final_entity)
   --local location = radiant.entities.get_world_grid_location(self._entity)
   --radiant.terrain.remove_entity(self._entity)
   --radiant.terrain.place_entity(new_entity, location)
   --radiant.events.trigger(self._entity, 'stonehearth:crop_swap', {final_crop = new_entity})
   --radiant.entities.destroy_entity(self._entity)

   --TODO: when this new crop is harvested, tell the ground it's standing on that that it's gone
end

return CropComponent