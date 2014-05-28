--[[
   Sets the personality, history, and story-attributes for a character.
   The output is saved in a log. Log entries are grouped by day 
]]

local Personality = class()

local calendar = stonehearth.calendar
local personality_service = stonehearth.personality
local rng = _radiant.csg.get_default_rng()

function Personality:__init()
   local a = 1
end

function Personality:initialize(entity, json)
   self._entity = entity
   self._first_entry = true

   self._sv = self.__saved_variables:get_data()
   if not self._sv.log then
      self._sv.log = {}
      self._sv.substitutions = {}
      self._sv.todays_events = {}    --Each notable thing that's happened today
   end
   radiant.events.listen(calendar, 'stonehearth:midnight', self, self.on_midnight)
end

function Personality:set_personality(personality)
   self._sv.personality = personality
   self.__saved_variables:mark_changed()
end

function Personality:get_personality()
   return self._sv.personality
end

--- Call this function to add a substitution given a parameter
--  For example, look up the deity for the ascendancy (Cid).
--  TODO: allow for value to be a variable/ptr to other values
--  @param key - the variable that we want to assign to
--  @param parameter - the parameter that will help us look up the correct
--                 string value for the key
--  @param namespace - optional, the scope of the substitution. Defaults to stonehearth
function Personality:add_substitution_by_parameter(key, parameter, namespace)
   if not namespace then 
      namespace = 'stonehearth'
   end
   local value = personality_service:get_substitution(namespace, key, parameter)
   self._sv.substitutions[key] = value
   self.__saved_variables:mark_changed()
end

--- Call this function to add a substitution when you already know the value
--  For example, if you know that "target_food" should be "Berries"
--  TODO: allow for value to be a variable/ptr to other values
--  @param key - the name of the key to look up
--  @param value - the string value associated with the key
function Personality:add_substitution(key, value)
   local data = {}
   data.type = 'string'
   data.value = value
   self._sv.substitutions[key] = data
   self.__saved_variables:mark_changed()
end

--Every night, dump the old log
--TODO: Possibly add backstory, events, etc
function Personality:on_midnight(e)
   self._sv.todays_events = {}
   self.__saved_variables:mark_changed()
end

--- Pass event name and % likelihood of logging the event.
--  If the event has not yet happened today, and we roll
--  within the percent chance, get a log given the event name and personality
--  add the event to todays_events and the permanent log
--  @param event_name: name of the event to look up
--  @param percent_chance: number between 1 and 100
--  @param namespace - The scope of the substitution. Optional
--  @param score_metadata - optional, if present, gives data about the score this entry corresponds to
function Personality:register_notable_event(event_name, percent_chance, namespace, score_metadata)
   if self._sv.todays_events[event_name] == nil then
      local roll = rng:get_int(1, 100)
      if roll <= percent_chance then
         local title, log = personality_service:get_activity_log(namespace, event_name, self._sv.personality, self._sv.substitutions)
         if log then
            return self:_add_log_entry(title, log, score_metadata)
         end
      end
      self._sv.todays_events[event_name] = true
      self.__saved_variables:mark_changed()
   end
end

--- Insert a brand new entry into the log
function Personality:_add_log_entry(entry_title, entry_text, score_metadata)
   --Are there any entries yet for this day? If not, add one
   local todays_date = calendar:format_date()
   if #self._sv.log == 0 or self._sv.log[1].date ~= todays_date then
      self:_init_day(todays_date)
   end

   --Create data for the new entry
   local entry = {}
   entry.text = entry_text
   entry.title = entry_title
   entry.score_metadata = score_metadata
   entry.person_name = radiant.entities.get_name(self._entity)
   entry.date = todays_date
   table.insert(self._sv.log[1].entries, entry)

   --If there is score metadata and if it the modifier is negative, show as warning
   local log_type = 'info'
   if score_metadata and score_metadata.score_mod < 0 then
      log_type = 'warning'
   end
   --For now, put the note in the scrolling event log too
   local name = radiant.entities.get_display_name(self._entity)
   stonehearth.events:add_entry(name .. ': ' .. entry_text, log_type)

   --Let the journal UI know to update itself, if visible
   self.__saved_variables:mark_changed()

   --If the show journal command is not yet enabled, enable it
   if self._first_entry then
      local entity_commands = self._entity:get_component('stonehearth:commands')
      entity_commands:enable_command('show_journal', true)
      self._first_entry = false
   end

   return entry
end

--- Insert the new day's data at the TOP of the log
function Personality:_init_day(day)
   local new_day_data = {}
   new_day_data.date = day
   new_day_data.entries = {}
   table.insert(self._sv.log, 1, new_day_data)
   self.__saved_variables:mark_changed()
end


return Personality