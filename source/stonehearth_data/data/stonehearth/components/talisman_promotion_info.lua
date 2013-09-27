--[[
   This component stores data about the profession associated with
   an object.
]]

local TalismanPromotionInfo = class()

function TalismanPromotionInfo:__init()
   self._class_script = nil
   self._promotion_data = nil
end

function TalismanPromotionInfo:extend(json)
   if json and json.class_script then
      self._class_script = json.class_script
   end
end

function TalismanPromotionInfo:get_class_script()
   return self._class_script
end

--[[Place to store profession-specific data
--]]

function TalismanPromotionInfo:set_promotion_data(data)
   self._promotion_data = data
end

function TalismanPromotionInfo:get_promotion_data()
   return self._promotion_data
end

return TalismanPromotionInfo
