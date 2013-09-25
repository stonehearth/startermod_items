local timekeeper = require 'lib.calendar.timekeeper'

local StonehearthSky = class()
local Vec3 = _radiant.csg.Point3f

--[[
   Expose so other classes can do things at certain times of the day. Makes
   sense since people look at the sky to tell them when to do things.
   TODO: day lengths should change based on seasons.
]]

local constants = {
   rise_set_length = 30,
   transition_length = 10
}

function StonehearthSky:__init()
   StonehearthSky:set_sky_constants()
   self._celestials = {}

   self:_init_sun()
   self:_init_moon()
   _radiant.call_obj('stonehearth', 'get_clock_object')
      :done(
         function (o)
            self._clock_object = o.clock_object
            self._clock_promise = self._clock_object:trace('drawing sky')
            self._clock_promise:on_changed(
               function ()
                  local date = self._clock_object:get_data()
                  self:_update(date.minute + (date.hour * 60))
               end
            )
         end
      )
end

function StonehearthSky:set_sky_constants()
   self.timing = {}

   --get some times from the calendar
   local base_times = timekeeper:get_curr_times_of_day()
   local time_constants = timekeeper:get_constants()

   self.timing.midnight = base_times.midnight * time_constants.MINUTES_IN_HOUR
   self.timing.sunrise_start = base_times.sunrise *time_constants.MINUTES_IN_HOUR
   self.timing.sunrise_end = (base_times.sunrise * time_constants.MINUTES_IN_HOUR) + constants.rise_set_length
   self.timing.midday = base_times.midday * time_constants.MINUTES_IN_HOUR
   self.timing.sunset_start = base_times.sunset * time_constants.MINUTES_IN_HOUR
   self.timing.sunset_end = (base_times.sunset * time_constants.MINUTES_IN_HOUR) + constants.rise_set_length
   self.timing.day_length = time_constants.HOURS_IN_DAY * time_constants.MINUTES_IN_HOUR
   self.timing.transition_length = constants.transition_length
end


function StonehearthSky:add_celestial(name, colors, angles, ambient_colors)
   -- TODO: how do we support multiple (deferred) renderers here?
   local light_mat = h3dAddResource(H3DResTypes.Material, "materials/light.material.xml", 0)
   local new_celestial = {
      name = name,
      node = h3dAddLightNode(H3DRootNode, name, light_mat, "DIRECTIONAL_LIGHTING", "DIRECTIONAL_SHADOWMAP"),
      colors = colors,
      ambient_colors = ambient_colors,
      angles = angles,
      light_mat = light_mat
   }

   h3dSetNodeTransform(new_celestial.node,  0, 0, 0, 0, 0, 0, 1, 1, 1)
   h3dSetNodeParamF(new_celestial.node, H3DLight.RadiusF, 0, 10000)
   h3dSetNodeParamF(new_celestial.node, H3DLight.FovF, 0, 360)
   h3dSetNodeParamI(new_celestial.node, H3DLight.ShadowMapCountI, 1)
   h3dSetNodeParamI(new_celestial.node, H3DLight.DirectionalI, 1)
   h3dSetNodeParamF(new_celestial.node, H3DLight.ShadowSplitLambdaF, 0, 0.5)
   h3dSetNodeParamF(new_celestial.node, H3DLight.ShadowMapBiasF, 0, 0.001)

   table.insert(self._celestials, new_celestial)

   self:_update_light(0, new_celestial)
end

function StonehearthSky:get_celestial(name)
   for _, o in pairs(self._celestials) do
      if o.name == name then
         return o
      end
   end
   return nil
end

function StonehearthSky:_update(minutes)
   for _, o in pairs(self._celestials) do
      self:_update_light(minutes, o)
   end
end

function StonehearthSky:_update_light(minutes, light)
   local color = self:_find_value(minutes, light.colors)
   local ambient_color = self:_find_value(minutes, light.ambient_colors)
   local angles = self:_find_value(minutes, light.angles)

   self:_light_color(light, color.x, color.y, color.z)
   self:_light_ambient_color(light, ambient_color.x, ambient_color.y, ambient_color.z)
   self:_light_angles(light, angles.x, angles.y, angles.z)

   if color.x == 0 and color.y == 0 and color.z == 0 then
      h3dSetNodeFlags(light.node, H3DNodeFlags.Inactive, false)
   else
      h3dSetNodeFlags(light.node, 0, false)      
   end
end

function StonehearthSky:_find_value(time, data)
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

function StonehearthSky:_interpolate(time, t_start, t_end, v_start, v_end)
   if t_start > t_end then
      t_end = t_end + self.timing.day_length
      time = time + self.timing.day_length
   end

   local frac = (time - t_start) / (t_end - t_start)
   return self:_vec_interpolate(v_start, v_end, frac)
end

function StonehearthSky:_vec_interpolate(a, b, frac)
   local result = Vec3(0, 0, 0)
   result.x = (a.x * (1.0 - frac)) + (b.x * frac)
   result.y = (a.y * (1.0 - frac)) + (b.y * frac)
   result.z = (a.z * (1.0 - frac)) + (b.z * frac)
   return result
end

