local sun
local moon
local lightMatRes

local stonehearth_sky = {}

local constants = {
    ANGLE_X_EAST = 0,
    ANGLE_X_WEST = -180,
    ANGLE_Y = 90
}

local sky_transitions = {
   { 
      t = 0,
      sun_angle = { constants.ANGLE_X_EAST, 90, 0 }
   },
   {
      t = 1,
      sun_angle = { constants.ANGLE_X_WEST, 90, 0 }
   }
}


function stonehearth_sky.__init()
   --stonehearth_sky.add_lights()
end

function stonehearth_sky.add_lights()
   --radiant.events.listen('radiant.events.calendar.minutely', stonehearth_calendar._on_minute)
   lightMatRes = h3dAddResource(H3DResTypes.Material, "materials/light.material.xml", 0)
   h3dSetMaterialUniform(lightMatRes, "ambientLightColor", 1, 1, 1, 1 )
   
   stonehearth_sky._initSun()
   stonehearth_sky._initMoon()

   stonehearth_sky.dawn()
   --stonehearth_sky.noon()
   --stonehearth_sky.dusk()
   --stonehearth_sky.night()
end

-- hardcoded times of day for now
function stonehearth_sky.dawn()
   stonehearth_sky._light_off(moon)
   stonehearth_sky._light_move(sun, constants.ANGLE_X_EAST)
end

function stonehearth_sky.noon()
   stonehearth_sky._light_off(moon)
   stonehearth_sky._light_move(sun, (constants.ANGLE_X_EAST + constants.ANGLE_X_WEST) / 2)
end

function stonehearth_sky.dusk()
   stonehearth_sky._light_off(moon)
   stonehearth_sky._light_move(sun, constants.ANGLE_X_WEST)
end

function stonehearth_sky.night()
   stonehearth_sky._light_off(sun)
   stonehearth_sky._light_color(moon, 0, 0, .5)
   stonehearth_sky._light_move(moon, (constants.ANGLE_X_EAST + constants.ANGLE_X_WEST) / 4)
end

function stonehearth_sky._initSun()
   -- put the sun in the sky
   sun = h3dAddLightNode(H3DRootNode, "Sun", lightMatRes, "DIRECTIONAL_LIGHTING", "DIRECTIONAL_SHADOWMAP")
   h3dSetNodeTransform(sun, 0, 0, 0, -45, 90, 0, 1, 1, 1)
   h3dSetNodeParamF(sun, H3DLight.RadiusF, 0, 100000)
   h3dSetNodeParamF(sun, H3DLight.FovF, 0, 0)
   h3dSetNodeParamI(sun, H3DLight.ShadowMapCountI, 1)
   h3dSetNodeParamI(sun, H3DLight.DirectionalI, 1)
   h3dSetNodeParamF(sun, H3DLight.ShadowSplitLambdaF, 0, 0.9)
   h3dSetNodeParamF(sun, H3DLight.ShadowMapBiasF, 0, 0.001)
   h3dSetNodeParamF(sun, H3DLight.ColorF3, 0, 0.5)
   h3dSetNodeParamF(sun, H3DLight.ColorF3, 1, 0.5)
   h3dSetNodeParamF(sun, H3DLight.ColorF3, 2, 0.5)
end

function stonehearth_sky._initMoon()
   -- put the sun in the sky
   moon = h3dAddLightNode(H3DRootNode, "Moon", lightMatRes, "DIRECTIONAL_LIGHTING", "DIRECTIONAL_SHADOWMAP")
   h3dSetNodeTransform(moon, 0, 0, 0, -45, 90, 0, 1, 1, 1)
   h3dSetNodeParamF(moon, H3DLight.RadiusF, 0, 100000)
   h3dSetNodeParamF(moon, H3DLight.FovF, 0, 0)
   h3dSetNodeParamI(moon, H3DLight.ShadowMapCountI, 1)
   h3dSetNodeParamI(moon, H3DLight.DirectionalI, 1)
   h3dSetNodeParamF(moon, H3DLight.ShadowSplitLambdaF, 0, 0.9)
   h3dSetNodeParamF(moon, H3DLight.ShadowMapBiasF, 0, 0.001)
   h3dSetNodeParamF(moon, H3DLight.ColorF3, 0, 0.2)
   h3dSetNodeParamF(moon, H3DLight.ColorF3, 1, 0.2)
   h3dSetNodeParamF(moon, H3DLight.ColorF3, 2, 0.5)
end

function stonehearth_sky._light_move(node, angle) 
    h3dSetNodeTransform(node, 0, 0, 0, angle, 90, 0, 1, 1, 1)
end

function stonehearth_sky._light_color(node, r, g, b) 
   h3dSetNodeParamF(node, H3DLight.ColorF3, 0, r)
   h3dSetNodeParamF(node, H3DLight.ColorF3, 1, g)
   h3dSetNodeParamF(node, H3DLight.ColorF3, 2, b)
end

function stonehearth_sky._light_off(node)
   stonehearth_sky._light_color(node, 0.0, 0.0, 0.0)
end

-- stub event handler, for when we can pipe the time of day into this thing
function stonehearth_sky._on_minute(_, now)

end

stonehearth_sky.__init()
return stonehearth_sky


