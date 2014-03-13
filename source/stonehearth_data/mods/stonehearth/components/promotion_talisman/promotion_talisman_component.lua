--[[
   This component stores data about the profession associated with
   an object.
]]

local PromotionTalismanComponent = class()

function PromotionTalismanComponent:initialize(entity, json)
   self._entity = entity
   self._info = json
   self._profession_name = json.profession_name or ''

   radiant.events.listen(entity, 'stonehearth:entity:post_create', function()
         self:_set_profession_name()
         return radiant.events.UNLISTEN
      end)
end

function PromotionTalismanComponent:set_info(info)
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
