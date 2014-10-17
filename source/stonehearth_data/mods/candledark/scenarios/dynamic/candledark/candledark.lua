local Candledark = class()
local rng = _radiant.csg.get_default_rng()

function Candledark:initialize()
   self._sv.player_id = 'player_1'
   self._sv.nights_survived = 0
   self._scenario_data = radiant.resources.load_json('/candledark/scenarios/dynamic/candledark/candledark.json').scenario_data

   self.__saved_variables:mark_changed()
end

function Candledark:restore()
   --If we made an expire timer then we're waiting for the player to acknowledge the traveller
   --Start a timer that will expire at that time
   --[[
   if self._sv.timer_expiration then
      radiant.events.listen(radiant, 'radiant:game_loaded', function(e)
         local duration = self._sv.timer_expiration - stonehearth.calendar:get_elapsed_time()
         self:_create_timer(duration)
         return radiant.events.UNLISTEN
      end)
   end
   ]]
end

function Candledark:start()
   self:_post_intro_bulletin()
   self._night_listener = radiant.events.listen(stonehearth.calendar, 'stonehearth:midnight', self, self._on_midnight)
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
   if self._sv.bulletin ~= nil then
      self._sv.bulletin:modify_data({
         title = self._scenario_data.bulletins.initial_warning.post_accept_title,
      })
   end
end

function Candledark:_on_midnight()
   stonehearth.dynamic_scenario:force_spawn_scenario('candledark:scenarios:candledark:skeleton_invasion')
end

function Candledark:destroy()
   if self._night_listener ~= nil then
      self._night_listener:destroy()
      self._night_listener = nil
   end
end



return Candledark