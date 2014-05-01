local log = radiant.log.create_logger('combat')

TargetTable = class()

-- TODO: decay scores over time and remove if below 0
function TargetTable:__init()
   self._targets = {}
   self._starting_score = 0
end

function TargetTable:get_top()
   -- TODO: return target with highest score
   local target_id = next(self._targets)
   local target = nil

   if target_id ~= nil then
      target = self:_get_valid_target_or_prune(target_id)
   end

   return target
end

function TargetTable:add(target)
   self:modify_score(target, 0)
end

function TargetTable:remove(target)
   self:set_score(target, nil)
   -- if target is not valid, it will get cleaned up in the combat_service:_clean_target_tables
end

function TargetTable:set_score(target, score)
   if target ~= nil and target:is_valid() then
      local target_id = target:get_id()
      self._targets[target_id] = score
   end
end

function TargetTable:get_score(target)
   local score = nil

   if target ~= nil and target:is_valid() then
      local target_id = target:get_id()
      score = self._targets[target_id]
   end

   return score
end

-- modifies the score of the target by delta
-- if target is not in the table, adds it first and adds delta to the starting score
function TargetTable:modify_score(target, delta)
   local score = nil

   if target ~= nil and target:is_valid() then
      local target_id = target:get_id()
      score = self._targets[target_id] or self._starting_score
      score = score + delta
      self._targets[target_id] = score
   end

   return score
end

function TargetTable:remove_invalid_targets()
   local target

   for target_id in pairs(self._targets) do
      self:_get_valid_target_or_prune(target_id)
   end
end

function TargetTable:_get_valid_target_or_prune(target_id)
   local target = radiant.entities.get_entity(target_id)

   if target == nil or not target:is_valid() then
      self._targets[target_id] = nil
      return nil
   end

   return target
end

return TargetTable
