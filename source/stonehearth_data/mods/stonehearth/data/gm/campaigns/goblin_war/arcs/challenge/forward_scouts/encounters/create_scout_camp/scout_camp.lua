local GoblinScoutCamp = class()

--Note all scripts, and all encuonters, are controllers

function GoblinScoutCamp:initialize()
end

function GoblinScoutCamp:restore()
end

--Called on both init and restore, can use to start/resume tasks, etc
function GoblinScoutCamp:activate()
   if self._sv.ctx then
   end
end

function GoblinScoutCamp:start(ctx)
   self._sv.ctx = ctx

   --These wolves sleep as long as their cage exists. If its destroyed by the player, the wolves escape. 
   --If it's destroyed by the timer/trainer, the wolves attack the town . 
   for i, wolf in ipairs(ctx.create_scout_camp.citizens.tame_wolf) do 
      if wolf and wolf:is_valid() then
         --Add the caged beast component 
         assert(ctx.create_scout_camp.entities.wolf_cage and ctx.create_scout_camp.entities.wolf_cage:is_valid())
         wolf:add_component('stonehearth:caged_entity'):set_cage(ctx.create_scout_camp.entities.wolf_cage)
      end
   end   
end

return GoblinScoutCamp
