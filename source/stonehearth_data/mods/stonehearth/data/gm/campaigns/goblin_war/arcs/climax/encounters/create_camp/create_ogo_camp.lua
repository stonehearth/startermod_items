
local CreateOgoCamp = class()

function CreateOgoCamp:start(ctx)
   --TODO
   --Create names for Ogo, the soothsayer, the bannerman, and Mountain
   --Create the fakeout entity
   ctx.ogo_camp.fakeout_entity = radiant.entities.create(nil, { debug_text = 'for creating a fakeout shakedown' })
   
   --ogo_camp.fakeout_shakedown_entity
   --[[
   local boss = ctx.goblin_raiding_camp_1.boss

   assert(boss and boss:is_valid())

   -- scale the boss up
   local render_info = boss:get_component('render_info')
   assert (render_info)
   render_info:set_scale(.120)

   -- rename the boss 
   -- XXX, make this a boss name lookup table
   -- XXX, make this localizable
   local name = radiant.entities.get_name(boss)
   radiant.entities.set_name(boss, 'Chieftan ' .. name)

   ]]
   
end

return CreateOgoCamp
