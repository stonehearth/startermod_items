--[[
   Attached to entities that are arranged artfully around the a center of attention.
   (For example, denotes seats around a fire, spots near a juggler, etc.)
   Has a lease component attached, since people shouldn't overlap on spots.
]]

local CenterOfAttnSpot = class()

function CenterOfAttnSpot:__init(entity)
   self._entity = entity
   self._coa_entity = nil
   self._spot_id = nil
   self._lease = entity:add_component('stonehearth:lease_component')
end

function CenterOfAttnSpot:extend(json)
end

--- Add this component/entity duo to a center of attention
-- @param coe_entity The entity that people are clustered around
-- @param seat_id Optionally, the unique ID of this spot relative to the coe
function CenterOfAttnSpot:add_to_center_of_attn(coa_entity, spot_id)
   self._coa_entity = coa_entity
   self._spot_id = spot_id
end

function CenterOfAttnSpot:get_center_of_attn()
   return self._coa_entity
end

function CenterOfAttnSpot:get_spot_id()
   return self._spot_id
end

function CenterOfAttnSpot:get_lease()
   return self._lease
end

return CenterOfAttnSpot