local Buff = class()

function Buff:__init(entity, uri)
   self._entity = entity
   self:_set_info(uri)
end

function Buff:_set_info(uri)
   local json = radiant.resources.load_json(uri)
   assert (json.type and (json.type == 'buff' or json.type == 'debuff'), "tried to load a buff file who's type is not buff!")
   for name,value in pairs(json) do
      self[name] = value
   end
end

function Buff:get_type()
   return self.type
end

function Buff:get_modifiers()
   return self.modifiers
end

function Buff:get_render_info()
   return self.render
end

function Buff:remove()
   entity:add_component('stonehearth:buffs'):remove_buff(self)
   self.destroy()
end

return Buff
