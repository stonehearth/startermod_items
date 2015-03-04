local rng = _radiant.csg.get_default_rng()

local Sound = class()

local IDLE_DAY_AMBIENT = {
   track    = 'stonehearth:ambient:summer_day',
   fade     = 4000,
   volume   = 60,
}

local IDLE_NIGHT_AMBIENT = {
   track    = 'stonehearth:ambient:summer_night',
   fade     = 4000,
   volume   = 20,
}

local IDLE_DAY_MUSIC = {
   track    = {
      'stonehearth:music:levelmusic_spring_day_01',
      'stonehearth:music:levelmusic_spring_day_02',
      'stonehearth:music:levelmusic_spring_day_03',
   },
   fade     = 4000,
   volume   = 35,
}

local IDLE_NIGHT_MUSIC = {
   track    = {
      'stonehearth:music:levelmusic_spring_night_01',
      'stonehearth:music:levelmusic_spring_night_02',
   },
   fade     = 4000,
   volume   = 35,
}
local TITLE_MUSIC = {
   track    = 'stonehearth:music:title_screen',
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

   self._current_music = {}
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

   radiant.events.listen(radiant, 'radiant:client:ui_screen_changed', self, self._on_ui_screen_changed)
   self:_on_ui_screen_changed()

   --[[
   -- currently racy...
   _radiant.call_obj('stonehearth.population', 'get_population_command')
               :done(function(o)
                     local pop = o.uri
                     self._pop_trace = pop:trace('music service')
                                             :on_changed(function()
                                                   self:_on_population_changed(pop:get_data())
                                                end)
                                             :push_object_state()
                  end)
   ]]

   _radiant.call('stonehearth:get_clock_object')
               :done(function (o)
                     local clock = o.clock_object
                     self._clock_trace = clock:trace('music service')
                                                :on_changed(function()
                                                      self:_on_time_changed(clock:get_data())
                                                   end)
                                                :push_object_state()
                  end)

   self._render_promise = _radiant.client.trace_render_frame()
                              :on_frame_start('change music', function(now, interpolate)
                                    self:_change_music(now, interpolate)
                                 end)

end

function Sound:_on_ui_screen_changed()
   self._current_ui_screen = _radiant.client.get_current_ui_screen()
end

function Sound:_on_population_changed(data)
   self._threat_level = data.threat_level
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
      self:_play_sound('stonehearth:sounds:rooster_call')
      self:_play_sound('stonehearth:music:daybreak_stinger_01')
   end
   if date.hour == event_times.sunset and not self._sunset_sound_played then
      self._sunset_sound_played = true
      self:_play_sound('stonehearth:sounds:owl_call')
      self:_play_sound('stonehearth:music:nightfall_stinger_01')
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
   channels[channel_name] = info
end

function Sound:_change_music(now)
   assert(self._current_ui_screen)

   local channels = self._screens[self._current_ui_screen]
   assert(channels)
   
   for channel_name, _ in pairs(ALL_CHANNELS) do
      local requestor, info = self:_choose_best_track(channels[channel_name])
      self:_update_music_channel(channel_name, info)
   end
end

function Sound:_play_sound(track)
   _radiant.call('radiant:play_sound', {
         track = track
      });
end

function Sound:_choose_best_track(channels)
   -- could be betta...
   return next(channels)
end

function Sound:_update_music_channel(channel_name, info)
   local current_track = self._current_music[channel_name]
   local new_track = self:_get_track_from_track_info(current_track, info)

   if current_track ~= new_track then
      self._current_music[channel_name] = new_track

      local params = radiant.shallow_copy(info or {})
      params.track   = new_track
      params.channel = channel_name

      _radiant.call('radiant:play_music', params);
   end
end

function Sound:_get_track_from_track_info(current_track, info)
   if not info then
      return ''
   end

   if type(info.track) == 'string' then
      return info.track
   end

   if type(info.track) == 'table' then
      for _, track in pairs(info.track) do
         if current_track == track then
            return track
         end
      end
      return info.track[rng:get_int(1, #info.track)]
   end
   
   return ''
end

return Sound
