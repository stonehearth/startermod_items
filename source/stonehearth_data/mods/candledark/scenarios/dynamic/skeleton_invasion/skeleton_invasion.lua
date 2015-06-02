local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()
local SkeletonInvasion = class()

function SkeletonInvasion:initialize(params)
   self._sv.player_id = 'player_1'
   self._sv.wave_number = params.wave
   self._sv.wave_complete = true
   self:restore()

   self.__saved_variables:mark_changed()
end

function SkeletonInvasion:restore()
   self._scenario_data = radiant.resources.load_json('/candledark/scenarios/dynamic/skeleton_invasion/skeleton_invasion.json').scenario_data

   -- Begin hack #1: We want some reasonable place to put faction initialization; in some random scenario
   -- is likely not the correct place.
   local session = {
      player_id = 'candledark_undead',
   }
   if stonehearth.town:get_town(session.player_id) == nil then
      stonehearth.town:add_town(session)
      self._population = stonehearth.population:add_population(session, 'candledark:kingdoms:undead')
   else
      self._inventory = stonehearth.inventory:get_inventory(session.player_id)
      self._population = stonehearth.population:get_population(session.player_id)
   end
   -- End hack

   --we're loading in the middle of a wave, load the rest of the wave
   if not self._sv.wave_complete and self._sv.num_skeletons_remaining then
      self:spawn_skeleton_wave(self._sv.num_skeletons_remaining)
   end
end

function SkeletonInvasion:start()
   local wave_sizes = self._scenario_data.config.invasion_sizes
   local num_skeletons = wave_sizes[self._sv.wave_number]
   self:spawn_skeleton_wave(num_skeletons)
end

function SkeletonInvasion:spawn_skeleton_wave(num_skeletons)
   self._sv.wave_complete = false
   self._sv.num_skeletons_remaining = num_skeletons
   for i = 1, num_skeletons do
      radiant.set_realtime_timer("SkeletonInvasion spawn_skeleton_wave", 2000 * i, function()
         self:_spawn_skeleton(i)
         self._sv.num_skeletons_remaining = num_skeletons - i
         if self._sv.num_skeletons_remaining == 0 then
            self._sv.wave_complete = true
         end
      end)
   end
   
end

function SkeletonInvasion:_post_bulletin(skeleton)
   local titles = self._scenario_data.bulletins.attack.titles
   local title = titles[self._sv.wave_number]
   self._sv.bulletin = stonehearth.bulletin_board:post_bulletin(self._sv.player_id)
      :set_type('alert')
      :set_data({ 
         title = title, 
         zoom_to_entity = skeleton
      })
end

function SkeletonInvasion:_spawn_skeleton(index_in_wave)
   local skeleton = self:_create_skeleton()
   local spawn_point = stonehearth.spawn_region_finder:find_point_outside_civ_perimeter_for_entity(skeleton, 80)

   if spawn_point then
      radiant.terrain.place_entity(skeleton, spawn_point)
   end

   --If this is the first skeleton in a wave, post the bulletin about the skeleton
   --destroy the bulletin if the skeleton dies
   if index_in_wave == 1 then
      self:_post_bulletin(skeleton)
      radiant.events.listen_once(skeleton, 'radiant:entity:pre_destroy', function()
         if self._sv.bulletin then
            local bulletin_id = self._sv.bulletin:get_id()
            stonehearth.bulletin_board:remove_bulletin(bulletin_id)
            self._sv.bulletin = nil
         end
      end)
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
