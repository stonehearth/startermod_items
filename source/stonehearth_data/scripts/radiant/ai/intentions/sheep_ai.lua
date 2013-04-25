local SheepAI = class()
local om = require 'radiant.core.om'
local ai_mgr = require 'radiant.ai.ai_mgr'
local ani_mgr = require 'radiant.core.animation'

ai_mgr:register_action('radiant.actions.sheep_ai', SheepAI)
ai_mgr:register_intention('radiant.intentions.sheep_ai', SheepAI)

function SheepAI:recommend_activity(entity)
   return 999, { 'radiant.actions.sheep_ai'}
end

function SheepAI:run(ai, entity)
   local r = 4
   local home = om:get_component(entity, 'mob'):get_world_location()
   
   local idle = function()
      --om:turn_to(entity, math.random(0, 359))
      local effect = ani_mgr:get_animation(entity):start_action('idle')
      ai:wait_until(om:now() + math.random(1000, 5000))
      effect:stop()
   end
   local lookaround = function()
      --om:turn_to(entity, math.random(0, 359))
      ai:execute('radiant.actions.perform', 'look_around')
   end
   
   local actions = {
      idle,
      idle,
      lookaround
   }
   while true do
      actions[math.random(#actions)]()
   end
end
