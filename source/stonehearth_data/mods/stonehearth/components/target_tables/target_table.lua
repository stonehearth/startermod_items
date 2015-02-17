TargetTable = class()

-- TODO: decay scores over time and remove if below 0
function TargetTable:initialize()
   assert(self._sv)
   self._sv.targets = {}
   self.__saved_variables:mark_changed()
end

function TargetTable:destroy()
   radiant.events.unpublish(self)
end

function TargetTable:get_top(fitness_fn)
   local best_entity, best_score
   local changed = false

   for id, entry in pairs(self._sv.targets) do
      local entity, score = entry.entity, entry.score
      if not entity:is_valid() then
         self._sv.targets[id] = nil
         changed = true
      else
         local score = fitness_fn and fitness_fn(entity, score) or score
         if score then
            if not best_score or score > best_score then
               best_score = score
               best_entity = entity
            end
         end
      end
   end
   if changed then
      self.__saved_variables:mark_changed()
   end
   return best_entity, best_score
end

function TargetTable:get_score(target)
   if not target or not target:is_valid() then
      return
   end
   local id = target:get_id()
   local entry = self._sv.targets[id]
   if not entry then
      return
   end
   return entry.score
end

-- modifies the score of the target by delta
function TargetTable:modify_score(target, delta)
   if not target or not target:is_valid() then
      return
   end

   local id = target:get_id()
   local entry = self._sv.targets[id]
   if not entry then
      entry = {
         entity = target,
         score = 0
      }
      self._sv.targets[id] = entry
   end
   entry.score = entry.score + delta
   self.__saved_variables:mark_changed()
   self:_signal_changed()
end

function TargetTable:_signal_changed()
   radiant.events.trigger_async(self, 'stonehearth:target_table_changed')
end

return TargetTable
