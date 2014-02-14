local Buff = class()

function Buff:__init(entity, uri)
   self._entity = entity
   self:_set_info(uri)
end

function Buff:_set_info(uri)
   local json = radiant.resources.load_json(uri)
   for name,value in pairs(json) do
      self[name] = value
   end
end

function Buff:get_controller()
   if not self._controller then      
      local controller = self.controller
      if controller then
         -- instantiate the controller and return it
         local controller_script = radiant.mods.load_script(controller)
         self._controller = controller_script()
      end
   end
   return self._controller
end

function Buff:get_type()
   return self.type
end

function Buff:get_effect()
   return self.effect
end

function Buff:get_modifiers()
   return self.modifiers
end

function Buff:get_duration()
   return self.duration
end

function Buff:get_injected_ai()
   return self.injected_ai 
end

function Buff:get_render_info()
   return self.render
end

function Buff:remove()
   entity:add_component('stonehearth:buffs'):remove_buff(self)
   self.destroy()
end

return Buff
