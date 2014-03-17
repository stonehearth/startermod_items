--[[
   Attached to entities that are arranged artfully around the a center of attention.
   (For example, denotes seats around a fire, spots near a juggler, etc.)
]]

local CenterOfAttentionSpot = class()

function CenterOfAttentionSpot:__init()
   self._coa_entity = nil
   self._spot_id = nil
end

function CenterOfAttentionSpot:initialize(entity, json)
   self._entity = entity
   self.__saved_variables = {}
end

function CenterOfAttentionSpot:restore(entity, saved_variables)
   self.__saved_variables = saved_variables
end

--- Add this component/entity duo to a center of attention
-- @param coe_entity The entity that people are clustered around
-- @param seat_id Optionally, the unique ID of this spot relative to the coe
function CenterOfAttentionSpot:add_to_center_of_attention(coa_entity, spot_id)
   self.__saved_variables = {
      coa_entity = coa_entity,
      spot_id = spot_id,
   }
end

function CenterOfAttentionSpot:get_center_of_attention()
   return self.__saved_variables.coa_entity
end

function CenterOfAttentionSpot:get_spot_id()
   return self.__saved_variables.spot_id
end

return CenterOfAttentionSpot