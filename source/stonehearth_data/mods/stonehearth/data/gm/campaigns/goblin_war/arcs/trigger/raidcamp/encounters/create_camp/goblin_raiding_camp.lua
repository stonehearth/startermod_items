
local GoblinRaidingCampScript = class()

function GoblinRaidingCampScript:start(ctx)
   local boss = ctx.npc_boss_entity

   assert(boss and boss:is_valid())

   -- scale the boss up
   local render_info = boss:get_component('render_info')
   assert (render_info)
   render_info:set_scale(.120)

   -- give him the boss gear
   local crown = radiant.entities.create_entity('stonehearth:monsters:goblins:armor:miniboss_crown')
   radiant.entities.equip_item(boss, crown)

   -- rename the boss 
   local name = radiant.entities.get_name(boss)
   radiant.entities.set_name(boss, 'Chieftan ' .. name)
   -- XXX, make this a boss name lookup table
end

return GoblinRaidingCampScript
