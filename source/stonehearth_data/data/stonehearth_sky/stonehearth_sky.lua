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

function stonehearth_sky:add_celestial(name, colors, angles)
   -- TODO: how do we support multiple (deferred) renderers here?
   local light_mat = h3dAddResource(H3DResTypes.Material, "materials/light.material.xml", 0)
   local new_celestial = {
      name = name,
      node = h3dAddLightNode(H3DRootNode, name, light_mat, "DIRECTIONAL_LIGHTING", "DIRECTIONAL_SHADOWMAP"),
      colors = colors,
      angles = angles
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
   local angles = self:_find_value(minutes, light.angles)
   self:_light_color(light.node, color.x, color.y, color.z)
   self:_light_angles(light.node, angles.x, angles.y, angles.z)
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
      sunrise_ang = Vec3(0, 35, 0),
      midday_ang = Vec3(-90, 0, 0),
      sunset_ang = Vec3(-180, 35, 0)
   }

   local colors = {
      sunrise_col = Vec3(0.6, 0.25, 0.1),
      morning_col = Vec3(0.8, 0.6, 0.7),
      midday_col = Vec3(1, 1, 1),
      afternoon_col = Vec3(0.8, 0.6, 0.7),
      sunset_col = Vec3(0.6, 0.25, 0.1),
      night_col = Vec3(0.01, 0.1, 0.41)
   }
   local sun_colors = {
      {self.timing.min, self.timing.pre_sunrise, colors.night_col, colors.night_col},
      {self.timing.pre_sunrise, self.timing.sunrise, colors.night_col, colors.sunrise_col},
      {self.timing.sunrise, self.timing.morning, colors.sunrise_col, colors.morning_col},
      {self.timing.morning, self.timing.midday, colors.morning_col, colors.midday_col},
      {self.timing.midday, self.timing.afternoon, colors.midday_col, colors.afternoon_col},
      {self.timing.afternoon, self.timing.sunset, colors.afternoon_col, colors.sunset_col},
      {self.timing.sunset, self.timing.post_sunset, colors.sunset_col, colors.night_col},
      {self.timing.post_sunset, self.timing.max, colors.night_col, colors.night_col}
   }

   local sun_angles = {
      {self.timing.min, self.timing.sunrise, angles.sunrise_ang, angles.sunrise_ang},
      {self.timing.sunrise, self.timing.midday, angles.sunrise_ang, angles.midday_ang},
      {self.timing.midday, self.timing.sunset, angles.midday_ang, angles.sunset_ang},
      {self.timing.sunset, self.timing.max, angles.sunset_ang, angles.sunset_ang}
   }

   self:add_celestial("sun", sun_colors, sun_angles)
end

function stonehearth_sky:_light_color(node, r, g, b)
   h3dSetNodeParamF(node, H3DLight.ColorF3, 0, r)
   h3dSetNodeParamF(node, H3DLight.ColorF3, 1, g)
   h3dSetNodeParamF(node, H3DLight.ColorF3, 2, b)
end

function stonehearth_sky:_light_angles(node, x, y, z) 
   h3dSetNodeTransform(node, 0, 10, 0, x, y, z, 1, 1, 1)
end

stonehearth_sky:__init()
return stonehearth_sky
