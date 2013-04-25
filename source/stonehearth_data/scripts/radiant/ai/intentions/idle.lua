local Idle = class()

local ai_mgr = require 'radiant.ai.ai_mgr'
ai_mgr:register_intention('radiant.intentions.idle', Idle)

function Idle:recommend_activity(entity)
   return 1, { 'radiant.actions.idle'}
end
