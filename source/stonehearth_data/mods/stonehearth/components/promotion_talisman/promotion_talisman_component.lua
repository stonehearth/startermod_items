--[[
   This component stores data about the job associated with
   an object.
]]

local PromotionTalismanComponent = class()

function PromotionTalismanComponent:initialize(entity, json)
   self._entity = entity
   self._json = json
   self._sv = self.__saved_variables:get_data()
   self._sv.job = self:get_job();
end

function PromotionTalismanComponent:get_job()
   return self._json.job
end

-- A talisman may be associated with a workshop or some other items related
-- to the profession that it was used with. Allow key-value pair associations.
-- @param entity_key - the name of the key we will use to look up the entity 
-- @param target_entity - the entity to associate it with
function PromotionTalismanComponent:associate_with_entity(entity_key, target_entity)
   self._sv[entity_key] = target_entity
end

-- Returns an entity associated with the promotion talisman, nil if there is no key
-- for the entity. 
function PromotionTalismanComponent:get_associated_entity(entity_key)
   return self._sv[entity_key]
end

return PromotionTalismanComponent
