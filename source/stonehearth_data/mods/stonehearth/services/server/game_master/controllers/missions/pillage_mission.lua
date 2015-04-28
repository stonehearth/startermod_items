local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()

local Mission = require 'services.server.game_master.controllers.missions.mission'

local PillageMission = class()
radiant.mixin(PillageMission, Mission)

function PillageMission:activate()
   Mission.activate(self)
   if self._sv.party then
      self._arrive_listener = radiant.events.listen(party, 'stonehearth:party:arrived_at_banner', function()
            self:_change_pillage_location()
         end)
   end
end

function PillageMission:start(ctx, info)
   Mission.start(self, ctx, info)
   local party = self._sv.party

   assert(info.pillage_radius)
   assert(info.pillage_radius.min)
   assert(info.pillage_radius.max)

   assert(not self._arrive_listener)
   self._arrive_listener = radiant.events.listen(party, 'stonehearth:party:arrived_at_banner', function()
         self:_change_pillage_location()
      end)
   self:_change_pillage_location()
end

function PillageMission:stop()
   Mission.stop(self)
   if self._arrive_listener then
      self._arrive_listener:destroy()
      self._arrive_listener = nil
   end
end

function PillageMission:_change_pillage_location()
   local town = stonehearth.town:get_town(self._sv.ctx.player_id)
   if not town then
      return
   end
   local camp_standard = town:get_banner()   
   if not camp_standard then
      return
   end

   local party = self._sv.party
   local origin = radiant.entities.get_world_grid_location(camp_standard)
   local location, found
   local pillage_radius = self._sv.info.pillage_radius
   for i=1,10 do
      location, found = radiant.terrain.find_placement_point(origin, pillage_radius.min, pillage_radius.max)
      if found then
         break
      end
   end
   party:attack_move_to(location)
end

return PillageMission
