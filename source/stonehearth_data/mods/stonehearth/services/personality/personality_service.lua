--[[
   This service issues personalities and log entries based on those personalities. To prevent the
   same entries from coming up repeatedly, it manages which entries have been used, and for
   entries where repeating is OK, only allows repeats after all entries in a given category have
   been exhasted. 

   When a notable event happens, we fire an event called stonehearth:journal_event which
   this service listens on. The event passes in the relevant entity and an event trigger id
   which is mapped to an activity structure. This service and the entity's personality_component
   then figure out together whether a log should be added to that entity's history.

   TODO: Load all the log files from the manifests, not from a function here. 
   TODO: a stub implementation for "sequential" (ie, storytelling, non-random) log entries is
   implemented here, but it doesn't yet work if multiple people have the same personality.
   Improve by indexing these entries by entity_id
   TODO: Mad Libs! Entries should allow for %s and %d and name substitutions.
]]
PersonalityService = class()

function PersonalityService:__init()
   --Load all the personality types
   self._personality_types = radiant.resources.load_json('stonehearth:personality_types').personalities
   self._personality_unused_table = {}

   --Tables to store tables of potential log entries
   self._activity_logs = {}
   self._trigger_to_activity = {}

   --Event that will fire every time we could note something in the log
   radiant.events.listen(self, 'stonehearth:journal_event', self, self._on_journal)

   --Pre-load all log files. TODO: read from manifests
   self:_load_log_files()
end

--- When a notable event happens, produce a log entry for the relevant entity if appropriate
--  In order to make this happen, an activity needs to have a trigger event id that matches
--  the description in the event. If this maps to an activity, then grab the activity and
--  ask the person's personality service if the log should be 
function PersonalityService:_on_journal(e)
   local person = e.entity
   local description = e.description
   if person and description then 
      local personality_component = person:get_component('stonehearth:personality')
      local trigger_data = self._trigger_to_activity[description]
      if personality_component and trigger_data then
         local probability = trigger_data.probability
         local activity_name = trigger_data.activity_id
         personality_component:register_notable_event(activity_name, probability)
      end
   end
end

--- Loads relevant log files
--  TODO: change this to read them out of the manifests; look at radiant.call in app.js
function PersonalityService:_load_log_files()
   self:_load_activity('stonehearth:personal_logs:chopping_wood')
   self:_load_activity('stonehearth:personal_logs:eating_berries')
   self:_load_activity('stonehearth:personal_logs:embarking')
end

--- Returns a personality type. Will only return repeats if all
--  existing personalities have been used
function PersonalityService:get_new_personality()
   return self:_get_random_unused_from_table(self._personality_types, self._personality_unused_table)
end

--- Load in a file of new activities. 
--  If we've already got that activity, skip
function PersonalityService:_load_activity(url)
   local new_activity_blob = radiant.resources.load_json(url)
   for i, activity in ipairs(new_activity_blob.activities) do
      if not self._activity_logs[activity.id] then
         self._activity_logs[activity.id] = activity
         
         --For each type of trigger, map the trigger to the activity
         for i, trigger_data in ipairs(self._activity_logs[activity.id].trigger_policy) do
            local data = {}
            data.activity_id = activity.id
            data.probability = trigger_data.probability
            self._trigger_to_activity[trigger_data.trigger_id] = data
         end

         --For each personality, read in possible logs 
         --TODO: for sequential, may be more than 1 set per person
         local logs_by_personality = self._activity_logs[activity.id].logs_by_personality
         for personality, log_data in pairs(logs_by_personality) do
            log_data.use_counter = 0
            log_data.unused_table = {}
         end
      end
   end
end

--- Given an activity name and a personality type, return an unused activity log of that type
--  If the activity is random, return a random unused activity. Will re-use logs when
--  all have been used once. 
--  If the activity is sequential, return the next unused activity. Will never re-use logs
--  @return title, log
function PersonalityService:get_activity_log(activity_name, personality_type)
   local activity_data = self._activity_logs[activity_name]
   if activity_data then
      local prefix = activity_data.text
      local log_entry = nil
      if activity_data and activity_data.logs_by_personality[personality_type] then
         local log_data = activity_data.logs_by_personality[personality_type]
         if activity_data.type == 'random' then
            log_entry = self:_get_random_unused_from_table(log_data.logs,
                                                           log_data.unused_table)
         elseif activity_data.type == 'sequential' then
            log_entry = self:_get_sequential_unused_from_table(log_data.logs, log_data.use_counter)
         end
         log_data.use_counter = log_data.use_counter + 1
      end
      return prefix, log_entry
   end
end

--- Return a random value that has not yet been used. 
--  @param root_array - array that contains all the relevant values
--  @param use_array - array that keeps track of use of those values
function PersonalityService:_get_random_unused_from_table(root_array, unused_values)
   -- if we're out of options, refill the unused_values array
   if #unused_values == 0 then
      for i, value in ipairs(root_array) do 
         table.insert(unused_values, value)
      end
   end
   -- pick a random one to return
   local i = math.random(#unused_values)
   local value = unused_values[i]
   
   -- remove the random one from the array so we don't pick it next time.
   -- we can do this really fast by moving the one in the back into this
   -- slot and reducing the table length by 1
   unused_values[i] = unused_values[#unused_values]
   table.remove(unused_values)
   
   -- now return the value!
   return value
end

--- Return the next unused element from a table. Once an item is used, never use it again.
--  Use for tables like backstory which are sequential and unique
--  TODO: test this later
--  TODO: make sure we have an analogous structure that gives 1 of each type out ever; no repeats
function PersonalityService:_get_sequential_unused_from_table(root_array, tracker_var)
   -- this doesn't quite work.  tracker_var is passed in by value, so modifications we make
   -- to it don't stick.  luckily, no one uses it at all yet. =O   Fix it if it ever comes
   -- to that -tony
   assert(false, 'not really working yet')
   tracker_var = tracker_var + 1
   if tracker_var <= (#root_array) then 
      return root_array[tracker_var]
   end
   return ""
end

return PersonalityService()