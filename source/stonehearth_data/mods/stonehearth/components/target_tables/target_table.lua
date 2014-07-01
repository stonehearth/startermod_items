TargetTable = class()

function TargetTable:__init()
   self._starting_score = 1
end

-- TODO: decay scores over time and remove if below 0
function TargetTable:initialize()
   assert(self._sv)

   self._sv.targets = {}
   self._sv.top = nil
   self._sv.top_score = nil
   self._sv.top_is_dirty = false
end

function TargetTable:get_top()
   if not self:_top_is_valid() then
      self:_calculate_top()
   end

   return self._sv.top, self._sv.top_score
end

function TargetTable:get_targets()
   return self._sv.targets
end

function TargetTable:add(target)
   self:modify_score(target, 0)
end

function TargetTable:remove(target)
   if target ~= nil and target:is_valid() then
      local target_id = target:get_id()
      self._sv.targets[target_id] = nil
      self.__saved_variables:mark_changed()

      if target == self._sv.top then
         self:_mark_top_dirty()
      end
   end
   -- if target is not valid, it will get cleaned up in the combat_service:_clean_target_tables
end

function TargetTable:set_score(target, score)
   if target ~= nil and target:is_valid() then
      local target_id = target:get_id()
      self._sv.targets[target_id] = score
      self.__saved_variables:mark_changed()
      self:_check_update_top(target, score)
   end
end

function TargetTable:get_score(target)
   local score = nil

   if target ~= nil and target:is_valid() then
      local target_id = target:get_id()
      score = self._sv.targets[target_id]
   end

   return score
end

-- modifies the score of the target by delta
-- if target is not in the table, adds it first and adds delta to the starting score
function TargetTable:modify_score(target, delta)
   local score = nil

   if target ~= nil and target:is_valid() then
      local target_id = target:get_id()
      score = self._sv.targets[target_id] or self._starting_score
      score = score + delta
      self._sv.targets[target_id] = score
      self.__saved_variables:mark_changed()
      self:_check_update_top(target, score)
   end

   return score
end

function TargetTable:remove_invalid_targets()
   local target
   local changed = false

   for target_id in pairs(self._sv.targets) do
      target = self:_get_valid_target_or_prune(target_id)
      if target then
         changed = true
      end
   end

   if changed then
      self.__saved_variables:mark_changed()
   end
end

function TargetTable:_calculate_top()
   local top_entity, top_id, top_score

   while true do
      if next(self._sv.targets) == nil then
         self:_set_top(nil, nil)
         return
      end

      top_id = nil

      for id, score in pairs(self._sv.targets) do
         if top_id == nil or score > top_score then
            top_id, top_score = id, score
         end
      end

      top_entity = self:_get_valid_target_or_prune(top_id)
      if top_entity then
         self:_set_top(top_entity, top_score)
         return
      end
   end
end

function TargetTable:_set_top(target, score)
   self._sv.top = target
   self._sv.top_score = score
   self._sv.top_is_dirty = false
   self.__saved_variables:mark_changed()
end

function TargetTable:_check_update_top(target, score)
   if self._sv.top_is_dirty then
      return
   end
   if self._sv.top == nil or score > self._sv.top_score then
      self:_set_top(target, score)
   end
end

function TargetTable:_mark_top_dirty()
   self._sv.top_is_dirty = true
   self.__saved_variables:mark_changed()
end

function TargetTable:_top_is_valid()
   if self._sv.top_is_dirty then
      return false
   end

   if self._sv.top == nil then
      return true
   end

   return self._sv.top:is_valid()
end

function TargetTable:_get_valid_target_or_prune(target_id)
   local target = radiant.entities.get_entity(target_id)

   if target == nil or not target:is_valid() then
      self._sv.targets[target_id] = nil
      return nil
   end

   return target
end

return TargetTable
