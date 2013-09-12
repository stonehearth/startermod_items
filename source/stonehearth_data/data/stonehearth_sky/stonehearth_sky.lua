radiant.mods.require('stonehearth_calendar')

local stonehearth_sky = class()
local Vec3 = _radiant.csg.Point3f

function stonehearth_sky:__init()
   self.timing = {
      min = 0,
      midnight = 0,
      sunrise_start = 360,
      sunrise_end = 390,
      midday = 720,
      sunset_start = 1080,
      sunset_end = 1110,
      max = 1440,
      transition_length = 10
   }
   self._celestials = {}

   self:_init_sun()
   self:_init_moon()
   self._minutes = 1000

   self._promise = _client:trace_object('/server/objects/stonehearth_calendar/clock', 'rendering the sky')
   if self._promise then
      self._promise:progress(function (data)
            --self._minutes = self._minutes + 10
            --if self._minutes >= 1440 then
            --   self._minutes = 0
            --end
            --self:_update(self._minutes)
            self:_update(data.date.minute + (data.date.hour * 60))
         end)
   end
end

function stonehearth_sky:add_celestial(name, colors, angles, ambient_colors)
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

   h3dSetNodeTransform(new_celestial.node,  0, 0, 100, 0, 0, 0, 1, 1, 1)
   h3dSetNodeParamF(new_celestial.node, H3DLight.RadiusF, 0, 100000)
   h3dSetNodeParamF(new_celestial.node, H3DLight.FovF, 0, 360)
   h3dSetNodeParamI(new_celestial.node, H3DLight.ShadowMapCountI, 1)
   h3dSetNodeParamI(new_celestial.node, H3DLight.DirectionalI, 1)
   h3dSetNodeParamF(new_celestial.node, H3DLight.ShadowSplitLambdaF, 0, 0.9)
   h3dSetNodeParamF(new_celestial.node, H3DLight.ShadowMapBiasF, 0, 0.001)

   table.insert(self._celestials, new_celestial)

   self:_update_light(0, new_celestial)
end

function stonehearth_sky:get_celestial(name)
   for _, o in pairs(self._celestials) do
      if o.name == name then
         return o
      end
   end
   return nil
end

function stonehearth_sky:_update(minutes)
   for _, o in pairs(self._celestials) do
      self:_update_light(minutes, o)
   end
end

function stonehearth_sky:_update_light(minutes, light)
   local color = self:_find_value(minutes, light.colors)
   local ambient_color = self:_find_value(minutes, light.ambient_colors)
   local angles = self:_find_value(minutes, light.angles)
   self:_light_color(light, color.x, color.y, color.z)
   self:_light_ambient_color(light, ambient_color.x, ambient_color.y, ambient_color.z)
   self:_light_angles(light, angles.x, angles.y, angles.z)
end

function stonehearth_sky:_find_value(time, data)
   for _,d in pairs(data) do
      local t_start = d[1]
      local t_end = d[2]
      local v_start = d[3]
      local v_end = d[4]

      if time >= t_start and time < t_end then
         local frac = (time - t_start) / (t_end - t_start)
         return self:_interpolate(v_start, v_end, frac)
      end
   end
   return Vec3(0, 0, 0)
end

function stonehearth_sky:_interpolate(a, b, frac)
   local result = Vec3(0, 0, 0)
   result.x = (a.x * (1.0 - frac)) + (b.x * frac)
   result.y = (a.y * (1.0 - frac)) + (b.y * frac)
   result.z = (a.z * (1.0 - frac)) + (b.z * frac)
   return result
end

