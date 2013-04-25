local Promote = class()
local ch = require 'radiant.core.ch'
local md = require 'radiant.core.md'
md:register_msg_handler("radiant.xxx.promote", Promote)

Promote['radiant.md.create'] = function(self, entity, upgrade_item)
   self._entity = entity
   log:debug('constructing new promote command')
   
   self.co = Coroutine(entity)
   self.co:start(nil, function()
      self.co:travel_to_item(upgrade_item)
      self.co:perform('radiant.effects.civ_promote', self, { talisman = upgrade_item })
      --self.co:pickup(item)     
      om:destroy_entity(upgrade_item)
   end)
end

Promote['radiant.animation.on_trigger'] = function(self, info, effect)
   if info.event == "change_outfit" then
      --local upgrade_item = effect:get_param('talisman')
      --om:add_component(upgrade_item, 'render_info'):set_display_iconic(false)
      md:remove_msg_handler(self._entity, 'radiant.professions.worker')
      md:add_msg_handler(self._entity, 'radiant.professions.carpenter')
      
      local profession = om:add_component(self._entity, 'profession')
      --profession:reset()
      --profession:set_name('carpenter')
      profession:learn_recipe('wooden_practice_sword')
   end
end

-- Behavior messages 

Promote['radiant.ai.behaviors.get_priority'] = function(self, activity)
   return 2
end

Promote['radiant.ai.behaviors.get_next_action'] = function(self, activity)  
   return self.co:yield_next()
end

--

ch:register_cmd("radiant.commands.promote_carpenter", function(item, entity)
   check:is_entity(item)   
   check:is_entity(entity)
   print('promoting %s to carpeneter.', om:get_display_name(entity))

   md:send_msg(entity, 'radiant.queue_user_behavior', 'radiant.xxx.promote', entity, item)
   return {}
end)
