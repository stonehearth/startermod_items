--[[
   This component stores data about the profession associated with
   an object.
]]

local PromotionTalismanComponent = class()

function PromotionTalismanComponent:initialize(entity, json)
   self._entity = entity
   self._json = json
   self._sv = self.__saved_variables:get_data()
   self._sv.profession = self:get_profession();
end

function PromotionTalismanComponent:get_profession()
   return self._json.profession
end

return PromotionTalismanComponent
