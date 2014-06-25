local AdmireFire = class()
local constants = require 'constants'

AdmireFire.name = 'admire fire'
AdmireFire.args = {}
AdmireFire.does = 'stonehearth:admire_fire'
AdmireFire.status_text = 'resting by the fire'
AdmireFire.version = 2
AdmireFire.priority = 1

--- Given an item, determine if it's an unoccupied seat by a lit firepit
-- @param item The item to evaluate
-- @returns True if all conditions are met, false otherwise
function is_seat_by_lit_firepit(item)
   local center_of_attention_spot_component = item:get_component('stonehearth:center_of_attention_spot')
   if center_of_attention_spot_component then
      local center_of_attention = center_of_attention_spot_component:get_center_of_attention()
      if center_of_attention then
         local firepit_component = center_of_attention:get_component('stonehearth:firepit')
         if firepit_component and firepit_component:is_lit() then
            return true
         end
      end
   end
   --To get here, there is either no lease, the lease is taken or it's not a fire
   --or it is but the fire isn't lit
   return false
end

local ai = stonehearth.ai
return ai:create_compound_action(AdmireFire)
            :execute('stonehearth:drop_carrying_now', {})
            :execute('stonehearth:goto_entity_type', {
               description = 'find lit fire',
               filter_fn = is_seat_by_lit_firepit,
            })
            :execute('stonehearth:reserve_entity', { entity = ai.PREV.destination_entity })
            :execute('stonehearth:admire_fire_adjacent', { seat = ai.BACK(1).entity })
