local Idle = class()

local om = require 'radiant.core.om'
local util = require 'radiant.core.util'
local ai_mgr = require 'radiant.ai.ai_mgr'

ai_mgr:register_action('radiant.actions.idle', Idle)

function Idle:run(ai, entity)
   while true do
      local carrying = om:get_carrying(entity)
      local action = carrying and 'carry_idle' or 'idle_breathe'
      ai:execute('radiant.actions.perform', action)
   end
end
