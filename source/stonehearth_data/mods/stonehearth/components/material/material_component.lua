--[[
   Todo, document this thing
]]

local MaterialComponent = class()

function MaterialComponent:__init()
   self._tags = {}
end

function MaterialComponent:__create(entity, json)
   if json.tags then
      for _, tag in ipairs(self:_split_string(json.tags)) do
         self._tags[tag] = true
      end
   end
   self.__savestate = radiant.create_datastore({
         tags = self._tags
      })
end

function MaterialComponent:is(tags_string)
   assert(type(tags_string) == 'string', 'first argument to is() must be a string')
   
   local tags = self:_split_string(tags_string)
   for _, tag in ipairs(tags) do
      if not self._tags[tag] then
         return false
      end
   end
   return true
end

function MaterialComponent:has_tag(tag)
   return self._tags[tag]
end

function MaterialComponent:add_tag(tag)
   if not self._tags then
      self._tags[tag] = true
      self.__savestate:mark_changed()
   end
end

function MaterialComponent:remove_tag(tag)
   if self._tags then
      self._tags[tag] = nil
      self.__savestate:mark_changed()
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
