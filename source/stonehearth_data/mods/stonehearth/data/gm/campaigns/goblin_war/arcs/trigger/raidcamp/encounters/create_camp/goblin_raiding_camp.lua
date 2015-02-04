
local GoblinRaidingCampScript = class()

function GoblinRaidingCampScript:start(ctx)
   local boss = ctx.npc_boss_entity

   assert(boss and boss:is_valid())
   
   radiant.log.write('', 0, 'add customization here!')   
end

return GoblinRaidingCampScript