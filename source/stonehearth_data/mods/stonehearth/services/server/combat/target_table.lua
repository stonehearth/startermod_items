local log = radiant.log.create_logger('combat')

TargetTable = class()

-- TODO: decay scores over time and remove if 0
function TargetTable:__init()
   self._targets = {}
   self._starting_score = 1
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

-- target can be an entity or and entity_id
function TargetTable:add_entry(target)
   if target ~= nil and target:is_valid() then
      local target_id = target:get_id()
      local score = self._targets[target_id]

      if score == nil then
         self._targets[target_id] = self._starting_score
      end
   end
end

function TargetTable:remove_entry(target)
   if target ~= nil and target:is_valid() then
      local target_id = target:get_id()
      self._targets[target_id] = nil
   end
   -- if target is not valid, it will get cleaned up in the combat_service:_clean_target_tables
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
