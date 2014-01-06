local ExecutionUnitV2 = require 'components.ai.execution_unit_v2'

local ARGS = {}
local PREV = {}

CALL_PLACEHOLDER_mt = {}
function CALL_PLACEHOLDER_mt.__call(t, compound_action)
   if t.type == ARGS then
      return compound_action:_get_arg(t.key)
   elseif t.type == PREV then
      return compound_action:_get_previous_run_arg(1, t.key)
   end
   assert(false, string.format('unknown placeholder type'))
end

PLACEHOLDER_mt = {}
function PLACEHOLDER_mt.__index(t, key)
   local result = {
      __placeholder = true,
      type = t,
      key = key
   }
   setmetatable(result, CALL_PLACEHOLDER_mt)
   return result
end
setmetatable(ARGS, PLACEHOLDER_mt)
setmetatable(PREV, PLACEHOLDER_mt)


local CompoundAction = class()
CompoundAction.ARGS = ARGS
CompoundAction.PREV = PREV

function CompoundAction:__init(action_unit, activities)
   -- initialize metadata   
   self._execution_unit = action_unit
   radiant.events.listen(self._execution_unit, 'state_changed', self, self._on_unit_state_change)
   self._activities = activities
   
   local action = self._execution_unit:get_action()
   self.name = action.name
   self.does = action.does
   self.priority = action.priority
   self.weight = action.weight
   self.name = action.name
   self.args = action.args
   self.version = 2
end

function CompoundAction:_on_unit_state_change(e)
   if e.state == 'ready' then
      assert(#self._execution_frames == 0)
      local run_args = self._execution_unit:get_run_args()
      table.insert(self._previous_run_args, run_args)
      self:_start_processing_next_activity()
   end
end

function CompoundAction:start_background_processing(ai, entity, ...)
   self._args = { ... }
   assert(not self._execution_frames)
   self._execution_frames = {}
   self._previous_run_args = {}
   self._current_activity = 1
   self._execution_unit:initialize(self._args)
   self._execution_unit:start_background_processing()
end


function CompoundAction:_start_processing_next_activity()
   local activity = self._activities[self._current_activity]
   local translated = self:_replace_placeholders(activity)
   local frame = self._execution_unit:spawn(unpack(translated))
   table.insert(self._execution_frames, frame)
   
   radiant.events.listen(frame, 'ready', function()
      local unit = frame:get_active_execution_unit()
      local run_args = unit:get_run_args()
      table.insert(self._previous_run_args, run_args)
      if self._current_activity < #self._activities then
         self._current_activity = self._current_activity + 1
         self:_start_processing_next_activity(ai)
      else
         self._execution_unit:complete_background_processing()
      end
      return radiant.events.UNLISTEN
   end)
   frame:start_background_processing()
end

function CompoundAction:_replace_placeholders(activity)
   local expanded  = { activity[1] }
   for i=2,#activity do
      local arg = activity[i]
      if type(arg) == 'table' and arg.__placeholder then
         expanded[i] = arg(self)
      else
         expanded[i] = arg
      end
   end
   return expanded
end

function CompoundAction:start()
   self._execution_unit:start()
   for i=1,#self._execution_frames do
      self._execution_frames[i]:start()
   end
end

function CompoundAction:run(ai, entity, ...)
   self._execution_unit:run()
   for i=1,#self._execution_frames do
      self._execution_frames[i]:run()
   end
end

function CompoundAction:stop()
   for i=#self._execution_frames,1,-1 do
      self._execution_frames[i]:stop()
   end
   self._execution_unit:stop()
end

function CompoundAction:stop_background_processing(ai, entity, ...)
   for i=#self._execution_frames,1,-1 do
      self._execution_frames[i]:stop_background_processing()
   end
   self._execution_unit:stop_background_processing()
end

function CompoundAction:_get_arg(i)
   return self._args[i]
end

function CompoundAction:_get_previous_run_arg(rewind_offset, i)
   local index = #self._previous_run_args - rewind_offset + 1
   return self._previous_run_args[index][i]
end

return CompoundAction
