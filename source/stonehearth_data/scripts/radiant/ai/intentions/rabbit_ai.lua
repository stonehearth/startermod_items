local RabbitAI = class()
local om = require 'radiant.core.om'
local ai_mgr = require 'radiant.ai.ai_mgr'
local ani_mgr = require 'radiant.core.animation'

ai_mgr:register_action('radiant.actions.rabbit_ai', RabbitAI)
ai_mgr:register_intention('radiant.intentions.rabbit_ai', RabbitAI)

function RabbitAI:recommend_activity(entity)
   return 999, { 'radiant.actions.rabbit_ai'}
end

function RabbitAI:run(ai, entity)
   local r = 4
   local home = om:get_component(entity, 'mob'):get_world_location()
   
   local explore = function ()
      local dx = math.random(0, 1) == 0 and math.random(-r, -r / 2) or math.random(r / 2, r)
      local dz = math.random(0, 1) == 0 and math.random(-r, -r / 2) or math.random(r / 2, r)
      local goto = RadiantPoint3(home.x + dx, home.y, home.z + dz)
      ai:execute('radiant.actions.goto_location', goto)
   end
   local idle = function()
      om:turn_to(entity, math.random(0, 359))
      local effect = ani_mgr:get_animation(entity):start_action('idle')
      ai:wait_until(om:now() + math.random(1000, 5000))
      effect:stop()
   end
   local lookaround = function()
      om:turn_to(entity, math.random(0, 359))
      ai:execute('radiant.actions.perform', 'look_around')
   end
   
   local actions = {
      explore,
      explore,
      explore,
      explore,
      idle,
      idle,
      lookaround
   }
   while true do
      actions[math.random(#actions)]()
   end
end
