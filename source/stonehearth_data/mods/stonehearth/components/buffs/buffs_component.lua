Buff = require 'components.buffs.buff'
AttributeModifier = require 'components.attributes.attribute_modifier'

local BuffsComponent = class()

function BuffsComponent:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()

   if not self._sv.initialized then
      self._sv.buffs = {}
      self._sv._ref_counts = radiant.create_controller('stonehearth:lib:reference_counter')
      self._sv.initialized = true
      self.__saved_variables:mark_changed()
   else
      radiant.events.listen_once(radiant, 'radiant:game_loaded', function(e)
            for uri, sv in pairs(self._sv.buffs) do
               self._sv.buffs[uri] = Buff(self._entity, sv)
            end
         end)
   end
end

function BuffsComponent:destroy()
   self._sv._ref_counts:clear()

   for uri, buff in pairs(self._sv.buffs) do
      self:remove_buff(uri)
   end

   self._sv._ref_counts:destroy()
end

function BuffsComponent:add_buff(uri)
   assert(not string.find(uri, '%.'), 'tried to add a buff with a uri containing "." Use an alias instead')

   local buff
   local ref_count = self._sv._ref_counts:add_ref(uri)

   if ref_count == 1 then
      buff = Buff(self._entity, uri)
      self._sv.buffs[uri] = buff
      self.__saved_variables:mark_changed()

      radiant.events.trigger_async(self._entity, 'stonehearth:buff_added', {
            entity = self._entity,
            uri = uri,
            buff = buff,
         })
   else
      buff = self._sv.buffs[uri]
      assert(buff)
   end

   return buff
end

function BuffsComponent:has_buff(uri)
   local result = self._sv._ref_counts:get_ref_count(uri) > 0
   return result
end

function BuffsComponent:remove_buff(uri)
   local ref_count = self._sv._ref_counts:dec_ref(uri)

   if ref_count == 0 then
      local buff = self._sv.buffs[uri]
      if buff then
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
end

return BuffsComponent
