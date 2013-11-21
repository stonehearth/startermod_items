--[[
   Sets the personality, history, and story-attributes for a character.
   The output is saved in a log. 
]]

local Personality = class()

local calendar = radiant.mods.load('stonehearth').calendar
local personality_service = require 'services.personality.personality_service'
local event_service = require 'services.event.event_service'

--REVIEW QUESTION: when to use radiant.mods.load and when to use require?
--Explain and I'll add it to the wiki

function Personality:__init(entity, data_store)
   self._entity = entity

   self._data = data_store:get_data()
   self._data.personality = nil
   self._data.log = {}
   self._data.daily_event_log = {}
   self._data_store = data_store
   self._data_store:mark_changed()

   radiant.events.listen(calendar, 'stonehearth:sunset', self, self.on_sunset)
end

function Personality:extend(json)
end

function Personality:set_personality(personality)
   self._data.personality = personality
end

function Personality:get_personality()
   return self._data.personality
end

--on sunset, dump the day's event log
--TODO: consider doing backstory/narrative elements
function Personality:on_sunset(e)
   self._data.daily_event_log = {}
end

--- Pass event name and % likelihood of logging the event.
--  If the event has not yet happened today, and we roll
--  within the percent chance, get a log given the event name and personality
--  add the event to the daily event log and the permanent log
--  @param event_name: name of the event to look up
--  @param percent_chance: number between 1 and 100
function Personality:register_notable_event(event_name, percent_chance)
   if self._data.daily_event_log[event_name] == nil then
      local roll = math.random(100)
      if roll <= percent_chance then
         local title, log = personality_service:get_activity_log(event_name, self._data.personality)
         self:_add_log_entry(title, log)
      end
      self._data.daily_event_log[event_name] = true
   end
end

function Personality:_add_log_entry(entry_title, entry_text)
   local entry = {}
   entry.date = calendar:format_date()
   entry.text = entry_text
   entry.title = entry_title
   table.insert(self._data.log, entry)

   --For now, put the note in the event log. Eventually, put in UI
   local name = radiant.entities.get_display_name(self._entity)
   event_service:add_entry(name .. ': ' .. entry_text)

   self._data_store:mark_changed()
end

return Personality