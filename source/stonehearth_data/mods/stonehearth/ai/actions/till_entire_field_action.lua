FarmerFieldComponent = require 'components.farmer_field.farmer_field_component'
local Entity = _radiant.om.Entity

local TillEntireField = class()
TillEntireField.name = 'till entire field'
TillEntireField.does = 'stonehearth:till_entire_field'
TillEntireField.args = {
   field = FarmerFieldComponent
}
TillEntireField.version = 2
TillEntireField.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(TillEntireField)
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.field:get_soil_layer() })
         :execute('stonehearth:reserve_entity_destination', { entity = ai.ARGS.field:get_soil_layer(),
                                                              location = ai.PREV.point_of_interest })
         :execute('stonehearth:run_effect', { effect = 'hoe', facing_point = ai.PREV.location })
         :execute('stonehearth:call_method', {
            obj = ai.ARGS.field,
            method = 'notify_till_location_finished',
            args = { ai.BACK(2).location }
         })
