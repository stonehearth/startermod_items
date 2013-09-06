radiant.mods.require('stonehearth_calendar')

local stonehearth_sky = class()


function stonehearth_sky:__init()
   radiant.log.info('init sky.')
   self:_initSun()
   radiant.events.listen('radiant.events.calendar.minutely', self)
end

stonehearth_sky['radiant.events.calendar.minutely'] = function(self, calendar)
   radiant.log.info('adjusting sky.')
   self:_light_color(self.sun, calendar.second / 60.0, calendar.second / 60.0, calendar.second / 60.0)
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


stonehearth_sky:__init()
return stonehearth_sky


