local Entity = _radiant.om.Entity

local TillEntireField = class()
TillEntireField.name = 'till entire field'
TillEntireField.status_text = 'tilling field'
TillEntireField.does = 'stonehearth:simple_labor'
TillEntireField.args = {}
TillEntireField.version = 2
TillEntireField.priority = stonehearth.constants.priorities.simple_labor.TILL_FIELD

local function till_layer_filter(entity)
   return entity:get_uri() == 'stonehearth:farmer:field_layer:tillable'
end

local ai = stonehearth.ai
return ai:create_compound_action(TillEntireField)
         :execute('stonehearth:clear_carrying_now')
         :execute('stonehearth:find_path_to_entity_type', {
                  filter_fn = till_layer_filter,
                  description = 'find till layer',
               })
         :execute('stonehearth:goto_entity', { entity = ai.PREV.destination })
         :execute('stonehearth:reserve_entity_destination', { entity = ai.BACK(2).destination,
                                                              location = ai.PREV.point_of_interest })
         :execute('stonehearth:run_effect', { effect = 'hoe', facing_point = ai.PREV.location })
         :execute('stonehearth:call_method', {
            obj = ai.BACK(4).destination
                              :get_component('stonehearth:farmer_field_layer')
                                 :get_farmer_field(),
            method = 'notify_till_location_finished',
            args = { ai.BACK(2).location }
         })
