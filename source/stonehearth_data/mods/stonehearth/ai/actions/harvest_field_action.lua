FarmerFieldComponent = require 'components.farmer_field.farmer_field_component'
local Entity = _radiant.om.Entity

local HarvestField = class()
HarvestField.name = 'harvest field'
HarvestField.status_text = 'harvesting field'
HarvestField.does = 'stonehearth:harvest_field'
HarvestField.args = {
   field = FarmerFieldComponent
}
HarvestField.version = 2
HarvestField.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(HarvestField)
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.field:get_harvestable_layer() })
         :execute('stonehearth:reserve_entity_destination', { entity = ai.ARGS.field:get_harvestable_layer(),
                                                              location = ai.PREV.point_of_interest })
         :execute('stonehearth:harvest_crop_adjacent', { crop = ai.ARGS.field:crop_at(ai.PREV.location) })
         :execute('stonehearth:trigger_event', {
            source = stonehearth.personality,
            event_name = 'stonehearth:journal_event',
            event_args = {
               entity = ai.ENTITY,
               description = 'harvest_entity',
            },
         })
         :execute('stonehearth:drop_carrying_if_stacks_full', {})