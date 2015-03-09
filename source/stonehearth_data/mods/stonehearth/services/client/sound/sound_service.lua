local rng = _radiant.csg.get_default_rng()

local Sound = class()

local MUSIC_PRIORITIES = {
   COMBAT = 10
}
-- music

local IDLE_DAY_AMBIENT = {
   track    = 'stonehearth:ambient:summer_day',
   loop     = true,
   fade_in  = 4000,
   volume   = 60,
}

local IDLE_NIGHT_AMBIENT = {
   track    = 'stonehearth:ambient:summer_night',
   loop     = true,
   fade_in  = 4000,
   volume   = 20,
}

local IDLE_DAY_MUSIC = {
   track    = {
      'stonehearth:music:levelmusic_spring_day_01',
      'stonehearth:music:levelmusic_spring_day_02',
      'stonehearth:music:levelmusic_spring_day_03',
   },
   loop     = true,
   fade_in  = 4000,
   volume   = 35,
}

local IDLE_NIGHT_MUSIC = {
   track    = {
      'stonehearth:music:levelmusic_spring_night_01',
      'stonehearth:music:levelmusic_spring_night_02',
   },
   loop     = true,
   fade_in  = 4000,
   volume   = 35,
}

local TITLE_MUSIC = {
   track    = 'stonehearth:music:title_screen',
   loop     = true,
}

local COMBAT_MUSIC = {
   playlist = {
      {
         fade_in  = 500,
         track    = 'stonehearth:music:combat_start'
      },
      {
         track    = {
            'stonehearth:music:combat_theme_a',
            'stonehearth:music:combat_theme_b',
            'stonehearth:music:combat_theme_c',
         },
         volume   = 60,
         fade_in  = 100,
         loop     = true,
      }
   },
   priority = MUSIC_PRIORITIES.COMBAT
}

local KILL_COMBAT_MUSIC = {
   fade_in = 500,
   track = '',
   priority = MUSIC_PRIORITIES.COMBAT
}

-- sounds
local COMBAT_FINISHED_SOUND = {
   track = 'stonehearth:sounds:combat_finished',
   volume = 100,
}

local ROOSTER_SOUND = {
   track = 'stonehearth:sounds:rooster_call',
   volume = 50,
}

local DAYBREAK_SOUND = {
   track = 'stonehearth:music:daybreak_stinger_01',
   volume = 50,
}

local OWL_SOUND = {
   track = 'stonehearth:sounds:owl_call',
   volume = 50,
}

local NIGHTFALL_SOUND = {
   track = 'stonehearth:music:nightfall_stinger_01',
   volume = 50,
}

local ALL_SCREENS = {
   game_screen    = true,
   title_screen   = true,
   loading_screen = true,
}

local ALL_CHANNELS = {
   ambient = true,
   music   = true,
}

function Sound:initialize()
   self._sv = self.__saved_variables:get_data()
   self._log = radiant.log.create_logger('sound')
   self._calendar_constants = radiant.resources.load_json('/stonehearth/data/calendar/calendar_constants.json')

   self._current_music_info = {}
   self._current_ui_screen = _radiant.client.get_current_ui_screen()

   self._screens = {}
   for screen_name, _  in pairs(ALL_SCREENS) do
      local channels = {}
      self._screens[screen_name] = channels
      for channel_name, _ in pairs(ALL_CHANNELS) do
         channels[channel_name] = {}
      end
   end

   self:recommend_music('default', 'title_screen', 'music', TITLE_MUSIC)

   self:_monitor_time()
   self:_install_listeners()
end

function Sound:destroy()
   if self._get_pop_timer then
      self._get_pop_timer:destroy()
      self._get_pop_timer = nil
   end
   if self._pop_trace then
      self._pop_trace:destroy()
      self._pop_trace = nil
   end
end

function Sound:_monitor_time()
   _radiant.call('stonehearth:get_clock_object')
               :done(function (o)
                     local clock = o.clock_object
                     self._clock_trace = clock:trace('music service')
                                                :on_changed(function()
                                                      self:_on_time_changed(clock:get_data())
                                                   end)
                                                :push_object_state()
                  end)
end

function Sound:_install_listeners()
   self._render_promise = _radiant.client.trace_render_frame()
                              :on_frame_start('change music', function(now, interpolate)
                                    self:_change_music(now, interpolate)
                                 end)

   radiant.events.listen(radiant, 'radiant:client:ui_screen_changed', self, self._on_ui_screen_changed)
   self:_on_ui_screen_changed()
end

function Sound:_on_ui_screen_changed()
   self._current_ui_screen = _radiant.client.get_current_ui_screen()
   if not self._get_pop_timer and self._current_ui_screen == 'game_screen' then
      self._get_pop_timer = radiant.set_realtime_interval(1000, function()
            self:_get_population()
         end)
   end
end


