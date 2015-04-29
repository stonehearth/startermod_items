
local GoblinRaidingCampScript = class()

function GoblinRaidingCampScript:start(ctx)
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
   
end

return GoblinRaidingCampScript
