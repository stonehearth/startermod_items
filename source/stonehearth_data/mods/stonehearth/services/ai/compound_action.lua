
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

function CompoundAction:__init(base_action, activities)
   -- initialize metadata
   self.name = base_action.name
   self.does = base_action.does
   self.priority = base_action.priority
   self.weight = base_action.weight
   self.name = base_action.name
   self.args = base_action.args
   self.version = 2
   self._activities = activities
   
   -- verify the base action doesn't implement anything...
   assert(not base_action.think)
   assert(not base_action.forget)
   assert(not base_action.start)
   assert(not base_action.run)
   assert(not base_action.stop)
end

function CompoundAction:start_background_processing(ai, entity, ...)
   self._args = { ... }
   
   assert(not self._execution_frames)
   self._execution_frames = {}
   self._previous_run_args = {}
   self._current_activity = 1
   self:_start_processing_next_activity(ai)
end

function CompoundAction:_start_processing_next_activity(ai)
   local activity = self._activities[self._current_activity]
   local translated = self:_replace_placeholders(activity)
   local frame = ai:spawn(unpack(translated))
   table.insert(self._execution_frames, frame)
   
   radiant.events.listen(frame, 'ready', function()
      local run_args = frame:get_active_execution_unit():get_run_args()
      table.insert(self._previous_run_args, run_args)
      if self._current_activity < #self._activities then
         self._current_activity = self._current_activity + 1
         self:_start_processing_next_activity(ai)
      else
         ai:complete_background_processing()
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
      end
   end
   return expanded
end

function CompoundAction:start()
   for i=1,#self._execution_frames do
      self._execution_frames[i]:start()
   end
end

function CompoundAction:run(ai, entity, ...)
   for i=1,#self._execution_frames do
      self._execution_frames[i]:run()
   end
end

function CompoundAction:stop()
   for i=#self._execution_frames,1,-1 do
      self._execution_frames[i]:stop()
   end
end

function CompoundAction:stop_background_processing(ai, entity, ...)
   for i=#self._execution_frames,1,-1 do
      self._execution_frames[i]:stop_background_processing()
   end
end

function CompoundAction:_get_arg(i)
   return self._args[i]
end

function CompoundAction:_get_previous_run_arg(rewind_offset, i)
   local index = #self._previous_run_args - rewind_offset + 1
   return self._previous_run_args[index][i]
end

return CompoundAction
