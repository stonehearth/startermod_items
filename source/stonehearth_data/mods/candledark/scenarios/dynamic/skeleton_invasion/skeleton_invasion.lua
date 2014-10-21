local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()
local SkeletonInvasion = class()

function SkeletonInvasion:initialize()
   --ID of the player who the goblin is attacking, not the player ID of the DM
   self._sv.player_id = 'player_1'
   self._scenario_data = radiant.resources.load_json('/candledark/scenarios/dynamic/skeleton_invasion/skeleton_invasion.json').scenario_data
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
         self:_post_bulletin()
         local num_skeletons = 30
         for i = 1, num_skeletons do
            radiant.set_realtime_timer(2000 * i, function()
               self:_spawn_skeleton()
            end)
         end
      end)
end

function SkeletonInvasion:_post_bulletin()
   local bulletin_data = self._scenario_data.bulletins.attack

   self._sv.bulletin = stonehearth.bulletin_board:post_bulletin(self._sv.player_id)
      :set_type('alert')
      :set_data(bulletin_data)
end

function SkeletonInvasion:_spawn_skeleton()
   local skeleton = self:_create_skeleton()
   local spawn_point = stonehearth.spawn_region_finder:find_point_outside_civ_perimeter_for_entity(skeleton, 80)

   if spawn_point then
      radiant.terrain.place_entity(skeleton, spawn_point)
   end
end

function SkeletonInvasion:_create_skeleton()
   -- Create a skeleton. The skeleton is a citizen of the undead population.
   local skeleton = self._population:create_new_citizen()
   
   -- Arm the skeleton, XXX randomize
   local weapon = radiant.entities.create_entity('stonehearth:weapons:jagged_cleaver')
   radiant.entities.equip_item(skeleton, weapon)
   
   -- Add ai to the skeleton, telling it to go to the town center
   local town = stonehearth.town:get_town(self._sv.player_id)
   skeleton:get_component('stonehearth:ai')
      :get_task_group('stonehearth:work')
      :create_task('stonehearth:goto_town_center', { town = town })
      :set_name('introduce self task')
      :set_priority(stonehearth.constants.priorities.goblins.RUN_TOWARDS_SETTLEMENT)
      :once()
      :start()

   return skeleton
end

return SkeletonInvasion
