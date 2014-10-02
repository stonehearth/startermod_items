
local StacksModelRenderer = class()

function StacksModelRenderer:initialize(entity, json)
   self._sv = self.__saved_variables:get_data()
   self._entity = entity

   --if self._sv._carried_item then
      --self:_create_carried_item_trace()
   --end

   self:_trace_object()
end

function StacksModelRenderer:_trace_object()
   self._item_component = self._entity:get_component('item')
   if self._item_component ~= nil then
      self._item_trace = self._item_component:trace_stacks('trace item stacks')
         :on_changed(function()
               self:_set_model_variant()
            end)
   end
end

function StacksModelRenderer:_set_model_variant()
   local stacks = self._item_component:get_stacks()
   stacks = stacks + 1
end

function StacksModelRenderer:destroy()
   if self._item_trace ~= nil then
      self._item_trace:destroy()
   end
end

return StacksModelRenderer
