--[[
   When we hear that the growth of this item is complete, 
   remove the old entity, put a new entity into the world in its place
   and destroy the old entity
]]
local CropComponent = class()

function CropComponent:__create(entity, json)
   self._entity = entity

   if json then
      if json.final_entity then
         self._final_entity = json.final_entity
         radiant.events.listen(self._entity, 'stonehearth:growth_complete', self, self._on_growth_complete)
      end
   end

end

function CropComponent:_on_growth_complete()
   radiant.events.unlisten(self._entity, 'stonehearth:growth_complete', self, self._on_growth_complete)
   local new_entity = radiant.entities.create_entity(self._final_entity)
   local location = radiant.entities.get_world_grid_location(self._entity)
   radiant.terrain.remove_entity(self._entity)
   radiant.terrain.place_entity(new_entity, location)
   radiant.events.trigger(self._entity, 'stonehearth:crop_swap', {final_crop = new_entity})
   radiant.entities.destroy_entity(self._entity)

   --TODO: when this new crop is harvested, tell the ground it's standing on that that it's gone
end

return CropComponent