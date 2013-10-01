--[[
   This component stores data about the profession associated with
   an object.
]]

local PromotionTalismanComponent = class()

function PromotionTalismanComponent:__init()
   self._info = {}
end

function PromotionTalismanComponent:extend(json)
   self:set_info(json)
end

function PromotionTalismanComponent:set_info(info)
   self._info = info
end

function PromotionTalismanComponent:get_script()
   return self._info.script
end

function PromotionTalismanComponent:set_script(value)
   self._info.script = value
end

function PromotionTalismanComponent:get_workshop()
   return self._info.workshop
end

function PromotionTalismanComponent:set_workshop(value)
   self._info.workshop = value
end

return PromotionTalismanComponent
