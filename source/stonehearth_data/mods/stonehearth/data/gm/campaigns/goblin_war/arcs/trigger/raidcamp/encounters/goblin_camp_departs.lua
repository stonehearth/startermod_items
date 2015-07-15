local Relations = require 'lib.player.relations'

local GoblinCampDepartsScript = class()

--When the camp departs, no matter how the player interacted with these goblins, 
--set amenity for all goblins back to hostile. 

function GoblinCampDepartsScript:start(ctx)
	stonehearth.player:set_amenity(
		ctx.npc_player_id,
		ctx.player_id,
		Relations.HOSTILE)
end

return GoblinCampDepartsScript