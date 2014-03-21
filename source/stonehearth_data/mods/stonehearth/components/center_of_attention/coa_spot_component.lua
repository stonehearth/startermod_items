--[[
   Attached to entities that are arranged artfully around the a center of attention.
   (For example, denotes seats around a fire, spots near a juggler, etc.)
]]

local CenterOfAttentionSpot = class()

function CenterOfAttentionSpot:initialize(entity, json)
   self._sv = self.__saved_variables:get_data()
   self._coa_entity = nil
   self._spot_id = nil
   self._entity = entity
end

--- Add this component/entity duo to a center of attention
-- @param coe_entity The entity that people are clustered around
-- @param seat_id Optionally, the unique ID of this spot relative to the coe
function CenterOfAttentionSpot:add_to_center_of_attention(coa_entity, spot_id)
   self._sv.spot_id = spot_id
   self._sv.coa_entity = coa_entity
   self.__saved_variables:mark_changed()
end

function CenterOfAttentionSpot:get_center_of_attention()
   return self._sv.coa_entity
end

function CenterOfAttentionSpot:get_spot_id()
   return self._sv.spot_id
end

return CenterOfAttentionSpot