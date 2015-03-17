TargetTable = class()

-- TODO: decay scores over time and remove if below 0
function TargetTable:initialize(name)
   assert(self._sv)
   self._sv.name = name
   self._sv.targets = {}
   self.__saved_variables:mark_changed()
end

function TargetTable:activate()
   self._log = radiant.log.create_logger('target_tables')
                              :set_prefix(self._sv.name or '??')
end

function TargetTable:destroy()
   radiant.events.unpublish(self)
end

function TargetTable:get_top(fitness_fn)
   local best_entity, best_score
   local changed = false

   for id, entry in pairs(self._sv.targets) do
      local entity, value = entry.entity, entry.value
      if not entity or not entity:is_valid() or value <= 0 then
         self._sv.targets[id] = nil
         changed = true
      else
         local score = value
         if fitness_fn then
            score = fitness_fn(entity, value)
         end
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

function TargetTable:get_value(target)
   if not target or not target:is_valid() then
      return
   end
   local id = target:get_id()
   local entry = self._sv.targets[id]
   if not entry then
      return
   end
   return entry.value
end

-- modifies the score of the target by delta
function TargetTable:modify_value(target, delta)
   if not target or not target:is_valid() then
      return
   end

   local id = target:get_id()
   local entry = self._sv.targets[id]
   if not entry then
      entry = {
         entity = target,
         value = 0
      }
      self._sv.targets[id] = entry
   end
   entry.value = entry.value + delta

   self._log:spam('value for %s is now %.2f (delta: %.2f)', target, entry.value, delta)
   self.__saved_variables:mark_changed()
   self:_signal_changed()
end

function TargetTable:_signal_changed()
   radiant.events.trigger_async(self, 'stonehearth:target_table_changed')
end

return TargetTable
