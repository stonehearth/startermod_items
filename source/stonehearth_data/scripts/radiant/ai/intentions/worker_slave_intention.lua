local SlaveIntention = class()

local ai_mgr = require 'radiant.ai.ai_mgr'
ai_mgr:register_intention('radiant.intentions.worker_scheduler_slave', SlaveIntention)

function SlaveIntention:__init(entity, scheduler)
   self._entity = entity
   self._scheduler = scheduler
   self._scheduler:add_worker(entity)
end

function SlaveIntention:destroy()
   self._scheduler:remove_worker(self._entity)
end

function SlaveIntention:recommend_activity(entity)
   return self._scheduler:recommend_activity_for(entity)
end
