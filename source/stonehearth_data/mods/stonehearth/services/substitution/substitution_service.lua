--[[
This service keeps namespaces of substition tables and provides text-substitution
function that use them. 

Substitution Tables are made of keys and values. Each value is one of the
following types: 
- random array (will pick one of the items randomly)
- variable (will contain a reference to another var or random array)
- parametrized_value (will take parameter and return the value associated with the parameter)

Substitution tables are divided into hierarchical namespaces. If a substitution can't be found
at specific namespace, we look up the tree to see if any parent (though not any sibling or cousin)
node has a substitution. The most general substitutions are kept at the stonehearth namespace.

--  JSON Format: 
--  "substition_table" : [
--     {
--     "namespace" : "stonehearth",
--     "keys" : {
--       "key_name" : {
--          "type" : "random_array",
--          "entries" : [
--              "__monsters__ were attacking our caravan."
--           ]
--        },
--      "key_name2" : {
--           "type" : "parametrized_value",
--           "entries" : {
--              "rayyas_children" : "Rayya",
--              "northern_alliance" : "Valin"
--           }
--        },
--      "key_name3" : {
--           "type" : "variable",
--           "value" : "other_key_name"
--       }
--      }
--    },
--    {
--     "namespace" : "stonehearth:foo",
--     "keys" : {
--       "key_name" : {
--          "type" : "random_array",
--          "entries" : [
--              "__monsters__ were attacking our caravan."
--           ]
--        }
--      }
--    }
--  ]
-- 
-- Resulting object format:
-- self._substitution_tables = {
--   stonehearth : {
--      keys : {
--         key_name : {type = random_array, entries = [.., ..]},
--         key_name2 : {type = parametrized_value, entries = [.., ..]}   
--         key_name3 : {type = variable, value = ...}   
--      }
--      foo : {
--         keys: {....}   
--         //other namespaces that may be children of stonehearth:foo:???
--      }
--   }    
-- }

More documenation can be found here: 
http://wiki.rad-ent.com/doku.php?id=journal_entries
]]

local rng = _radiant.csg.get_default_random_number_generator()

SubstitutionService = class()

--- Creates a new SubstitutionDictionary class 
function SubstitutionService:__init()
   self._substitution_tables = {}
end

--- Populate the dictionary with a data blob of json
--  For each item in the substitution table json, find it's namespace
--  split the namespace by : (or any other punctutation)
--  Find the proper location in our data object for the namespace
--  If the location doesn't yet exist, create it.
--  If it does exist, add the keys from this new item to that namespace node.
--  Child namespaces are added as keys to their parent object. 
function SubstitutionService:populate(data_blob)
   for i, namespace_data in ipairs(data_blob) do
      local namespace = namespace_data.namespace
      local curr_table = self:_find_namespace_object(namespace, true)
      self:_fill_table_entry(curr_table, namespace_data.keys)
   end
end

--  If a key already exists for a 'random array' type table, concat the new values
--  onto the old. If it's a "parametrized_value", then add the new keys, overriding old keys,
--  if necessary. If it's a "variable" do nothing (first entry wins).
function SubstitutionService:_fill_table_entry(table_entry, new_keys)
   for key, value in pairs(new_keys) do
      if table_entry.keys[key] then
         local existing_table = table_entry.keys[key].entries
         if value.type == 'random_array' then
            for i, v in ipairs(value.entries) do
               table.insert(existing_table, v)
            end
         elseif value.type == 'parametrized_value' or value.type == 'parametrized_variable' then
            for k, v in pairs(value.entries) do
               existing_table[k] = v
            end
         end
      else
         table_entry.keys[key] = value
      end
   end
end

--- Split the namespace into strings by : and find the part of the
--  substitution table object that corresponds to the namespace.
--  If should_create, we will create any missing tables.
--  @param namespace - string with sections separated by :
--  @param should_create - if true, will add new empty data structures to 
--                         empty nodes as it traverses.
--  @returns - target namespace. If namespace does not exist, will return an
--             empty if should_create = true, will return nil otherwise.
function SubstitutionService:_find_namespace_object(namespace, should_create)
   local curr_table = self._substitution_tables
   for namespace_segment in string.gmatch(namespace, "[^:]+") do
      local target_table = curr_table[namespace_segment]
      if not target_table then
         if should_create then
            target_table = {}
            target_table.keys = {}
            curr_table[namespace_segment] = target_table
         else
            return nil
         end
      end
      curr_table = target_table
   end
   return curr_table
end


