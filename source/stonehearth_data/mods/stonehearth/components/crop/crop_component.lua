--[[
   Every time the crop grows, update its resource node resource. More mature crops 
   yield better resources.
]]
local CropComponent = class()

function CropComponent:initialize(entity, json)
   self._entity = entity
   self._resource_pairings = json.resource_pairings
   self._harvest_threshhold = json.harvest_threshhold

   self._sv = self.__saved_variables:get_data()
   if not self._sv._initialized then
      self._sv._initialized = true
      self._sv.harvestable = false
   else
      --We're loading
      radiant.events.listen(radiant, 'radiant:game_loaded', function(e)
            if self._sv.harvestable then
               radiant.events.trigger(self._entity, 'stonehearth:crop_harvestable', {crop = self._entity})
            end
            return radiant.events.UNLISTEN
         end)
   end

   radiant.events.listen(entity, 'radiant:entity:post_create', self, self._on_create_complete)
end

function CropComponent:get_product()
   return self._sv.product
end

function CropComponent:set_dirt_plot(dirt_plot)
   self._dirt_plot = dirt_plot
end

function CropComponent:get_dirt_plot()
   return self._dirt_plot
end

function CropComponent:destroy()
   radiant.events.unlisten(self._entity, 'stonehearth:growing', self, self._on_grow_period)
end

--- When all components are created, double-check on whether this is a growing crop
function CropComponent:_on_create_complete()
   --If this is the sort of crop that will change as it grows
   if self._entity:get_component('stonehearth:growing') then
      radiant.events.listen(self._entity, 'stonehearth:growing', self, self._on_grow_period)
   end
   return radiant.events.UNLISTEN
end

--- As we grow, change the resources we yield and, if appropriate, command harvest
function CropComponent:_on_grow_period(e)
   self._sv.stage = e.stage
   if e.stage then
      local resource_pairing_uri = self._resource_pairings[self._sv.stage]
      if resource_pairing_uri then
         if resource_pairing_uri == "" then
            resource_pairing_uri = nil
         end
         self._sv.product = resource_pairing_uri
      end
      if self._sv.stage == self._harvest_threshhold then
         self._sv.harvestable = true
         radiant.events.trigger(self._entity, 'stonehearth:crop_harvestable', {crop = self._entity})
      end
   end
   if e.finished then
      --TODO: is growth ever really complete? Design the difference between "can't continue" and "growth complete"
      radiant.events.unlisten(self._entity, 'stonehearth:growing', self, self._on_grow_period)
   end
   self.__saved_variables:mark_changed()
end

--- Returns true if it's time to harvest, false otherwise
function CropComponent:is_harvestable()
   return self._sv.harvestable
end

return CropComponent