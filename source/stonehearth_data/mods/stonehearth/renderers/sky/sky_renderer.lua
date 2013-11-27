local SkyRenderer = class()
local Vec3 = _radiant.csg.Point3f

--[[
   Expose so other classes can do things at certain times of the day. Makes
   sense since people look at the sky to tell them when to do things.
   TODO: day lengths should change based on seasons.
]]

local constants = {
   rise_set_length = 0.5 * 3600
}

function SkyRenderer:__init()
   SkyRenderer:set_sky_constants()
   self._celestials = {}

   self:_init_sun()
   self:_init_moon()
   _radiant.call('stonehearth:get_clock_object')
      :done(
         function (o)
            self._clock_object = o.clock_object
            self._clock_promise = self._clock_object:trace_data('drawing sky')
            self._clock_promise:on_changed(
               function ()
                  local date = self._clock_object:get_data()
                  self:_update_time(date.second + (self._time_constants.seconds_per_minute * (date.minute + (self._time_constants.minutes_per_hour * date.hour))))
               end
            )
         end
      )
   self._render_promise = _radiant.client.trace_render_frame()
                              :on_frame_start('render sky', function(now, interpolate)
                                    self:_interpolate_time(now, interpolate)
                                 end)
end

function SkyRenderer:set_sky_constants()
   self.timing = {}

   --get some times from the calendar
   local time_constants = radiant.resources.load_json('/stonehearth/services/calendar/calendar_constants.json')
   local base_times = time_constants.event_times
   
   local seconds_per_hour = time_constants.seconds_per_minute * time_constants.minutes_per_hour
   self.timing.midnight = base_times.midnight * seconds_per_hour
   self.timing.sunrise_start = base_times.sunrise *seconds_per_hour
   self.timing.sunrise_end = (base_times.sunrise * seconds_per_hour) + constants.rise_set_length
   self.timing.midday = base_times.midday * seconds_per_hour
   self.timing.sunset_start = (base_times.sunset * seconds_per_hour) - constants.rise_set_length
   self.timing.sunset_end = base_times.sunset * seconds_per_hour
   self.timing.sunset_peak = self.timing.sunset_start + ((self.timing.sunset_end - self.timing.sunset_start) * 1/3)
   self.timing.day_length = time_constants.hours_per_day * seconds_per_hour
   
   self._time_constants = time_constants
end


function SkyRenderer:add_celestial(name, colors, angles, ambient_colors, depth_offsets)
   -- TODO: how do we support multiple (deferred) renderers here?
   local light_mat = h3dAddResource(H3DResTypes.Material, "materials/deferred_light.material.xml", 0)
   local new_celestial = {
      name = name,
      node = h3dAddLightNode(H3DRootNode, name, light_mat, "DIRECTIONAL_LIGHTING", "DIRECTIONAL_SHADOWMAP"),
      colors = colors,
      ambient_colors = ambient_colors,
      angles = angles,
      light_mat = light_mat,
      depth_offsets = depth_offsets
   }

   h3dSetNodeTransform(new_celestial.node,  0, 0, 0, 0, 0, 0, 1, 1, 1)
   h3dSetNodeParamF(new_celestial.node, H3DLight.RadiusF, 0, 10000)
   h3dSetNodeParamF(new_celestial.node, H3DLight.FovF, 0, 360)
   h3dSetNodeParamI(new_celestial.node, H3DLight.ShadowMapCountI, 4)
   h3dSetNodeParamI(new_celestial.node, H3DLight.DirectionalI, 1)
   h3dSetNodeParamF(new_celestial.node, H3DLight.ShadowSplitLambdaF, 0, 0.5)
   h3dSetNodeParamF(new_celestial.node, H3DLight.ShadowMapBiasF, 0, 0.001)

   table.insert(self._celestials, new_celestial)

   self:_update_light(0, new_celestial)
end

function SkyRenderer:get_celestial(name)
   for _, o in pairs(self._celestials) do
      if o.name == name then
         return o
      end
   end
   return nil
end

function SkyRenderer:_update_time(seconds)
   -- compute the interval between calls to _update so we can interpolate
   -- between frames
   if self._calendar_seconds then
      self._calendar_seconds_between_updates = seconds - self._calendar_seconds
      self._render_ms_between_updates = self._last_render_time_ms - self._render_time_at_last_update
   end
   self._calendar_seconds = seconds
   self._render_time_at_last_update = self._last_render_time_ms
   self:_update(self._calendar_seconds)
