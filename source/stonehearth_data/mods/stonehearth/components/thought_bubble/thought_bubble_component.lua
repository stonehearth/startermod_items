local ThoughtBubbleComponent = class()

--[[
Here are the rules
- Only one thought can be active at a time
- Setting a thought with a lower priority than the active thought is a no-op (maybe this sucks)
- Removing a thought that has never been set is a no-op, so it's ok.

]]
function ThoughtBubbleComponent:__init(entity, data_binding)
   self._data_binding = data_binding
   self._entity = entity
   self._thought_effect = nil
   self._thought_uri = nil
   self._thought_priority = 0
   --self._data_binding:update(self._statuses)
end

function ThoughtBubbleComponent:set_thought(uri, priority)
   if not priority then
      priority = 0
   end

   if uri == self._thought_uri and priority == self._thought_priority then
      return
   end
   
   if self._thought_effect then
      if priority >= self._thought_priority then
         self:unset_thought(self._thought_uri)
      else
         return
      end
   end

   self._thought_effect = radiant.effects.run_effect(self._entity, uri)
   self._thought_uri = uri
   self._thought_priority = priority

   --self._data_binding:mark_changed()   
   return self._thought_effect
end

function ThoughtBubbleComponent:unset_thought(uri)
   if uri == self._thought_uri and self._thought_effect then
      self._thought_effect:stop()
      self._thought_effect = nil
   end
end

return ThoughtBubbleComponent
