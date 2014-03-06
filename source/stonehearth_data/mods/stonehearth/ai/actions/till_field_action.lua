FarmerFieldComponent = require 'components.farmer_field.farmer_field_component'
local Entity = _radiant.om.Entity

local TillField = class()
TillField.name = 'till field'
TillField.does = 'stonehearth:till_field'
TillField.args = {
   field_spacer = Entity,
   field = FarmerFieldComponent
}
TillField.version = 2
TillField.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(TillField)
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.field_spacer})
         :execute('stonehearth:run_effect', { effect = 'work', facing_entity = ai.ARGS.field_spacer  })
         :execute('stonehearth:call_function', { fn = ai.ARGS.field.till_location, args = { ai.ARGS.field_spacer } })
