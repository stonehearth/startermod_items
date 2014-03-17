Buff = require 'components.buffs.buff'
AttributeModifier = require 'components.attributes.attribute_modifier'

local BuffsComponent = class()

function BuffsComponent:__init()
   self._buffs = {}
   self._controllers = {}
   self._attribute_modifiers = {}
   self._injected_ais = {}
   self._effects = {}
end

function BuffsComponent:initialize(entity, json)
   self._entity = entity   
   self.__saved_variables = radiant.create_datastore({
         buffs = self._buffs
      })
end

function BuffsComponent:restore(entity, savestate)
   self.__saved_variables:read_data(function (o)
         for uri, ss in pairs(o.buffs) do
            self.__buffs[uri] = Buff(self._entity, ss)
         end
      end)
end

function BuffsComponent:add_buff(uri)
   assert(not string.find(uri, '%.'), 'tried to add a buff with a uri containing "." Use an alias instead')

   local buff
   if self._buffs[uri] then
      -- xxx, check a policy for status vs. stacking buffs
      buff = self._buffs[uri]
   else 
      buff = Buff(self._entity, uri)
      self._buffs[uri] = buff
      self.__saved_variables:mark_changed()
      radiant.events.trigger(self._entity, 'stonehearth:buff_added', {
            entity = self._entity,
            uri = uri,
            buff = buff,
         })
   end

   return buff
end

function BuffsComponent:has_buff(uri)
   return self._buffs[uri] ~= nil
end

function BuffsComponent:remove_buff(uri)
   if self._buffs[uri] then
      local buff = self._buffs[uri]
      self._buffs[uri] = nil
      buff:destroy()
      self.__saved_variables:mark_changed()

      radiant.events.trigger(self._entity, 'stonehearth:buff_removed', {
            entity = self._entity,
            uri = uri,
            buff = buff,
         })
   end
end


return BuffsComponent