function Sound:_get_population()
   _radiant.call_obj('stonehearth.population', 'get_population_command')
               :done(function(o)
                     if self._get_pop_timer then
                        self._get_pop_timer:destroy()
                        self._get_pop_timer = nil
                     end

                     local pop = o.uri
                     self._pop_trace = pop:trace('music service')
                                             :on_changed(function()
                                                   self:_on_population_changed(pop:get_data())
                                                end)
                                             :push_object_state()
                  end)
end

function Sound:_on_population_changed(data)
   self._log:info('threat level is now %.2f', data.threat_level)

   if data.threat_level > 0 then
      self._combat_started = true
      self:recommend_game_music('combat', 'music', COMBAT_MUSIC)
   else
      if self._combat_started and self._current_music_info['music'] == COMBAT_MUSIC then
         self._combat_started = false

         -- combat music is going... first fade it out by queueing a track
         -- with no music file.  when that's done, play the stinger and let
         -- the next music in the series fade in by recommending on combat
         -- music at all.
         self:recommend_game_music('combat', 'music', KILL_COMBAT_MUSIC)
         radiant.set_realtime_timer(KILL_COMBAT_MUSIC.fade_in, function()
               self:recommend_game_music('combat', 'music', nil)
               self:_play_sound(COMBAT_FINISHED_SOUND)
            end)
      end
   end
end

function Sound:_on_time_changed(date)
   local event_times = self._calendar_constants.event_times

   -- play sounds
   if date.hour == 0 then
      self._sunset_sound_played = false
      self._sunrise_sound_played = false
   end

   -- sounds
   if date.hour == event_times.sunrise and not self._sunrise_sound_played then
      self._sunrise_sound_played = true
      self:_play_sound(ROOSTER_SOUND)
      self:_play_sound(DAYBREAK_SOUND)
   end
   if date.hour == event_times.sunset and not self._sunset_sound_played then
      self._sunset_sound_played = true
      self:_play_sound(OWL_SOUND)
      self:_play_sound(NIGHTFALL_SOUND)
   end

   -- music
   local is_day = date.hour >= event_times.sunrise and date.hour < event_times.sunset
   if is_day then
      self:recommend_game_music('clock', 'music',    IDLE_DAY_MUSIC)
      self:recommend_game_music('clock', 'ambient',  IDLE_DAY_AMBIENT)
   else
      self:recommend_game_music('clock', 'music',    IDLE_NIGHT_MUSIC)
      self:recommend_game_music('clock', 'ambient',  IDLE_NIGHT_AMBIENT)
   end

end

function Sound:recommend_game_music(requestor, channel, info)
   self:recommend_music(requestor, 'game_screen', channel, info)
end

function Sound:recommend_music(requestor, screen_name, channel_name, info)
   assert(ALL_SCREENS[screen_name])
   assert(ALL_CHANNELS[channel_name])

   local screen = self._screens[screen_name]
   local channels = screen[channel_name]
   channels[requestor] = info
end

function Sound:_change_music(now)
   assert(self._current_ui_screen)

   local channels = self._screens[self._current_ui_screen]
   assert(channels)
   
   for channel_name, _ in pairs(ALL_CHANNELS) do
      self._log:debug('choosing best track for %s', channel_name)
      local info = self:_choose_best_track(channels[channel_name])
      self:_update_music_channel(channel_name, info)
   end
end

function Sound:_play_sound(sound)
   _radiant.call('radiant:play_sound', sound);
end

function Sound:_choose_best_track(tracks)
   local best_priority, best_track = nil, {}

   for requestor, info in pairs(tracks) do
      local priority = info.priority or 0
      self._log:debug('comparing %s priority %d to current %s.', requestor, priority, tostring(best_priority))
      if not best_priority or priority > best_priority then
         self._log:debug('better!')
         best_priority, best_track = priority, info
      end
   end
   return best_track
end

function Sound:_update_music_channel(channel_name, info)
   local current_info = self._current_music_info[channel_name]

   if current_info == info then
      self._log:debug('keeping last music track')
      return
   end

   self._current_music_info[channel_name] = info

   if info.playlist then
      for i, info in ipairs(info.playlist) do
         local trackinfo = self:_get_trackinfo(info)
         if i == 1 then
            self._log:debug('playing music track %s', trackinfo.track)
            _radiant.audio.play_music(channel_name, trackinfo);
         else
            self._log:debug('queueing music track %s', trackinfo.track)
            _radiant.audio.queue_music(channel_name, trackinfo);
         end
      end
   else
      local trackinfo = self:_get_trackinfo(info)
      self._log:debug('playing music track %s', trackinfo.track)
      _radiant.audio.play_music(channel_name, trackinfo);
   end
end

function Sound:_get_trackinfo(info)
   local trackinfo = _radiant.audio.TrackInfo()
   trackinfo.loop = info.loop and true or false
   trackinfo.volume = info.volume or 100
   trackinfo.fade_in_duration = info.fade_in or 0
   trackinfo.track = self:_choose_random_track(info.track)
   return trackinfo
end

function Sound:_choose_random_track(tracks)
   if type(tracks) == 'string' then
      return tracks
   end

   if type(tracks) == 'table' then
      return tracks[rng:get_int(1, #tracks)]
   end

   return ''
end

return Sound
