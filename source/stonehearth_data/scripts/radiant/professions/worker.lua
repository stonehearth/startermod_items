local Worker = class()

local om = require 'radiant.core.om'
local md = require 'radiant.core.md'
local sh = require 'radiant.core.sh'
local ai_mgr = require 'radiant.ai.ai_mgr'

md:register_msg_handler("radiant.professions.worker", Worker)

Worker['radiant.md.create'] = function(self, entity)
   self._entity = entity
   self._intention = ai_mgr:add_intention(entity, 'radiant.intentions.worker_scheduler_slave', sh:get_worker_scheduler())
   om:add_rig_to_entity(entity, 'models/worker_outfit.qb')
end

Worker['radiant.md.destroy'] = function(self)
   self._intention:destroy()
   om:remove_rig_from_entity(self._entity, 'models/worker_outfit.qb')
end
