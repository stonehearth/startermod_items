GameSpeedService = class()

function GameSpeedService:_init()
end

--On first init of game, get the game speed from the user settings/C++ defaults
--On subsequent loads of saved games, SET the game speed from the user's choices
function GameSpeedService:initialize()
   self.__saved_variables:set_controller(self)
   self._sv = self.__saved_variables:get_data()
   if not self._sv._initialized then
      self._sv._initialized = true

      -- Set saved speed based on user settings
      _radiant.call('radiant:game:get_game_speed')
         :done(function(r)
            --Will remote to the UI
            self._sv.curr_speed = r.game_speed
            
            --Temporary, save this in order to respect default speed "play" from user settings
            self._sv._default_game_speed = r.game_speed
            self._sv._user_requested_speed = self._sv._default_game_speed 
            self.__saved_variables:mark_changed()
            end)
   else
      --We're loading! Set the game speed based on the saved variable 
      _radiant.call('radiant:game:set_game_speed', self._sv._user_requested_speed)
      self._sv.curr_speed = self._sv._user_requested_speed
      self.__saved_variables:mark_changed()
   end
end

--- Get the last speed the user specified
function GameSpeedService:get_user_requested_speed()
   return self._sv._user_requested_speed
end

--- Get the "default speed"
--  TODO: Depending on the final UI, this may not be necessary, but while we want to 
--  respect the game speed set in user settings, this is relevant to getting the
--  originally set speed back to loaded games
function GameSpeedService:get_default_speed()
   return self._sv._default_game_speed
end

--- Set the speed of the game
--  @param speed: a float representing the speed
--  @param user_set: true if the speed change was requested by the user
--  false if it was requested by the game (for a cut scene, dialog, etc)
--  Note: if the game changes the speed automatically don't save it
--  to the _sv._user_speed variable, so we can use the value in the _sv
--  variable to restore the speed to its previous user requested setting. 
function GameSpeedService:set_game_speed(speed, user_set)
   if user_set then
      self._sv._user_requested_speed = speed
   end
   _radiant.call('radiant:game:set_game_speed', speed)
   self._sv.curr_speed = speed
   self.__saved_variables:mark_changed()
end

return GameSpeedService