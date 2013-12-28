--[[
   This component stores data about the profession associated with
   an object.
]]

local PromotionTalismanComponent = class()

function PromotionTalismanComponent:__init(entity)
   self._info = {}
   self._entity = entity
end

function PromotionTalismanComponent:extend(json)
   self:set_info(json)
end

function PromotionTalismanComponent:set_info(info)
   self._info = info
   self:_set_promotion_cmd_info(info)
end

function PromotionTalismanComponent:_set_promotion_cmd_info(info)
   local command_component = self._entity:get_component('stonehearth:commands')
   if command_component then
      command_component:modify_command('promote_to_profession', function(command) 
            command.event_data.promotion_name = info.profession_name      
         end)
   end
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
