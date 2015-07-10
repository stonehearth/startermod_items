FarmerFieldComponent = require 'components.farmer_field.farmer_field_component'
local Entity = _radiant.om.Entity

local HarvestField = class()
HarvestField.name = 'harvest field'
HarvestField.status_text = 'harvesting field'
HarvestField.does = 'stonehearth:simple_labor'
HarvestField.args = {}
HarvestField.version = 2
HarvestField.priority = stonehearth.constants.priorities.simple_labor.HARVEST_FIELD

local function harvest_layer_filter(entity)
   return entity:get_uri() == 'stonehearth:farmer:field_layer:harvestable'
end

local ai = stonehearth.ai
return ai:create_compound_action(HarvestField)
         :execute('stonehearth:find_path_to_entity_type', {
                  filter_fn = harvest_layer_filter,
                  description = 'find harvest layer',
               })
         :execute('stonehearth:goto_entity', { entity = ai.PREV.destination })
         :execute('stonehearth:reserve_entity_destination', { entity = ai.BACK(2).destination,
                                                              location = ai.PREV.point_of_interest })
         :execute('stonehearth:harvest_crop_adjacent', { crop = ai.BACK(3).destination
                                                                     :get_component('stonehearth:farmer_field_layer')
                                                                        :get_farmer_field()
                                                                           :crop_at(ai.PREV.location) })
         :execute('stonehearth:trigger_event', {
            source = stonehearth.personality,
            event_name = 'stonehearth:journal_event',
            event_args = {
               entity = ai.ENTITY,
               description = 'harvest_entity',
            },
         })
         :execute('stonehearth:drop_carrying_if_stacks_full', {})