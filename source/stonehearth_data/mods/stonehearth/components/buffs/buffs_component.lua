Buff = require 'components.buffs.buff'
AttributeModifier = require 'components.attributes.attribute_modifier'

local BuffsComponent = class()

function BuffsComponent:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()

   self._sv.buffs = self._sv.buffs or {}
   for uri, sv in pairs(self._sv.buffs) do
      self._sv.buffs[uri] = Buff(self._entity, sv)
   end
end

function BuffsComponent:add_buff(uri)
   assert(not string.find(uri, '%.'), 'tried to add a buff with a uri containing "." Use an alias instead')

   local buff
   if self._sv.buffs[uri] then
      -- xxx, check a policy for status vs. stacking buffs
      buff = self._sv.buffs[uri]
   else 
      buff = Buff(self._entity, uri)
      self._sv.buffs[uri] = buff
      self.__saved_variables:mark_changed()

      radiant.events.trigger_async(self._entity, 'stonehearth:buff_added', {
            entity = self._entity,
            uri = uri,
            buff = buff,
         })
   end

   return buff
end

function BuffsComponent:has_buff(uri)
   return self._sv.buffs[uri] ~= nil
end

function BuffsComponent:remove_buff(uri)
   if self._sv.buffs[uri] then
      local buff = self._sv.buffs[uri]
      self._sv.buffs[uri] = nil
      buff:destroy()
      self.__saved_variables:mark_changed()

      radiant.events.trigger_async(self._entity, 'stonehearth:buff_removed', {
            entity = self._entity,
            uri = uri,
            buff = buff,
         })
   end
end


return BuffsComponent