function StonehearthSky:_init_sun()
   local t = self.timing.transition_length;

   local angles = {
      sunrise = Vec3(0, 35, 0),
      midday = Vec3(-90, 0, 0),
      sunset = Vec3(-180, 35, 0),
   }

   local colors = {
      sunrise = Vec3(0.5, 0.4, 0.2),
      midday = Vec3(0.6, 0.6, 0.6),
      sunset = Vec3(0.6, 0.2, 0.0),
      night = Vec3(0.0, 0.0, 0.0)
   }

   local ambient_colors = {
      sunrise = Vec3(0.4, 0.3, 0.1),
      midday = Vec3(0.4, 0.4, 0.4),
      sunset = Vec3(0.3, 0.1, 0.0),
      night = Vec3(0.0, 0.0, 0.0)
   }

   local sun_colors = {
      -- night
      {self.timing.sunrise_start, colors.night},

      -- sunrise
      --{self.timing.sunrise_start + t, colors.sunrise},
      --{self.timing.sunrise_end, colors.sunrise},
      --{self.timing.sunrise_end + t, colors.midday},
      {self.timing.sunrise_end, colors.midday},

      -- midday
      {self.timing.sunset_start, colors.midday},

      -- sunset
      --{self.timing.sunset_start + t, colors.sunset},
      --{self.timing.sunset_end, colors.sunset},
      --{self.timing.sunset_end + t, colors.night}
      {self.timing.sunset_end, colors.night}
   }

   local sun_ambient_colors = {
      -- night
      {self.timing.sunrise_start, ambient_colors.night},

      -- sunrise
      --{self.timing.sunrise_start + t, ambient_colors.sunrise},
      --{self.timing.sunrise_end, ambient_colors.sunrise},
      --{self.timing.sunrise_end + t, ambient_colors.midday},
      {self.timing.sunrise_end, ambient_colors.midday},

      -- midday
      {self.timing.sunset_start, ambient_colors.midday},

      -- sunset
      --{self.timing.sunset_start + t, ambient_colors.sunset},
      --{self.timing.sunset_end, ambient_colors.sunset},
      --{self.timing.sunset_end + t, ambient_colors.night},
      {self.timing.sunset_end, ambient_colors.night},
   }

   local sun_angles = {
      {self.timing.sunrise_start, angles.sunrise},
      {self.timing.midday, angles.midday},
      {self.timing.sunset_start, angles.sunset}
   }

   self:add_celestial("sun", sun_colors, sun_angles, sun_ambient_colors)
end


function StonehearthSky:_init_moon()
   local t = self.timing.transition_length;

   local angles = {
      sunset = Vec3(-60, -25, 0),
      midnight = Vec3(-90, -25, 0),
      sunrise = Vec3(-120, -25, 0),
   }

   local colors = {
      sunset_start = Vec3(0.0, 0.0, 0.0),
      sunset_end = Vec3(0.1, 0.1, 0.1),
      sunrise_start = Vec3(0.1, 0.1, 0.1),
      sunrise_end = Vec3(0.0, 0.0, 0.0),
   }

   local ambient_colors = {
      night = Vec3(0.3, 0.3, 0.6),
      day = Vec3(0.0, 0.0, 0.0)
   }

   local moon_colors = {
      {self.timing.midnight, colors.sunset_end},
      {self.timing.sunrise_start, colors.sunrise_start},
      {self.timing.sunrise_end, colors.sunrise_end},
      {self.timing.sunset_start, colors.sunset_start},
      {self.timing.sunset_end, colors.sunset_end},
      {self.timing.day_length, colors.sunset_end}
   }

   local moon_ambient_colors = {
      {self.timing.midnight, ambient_colors.night},
      {self.timing.sunrise_start, ambient_colors.night},
      {self.timing.sunrise_end, ambient_colors.day},
      {self.timing.sunset_start, ambient_colors.day},
      {self.timing.sunset_end, ambient_colors.night},
      {self.timing.day_length, ambient_colors.night}
   }

   local moon_angles = {
      {self.timing.midnight, angles.midnight},
      {self.timing.sunrise_start, angles.sunrise},
      {self.timing.sunset_end, angles.sunset},
      {self.timing.day_length, angles.midnight}
   }

   self:add_celestial("moon", moon_colors, moon_angles, moon_ambient_colors)
end

function StonehearthSky:_light_color(light, r, g, b)
   h3dSetNodeParamF(light.node, H3DLight.ColorF3, 0, r)
   h3dSetNodeParamF(light.node, H3DLight.ColorF3, 1, g)
   h3dSetNodeParamF(light.node, H3DLight.ColorF3, 2, b)
end

function StonehearthSky:_light_ambient_color(light, r, g, b)
   h3dSetNodeParamF(light.node, H3DLight.AmbientColorF3, 0, r)
   h3dSetNodeParamF(light.node, H3DLight.AmbientColorF3, 1, g)
   h3dSetNodeParamF(light.node, H3DLight.AmbientColorF3, 2, b)
end

function StonehearthSky:_light_angles(light, x, y, z)
   h3dSetNodeTransform(light.node, 0, 0, 0, x, y, z, 1, 1, 1)
end

local sky = StonehearthSky()
