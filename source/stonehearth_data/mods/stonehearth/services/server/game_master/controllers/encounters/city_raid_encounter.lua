
local CityRaid = class()

local MISSION_CONTROLLER = 'stonehearth:game_master:missions:'

function CityRaid:initialize(info)
   self._sv.missions = {}
   self._sv._info = info
   self:_create_raid(info)
end

function CityRaid:can_start(ctx, info)
   for name, mission in pairs(self._sv.missions) do
      if not mission:can_start(ctx, info) then
         return false
      end
   end
   return true
end

function CityRaid:start(ctx, info)
   assert(ctx.enemy_location)

   self._sv.ctx = ctx
   for name, mission in pairs(self._sv.missions) do
      mission:start(ctx, info.missions[name])
   end

   ctx.arc:trigger_next_encounter(ctx)
end

function CityRaid:stop()
   --radiant.not_yet_implemented()
end

function CityRaid:get_ctx()
   return self._sv.ctx
end

function CityRaid:_create_raid(info)
   assert(info.missions)
   for name, info in pairs(info.missions) do
      assert(info.role)
      local controller_name = MISSION_CONTROLLER .. info.role
      
      local mission = radiant.create_controller(controller_name, info)
      assert(mission)

      self._sv.missions[name] = mission
   end
end

return CityRaid

