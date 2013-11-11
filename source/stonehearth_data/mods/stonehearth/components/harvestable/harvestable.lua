--[[
   This component should be attached to bushes, plants, and other things that
   regrow resources for havest. (Berry bushes, etc.)
]]

local calendar = radiant.mods.load('stonehearth').calendar

local Harvestable = class()

function Harvestable:__init(entity, data_store)
   self._entity = entity

   self._renewal_time = nil         --num in-game hours till renewal
   self.takeaway_entity_type = nil  --entity to create on harvest

   self._data = data_store:get_data()
   self._data.hours_till_next_growth = nil
   self._data_store = data_store

   radiant.events.listen(calendar, 'stonehearth:hourly', self, self.on_hourly)
end

function Harvestable:extend(json)
   if json then
      if json.renewal_time then
         self._renewal_time = json.renewal_time
      end
      if json.takeaway_entity then
         self.takeaway_entity_type = json.takeaway_entity
      end
      if json.harvest_command then
         self._harvest_command_name = json.harvest_command
      end
   end
end

--- Let plant know it's giving up resources
-- Disable the icon
-- Produce the harvestable thing
-- Set the clock so the renewal period can count down
function Harvestable:harvest()
   local bush_commands = self._entity:get_component('stonehearth:commands')
   bush_commands:enable_command('harvest_berries', false)

   --TODO: change model to be the version w/o the collectable

   local basket = radiant.entities.create_entity(self.takeaway_entity_type)
   radiant.terrain.place_entity(basket, self._entity:get_component('mob'):get_world_grid_location())

   self._data.hours_till_next_growth = self._renewal_time
end

--- Every hour, if there are no harvestable items, check if they've regrown yet.
function Harvestable:on_hourly()
   --There's something harvestable on the plant. Return.
   if self._data.hours_till_next_growth == nil then
      return
   end

   --- Countdown till we're harvestable again.
   -- Once we're harvestable, change the bush model and the UI. 
   if self._data.hours_till_next_growth <= 0 then

      --TODO: change the model to be the version with the collectable

      local bush_commands = self._entity:get_component('stonehearth:commands')
      bush_commands:enable_command('harvest_berries', true)
      self._data.hours_till_next_growth = nil
   else  
      self._data.hours_till_next_growth = self._data.hours_till_next_growth - 1
   end
end

return Harvestable
