local GameSpeedCallHandler = class()

function GameSpeedCallHandler:get_game_speed_service(session, request)
   return {game_speed_service = stonehearth.game_speed}
end

--- Call to get the default speed, usually called once at the start of the game
--  Possibly temporary, allows the play button to be overwritten by the speed set in user settings. 
function GameSpeedCallHandler:get_default_speed(session, request)
   --local speed = stonehearth.game_speed:get_user_requested_speed()
   local default_speed = stonehearth.game_speed:get_default_speed()

   --If the game speed service hasn't gotten its speed back yet, it's a new game and 
   --we can try getting it directly from the server. 
   if not default_speed then
      _radiant.call('radiant:game:get_game_speed')
         :done(function(r)
            return {default_speed = r.game_speed}
            end)
   else
      return {default_speed = default_speed}  
   end
end

--- Call to set the speed as requested by the user
--  @param speed: a number representing the speed
function GameSpeedCallHandler:set_player_game_speed(session, request, speed)
   stonehearth.game_speed:set_game_speed(speed, true)
   return {}
end

-- Call when stonehearth pauses the user's game
-- (ie, esc menu, cut scenes, etc.)
function GameSpeedCallHandler:dm_pause_game(session, request)
   stonehearth.game_speed:set_game_speed(0, false)
   return {}
end

--- Call when stonehearth resumes the user's game, resume to last saved speed
function GameSpeedCallHandler:dm_resume_game(session, request)
   local speed = stonehearth.game_speed:get_user_requested_speed()
   stonehearth.game_speed:set_game_speed(speed, false)
   
   return {}
end

return GameSpeedCallHandler 