radiant.mods.require('stonehearth_calendar')

local stonehearth_sky = class()
local Vec3 = _radiant.csg.Point3f

function stonehearth_sky:__init()
   self.timing = {
      min = 0,
      pre_sunrise = 280,
      sunrise = 360,
      morning = 540,
      midday = 720,
      afternoon = 900,
      sunset = 1080,
      post_sunset = 1160,
      max = 1440
   }
   self._celestials = {}

   self:_init_sun()

   self._promise = _client:trace_object('/server/objects/stonehearth_calendar/clock', 'rendering the sky')
   if self._promise then
      self._promise:progress(function (data)
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
   local angles = {
      sunrise = Vec3(0, 35, 0),
      midday = Vec3(-90, 0, 0),
      sunset = Vec3(-180, 35, 0)
   }

   local colors = {
      sunrise = Vec3(0.6, 0.25, 0.1),
      morning = Vec3(0.8, 0.6, 0.7),
      midday = Vec3(1, 1, 1),
      afternoon = Vec3(0.8, 0.6, 0.7),
      sunset = Vec3(0.6, 0.25, 0.1),
      night = Vec3(0.0, 0.0, 0.0)
   }
   local ambient_colors = {
      sunrise = Vec3(0.1, 0.1, 0.1),
      midday = Vec3(0.4, 0.4, 0.4),
      sunset = Vec3(0.1, 0.1, 0.1),
      night = Vec3(0, 0, 0)
   }
   local sun_colors = {
      {self.timing.min, self.timing.pre_sunrise, colors.night, colors.night},
      {self.timing.pre_sunrise, self.timing.sunrise, colors.night, colors.sunrise},
      {self.timing.sunrise, self.timing.morning, colors.sunrise, colors.morning},
      {self.timing.morning, self.timing.midday, colors.morning, colors.midday},
      {self.timing.midday, self.timing.afternoon, colors.midday, colors.afternoon},
      {self.timing.afternoon, self.timing.sunset, colors.afternoon, colors.sunset},
      {self.timing.sunset, self.timing.post_sunset, colors.sunset, colors.night},
      {self.timing.post_sunset, self.timing.max, colors.night, colors.night}
   }
   local sun_ambient_colors = {
      {self.timing.min, self.timing.sunrise, ambient_colors.night, ambient_colors.sunrise},
      {self.timing.sunrise, self.timing.midday, ambient_colors.sunrise, ambient_colors.midday},
      {self.timing.midday, self.timing.sunset, ambient_colors.midday, ambient_colors.sunset},
      {self.timing.sunset, self.timing.max, ambient_colors.night, ambient_colors.night}
   }

   local sun_angles = {
      {self.timing.min, self.timing.sunrise, angles.sunrise, angles.sunrise},
      {self.timing.sunrise, self.timing.midday, angles.sunrise, angles.midday},
      {self.timing.midday, self.timing.sunset, angles.midday, angles.sunset},
      {self.timing.sunset, self.timing.max, angles.sunset, angles.sunset}
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
   h3dSetNodeTransform(light.node, 0, 10, 0, x, y, z, 1, 1, 1)
end

stonehearth_sky:__init()
return stonehearth_sky
