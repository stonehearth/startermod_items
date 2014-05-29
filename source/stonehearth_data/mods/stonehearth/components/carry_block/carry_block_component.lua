local CarryBlock = class()

function CarryBlock:initialize(entity, json)
   self._sv = self.__saved_variables:get_data()
   self._entity = entity
   self._attached_items = entity:add_component('attached_items')

   if self._sv._carried_item then
      self:_set_carrying_internal(self._sv._carried_item)
   end
end

function CarryBlock:get_carrying()
   if self._sv._carried_item ~= nil and self._sv._carried_item:is_valid() then
      return self._sv._carried_item
   end
   return nil
end

function CarryBlock:is_carrying()
   return self:get_carrying() ~= nil
end

function CarryBlock:set_carrying(new_item)
   -- if new_item is not valid, just remove what we're carrying
   if new_item == nil or not new_item:is_valid() then
      self:_remove_carrying()
      return
   end

   if self._sv._carried_item ~= nil and self._sv._carried_item:is_valid() then
      -- if item hasn't changed, do nothing
      if new_item:get_id() == self._sv._carried_item:get_id() then
         return
      end
      self:_destroy_carried_item_trace()
   else
      -- only set these if we weren't carrying anything before
      radiant.entities.set_posture(self._entity, 'stonehearth:carrying')
      radiant.entities.add_buff(self._entity, 'stonehearth:buffs:carrying')      
   end

   self:_set_carrying_internal(new_item)

   radiant.events.trigger_async(self._entity, 'stonehearth:carry_block:carrying_changed')
end

function CarryBlock:_set_carrying_internal(new_item)
   self._sv._carried_item = new_item
   self._attached_items:add_item('carry', new_item)
   self:_create_carried_item_trace()
   self.__saved_variables:mark_changed()
end

function CarryBlock:_remove_carrying()
   -- don't test for is_valid as it may already be destroyed
   if self._sv._carried_item ~= nil then
      self._attached_items:remove_item('carry')
      self:_destroy_carried_item_trace()

      radiant.entities.unset_posture(self._entity, 'stonehearth:carrying')
      radiant.entities.remove_buff(self._entity, 'stonehearth:buffs:carrying')

      radiant.events.trigger_async(self._entity, 'stonehearth:carry_block:carrying_changed')

      self._sv._carried_item = nil
      self.__saved_variables:mark_changed()
   end
end

function CarryBlock:_create_carried_item_trace()
   self._carried_item_trace = self._sv._carried_item:trace_object('trace carried item')
      :on_destroyed(function()
            self:_remove_carrying()
         end)
end

function CarryBlock:_destroy_carried_item_trace()
   if self._carried_item_trace then
      self._carried_item_trace:destroy()
      self._carried_item_trace = nil
   end
end

function CarryBlock:destroy()
   if self._attached_items_trace then
      self._attached_items_trace:destroy()
      self._attached_items_trace = nil
   end

   self:_destroy_carried_item_trace()
end

return CarryBlock
