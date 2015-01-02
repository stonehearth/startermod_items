local Party = require 'services.server.game_master.controllers.npc_parties.party'

local RaidStockpilesParty = class()
mixin_class(RaidStockpilesParty, Party)

function RaidStockpilesParty:initialize(ctx, info)
   assert(ctx.enemy_location)
   self:_create_party(ctx, info)
end

return RaidStockpilesParty
