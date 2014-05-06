local CarryObserver = class()

function CarryObserver:initialize(entity, json)
   self._carried_item = nil
   self._entity = entity
   self._attached_items = entity:add_component('attached_items')

   -- TOOD: test save
   -- push_object_changes should restore carried item and carried item trace on load
   self._attached_items_trace = self._attached_items:trace_items('carry observer')
      :on_added(
         function (bone_name, item)
            if bone_name == 'carry' then
               self:_on_carried_item_added(item)
            end
         end
      )
      :on_removed(
         function (bone_name)
            if bone_name == 'carry' then
               self:_on_carried_item_removed()
            end
         end
      )
      :push_object_state()
end

local function is_valid_item(item)
   return item ~= nil and item:is_valid()
end

function CarryObserver:_on_carried_item_added(new_item)
   -- if new_item is not valid, just remove what we're carrying
   if new_item == nil or not new_item:is_valid() then
      self:_on_carried_item_removed()
      return
   end

   local start_carrying = false

   if self._carried_item ~= nil and self._carried_item:is_valid() then
      -- if item hasn't changed, do nothing
      if new_item:get_id() == self._carried_item:get_id() then
         return
      end
      self:_destroy_carried_item_trace()
   else
      start_carrying = true
   end

   self._carried_item = new_item
   self:_create_carried_item_trace()

   if start_carrying then
      -- only set these if we weren't carrying anything before
      radiant.entities.set_posture(self._entity, 'carrying')
      radiant.entities.add_buff(self._entity, 'stonehearth:buffs:carrying')
   end

   radiant.events.trigger_async(self._entity, 'stonehearth:attached_items:carrying_changed')
end

function CarryObserver:_on_carried_item_removed()
   self:_destroy_carried_item_trace()
   self._carried_item = nil

   radiant.entities.unset_posture(self._entity, 'carrying')
   radiant.entities.remove_buff(self._entity, 'stonehearth:buffs:carrying')

   radiant.events.trigger_async(self._entity, 'stonehearth:attached_items:carrying_changed')
end

function CarryObserver:_create_carried_item_trace()
   self._carried_item_trace = self._carried_item:trace_object('trace carried item')
      :on_destroyed(
         function()
            self._attached_items:remove_item('carry')
         end
      )
end

function CarryObserver:_destroy_carried_item_trace()
   if self._carried_item_trace then
      self._carried_item_trace:destroy()
      self._carried_item_trace = nil
   end
end

function CarryObserver:destroy()
   if self._attached_items_trace then
      self._attached_items_trace:destroy()
      self._attached_items_trace = nil
   end

   self:_destroy_carried_item_trace()
end

return CarryObserver
