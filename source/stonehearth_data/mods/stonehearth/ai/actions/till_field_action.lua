FarmerFieldComponent = require 'components.farmer_field.farmer_field_component'
local Entity = _radiant.om.Entity

local TillField = class()
TillField.name = 'till field'
TillField.status_text_key = 'ai_status_text_till_field'
TillField.does = 'stonehearth:till_field'
TillField.args = {
   field_spacer = Entity,
   field = FarmerFieldComponent
}
TillField.version = 2
TillField.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(TillField)
         :execute('stonehearth:drop_carrying_now')
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.field_spacer})
         :execute('stonehearth:run_effect', { effect = 'work', facing_entity = ai.ARGS.field_spacer  })
         :execute('stonehearth:call_method', {
                  obj = ai.ARGS.field,
                  method = 'till_location',
                  args = {  ai.ARGS.field_spacer }
               })
