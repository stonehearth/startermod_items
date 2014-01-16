--[[
This class provides takes a dictionary and provides text-substitution functions
using that dictionary.

Dictionaries are made of keys and values. Each value is one of the following types: 
- random array (will pick one of the items randomly)
- variable (will contain a reference to another var or random array)
- subst_table (will take parameter and return the value associated with the parameter)

More documenation can be found here: 
http://wiki.rad-ent.com/doku.php?id=journal_entries
]]

local rng = _radiant.csg.get_default_random_number_generator()

SubstitutionDictionary = class()

--- Creates a new SubstitutionDictionary class 
function SubstitutionDictionary:__init()
   self._dictionary = {}
end

--- Populate the dictionary with a data blob of json
--  Variables are set off by a dual underscore
--  JSON Format: 
--  "dictionaries" : {
--     "key_name" : {
--        "type" : "random_array",
--        "entries" : [
--            "__monsters__ were attacking our caravan."
--         ]
--      },
--    "key_name2" : {
--         "type" : "subst_table",
--         "entries" : {
--            "rayyas_children" : "Rayya",
--            "northern_alliance" : "Valin"
--         }
--      },
--    "key_name3" : {
--         "type" : "variable",
--         "value" : "other_key_name"
--      }
--  }
--  If a key already exists for a 'random array' type table, concat the new values
--  onto the old. If it's a "subst_table", then add the new keys, overriding old keys,
--  if necessary. If it's a "variable" do nothing (first entry wins).
function SubstitutionDictionary:populate_dictionary(data_blob)
   for key, value in pairs(data_blob.dictionaries) do
      if self._dictionary[key] then
         local existing_table = self._dictionary[key].entries
         if value.type == 'random_array' then
            for i, v in ipairs(value.entries) do
               table.insert(existing_table, v)
            end
         elseif value.type == 'subst_table' then
            for k, v in pairs(value.entries) do
               existing_table[k] = v
            end
         end
      else
         self._dictionary[key] = value
      end
   end
end

--- Given a field of type subst-table, return the value associated
--  with the relevant key. For example, get_substitution(deity, ascendency)
--  yields "Cid".
--  @param field - the field that will appear in the text
--  @param key   - the version of the value we want
function SubstitutionDictionary:get_substitution(field, key)
   local field_entry = self._dictionary[field]
   if field_entry and field_entry.type == 'subst_table' then
      return field_entry.entries[key]
   end
end

--- Look for variables in the string set aside via __foo__ markings
--  Check for them in the substitution dictionary. If their type is 'one of random',
--  pick a random value and use that. If their type is 'variable,' that means 
--  its value is another key, which should be looked up. In the meantime, we
--  will likely use this variable multiple times, and whatever value is eventually
--  found for it should be used in all instances.
--  This funcition will recursively find substitutions, for nested entries.
--  @param target_string - the string with the variables to substitute
--  @param overrides - a dictionary that takes precedence over the default dictionary
function SubstitutionDictionary:substitute_variables(target_string, overrides)
   local temp_sub_table  = {}
   return string.gsub(target_string,"__(.-)__", function(variable_name)
      --check the subsitution table first. If present, return
      if overrides[variable_name] then
         return overrides[variable_name]
      elseif temp_sub_table[variable_name] then
         --Is it in the local sub table? If so, use that 
         return temp_sub_table[variable_name]
      else 
         --If not present in either table, find a good random value 
         local random_value =  self:_find_one_of_random_array(variable_name, temp_sub_table)
         if random_value then
            --If the substitution itself contains variables, run that too
            random_value = self:substitute_variables(random_value, overrides)
            return random_value
         else
            --Indicate that we couldn't find the substitution
            return '???' 
         end
      end
   end)
end

--TODO: Capitalize first of every sentence.
--TODO: Functions to handle articles and plurality
function SubstitutionDictionary:capitalize_first(target_string)
   return string.gsub(target_string, "^%l", string.upper)
end

--- Recursively search through the table till we get to an entry with an actual
--  value (in this case, random_array). Once there, use that. 
--  @param variable_name - var name we're searching for
--  @param temp_sub_table - table of variables (as opposed to simple substitutions) 
--                          we're trying to find values for. 
--  @returns - the string we found the substitution for.  
function SubstitutionDictionary:_find_one_of_random_array(variable_name, temp_sub_table)
   local value_data = self._dictionary[variable_name]
   if value_data.type == 'random_array' then
      local roll = rng:get_int(1, #value_data.entries)
      return value_data.entries[roll]
   elseif value_data.type == 'variable' then
      --if the variable is of type variable, add the result to the temp sub table for use later
      local result = self:_find_one_of_random_array(value_data.value)
      temp_sub_table[variable_name] = result
      return result
   end
end

return SubstitutionDictionary