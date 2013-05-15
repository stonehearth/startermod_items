local Die = class()

local om = require 'radiant.core.om'
local ai_mgr = require 'radiant.ai.ai_mgr'

ai_mgr:register_action('radiant.actions.die', Die)

function Die:run(ai, entity, damage_type, damage_amount)
   ai:execute('radiant.actions.perform', 'combat/1h_downed')
   om:destroy_entity(entity)
end
