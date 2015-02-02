
local CityRaid = class()

local MISSION_CONTROLLER = 'stonehearth:game_master:missions:'

function CityRaid:start(ctx, info)
   assert(ctx.enemy_location)

   self._sv.ctx = ctx
   self._sv.info = info
   self._sv.missions = {}

   self:_create_raid(info)
end

function CityRaid:stop()
   radiant.not_yet_implemented()
end

function CityRaid:get_ctx()
   return self._sv.ctx
end

function CityRaid:_create_raid(info)
   local ctx = self._sv.ctx
   
   assert(info.missions)
   for name, info in pairs(info.missions) do
      assert(info.role)
      local controller_name = MISSION_CONTROLLER .. info.role
      
      local mission = radiant.create_controller(controller_name, ctx, info)
      assert(mission)

      self._sv.missions[name] = mission
   end
end

return CityRaid

