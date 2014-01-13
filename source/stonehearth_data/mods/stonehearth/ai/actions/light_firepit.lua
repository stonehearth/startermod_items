--[[
   Task that represents a worker bringing a piece of wood to a firepit and setting it on fire.
]]
FirepitComponent  = require 'components.firepit.firepit_component'
local Point3 = _radiant.csg.Point3

local LightFirepit = class()
LightFirepit.name = 'light fire'
LightFirepit.does = 'stonehearth:light_firepit'
LightFirepit.args = {
   firepit = FirepitComponent, -- the firepit component of the entity to light
}
LightFirepit.version = 2
LightFirepit.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(LightFirepit)
            :execute('stonehearth:pickup_material', { material = ai.ARGS.firepit:get_fuel_material() })
            :execute('stonehearth:drop_carrying_into_entity', { entity = ai.ARGS.firepit:get_entity() })
            :execute('stonehearth:run_effect', { effect = 'light_fire' })
            :execute('stonehearth:trigger_event', { source = ai.ARGS.firepit, event_name = 'stonehearth:firepit_light_up' })

