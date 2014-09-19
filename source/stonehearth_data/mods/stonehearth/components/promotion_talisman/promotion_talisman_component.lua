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

return PromotionTalismanComponent
