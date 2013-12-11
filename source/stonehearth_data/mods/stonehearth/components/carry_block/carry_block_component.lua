--[[
   Attached to entities that are arranged artfully around the a center of attention.
   (For example, denotes seats around a fire, spots near a juggler, etc.)
]]

local CarryBlock = class()

function CarryBlock:__init(entity)
   self._entity = entity
   self._carry_block = entity:add_component('carry_block')

   self._trace = self._carry_block:trace_carrying('adding buffs')
      :on_changed(function(item)
         self:_on_carrying_changed(item)
      end)
end

function CarryBlock:set_carrying(item)
   if not item or not item:is_valid() then
      self._carry_block:clear_carrying()
      radiant.entities.unset_posture(self._entity, 'carrying')
      radiant.entities.remove_buff(self._entity, 'stonehearth:buffs:carrying')
   else
      self._carry_block:set_carrying(item)
      radiant.entities.set_posture(self._entity, 'carrying')
      radiant.entities.add_buff(self._entity, 'stonehearth:buffs:carrying')      
   end
end

function CarryBlock:get_carrying()
   if self._carry_block:is_carrying() then
      return self._carry_block:get_carrying()
   end
   return nil
end

function CarryBlock:_on_carrying_changed(item)
   if item and item:is_valid() then     
      self._carrying_item_trace = item:trace_object('carry destroy trace')
         :on_destroyed(function()
            self:set_carrying(nil)
         end)
   else
      if self._carrying_item_trace then
         self._carrying_item_trace:destroy()
         self._carrying_item_trace = nil
      end
   end
end

return CarryBlock
