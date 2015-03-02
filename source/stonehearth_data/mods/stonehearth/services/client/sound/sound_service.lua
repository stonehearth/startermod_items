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


function Sound:initialize()
   self._sv = self.__saved_variables:get_data()
   self._log = radiant.log.create_logger('sound')
   self._calendar_constants = radiant.resources.load_json('/stonehearth/data/calendar/calendar_constants.json')
   self._music_channels = {}
   self._current_music = {}

   _radiant.call_obj('stonehearth.population', 'get_population_command')
               :done(function(o)
                     local pop = o.uri
                     self._pop_trace = pop:trace('music service')
                                             :on_changed(function()
                                                   self:_on_population_changed(pop:get_data())
                                                end)
                                             :push_object_state()
                  end)

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

function Sound:_on_population_changed(data)
   self._log:write(0, 'population changed...')
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
      self:recommend_music('clock', 'music',    IDLE_DAY_MUSIC)
      self:recommend_music('clock', 'ambient',  IDLE_DAY_AMBIENT)
   else
      self:recommend_music('clock', 'music',    IDLE_NIGHT_MUSIC)
      self:recommend_music('clock', 'ambient',  IDLE_NIGHT_AMBIENT)
   end

end

function Sound:recommend_music(requestor, channel, info)
   local channels = self._music_channels[channel]
   if not channels then
      channels = {}
      self._music_channels[channel] = channels
   end
   channels[channel] = info
end

function Sound:_change_music(now)
   for chan, channels in pairs(self._music_channels) do
      local requestor, info = self:_choose_best_channel(chan, channels)
      self:_update_music_channel(chan, info)
   end
end

function Sound:_play_sound(track)
   _radiant.call('radiant:play_sound', {
         track = track
      });
end

function Sound:_choose_best_channel(name, channels)
   return next(channels)
end

function Sound:_update_music_channel(channel_name, info)
   local new_track
   local current_track = self._current_music[channel_name]

   if type(info.track) == 'string' then
      new_track = info.track
   end

   if type(info.track) == 'table' then
      for _, track in pairs(info.track) do
         if current_track == track then
            new_track = track
            break
         end
      end
      if not new_track then
         new_track = info.track[rng:get_int(1, #info.track)]
      end
   end

   if current_track ~= new_track then
      self._current_music[channel_name] = new_track

      local params = radiant.shallow_copy(info)
      params.track   = new_track
      params.channel = channel_name

      _radiant.call('radiant:play_music', params);
   end
end

return Sound
