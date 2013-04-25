
local AutomationQueueMgr = class()

local md = require 'radiant.core.md'
local om = require 'radiant.core.om'

md:register_msg('radiant.add_automation_task')
md:register_msg_handler('radiant.behaviors.run_automation_queue', AutomationQueueMgr)
md:register_activity('radiant.activities.run_automation_queue')

AutomationQueueMgr['radiant.md.create'] = function(self, entity)
   self._entity = entity
   self._queue = om:add_component(entity, 'automation_queue')
   
   md:listen(entity, 'radiant.add_automation_task', self);
   md:send_msg(entity, 'radiant.ai.register_need', self, true)
   md:send_msg(entity, 'radiant.ai.register_behavior', self, true)
end

-- Need messages

AutomationQueueMgr['radiant.ai.needs.recommend_activity'] = function(self)   
   local task = self:get_next_task()
   if not task then
      return 0, nil
   end
   return 3, md:create_activity('radiant.activities.run_automation_queue', task)
end

-- Behavior messages

AutomationQueueMgr['radiant.ai.behaviors.get_priority'] = function(self, activity)
   return activity.name == 'radiant.activities.run_automation_queue' and 1 or 0
end

AutomationQueueMgr['radiant.ai.behaviors.get_next_action'] = function(self, activity)
   local task = activity.args[1]
   assert(task)
  
   local handler = self._current_task:get_handler()
   return md:call_handler(handler, 'radiant.ai.behaviors.get_next_action')
end

-- AutomationQueueMgr messages

-- xxx: why is this a message??!
AutomationQueueMgr['radiant.add_automation_task'] = function(self, name, ...)
   local handler = md:create_msg_handler(name, ...)
   self._queue:add_task(handler) -- xxx: how is this supposed to get saved?  pass the name too, please
end

function AutomationQueueMgr:get_next_task()
   -- always start searching at the beginning of the queue so that
   -- higher priority tasks can pre-empt lower prioriting ones
   for task in self._queue:contents() do
      if not task:finished() then
         return task
      end
   end
   return nil
end
