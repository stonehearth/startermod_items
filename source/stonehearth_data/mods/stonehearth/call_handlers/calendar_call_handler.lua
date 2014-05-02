local calendar = stonehearth.calendar

local CalendarCallHandler = class()

local clock_object = nil

function CalendarCallHandler:get_clock_object(session, request)
   if not clock_object then
      clock_object = radiant.create_datastore()
      stonehearth.calendar:set_interval('1m', function(e)
            clock_object:set_data(calendar:get_time_and_date())
         end)
      clock_object:set_data(calendar:get_time_and_date())
   end
   return { clock_object = clock_object }
end

--- Call to get the last speed requested by the player
function CalendarCallHandler:get_player_speed(session, request)
   local speed = calendar:get_user_requested_speed()
   local default_speed = calendar:get_default_speed()

   --If the calendar hasn't gotten its speed back yet, it's a new game and 
   --we can try getting it directly from the server. 
   if not speed then
      _radiant.call('radiant:game:get_game_speed')
         :done(function(r)
            return {player_set_speed = r.game_speed, 
                    default_speed = r.game_speed}
            end)
   else
      return {player_speed = speed, default_speed = default_speed}  
   end
end

--- Call to set the speed as requested by the user
--  @param speed: a number representing the speed
function CalendarCallHandler:set_player_game_speed(session, request, speed)
   calendar:set_game_speed(speed, true)
   return {}
end

--Call when stonehearth pauses the user's game
--(ie, esc menu, cut scenes, etc.)
function CalendarCallHandler:dm_pause_game(session, request)
   calendar:set_game_speed(0, false)
   return {}
end

--Call when stonehearth resumes the user's game, resume to last saved speed
function CalendarCallHandler:dm_resume_game(session, request)
   local speed = calendar:get_user_requested_speed()
   calendar:set_game_speed(speed, false)
   return {}
end

return CalendarCallHandler
