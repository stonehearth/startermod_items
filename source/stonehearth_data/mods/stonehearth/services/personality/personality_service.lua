PersonalityService = class()

function PersonalityService:__init()
   self._personality_types = radiant.resources.load_json('stonehearth:personality_types').personalities
   self._num_personalities_used = 0
   self._personality_unused_table = {}
   self:_make_use_table(self._personality_types, self._personality_unused_table, true)

   self._activity_logs = {}
end

--- Returns a personality type. Will only return repeats if all
--  existing personalities have been used
function PersonalityService:get_new_personality()
   return self:_get_random_unused_from_table(self._personality_types, self._personality_unused_table, self._num_personalities_used)
end

--- Load in a file of new activities. 
--  If we've already got that activity, skip
function PersonalityService:load_activity(url)
   local new_activity_blob = radiant.resources.load_json(url)
   for i, activity in ipairs(new_activity_blob.activities) do
      if not self._activity_logs[activity.id] then
         self._activity_logs[activity.id] = activity
         local logs_by_personality = self._activity_logs[activity.id].logs_by_personality
         for personality, log_data in pairs(logs_by_personality) do
            log_data.use_counter = 0
            log_data.unused_table = {}
            self:_make_use_table(log_data.logs, log_data.unused_table, true)
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
   local prefix = activity_data.text
   local log_entry = ""
   if activity_data and activity_data.logs_by_personality[personality_type] then
      local log_data = activity_data.logs_by_personality[personality_type]
      if activity_data.type == 'random' then
         log_entry = self:_get_random_unused_from_table(log_data.logs,
                                                   log_data.unused_table, 
                                                   log_data.use_counter)
      elseif activity_data.type == 'sequential' then
         
         log_entry = self:_get_sequential_unused_from_table(log_data.logs, log_data.use_counter)
      end
   end
   return prefix, log_entry
end

--- Create a new array to track the use of a parent array
-- @param root_array - array of things we're tracking
-- @param new_array - array we're creating to track the root_array's use
-- @default value - default value for each tracked thing (usually true or false)
function PersonalityService:_make_use_table(root_array, new_array, default_value)
   for i, v in ipairs(root_array) do
      new_array[i] = default_value
   end
end

--- Return a random value that has not yet been used. 
--  @param root_array - array that contains all the relevant values
--  @param use_array - array that keeps track of use of those values
--  @param tracker_var - var keeps track of # of things used in the array
function PersonalityService:_get_random_unused_from_table(root_array, use_array, tracker_var)
   --Randomly sort through options in the root_array till we find an unused one
   local unused_index
   local target_return_value
   repeat
      unused_index = math.random(#root_array)
   until use_array[unused_index]

   --Mark it used
   use_array[unused_index] = false
   tracker_var = tracker_var + 1
   target_return_value = root_array[unused_index]

   --If the table is full clear it
   if tracker_var >= #root_array then
      self:_clear_table(use_array, true, tracker_var)
   end

   --return the unused personality
   return target_return_value
end

--- Return the next unused element from a table. Once an item is used, never use it again.
--  Use for tables like backstory which are sequential and unique
--  TODO: test this later
--  TODO: make sure we have an analogous structure that gives 1 of each type out ever; no repeats
function PersonalityService:_get_sequential_unused_from_table(root_array, tracker_var)
   tracker_var = tracker_var + 1
   if tracker_var <= (#root_array) then 
      return root_array[tracker_var]
   end
   return ""
end

--- Take an array and sets every value to value
-- @param table - the table to clear
-- @param value - the value to set the table to
-- @counter - counter for # used
function PersonalityService:_clear_table(table, value, counter)
   for i, v in ipairs(table) do
      table[i] = value
   end
   counter = 0
end

return PersonalityService()