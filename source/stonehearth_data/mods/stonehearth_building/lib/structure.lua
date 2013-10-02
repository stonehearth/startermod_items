local Structure = class()

function Structure:__init(entity)
   self._entity = entity
   
   local destination = self._entity:add_component('destination')
   local render_region = self._entity:add_component('render_region')  
   local rgn = destination:get_region()
   render_region:set_pointer(rgn)
end

function Structure:_modify_region()
   return self._entity:get_component('destination'):get_region():modify()
end

function Structure:_get_region()
   return self._entity:get_component('destination'):get_region():get()
end

return Structure
