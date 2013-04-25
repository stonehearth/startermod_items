local UserQueue = class()

local log = require 'radiant.core.log'
local md = require 'radiant.core.md'

md:register_msg('radiant.queue_user_behavior')
md:register_msg_handler('radiant.behaviors.run_user_queue', UserQueue)
md:register_activity('radiant.activities.run_user_queue')

UserQueue['radiant.md.create'] = function(self, entity)
   log:debug('constructing new user_queue handler')
   
   self.entity = entity
   --self._queue = om:add_component(entity, 'user_front_queue')
   self._queue = {}

   log:debug('attaching user_queue handler to entity.')
   md:listen(entity, 'radiant.queue_user_behavior', self);
   md:send_msg(entity, 'radiant.ai.register_need', self, true)
   md:send_msg(entity, 'radiant.ai.register_behavior', self, true)
end


-- Need messages

UserQueue['radiant.ai.needs.recommend_activity'] = function(self)   
   if #self._queue == 0 then
      return 0, nil
   end
   return 2, md:create_activity('radiant.activities.run_user_queue')
end

-- Behavior messages

UserQueue['radiant.ai.behaviors.get_priority'] = function(self, activity)
   return activity.name == 'radiant.activities.run_user_queue' and 1 or 0
end

UserQueue['radiant.ai.behaviors.get_next_action'] = function(self, activity)
   local action = nil

   while not action and #self._queue > 0 do
      local handler = self._queue[1]
      action = md:call_handler(handler, 'radiant.ai.behaviors.get_next_action')
      if not action then
         table.remove(self._queue, 1)
      end
   end
   return action
end

-- UserQueue messages

UserQueue['radiant.queue_user_behavior'] = function(self, name, ...)
   local handler = md:create_msg_handler(name, ...)
   table.insert(self._queue, handler)
end
