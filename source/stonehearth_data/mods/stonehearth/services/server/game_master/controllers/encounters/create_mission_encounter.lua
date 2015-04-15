
local CreateMission = class()

local MISSION_CONTROLLER = 'stonehearth:game_master:missions:'

function CreateMission:start(ctx, info)
   assert(info.spawn_range)
   assert(info.spawn_range.min)
   assert(info.spawn_range.max)

   local min = info.spawn_range.min
   local max = info.spawn_range.max

   self._sv.ctx = ctx
   self._sv.info = info
   self._sv.searcher = radiant.create_controller('stonehearth:game_master:util:choose_location_outside_town',
                                                 ctx.player_id, min, max,
                                                 radiant.bind(self, '_start_mission'))
end

function CreateMission:stop()
end

function CreateMission:get_ctx()
   return self._sv.ctx
end

function CreateMission:_start_mission(location)
   local info = self._sv.info
   local ctx = self._sv.ctx

   assert(info.mission)
   assert(info.mission.role)

   ctx.enemy_location = location

   local mission_controller_name = MISSION_CONTROLLER .. info.mission.role     
   local mission = radiant.create_controller(mission_controller_name, info)
   assert(mission)

   self._sv.mission = mission
   mission:start(ctx, info.mission)

   -- all done!  trigger the next guy
   ctx.arc:trigger_next_encounter(ctx)
end

return CreateMission

