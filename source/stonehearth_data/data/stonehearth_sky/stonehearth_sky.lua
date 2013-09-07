radiant.mods.require('stonehearth_calendar')

local stonehearth_sky = class()
local Vec3 = _radiant.csg.Point3f

local timing = {
   min = 0,
   preSunrise = 280,
   sunrise = 360,
   morning = 540,
   midday = 720,
   afternoon = 900,
   twilight = 1080,
   postTwilight = 1160,
   max = 1440
}

local angles = {
   sunriseAng = Vec3(0, 35, 0),
   middayAng = Vec3(-90, 0, 0),
   twilightAng = Vec3(-180, 35, 0)
}

local colors = {
   sunriseCol = Vec3(0.6, 0.25, 0.1),
   morningCol = Vec3(0.8, 0.6, 0.7),
   middayCol = Vec3(1, 1, 1),
   afternoonCol = Vec3(0.8, 0.6, 0.7),
   twilightCol = Vec3(0.6, 0.25, 0.1),
   nightCol = Vec3(0.01, 0.1, 0.41)
}


function stonehearth_sky:__init()
   self._minutes = 0
   self:_initSun()
   radiant.events.listen('radiant.events.calendar.minutely', self)
end

stonehearth_sky['radiant.events.calendar.minutely'] = function(self, now)
   self._minutes = self._minutes + 5
   local color = self:_minutesToColor()
   local angles = self:_minutesToAngles()
   self:_light_color(self.sun, color.x, color.y, color.z)
   self:_light_angles(self.sun, angles.x, angles.y, angles.z)

   if self._minutes >= 1440 then
      self._minutes = 0
   end
end

function stonehearth_sky:_find_value(time, data)
   for _,d in pairs(data) do
      local tStart = d[1]
      local tEnd = d[2]
      local vStart = d[3]
      local vEnd = d[4]

      if time >= tStart and time < tEnd then
         local frac = (time - tStart) / (tEnd - tStart)
         return self:_interpolate(vStart, vEnd, frac)
      end
   end
   return Vec3(0, 0, 0)
end


function stonehearth_sky:_minutesToAngles()
   local data = {
      {timing.min, timing.sunrise, angles.sunriseAng, angles.sunriseAng},
      {timing.sunrise, timing.midday, angles.sunriseAng, angles.middayAng},
      {timing.midday, timing.twilight, angles.middayAng, angles.twilightAng},
      {timing.twilight, timing.max, angles.twilightAng, angles.twilightAng}
   }

   return self:_find_value(self._minutes, data)
end

function stonehearth_sky:_interpolate(a, b, frac)
   local result = Vec3(0, 0, 0)
   result.x = (a.x * (1.0 - frac)) + (b.x * frac)
   result.y = (a.y * (1.0 - frac)) + (b.y * frac)
   result.z = (a.z * (1.0 - frac)) + (b.z * frac)
   return result
end

function stonehearth_sky:_minutesToColor()
   local data = {
      {timing.min, timing.preSunrise, colors.nightCol, colors.nightCol},
      {timing.preSunrise, timing.sunrise, colors.nightCol, colors.sunriseCol},
      {timing.sunrise, timing.morning, colors.sunriseCol, colors.morningCol},
      {timing.morning, timing.midday, colors.morningCol, colors.middayCol},
      {timing.midday, timing.afternoon, colors.middayCol, colors.afternoonCol},
      {timing.afternoon, timing.twilight, colors.afternoonCol, colors.twilightCol},
      {timing.twilight, timing.postTwilight, colors.twilightCol, colors.nightCol},
      {timing.postTwilight, timing.max, colors.nightCol, colors.nightCol}
   }

   return self:_find_value(self._minutes, data)
end

function stonehearth_sky:_initSun()
   self.lightMatRes = h3dAddResource(H3DResTypes.Material, "materials/light.material.xml", 0)
   self.sun = h3dAddLightNode(H3DRootNode, "Sun", self.lightMatRes, "DIRECTIONAL_LIGHTING", "DIRECTIONAL_SHADOWMAP")
   h3dSetNodeTransform(self.sun,  0, 0, 0, -30, -30, 0, 1, 1, 1)
   h3dSetNodeParamF(self.sun, H3DLight.RadiusF, 0, 100000)
   h3dSetNodeParamF(self.sun, H3DLight.FovF, 0, 360)
   h3dSetNodeParamI(self.sun, H3DLight.ShadowMapCountI, 1)
   h3dSetNodeParamI(self.sun, H3DLight.DirectionalI, 1)
   h3dSetNodeParamF(self.sun, H3DLight.ShadowSplitLambdaF, 0, 0.9)
   h3dSetNodeParamF(self.sun, H3DLight.ShadowMapBiasF, 0, 0.001)
   h3dSetNodeParamF(self.sun, H3DLight.ColorF3, 0, 0.05)
   h3dSetNodeParamF(self.sun, H3DLight.ColorF3, 1, 0.3)
   h3dSetNodeParamF(self.sun, H3DLight.ColorF3, 2, 0.4)
end

function stonehearth_sky:_light_color(node, r, g, b)
   h3dSetNodeParamF(node, H3DLight.ColorF3, 0, r)
   h3dSetNodeParamF(node, H3DLight.ColorF3, 1, g)
   h3dSetNodeParamF(node, H3DLight.ColorF3, 2, b)
end

function stonehearth_sky:_light_angles(node, x, y, z) 
   h3dSetNodeTransform(node, 0, 0, 0, x, y, z, 1, 1, 1)
end

stonehearth_sky:__init()
return stonehearth_sky
