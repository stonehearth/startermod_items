--[[
   Sets the personality, history, and story-attributes for a character.
   The output is saved in a log. Log entries are grouped by day 
]]

local Personality = class()

local calendar = radiant.mods.load('stonehearth').calendar
local personality_service = require 'services.personality.personality_service'
local event_service = require 'services.event.event_service'

--REVIEW QUESTION: when to use radiant.mods.load and when to use require?
--Explain and I'll add it to the wiki

function Personality:__init(entity, data_store)
   self._entity = entity
   self._first_entry = true

   self._data = data_store:get_data()
   self._data.personality = nil
   
   self._data.log = {}              --Full list of entries for a person
   self._data.todays_events = {}    --Each notable thing that's happened today

   self._data_store = data_store
   self._data_store:mark_changed()

   radiant.events.listen(calendar, 'stonehearth:midnight', self, self.on_midnight)
end

function Personality:extend(json)
end

function Personality:set_personality(personality)
   self._data.personality = personality
end

function Personality:get_personality()
   return self._data.personality
end

--Every night, dump the old log
--TODO: Possibly add backstory, events, etc
function Personality:on_midnight(e)
   self._data.todays_events = {}
end

--- Pass event name and % likelihood of logging the event.
--  If the event has not yet happened today, and we roll
--  within the percent chance, get a log given the event name and personality
--  add the event to todays_events and the permanent log
--  @param event_name: name of the event to look up
--  @param percent_chance: number between 1 and 100
function Personality:register_notable_event(event_name, percent_chance)
   if self._data.todays_events[event_name] == nil then
      local roll = math.random(100)
      if roll <= percent_chance then
         local title, log = personality_service:get_activity_log(event_name, self._data.personality)
         if log then
            self:_add_log_entry(title, log)
         end
      end
      self._data.todays_events[event_name] = true
   end
end

--- Insert a brand new entry into the log
function Personality:_add_log_entry(entry_title, entry_text)
   --Are there any entries yet for this day? If not, add one
   local todays_date = calendar:format_date()
   if #self._data.log == 0 or self._data.log[1].date ~= todays_date then
      self:_init_day(todays_date)
   end

   --Create data for the new entry
   local entry = {}
   entry.text = entry_text
   entry.title = entry_title
   table.insert(self._data.log[1].entries, entry)

   --For now, put the note in the scrolling event log too
   local name = radiant.entities.get_display_name(self._entity)
   event_service:add_entry(name .. ': ' .. entry_text)

   --Let the journal UI know to update itself, if visible
   self._data_store:mark_changed()

   --If the show journal command is not yet enabled, enable it
   if self._first_entry then
      local entity_commands = self._entity:get_component('stonehearth:commands')
      entity_commands:enable_command('show_journal', true)
      self._first_entry = false
   end
end

--- Insert the new day's data at the TOP of the log
function Personality:_init_day(day)
   local new_day_data = {}
   new_day_data.date = day
   new_day_data.entries = {}
   table.insert(self._data.log, 1, new_day_data)
end


return Personality