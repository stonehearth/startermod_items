local Point3 = _radiant.csg.Point3

local GhostFormRenderer = class()

function GhostFormRenderer:initialize(render_entity, datastore)
   self._datastore = datastore
   self._parent_node = render_entity:get_node()
   self._ghost_item_rendered = nil

   self._promise = self._datastore:trace_data('rendering a ghost item')
                                 :on_changed(function()
                                       self:_update()
                                    end)
                                 :push_object_state()
end

function GhostFormRenderer:_update()
   local data = self._datastore:get_data()
   if data and data.full_sized_mod_url ~= '' then
      self._ghost_entity = radiant.entities.create_entity(data.full_sized_mod_url)
      self._ghost_entity:add_component('render_info')
                           :set_material('materials/ghost_item.xml')

      self._ghost_item_rendered = _radiant.client.create_render_entity(self._parent_node, self._ghost_entity)
   end
end

function GhostFormRenderer:destroy()
   --Note: when we finally get an effect in here, destroying the main entity will nuke the effect too
   if self._ghost_entity then
      _radiant.client.destroy_authoring_entity(self._ghost_entity:get_id())
   end
   if self._promise then
      self._promise:destroy()
      self._promise = nil
   end
end

return GhostFormRenderer