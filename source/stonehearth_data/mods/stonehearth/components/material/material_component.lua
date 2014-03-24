--[[
   Todo, document this thing
]]

local MaterialComponent = class()

function MaterialComponent:initialize(entity, json)
   self._sv = self.__saved_variables:get_data()
   if not self._sv.tags then
      self._sv.tags = {}
      if json.tags then
         for _, tag in ipairs(self:_split_string(json.tags)) do
            self._sv.tags[tag] = true
         end
      end      
   end
end

function MaterialComponent:is(tags_string)
   assert(type(tags_string) == 'string', 'first argument to is() must be a string')
   
   local tags = self:_split_string(tags_string)
   for _, tag in ipairs(tags) do
      if not self._sv.tags[tag] then
         return false
      end
   end
   return true
end

function MaterialComponent:has_tag(tag)
   return self._sv.tags[tag]
end

function MaterialComponent:add_tag(tag)
   if not self._sv.tags then
      self._sv.tags[tag] = true
      self.__saved_variables:mark_changed()
   end
end

function MaterialComponent:remove_tag(tag)
   if self._sv.tags then
      self._sv.tags[tag] = nil
      self.__saved_variables:mark_changed()
   end
end

--- Split a string into a table (e.g. "foo bar baz" -> { "foo", "bar", "baz" }
-- Borrowed from http://lua-users.org/wiki/SplitJoin
function MaterialComponent:_split_string(str)
   local fields = {}
   local pattern = string.format("([^%s]+)", ' ')
   str:gsub(pattern, function(c) fields[#fields+1] = c end)
   return fields
end

return MaterialComponent