function stonehearth_sky:_init_sun()
   local t = self.timing.transition_length;

   local angles = {
      sunrise = Vec3(0, 35, 0),
      midday = Vec3(-90, 0, 0),
      sunset = Vec3(-180, 35, 0)
   }

   local colors = {
      sunrise = Vec3(0.5, 0.4, 0.2),
      midday = Vec3(0.6, 0.6, 0.6),
      sunset = Vec3(0.6, 0.2, 0.0),
      night = Vec3(0.0, 0.0, 0.0);
   }

   local ambient_colors = {
      sunrise = Vec3(0.4, 0.3, 0.1),
      midday = Vec3(0.4, 0.4, 0.4),
      sunset = Vec3(0.3, 0.1, 0.0),
      night = Vec3(0.3, 0.3, 0.6)
   }

   local sun_colors = {
      -- night
      {self.timing.min, self.timing.sunrise_start, 
         colors.night, colors.night},

      -- sunrise
      {self.timing.sunrise_start, self.timing.sunrise_start + t, 
         colors.night, colors.sunrise},
      {self.timing.sunrise_start + t, self.timing.sunrise_end, 
         colors.sunrise, colors.sunrise},
      {self.timing.sunrise_end , self.timing.sunrise_end + t, 
         colors.sunrise, colors.midday},

      -- midday
      {self.timing.sunrise_end + t , self.timing.sunset_start, 
         colors.midday, colors.midday},

      -- sunset
      {self.timing.sunset_start, self.timing.sunset_start + t, 
         colors.midday, colors.sunset},
      {self.timing.sunset_start + t, self.timing.sunset_end, 
         colors.sunset, colors.sunset},
      {self.timing.sunset_end, self.timing.sunset_end + t, 
         colors.sunset, colors.night},

      -- night
      {self.timing.sunset_end + t, self.timing.max, 
         colors.night, colors.night}
   }
   local sun_ambient_colors = {
      -- night
      {self.timing.min, self.timing.sunrise_start, 
         ambient_colors.night, ambient_colors.night},

      -- sunrise
      {self.timing.sunrise_start, self.timing.sunrise_start + t, 
         ambient_colors.night, ambient_colors.sunrise},
      {self.timing.sunrise_start + t, self.timing.sunrise_end, 
         ambient_colors.sunrise, ambient_colors.sunrise},
      {self.timing.sunrise_end , self.timing.sunrise_end + t, 
         ambient_colors.sunrise, ambient_colors.midday},

      -- midday
      {self.timing.sunrise_end + t , self.timing.sunset_start, 
         ambient_colors.midday, ambient_colors.midday},

      -- sunset
      {self.timing.sunset_start, self.timing.sunset_start + t, 
         ambient_colors.midday, ambient_colors.sunset},
      {self.timing.sunset_start + t, self.timing.sunset_end, 
         ambient_colors.sunset, ambient_colors.sunset},
      {self.timing.sunset_end, self.timing.sunset_end + t, 
         ambient_colors.sunset, ambient_colors.night},

      -- night
      {self.timing.sunset_end + t, self.timing.max, 
         ambient_colors.night, ambient_colors.night}
   }

   local sun_angles = {
      {self.timing.min, self.timing.sunrise_start, angles.sunrise, angles.sunrise},
      {self.timing.sunrise_start, self.timing.midday, angles.sunrise, angles.midday},
      {self.timing.midday, self.timing.sunset_start, angles.midday, angles.sunset},
      {self.timing.sunset_start, self.timing.max, angles.sunset, angles.sunset}
   }

   self:add_celestial("sun", sun_colors, sun_angles, sun_ambient_colors)
end


function stonehearth_sky:_init_moon()
   local angles = {
      moonrise = Vec3(0, -35, 0),
      midnight = Vec3(-90, 0, 0),
      moonset = Vec3(-180, -35, 0)
   }

   local colors = {
      moonrise = Vec3(0.01, 0.1, 0.2),
      midnight = Vec3(0.01, 0.21, 0.41),
      moonset = Vec3(0.01, 0.1, 0.2),
      daylight = Vec3(0, 0, 0)
   }
   local ambient_colors = {
      moonrise = Vec3(0.1, 0.1, 0.1),
      midnight = Vec3(0.2, 0.2, 0.2),
      moonset = Vec3(0.1, 0.1, 0.1),
      daylight = Vec3(0, 0, 0)
   }

   local moon_colors = {
      {self.timing.pre_moonrise, self.timing.moonrise, colors.daylight, colors.moonrise},
      {self.timing.moonrise, self.timing.max, colors.moonrise, colors.midnight},
      {self.timing.midnight, self.timing.moonset, colors.midnight, colors.moonset},
      {self.timing.sunrise, self.timing.sunset, colors.daylight, colors.daylight}
   }
   local moon_ambient_colors = {
      {self.timing.moonrise, self.timing.max, ambient_colors.moonrise, ambient_colors.midnight},
      {self.timing.midnight, self.timing.dawn_start, ambient_colors.midnight, ambient_colors.moonset},
      {self.timing.dawn_start, self.timing.moonset, ambient_colors.moonset, ambient_colors.daylight},
      {self.timing.sunrise, self.timing.sunset, ambient_colors.daylight, ambient_colors.daylight}
   }

   local moon_angles = {
      {self.timing.moonrise, self.timing.max, angles.moonrise, angles.midnight},
      {self.timing.midnight, self.timing.moonset, angles.midnight, angles.moonset},
      {self.timing.sunrise, self.timing.sunset, angles.moonset, angles.moonrise}
   }

   --self:add_celestial("moon", moon_colors, moon_angles, moon_ambient_colors)
end

function stonehearth_sky:_light_color(light, r, g, b)
   h3dSetNodeParamF(light.node, H3DLight.ColorF3, 0, r)
   h3dSetNodeParamF(light.node, H3DLight.ColorF3, 1, g)
   h3dSetNodeParamF(light.node, H3DLight.ColorF3, 2, b)
end

function stonehearth_sky:_light_ambient_color(light, r, g, b)
   h3dSetNodeParamF(light.node, H3DLight.AmbientColorF3, 0, r)
   h3dSetNodeParamF(light.node, H3DLight.AmbientColorF3, 1, g)
   h3dSetNodeParamF(light.node, H3DLight.AmbientColorF3, 2, b)
end

function stonehearth_sky:_light_angles(light, x, y, z) 
   h3dSetNodeTransform(light.node, 0, 10, 0, x, y, z, 1, 1, 1)
end

stonehearth_sky:__init()
return stonehearth_sky
