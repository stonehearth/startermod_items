local Candledark = class()
local rng = _radiant.csg.get_default_rng()

-- Called when the scenario is first initialized
function Candledark:initialize()
   self._scenario_data = radiant.resources.load_json('/candledark/scenarios/dynamic/candledark/candledark.json').scenario_data
   
   self._sv.player_id = 'player_1'
   self._sv.nights_until_candledark = self._scenario_data.config.nights_until_candledark
   self._sv.candledark_duration = self._scenario_data.config.candledark_duration
   self._sv.nights_survived = 0
   
   self.__saved_variables:mark_changed()
end

-- Called when the scenario is loaded (from a save)
function Candledark:restore()
   self._scenario_data = radiant.resources.load_json('/candledark/scenarios/dynamic/candledark/candledark.json').scenario_data
   
   if self._sv.started then
      self._sunset_listener = radiant.events.listen(stonehearth.calendar, 'stonehearth:sunset', self, self._on_sunset)
   end

   --We saved/loaded while the timer was running, so just start it again and spawn the skeletons later
   if self._sv.timer_running then
      self:_spawn_skeletons()
   end
end

-- Called when the scenario is started
function Candledark:start()
   if not self._sv.started then
      self:_post_intro_bulletin()
      self._sv.started = true
      self.__saved_variables:mark_changed()
   end
   --stonehearth.calendar:set_time_unit_test_only({ hour = 23, minute = 38 })
   self._sunset_listener = radiant.events.listen(stonehearth.calendar, 'stonehearth:sunset', self, self._on_sunset)
end

function Candledark:_post_intro_bulletin()
   local bulletin_data = self._scenario_data.bulletins.initial_warning

   --Send the notice to the bulletin service
   self._sv.bulletin = stonehearth.bulletin_board:post_bulletin(self._sv.player_id)
      :set_ui_view('StonehearthQuestBulletinDialog')
      :set_callback_instance(self)
      :set_type('quest')
      :set_sticky(true)
      :set_close_on_handle(self._sv.scenario_completed)
      :set_data(bulletin_data)

   self.__saved_variables:mark_changed()
end


-- After the initial quest is accepted, change the title of the bulletin
function Candledark:_on_intro_accepted()
   self:_update_bulletin()
end

-- Update the bulletin with the days remaining until Candledark starts
function Candledark:_update_bulletin()
   if self._sv.bulletin ~= nil then
      local title = self._scenario_data.bulletins.initial_warning.post_accept_title
      title = string.gsub(title, '__days__', self._sv.nights_until_candledark)

      self._sv.bulletin:modify_data({
         title = title,
      })
   end
end


function Candledark:_on_sunset()
   self._sv.nights_until_candledark = self._sv.nights_until_candledark - 1
   
   if self._sv.nights_survived == self._sv.candledark_duration then
      -- End the scenario if the player has suvived the length of candledark   

      -- nuke the old bulletin
      stonehearth.bulletin_board:remove_bulletin(self._sv.bulletin:get_id())

      -- post a new bulletin announcing the end of candledark
      local bulletin_data = self._scenario_data.bulletins.candledark_end
      self._sv.bulletin = stonehearth.bulletin_board:post_bulletin(self._sv.player_id)
         :set_ui_view('StonehearthQuestBulletinDialog')
         :set_callback_instance(self)
         :set_type('quest')
         :set_close_on_handle(true)
         :set_data(bulletin_data)

      self:destroy();
      return
   end

   if self._sv.nights_until_candledark > 0 then
      -- Candledark hasn't arrived. Update the bulletin with the days remaining
      self:_update_bulletin()
   else 
      -- Candledark has arrived
      
      -- The first night only, make a new bulletin announcing the arrival of Candledark
      if self._sv.nights_until_candledark == 0 then
         -- nuke the old bulletin
         stonehearth.bulletin_board:remove_bulletin(self._sv.bulletin:get_id())
         
         -- post a new one telling the player that candledark has begun!
         local bulletin_data = self._scenario_data.bulletins.candledark_start
         self._sv.bulletin = stonehearth.bulletin_board:post_bulletin(self._sv.player_id)
            :set_ui_view('StonehearthQuestBulletinDialog')
            :set_callback_instance(self)
            :set_type('quest')
            :set_close_on_handle(true)
            :set_data(bulletin_data)
      end

      -- time for skeletons!      
      self._sv.nights_survived = self._sv.nights_survived + 1

      self:_spawn_skeletons()

      self.__saved_variables:mark_changed()
      
   end
end

function Candledark:_spawn_skeletons()
   self._sv.timer_running = true
   radiant.set_realtime_timer(1000 * 15, function()
      self._sv.timer_running = false
      stonehearth.dynamic_scenario:force_spawn_scenario('candledark:scenarios:skeleton_invasion', { wave = self._sv.nights_survived })      
   end)

end

-- Called when the player clicks the "accept" button in the final bulletin in the scenario.
-- Spawn a bunch of candy around the banner
function Candledark:_on_survived_accepted()

   local town = stonehearth.town:get_town(self._sv.player_id)
   local banner = town:get_banner()
   local drop_origin = banner and radiant.entities.get_world_grid_location(banner)
   
   if not drop_origin then
      return false
   end

   local items = {}
   items['candledark:candy_basket'] = 9

   radiant.entities.spawn_items(items, drop_origin, 1, 3, self._sv.player_id)
end

function Candledark:destroy()
   if self._sunset_listener ~= nil then
      self._sunset_listener:destroy()
      self._sunset_listener = nil
   end
end



return Candledark