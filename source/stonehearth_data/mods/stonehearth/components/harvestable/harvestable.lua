--[[
   This component should be attached to bushes, plants, and other things that
   regrow resources for havest. (Berry bushes, etc.)
]]

local calendar = radiant.mods.load('stonehearth').calendar

local Harvestable = class()

function Harvestable:__init(entity, data_store)
   self._entity = entity

   self._renewable = false           --Whether the resource can regrow
   self._renewal_time = nil          --num in-game hours till renewal
   self._takeaway_entity_type = nil  --entity to create on harvest

   self._data = data_store:get_data()
   self._data.hours_till_next_growth = nil
   self._data_store = data_store

end

function Harvestable:extend(json)
   if json then
      if json.renewable then
         self._renewable = json.renewable
         radiant.events.listen(calendar, 'stonehearth:hourly', self, self.on_hourly)
      end
      if json.renewal_time then
         self._renewal_time = json.renewal_time
      end
      if json.takeaway_entity then
         self._takeaway_entity_type = json.takeaway_entity
      end
      if json.harvest_command then
         self._harvest_command_name = json.harvest_command
      end
   end
end

function Harvestable:get_takeaway_type()
   return self._takeaway_entity_type
end

--- Let plant know it's giving up resources
-- Disable the icon
-- Produce the harvestable thing
-- Set the clock so the renewal period can count down
function Harvestable:harvest()
   local bush_commands = self._entity:get_component('stonehearth:commands')
   bush_commands:enable_command(self._harvest_command_name, false)
   self._data.hours_till_next_growth = self._renewal_time
end

--- If the player has harvested the goods, check if it's time to re-grow them
function Harvestable:on_hourly()
   if self._data.hours_till_next_growth == nil then
      return
   end

   --- Countdown till we're harvestable again.
   -- Once we're harvestable, change the bush model and the UI. 
   if self._data.hours_till_next_growth <= 0 then
      local bush_commands = self._entity:get_component('stonehearth:commands')
      bush_commands:enable_command(self._harvest_command_name, true)
      self._data.hours_till_next_growth = nil
   else  
      self._data.hours_till_next_growth = self._data.hours_till_next_growth - 1
   end
end

return Harvestable