end

function SkyRenderer:_interpolate_time(now)
   if self._calendar_seconds_between_updates then
      -- assume the calendar moves foward at a rate equal to the amount it moved forward between
      -- the last two updates.  so to calculate where we are now, we just take the last update
      -- time and some fraction of the time elapsed between the last two updates.  that gap should
      -- be proportional to the percentage of time that's elapsed between the last and the next
      -- updates.  we'll use the render clock time as a gross approximation of the update timer,
      -- since we expect the calendar to move time forward at a constant rate      
      local interpolation_time = now - self._render_time_at_last_update
      if interpolation_time > self._render_ms_between_updates then
         -- cap the distance we're willing to look ahead, just in case there's some jitter in the
         -- update or render frequency
         interpolation_time = self._render_ms_between_updates
      end
      -- if we get two consecutive updates without getting a call to interpolate (e.g. if rendering
      -- a frame takes f o r e v 3 r ...) self._render_ms_between_updates will be 0 (sinnce we didn't
      -- tick the render clock).  if so, just don't bother. 
      if self._render_ms_between_updates ~= 0 then
         local interpolation_seconds = self._calendar_seconds_between_updates * (interpolation_time / self._render_ms_between_updates)
         self:_update(self._calendar_seconds + math.floor(interpolation_seconds))
      end
   end
   self._last_render_time_ms = now
end

function SkyRenderer:_update(seconds)
   for _, o in pairs(self._celestials) do
      self:_update_light(seconds, o)
   end
end
local last = 0

function SkyRenderer:_update_light(seconds, light)
   local color = self:_find_value(seconds, light.colors)
   local ambient_color = self:_find_value(seconds, light.ambient_colors)
   local angles = self:_find_value(seconds, light.angles)
   local depth_offset = self:_find_value(seconds, light.depth_offsets)

   last = seconds
   
   self:_light_color(light, color.x, color.y, color.z)
   self:_light_ambient_color(light, ambient_color.x, ambient_color.y, ambient_color.z)
   self:_light_angles(light, angles.x, angles.y, angles.z)

   if color.x == 0 and color.y == 0 and color.z == 0 then
      h3dSetNodeFlags(light.node, H3DNodeFlags.Inactive, false)
   else
      h3dSetNodeFlags(light.node, 0, false)      
   end
   h3dSetNodeParamF(light.node, H3DLight.ShadowMapBiasF, 0, depth_offset.x)
end

