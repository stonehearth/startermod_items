local ExecutionUnitV2 = require 'components.ai.execution_unit_v2'
local CompoundAction = class()

function CompoundAction:__init(action_unit, activities)
   -- initialize metadata   
   self._execution_unit = action_unit
   self._activities = activities
   self._run_frames = {}
   self._think_frames = {}
   local action = self._execution_unit:get_action()
   action.name = 'ca(' .. action.name .. ')'
   self.name = action.name
   self.does = action.does
   self.priority = action.priority
   self.weight = action.weight
   self.name = action.name
   self.args = action.args
   self.version = 2
end

function CompoundAction:set_debug_route(debug_route)
   self._execution_unit:set_debug_route(debug_route)
end

function CompoundAction:start_thinking(ai, entity, ...)
   self._args = { ... }
   assert(#self._run_frames == 0)
   assert(#self._think_frames == 0)
   self._previous_run_args = {}
   self._current_activity = 1
   
   radiant.events.listen(self._execution_unit, 'ready', function(_, state)
         assert(#self._think_frames == 0)
         local run_args = self._execution_unit:get_run_args()
         table.insert(self._previous_run_args, run_args)
         self:_start_processing_next_activity(ai)
         return radiant.events.UNLISTEN
      end)
      
   self._execution_unit:initialize(self._args, self._debug_route)
   self._execution_unit:start_thinking()
end

function CompoundAction:_start_processing_next_activity(ai)
   local activity = self._activities[self._current_activity]
   local translated = self:_replace_placeholders(activity)
   local frame = ai:spawn(unpack(translated))
   table.insert(self._think_frames, frame)
   
   radiant.events.listen(frame, 'ready', function()
      local unit = frame:get_active_execution_unit()
      local run_args = unit:get_run_args()
      table.insert(self._previous_run_args, run_args)
      if self._current_activity < #self._activities then
         self._current_activity = self._current_activity + 1
         self:_start_processing_next_activity(ai)
      else
         ai:set_run_arguments()
      end
      return radiant.events.UNLISTEN
   end)
   frame:start_thinking()
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
   self._run_frames = self._think_frames
   self._think_frames = {}

   self._execution_unit:start()
   for i=1,#self._run_frames do
      self._run_frames[i]:start()
   end
end

function CompoundAction:run(ai, entity, ...)
   self._execution_unit:run()
   for i=1,#self._run_frames do
      -- stop each action immediately after it completes.
      -- this lets it release resources after they are no longer needed
      -- instead of waiting until the entire action sequence completes
      self._run_frames[i]:run()
      self._run_frames[i]:stop()
   end
   self._execution_unit:stop()
end

function CompoundAction:stop()
   for i=#self._run_frames,1,-1 do
      self._run_frames[i]:stop()
   end
   self._run_frames = {}
   self._execution_unit:stop()
end

function CompoundAction:stop_thinking(ai, entity, ...)
   for i=#self._think_frames,1,-1 do
      self._think_frames[i]:stop_thinking()
   end  
   self._think_frames = {}
   self._previous_run_args = nil
   self._current_activity = nil
   self._execution_unit:stop_thinking()
end

function CompoundAction:_get_arg(i)
   return self._args[i]
end

function CompoundAction:_get_previous_run_arg(rewind_offset, i)
   local index = #self._previous_run_args - rewind_offset + 1
   return self._previous_run_args[index][i]
end

return CompoundAction
