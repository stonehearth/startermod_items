local ThoughtBubbleComponent = class()

--[[
Here are the rules
- Only one thought can be active at a time
- Setting a thought with a lower priority than the active thought is a no-op (maybe this sucks)
- Removing a thought that has never been set is a no-op, so it's ok.

]]
function ThoughtBubbleComponent:initialize(entity, json)
   self._entity = entity
   self._thoughts = {}
   self._next_id = 2
end

function ThoughtBubbleComponent:add_thought(uri, priority)
   local id = self:_add_thought(uri, priority)
   self:_update_thoughts()

   return radiant.lib.Destructor.new(function()
         self._thoughts[id] = nil
         self:_update_thoughts()
      end)
end

function ThoughtBubbleComponent:_add_thought(uri, priority)
   local id = self._next_id
   local thought = {
      id = id,
      uri = uri,
      priority = priority or 0
   }
   self._thoughts[id] = thought
   self._next_id = self._next_id + 1

   return id
end

function ThoughtBubbleComponent:_update_thoughts()
   local best = self:_get_best_thought()

   if best ~= self._active_thought then
      self._active_thought = best
      if self._thought_effect then
         self._thought_effect:stop()
         self._thought_effect = nil
      end
      if best then
         local id = best.id
         self._thought_effect = radiant.effects.run_effect(self._entity, best.uri)
         self._thought_effect:set_finished_cb(function()
               self._thoughts[id] = nil
               self:_update_thoughts()
            end)
      end
   end
end

function ThoughtBubbleComponent:_get_best_thought()
   local best, best_pri
   for _, thought in pairs(self._thoughts) do
      local pri = thought.priority
      if not best or pri > best_pri then
         best, best_pri = thought, pri
      end
   end
   return best
end

function ThoughtBubbleComponent:unset_thought(uri)
   if uri == self._thought_uri and self._thought_effect then
      self._thought_effect:stop()
      self._thought_effect = nil
      self._thought_uri = nil
      self._thought_priority = nil
   end
end

return ThoughtBubbleComponent