function SkyRenderer:_find_value(time, data)
   local t_start = data[1][1]
   local v_start = data[1][2]

   if time < t_start then
      return self:_interpolate(time, data[#data][1], t_start, data[#data][2], v_start)
   end

   local t_end = 0
   local v_end = 0
   for i = 2, #data do
      t_end = data[i][1]
      v_end = data[i][2]

      if time < t_end then
         return self:_interpolate(time, t_start, t_end, v_start, v_end)
      end

      t_start = t_end
      v_start = v_end
   end

   return self:_interpolate(time, t_end, data[1][1], v_end, data[1][2])
end

function SkyRenderer:_interpolate(time, t_start, t_end, v_start, v_end)
   if t_start > t_end then
      t_end = t_end + self.timing.day_length
      time = time + self.timing.day_length
   end
   if t_start == t_end then
      return v_start
   end
   local frac = (time - t_start) / (t_end - t_start)
   return self:_vec_interpolate(v_start, v_end, frac)
end

function SkyRenderer:_vec_interpolate(a, b, frac)
   local result = Vec3(0, 0, 0)
   result.x = (a.x * (1.0 - frac)) + (b.x * frac)
   result.y = (a.y * (1.0 - frac)) + (b.y * frac)
   result.z = (a.z * (1.0 - frac)) + (b.z * frac)
   return result
end

local sky_colors = {
   off = Vec3(0, 0, 0),
   day = Vec3(0.6, 0.6, 0.6),
   sunset = Vec3(1.0, 1.0, 0.4),
   night = Vec3(0.31, 0.25, 0.55)
}

local sky_ambient_colors = {
   off = Vec3(0, 0, 0),
   day = Vec3(0.4, 0.4, 0.4),
   sunset = Vec3(0.4, 0.35, 0.3),
   night = Vec3(0.0, 0.20, 0.52)
}

function SkyRenderer:_init_sun()

   local angles = {
      pre_sunrise = Vec3(0, 35, 0),
      sunrise = Vec3(0, 35, 0),
      midday = Vec3(-90, 0, 0),
      sunset = Vec3(-180, 35, 0),
   }

   local depth_offset_values = {
      sunrise_start = Vec3(0.001,0,0),
      sunrise_end = Vec3(0.001,0,0),
      midday = Vec3(0.001,0,0),
      sunset_start = Vec3(0.001,0,0),
      sunset_end = Vec3(0.001,0,0)
   }

   local sun_colors = {
      {0, sky_colors.off},
      {self.timing.sunrise_start, sky_colors.off},
      {self.timing.sunrise_end, sky_colors.day},
      {self.timing.sunset_start, sky_colors.day},
      {self.timing.sunset_peak, sky_colors.sunset},
      {self.timing.sunset_end, sky_colors.off}
   }

   local sun_ambient_colors = {
      {0, sky_ambient_colors.off},
      {self.timing.sunrise_start, sky_ambient_colors.off},
      {self.timing.sunrise_end, sky_ambient_colors.day},
      {self.timing.sunset_start, sky_ambient_colors.day},
      {self.timing.sunset_peak, sky_ambient_colors.sunset},
      {self.timing.sunset_end, sky_ambient_colors.off}
   }

   local sun_angles = {
      {self.timing.sunrise_start, angles.sunrise},
      {self.timing.midday, angles.midday},
      {self.timing.sunset_end, angles.sunset}
   }

   local depth_offsets = {
      {self.timing.sunrise_start, depth_offset_values.sunrise_start},
      {self.timing.sunrise_end, depth_offset_values.sunrise_end},
      {self.timing.midday, depth_offset_values.midday},
      {self.timing.sunset_start, depth_offset_values.sunset_start},
      {self.timing.sunset_end, depth_offset_values.sunset_end}
   }

   self:add_celestial("sun", sun_colors, sun_angles, sun_ambient_colors, depth_offsets)
end


function SkyRenderer:_init_moon()

   local angles = {
      sunset = Vec3(-60, -25, 0),
      midnight = Vec3(-90, -25, 0),
      sunrise = Vec3(-120, -25, 0),
   }

   local moon_colors = {
      {0, sky_colors.night},
      {self.timing.sunrise_start, sky_colors.night},
      {self.timing.sunrise_end, sky_colors.off},
      {self.timing.sunset_peak, sky_colors.off},
      {self.timing.sunset_end, sky_colors.night}
   }

   local moon_ambient_colors = {
      {0, sky_ambient_colors.night},
      {self.timing.sunrise_start, sky_ambient_colors.night},
      {self.timing.sunrise_end, sky_ambient_colors.off},
      {self.timing.sunset_peak, sky_ambient_colors.off},
      {self.timing.sunset_end, sky_ambient_colors.night}
   }

   local moon_angles = {
      {self.timing.midnight, angles.midnight},
      {self.timing.sunrise_end, angles.sunrise},
      {self.timing.sunset_start, angles.sunset},
      {self.timing.day_length, angles.midnight}
   }

   local depth_offsets = {
      {self.timing.midnight, Vec3(0.001, 0, 0)},
      {self.timing.day_length, Vec3(0.001, 0, 0)}
   }

   self:add_celestial("moon", moon_colors, moon_angles, moon_ambient_colors, depth_offsets)
end

function SkyRenderer:_light_color(light, r, g, b)
   h3dSetNodeParamF(light.node, H3DLight.ColorF3, 0, r)
   h3dSetNodeParamF(light.node, H3DLight.ColorF3, 1, g)
   h3dSetNodeParamF(light.node, H3DLight.ColorF3, 2, b)
end

function SkyRenderer:_light_ambient_color(light, r, g, b)
   h3dSetNodeParamF(light.node, H3DLight.AmbientColorF3, 0, r)
   h3dSetNodeParamF(light.node, H3DLight.AmbientColorF3, 1, g)
   h3dSetNodeParamF(light.node, H3DLight.AmbientColorF3, 2, b)
end

function SkyRenderer:_light_angles(light, x, y, z)
   h3dSetNodeTransform(light.node, 0, 0, 0, x, y, z, 1, 1, 1)
end

local sky = SkyRenderer()
