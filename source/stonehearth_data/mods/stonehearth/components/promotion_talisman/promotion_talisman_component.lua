--[[
   This component stores data about the profession associated with
   an object.
]]

local PromotionTalismanComponent = class()

function PromotionTalismanComponent:__init(entity, data_binding)
   self._info = {}
   self._entity = entity

   self._profession_name = ""

   radiant.events.listen(entity, 'stonehearth:entity_created', function()
         self:_set_profession_name()
         return radiant.events.UNLISTEN
      end)
end

function PromotionTalismanComponent:extend(json)
   self:set_info(json)
   if json.profession_name then
      self._profession_name = json.profession_name
   end
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

function PromotionTalismanComponent:_set_profession_name()
   local command_component = self._entity:get_component('stonehearth:commands')
   if command_component then
      command_component:modify_command('promote_to_profession', function(command) 
            command.event_data.promotion_name = self._profession_name
         end)
   end
end

return PromotionTalismanComponent
