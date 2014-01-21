local ExecutionUnitV2 = require 'components.ai.execution_unit_v2'
local CompoundAction = class()

function CompoundAction:__init(action_unit, activities, when_predicates, think_output_placeholders)
   -- initialize metadata   
   self._log = radiant.log.create_logger('ai.compound_action')
   self._execution_unit = action_unit
   self._when_predicates = when_predicates
   self._activities = activities
   self._think_output_placeholders = think_output_placeholders
   self._run_frames = {}
   self._think_frames = {}
   self._action = self._execution_unit:get_action()
   self.name = self._action.name
   self.does = self._action.does
   self.priority = self._action.priority
   self.weight = self._action.weight
   self.name = self._action.name
   self.args = self._action.args
   self.think_output = self._action.think_output
   self.version = 2
   self._id = stonehearth.ai:get_next_object_id()   
end

function CompoundAction:set_debug_route(debug_route)
   self._debug_route = debug_route .. ' ca:' .. tostring(self._id)
   local prefix = string.format('%s (%s)', self._debug_route, self.name)
   self._log:set_prefix(prefix)
   self._execution_unit:set_debug_route(self._debug_route)
end

function CompoundAction:start_thinking(ai, entity, args)
   self._ai = ai
   self._entity = entity
   self._args = args
   assert(#self._run_frames == 0)
   assert(#self._think_frames == 0)
   self._previous_think_output = {}
   self._previous_frames = {}
   self._current_activity = 1
   
   -- compound actions are not allowed to think so calling short_circuit_thinking
   -- should put it into the ready state
   self._args = self._execution_unit:initialize(args)
   self._execution_unit:short_circuit_thinking()
   assert(self._execution_unit:get_state() == 'ready')
   
   if self._action.transform_arguments then
      self._log:spam('transform_arguments...')
      self._transformed_args = self._action:transform_arguments(self, self._entity, args)
      self._log:spam('transform_arguments returned %s', stonehearth.ai:format_args(self._args))
   end

   for i, clause_fn in ipairs(self._when_predicates) do
      if clause_fn(ai, entity, args) then
         self._log:spam('when %d succeeded', i)
      else
         self._log:spam('when %d failed!  exiting start_thinking() early', i)
         return
      end
   end
   
   -- now wait until all the chained actions are ready
   self:_start_processing_next_activity()
end

function CompoundAction:_start_processing_next_activity()
   while self._current_activity <= #self._activities do
      assert(self._current_activity)
      
      local activity = self._activities[self._current_activity]
      local replaced = self:_replace_placeholders(activity.args)
      
      self:_spam_current_state('start_thinking (%d of %d - %s)', self._current_activity, #self._activities, activity.name)
      local frame = self._ai:spawn(activity.name, replaced)
      if not self._current_activity then
         -- somewhere deep in the spawn, something happened which caused us to stop_thinking.
         -- bail.
         self._log:spam('spawn failed.  aborting')
         return
      end
      frame:set_debug_route(self._debug_route)
      table.insert(self._think_frames, frame)
      
      frame:set_current_entity_state(self._ai.CURRENT)
      frame:start_thinking()
      self:_spam_current_state('after start_thinking (%d of %d - %s)', self._current_activity, #self._activities, activity.name)
      if self._current_activity == 2 then
         self._current_activity = 2
      end
      
      radiant.events.listen(frame, 'state_changed', self, self._on_execution_frame_state_change)
      if frame:get_state() == 'ready' then
         self:_on_frame_ready(frame)
         self._current_activity = self._current_activity + 1
      else
         self._log:spam('frame for %s did not become ready immediately.  returning', activity.name)
         return
      end
   end
   assert(self._current_activity > #self._activities)
   
   -- use the ones specified in the template or the ones generated by our
   -- base action
   self:_spam_current_state('compound action is ready!  setting think_output...')
   local think_output = self._args
   if self._think_output_placeholders then
      think_output = self:_replace_placeholders(self._think_output_placeholders)
   end
   self._ai:set_think_output(think_output)
   self:_spam_current_state('done setting think_output...')   
end

function CompoundAction:_on_frame_ready(frame)
   assert(self._previous_frames)
   assert(self._previous_think_output)
   assert(not self._previous_frames[frame])
   assert(self._current_activity ~= nil)
   self._previous_frames[frame] = true
   local think_output = frame:get_active_execution_unit():get_think_output()
   table.insert(self._previous_think_output, think_output)
end

function CompoundAction:_on_execution_frame_state_change(frame, state)
   if state == 'ready' then
      self:_on_frame_ready(frame)
      self._current_activity = self._current_activity + 1
      self:_start_processing_next_activity(self._ai)
   else
      if self._previous_frames[frame] then
         self._ai:abort('execution frame became un-ready (state:%s).  starting over.', state)
         return
      end
   end
end

function CompoundAction:stop_thinking(ai, entity, ...)
   for i=#self._think_frames,1,-1 do
      local frame = self._think_frames[i]
      radiant.events.unlisten(frame, 'state_changed', self, self._on_execution_frame_state_change)
      frame:stop_thinking()
   end  
   
   self._think_frames = {}
   self._previous_think_output = nil
   self._current_activity = nil
end


function CompoundAction:_replace_placeholders(args)
   local replaced  = {}
   for name, value in pairs(args) do
      if type(value) == 'table' and value.__placeholder then
         local result = value:__eval(self)
         if result == nil then
            self._ai:abort('placeholder %s failed to return a value in %s', tostring(value), self.name)
            return
         end
         replaced[name] = result
      elseif type(value) == 'table' and not radiant.util.is_instance(value) and not radiant.util.is_class(value) then
         replaced[name] = self:_replace_placeholders(value)
      else
         replaced[name] = value
      end
   end
   return replaced
end

function CompoundAction:start()
   self._run_frames = self._think_frames
   self._think_frames = {}

   -- unlisten to everything first.  we're committed at this point.
   for i=1,#self._run_frames do
      radiant.events.unlisten(self._run_frames[i], 'state_changed', self, self._on_execution_frame_state_change)
   end
   
   -- fire it up
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
      -- xxx: i don't think this is actually desired -- tony
      self._run_frames[i]:run()
      --self._run_frames[i]:stop()
   end
   --self._execution_unit:stop()
end

function CompoundAction:stop()
   for i=#self._run_frames,1,-1 do
      self._run_frames[i]:stop()
   end
   self._run_frames = {}
   self._execution_unit:stop()
end

function CompoundAction:get_debug_info()
   local info = {
      name = self.name,
      does = self.does,
      args = stonehearth.ai:format_args(self._args),
      priority = self.priority,
      execution_frames = {}
   }
   local ef = self._think_frames
   if #ef == 0 then
      ef = self._run_frames
   end
   for _, f in ipairs(ef) do
      table.insert(info.execution_frames, f:get_debug_info())
   end
   return info
end


function CompoundAction:_get_transformed_args()
   return self._transformed_args
end

function CompoundAction:_get_args()
   return self._args
end

function CompoundAction:_get_previous_think_output(rewind_offset)
   local index = #self._previous_think_output - rewind_offset + 1
   return self._previous_think_output[index]
end

function CompoundAction:_get_current_entity_state()
   return self._ai.CURRENT
end

function CompoundAction:_get_entity()
   return self._entity
end

function CompoundAction:_spam_current_state(format, ...)
   self._log:spam(string.format(format, ...))
   if self._ai.CURRENT then
      self._log:spam('  CURRENT is %s', tostring(self._ai.CURRENT))
      for key, value in pairs(self._ai.CURRENT) do      
         self._log:spam('  CURRENT.%s = %s', key, tostring(value))
      end   
   else
      self._log:spam('  no CURRENT state!')
   end
end


return CompoundAction
