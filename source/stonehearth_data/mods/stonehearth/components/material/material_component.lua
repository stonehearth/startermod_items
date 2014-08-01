--[[
   Todo, document this thing
]]

local MaterialComponent = class()

function MaterialComponent:initialize(entity, json)
   self._sv = self.__saved_variables:get_data()
   if not self._sv.tags then
      self:set_tags(json.tags)      
   end
end

function MaterialComponent:is(tags_string)
   assert(type(tags_string) == 'string', 'first argument to is() must be a string')
   
   local tags = radiant.util.split_string(tags_string)
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

function MaterialComponent:get_tags()
   return self._sv.tags_string
end

function MaterialComponent:set_tags(tag_string)
   self._sv.tags = {}
   self._sv.tags_string = tag_string
   if tag_string then
      for _, tag in ipairs(radiant.util.split_string(tag_string)) do
         self._sv.tags[tag] = true
      end
   end   
   self.__saved_variables:mark_changed()
end

function MaterialComponent:remove_tag(tag)
   if self._sv.tags then
      self._sv.tags[tag] = nil
      self.__saved_variables:mark_changed()
   end
end

return MaterialComponent
