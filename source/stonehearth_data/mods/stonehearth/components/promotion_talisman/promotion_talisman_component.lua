--[[
   This component stores data about the profession associated with
   an object.
]]

local PromotionTalismanComponent = class()

function PromotionTalismanComponent:__init()
   self._class_script = nil
   self._promotion_data = nil
end

function PromotionTalismanComponent:extend(json)
   if json and json.class_script then
      self._class_script = json.class_script
   end
end

function PromotionTalismanComponent:get_class_script()
   return self._class_script
end

--[[Place to store profession-specific data
--]]

function PromotionTalismanComponent:set_promotion_data(data)
   self._promotion_data = data
end

function PromotionTalismanComponent:get_promotion_data()
   return self._promotion_data
end

return PromotionTalismanComponent
