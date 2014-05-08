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

   radiant.events.listen(entity, 'radiant:entity:post_create', function()
         self:_set_profession_name()
         return radiant.events.UNLISTEN
      end)
end

function PromotionTalismanComponent:get_profession()
   return self._json.profession
end

function PromotionTalismanComponent:_set_profession_name()
   local command_component = self._entity:get_component('stonehearth:commands')
   if command_component then
      command_component:modify_command('promote_to_profession', function(command)          
            local profession = radiant.resources.load_json(self._json.profession, true)            
            command.event_data.promotion_name = profession.name
         end)
   end
end

return PromotionTalismanComponent
