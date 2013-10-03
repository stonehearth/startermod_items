local BlueprintRenderer = class()

function BlueprintRenderer:__init(render_entity)
   self._node = render_entity:get_node()
   self._create_fabricator()

   if self._fabricator then
      local rc = render_entity:get_entity():get_component('region_collision_shape')
      self._rgn = rc:get_region()
      self._rgn_promise = rc:trace_region('rendering blueprint')
                              :on_changed(function ()
                                 self._update_shape()
                              end)           

      self._update_shape()
   end
end

function BlueprintRenderer:_create_fabricator()
   if self._fabricator then
      self._fabricator:destroy()
      self._fabricator = nil
   end

   local data = radiant.entities.get_entity_data('stonehearth:construction')
   if data then
      self._fabricator = _radiant.sim.create_fabricator(data)
   end
end

function BlueprintRenderer:_update_shape()
   local cursor = self._rgn:get()
   if self._construction then
      self._construction:destroy()
      self._construction = nil
   end
   self._construction = self._fabricator:construct_render_object(self._node, cursor)
end

function BlueprintRenderer:destroy()
   if self._fabricator then
      self._fabricator:destroy()
      self._fabricator = nil
   end
   if self._construction then
      self._construction:destroy()
      self._construction = nil
   end
end

return BlueprintRenderer
