local Relations = require 'lib.player.relations'

local BossDeathScript = class()

--When the camp departs, no matter how the player interacted with these goblins, 
--set amenity for all goblins back to hostile. 

function BossDeathScript:start(ctx)
   stonehearth.player:set_amenity(
      ctx.npc_player_id,
      ctx.player_id,
      Relations.HOSTILE)
end

return BossDeathScript