--- Given a key of type subst-table, return the value associated
--  with the relevant parameter. For example, get_substitution(teacher, ascendency)
--  yields a data node with a type (either string or variable) and the value--
--  either a string to put into the substitution, or a variable that points to 
--  a value that can be used in the substitution
--  If the parameter is not found but a default ('generic') parameter is defined, used
--  that instead.
--  @param namespace - the namespace to search
--  @param key - the field that will appear in the text
--  @param param   - the version of the value we want
function SubstitutionService:get_substitution(namespace, key, param)
   local data_by_namespace = self:_find_key_data(namespace, key)
   if data_by_namespace then
      local data = {}
      if data_by_namespace.type == 'parametrized_value' then       
         data.type = 'string'
      elseif data_by_namespace and data_by_namespace.type == 'parametrized_variable' then
         data.type = 'variable'
      end
      data.value = data_by_namespace.entries[param]
      if not data.value then
         data.value = data_by_namespace.entries['generic']
      end
      return data
   end
end

--- Given a namespace, find the data associated with a certain key. 
--  @namespace: string in the form of stonehearth:foo:bar
--  @key: string whose data we're looking for (ie, "deity")
function SubstitutionService:_find_key_data(namespace, key)
   local namespace_table = {}
   for namespace_segment in string.gmatch(namespace, "[^:]+") do
      --insert each child lower in than parent
      table.insert(namespace_table, namespace_segment)
   end
   local curr_table = self._substitution_tables
   return self:_recursively_find_key(curr_table, namespace_table, key)
end

--- Recursively descend the tree to find the key. 
--  Return the match at the lowest (most specific level). Look at each level
--  higher if it's not found. Can return nil if the value is not in the table.
--  @param curr_table: the namespace to look at
--  @namespace_table: a table of all the namespaces, sorted from most general to most specific, top to bottom
--  @key: the key whose data we're looking for
--  @returns: the data for the key, or nil if not found. 
function SubstitutionService:_recursively_find_key(curr_table, namespace_table, key)
   if #namespace_table == 0 then
      -- We're at the lowest level. If the key is present here, return it. 
      return curr_table.keys[key]
   else
      -- Figure out the next namespace and look there for the key
      local next_namespace = table.remove(namespace_table, 1)
      local next_table = curr_table[next_namespace]
      assert(next_table, "Oops, we forgot to create this node")
      local data_for_key = self:_recursively_find_key(next_table, namespace_table, key)
      
      --If the key isn't in any namespace lower than us, look in us
      if not data_for_key and curr_table.keys then
         data_for_key = curr_table.keys[key]
      end
      return data_for_key
   end
end

--- Look for variables in the string set aside via __foo__ markings
--  Check for them in the substitution dictionary. If their type is 'one of random',
--  pick a random value and use that. If their type is 'variable,' that means 
--  its value is another key, which should be looked up. In the meantime, we
--  will likely use this variable multiple times, and whatever value is eventually
--  found for it should be used in all instances.
--  This funcition will recursively find substitutions, for nested entries.
--  TODO: Allow the override table to contain variables
--  @param namespace - the namespace from which to draw the variable values
--  @param target_string - the string with the variables to substitute
--  @param overrides - a dictionary that takes precedence over the default dictionary
function SubstitutionService:substitute_variables(namespace, target_string, overrides)
   local temp_sub_table  = {}
   return string.gsub(target_string,"__(.-)__", function(variable_name)
      --check the subsitution table first. If present, return
      local override_entry = overrides[variable_name]
      if override_entry then
         if override_entry.type == 'string' then
            return override_entry.value
         elseif override_entry.type == 'variable' then
            variable_name = override_entry.value
         end
      elseif temp_sub_table[variable_name] then
         --Is it in the local sub table? If so, use that 
         return temp_sub_table[variable_name]
      end

      --If we haven't yet found a substitution and returned, find a good random value 
      local random_value =  self:_find_one_of_random_array(namespace, variable_name, temp_sub_table)
      if random_value then
         --If the substitution itself contains variables, run that too
         random_value = self:substitute_variables(namespace, random_value, overrides)
         return random_value
      else
         --Indicate that we couldn't find the substitution
         return '???' 
      end
      
   end)
end



--TODO: Capitalize first of every sentence.
--TODO: Functions to handle articles and plurality
function SubstitutionService:capitalize_first(target_string)
   return string.gsub(target_string, "^%l", string.upper)
end

--- Recursively search through the table till we get to an entry with an actual
--  value (in this case, random_array). Once there, use that. 
--  @param key - var name we're searching for
--  @param temp_sub_table - table of variables (as opposed to simple substitutions) 
--                          we're trying to find values for. 
--  @returns - the string we found the substitution for.  
function SubstitutionService:_find_one_of_random_array(namespace, key, temp_sub_table)
   local value_data = self:_find_key_data(namespace, key)
   if value_data and value_data.type == 'random_array' then
      local roll = rng:get_int(1, #value_data.entries)
      return value_data.entries[roll]
   elseif value_data and value_data.type == 'variable' then
      --if the variable is of type variable, add the result to the temp sub table for use later
      local result = self:_find_one_of_random_array(namespace, value_data.value)
      temp_sub_table[key] = result
      return result
   end
end

return SubstitutionService()