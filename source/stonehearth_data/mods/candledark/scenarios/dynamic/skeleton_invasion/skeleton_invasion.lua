local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()
local SkeletonInvasion = class()

function SkeletonInvasion:initialize(params)
   self._scenario_data = radiant.resources.load_json('/candledark/scenarios/dynamic/skeleton_invasion/skeleton_invasion.json').scenario_data

   self._sv.player_id = 'player_1'
   self._sv.wave_number = params.wave
   self.__saved_variables:mark_changed()
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

   self:spawn_skeleton_wave()
end

function SkeletonInvasion:spawn_skeleton_wave()
   local wave_sizes = self._scenario_data.config.invasion_sizes
   local num_skeletons = wave_sizes[self._sv.wave_number]
   
   self:_post_bulletin()
   
   for i = 1, num_skeletons do
      radiant.set_realtime_timer(2000 * i, function()
         self:_spawn_skeleton()
      end)
   end
end

function SkeletonInvasion:_post_bulletin()
   local titles = self._scenario_data.bulletins.attack.titles
   local title = titles[self._sv.wave_number]
   self._sv.bulletin = stonehearth.bulletin_board:post_bulletin(self._sv.player_id)
      :set_type('alert')
      :set_data({ title = title })
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
