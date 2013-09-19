local stonehearth_sky = class()
local Vec3 = _radiant.csg.Point3f

function stonehearth_sky:__init()
   self.timing = {
      midnight = 0,
      sunrise_start = 360,
      sunrise_end = 390,
      midday = 720,
      sunset_start = 1080,
      sunset_end = 1110,
      day_length = 1440,
      transition_length = 10
   }
   self._celestials = {}

   self:_init_sun()
   self._minutes = 0

   self._promise = _radiant.client.trace_object('stonehearth_calendar.clock', 'rendering the sky')
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
   local color = self:_find_value2(minutes, light.colors)
   local ambient_color = self:_find_value2(minutes, light.ambient_colors)
   local angles = self:_find_value2(minutes, light.angles)

   self:_light_color(light, color.x, color.y, color.z)
   self:_light_ambient_color(light, ambient_color.x, ambient_color.y, ambient_color.z)
   self:_light_angles(light, angles.x, angles.y, angles.z)
end

function stonehearth_sky:_find_value2(time, data)
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

function stonehearth_sky:_interpolate(time, t_start, t_end, v_start, v_end)
   if t_start > t_end then
      t_end = t_end + self.timing.day_length
      time = time + self.timing.day_length
   end

   local frac = (time - t_start) / (t_end - t_start)
   return self:_vec_interpolate(v_start, v_end, frac)
end

function stonehearth_sky:_vec_interpolate(a, b, frac)
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
      night = Vec3(0.0, 0.0, 0.0)
   }

   local ambient_colors = {
      sunrise = Vec3(0.4, 0.3, 0.1),
      midday = Vec3(0.4, 0.4, 0.4),
      sunset = Vec3(0.3, 0.1, 0.0),
      night = Vec3(0.3, 0.3, 0.6)
   }

   local sun_colors = {
      -- night
      {self.timing.sunrise_start, colors.night},

      -- sunrise
      {self.timing.sunrise_start + t, colors.sunrise},
      {self.timing.sunrise_end, colors.sunrise},
      {self.timing.sunrise_end + t, colors.midday},

      -- midday
      {self.timing.sunset_start, colors.midday},

      -- sunset
      {self.timing.sunset_start + t, colors.sunset},
      {self.timing.sunset_end, colors.sunset},
      {self.timing.sunset_end + t, colors.night}
   }

   local sun_ambient_colors = {
      -- night
      {self.timing.sunrise_start, ambient_colors.night},

      -- sunrise
      {self.timing.sunrise_start + t, ambient_colors.sunrise},
      {self.timing.sunrise_end, ambient_colors.sunrise},
      {self.timing.sunrise_end + t, ambient_colors.midday},

      -- midday
      {self.timing.sunset_start, ambient_colors.midday},

      -- sunset
      {self.timing.sunset_start + t, ambient_colors.sunset},
      {self.timing.sunset_end, ambient_colors.sunset},
      {self.timing.sunset_end + t, ambient_colors.night},
   }

   local sun_angles = {
      {self.timing.sunrise_start, angles.sunrise},
      {self.timing.midday, angles.midday},
      {self.timing.sunset_start, angles.sunset}
   }

   self:add_celestial("sun", sun_colors, sun_angles, sun_ambient_colors)
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
   h3dSetNodeTransform(light.node, 0, 0, 0, x, y, z, 1, 1, 1)
end

stonehearth_sky:__init()
return stonehearth_sky
