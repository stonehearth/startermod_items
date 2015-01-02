
local CityRaid = class()

local PARTY_CONTROLLER = 'stonehearth:game_master:npc_parties:'

function CityRaid:start(ctx, info)
   assert(ctx.enemy_location)

   self._sv.ctx = ctx
   self._sv.info = info
   self._sv.parties = {}

   self:_create_raid(info)
end

function CityRaid:get_ctx()
   return self._sv.ctx
end

function CityRaid:_create_raid(info)
   local ctx = self._sv.ctx
   
   assert(info.parties) 
   for name, party_info in pairs(info.parties) do
      assert(party_info.role)
      local controller_name = PARTY_CONTROLLER .. party_info.role
      
      local party = radiant.create_controller(controller_name, ctx, party_info)
      assert(party)

      self._sv.parties[name] = party
   end
end

return CityRaid

