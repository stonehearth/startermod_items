local Point3 = _radiant.csg.Point3
local TraceCategories = _radiant.dm.TraceCategories

local CarryBlock = class()

function CarryBlock:initialize(entity, json)
   self._sv = self.__saved_variables:get_data()
   self._entity = entity

   if self._sv._carried_item then
      self:_create_carried_item_trace()
   end
end

function CarryBlock:get_carrying()
   if self._sv._carried_item and self._sv._carried_item:is_valid() then
      return self._sv._carried_item
   end
   return nil
end

function CarryBlock:is_carrying()
   return self:get_carrying() ~= nil
end

function CarryBlock:set_carrying(new_item)
   -- if new_item is not valid, just remove what we're carrying
   if not new_item or not new_item:is_valid() then
      self:_remove_carrying()
      return
   end

   -- if item hasn't changed, do nothing
   if new_item == self._sv._carried_item then
      return
   end

   if self._sv._carried_item then
      self:_destroy_carried_item_trace()
   else
      -- only set these if we weren't carrying anything before
      radiant.entities.set_posture(self._entity, 'stonehearth:carrying')
      radiant.entities.add_buff(self._entity, 'stonehearth:buffs:carrying')      
   end
   
   self._sv._carried_item = new_item
   self.__saved_variables:mark_changed()
   
   self._entity:add_component('entity_container')
                     :add_child_to_bone(new_item, 'carry')
   radiant.entities.move_to(new_item, Point3.zero)
   
   self:_create_carried_item_trace()

   radiant.events.trigger_async(self._entity, 'stonehearth:carry_block:carrying_changed')
end

function CarryBlock:_remove_carrying()
   -- don't test for is_valid as it may already be destroyed
   if self._sv._carried_item then
      self:_destroy_carried_item_trace()

      if self._sv._carried_item:is_valid() then
         self._entity:add_component('entity_container')
                           :remove_child(self._sv._carried_item:get_id())
      end

      self._sv._carried_item = nil
      self.__saved_variables:mark_changed()

      radiant.entities.unset_posture(self._entity, 'stonehearth:carrying')
      radiant.entities.remove_buff(self._entity, 'stonehearth:buffs:carrying')

      radiant.events.trigger_async(self._entity, 'stonehearth:carry_block:carrying_changed')
   end
end

function CarryBlock:_create_carried_item_trace()
   self._carried_item_trace = self._sv._carried_item:trace_object('trace carried item')
      :on_destroyed(function()
            self:_remove_carrying()
         end)

   local mob = self._sv._carried_item:get_component('mob')
   if mob then
      self._mob_parent_trace = mob:trace_parent('trace carried item', TraceCategories.SYNC_TRACE)
         :on_changed(function(parent)
               if parent ~= self._entity then
                  self:_remove_carrying()
               end
            end)
   end
end

function CarryBlock:_destroy_carried_item_trace()
   if self._carried_item_trace then
      self._carried_item_trace:destroy()
      self._carried_item_trace = nil
   end
   if self._mob_parent_trace then
      self._mob_parent_trace:destroy()
      self._mob_parent_trace = nil
   end
end

function CarryBlock:destroy()
   self:_destroy_carried_item_trace()
end

return CarryBlock
