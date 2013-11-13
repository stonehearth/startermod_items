--[[
   Todo, document this thing
]]

local MaterialComponent = class()

function MaterialComponent:__init(entity, data_binding)
   self._data_binding = data_binding
   self._data = self._data_binding:get_data()
   if not self._data.tags then
      self._data.tags = {}
   end
end

function MaterialComponent:extend(json)
   if json and json.tags then
      for _, tag in ipairs(self:_split_string(json.tags)) do
         self._data.tags[tag] = true
      end
      self._data_binding:mark_changed()
   end
end

function MaterialComponent:is(tags_string)
   local tags = self:_split_string(tags_string)
   for _, tag in ipairs(tags) do
      if not self._data.tags[tag] then
         return false
      end
   end
   return true
end

function MaterialComponent:has_tag(tag)
   return self._data.tags[tag]
end

function MaterialComponent:add_tag(tag)
   if not self._data.tags then
      self._data.tags[tag] = true
      self._data_binding:mark_changed()
   end
end

function MaterialComponent:remove_tag(tag)
   if self._data.tags then
      self._data.tags[tag] = nil
      self._data_binding:mark_changed()
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
