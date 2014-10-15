local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()
local SkeletonInvasion = class()

function SkeletonInvasion:can_spawn()
   return false --true
end

function SkeletonInvasion:initialize()
   --ID of the player who the goblin is attacking, not the player ID of the DM
   self._sv.player_id = 'player_1'
end

function SkeletonInvasion:start()
   -- Begin hack #1: We want some reasonable place to put faction initialization; in some random scenario
   -- is likely not the correct place.
   local session = {
      player_id = 'game_master',
      faction = 'undead',
      kingdom = 'candledark:kingdoms:undead'
   }
   if stonehearth.town:get_town(session.player_id) == nil then
      stonehearth.town:add_town(session)
      self._population = stonehearth.population:add_population(session)
      self._population:create_town_name()
   else
      self._inventory = stonehearth.inventory:get_inventory(session.player_id)
      self._population = stonehearth.population:get_population(session.player_id)
   end
   -- End hack

   self:_schedule_next_spawn(1)
end

function SkeletonInvasion:_schedule_next_spawn(t)
   stonehearth.calendar:set_timer(t, function()
         self:_spawn_skeleton()
      end)
end

function SkeletonInvasion:_spawn_skeleton()
   local skeleton = self:_create_skeleton()

   --[[
   local spawn_point = stonehearth.spawn_region_finder:find_point_outside_civ_perimeter_for_entity(self._sv._goblin, 80)

   if not spawn_point then
      -- Couldn't find a spawn point, so reschedule to try again later.
      radiant.entities.destroy_entity(self._sv._goblin)
      self._sv._goblin = nil
      self:_schedule_next_spawn(rng:get_int(3600 * 0.5, 3600 * 1))
      return
   end
   ]]

   radiant.terrain.place_entity(skeleton, Point3(5, 1, 5))
end

function SkeletonInvasion:_create_skeleton()
   local skeleton = self._population:create_new_citizen()
   local weapon = radiant.entities.create_entity('stonehearth:weapons:jagged_cleaver')
   radiant.entities.equip_item(skeleton, weapon)
   --stonehearth.combat:set_stance(skeleton, 'defensive')
   return skeleton
end

return SkeletonInvasion
