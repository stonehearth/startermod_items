local ConstructionRenderTracker = require 'services.client.renderer.construction_render_tracker'
local Point3 = _radiant.csg.Point3

local GhostItemRenderer = class()

function GhostItemRenderer:initialize(render_entity, datastore)
   self._entity = render_entity:get_entity()
   self._datastore = datastore
   self._parent_node = render_entity:get_node()
   self._ghost_item_rendered = nil

   self._promise = self._datastore:trace_data('rendering a ghost item')
                                 :on_changed(function()
                                       self:_update()
                                    end)
                                 :push_object_state()
end

function GhostItemRenderer:_update()
   local data = self._datastore:get_data()
   if data and data.full_sized_mod_url ~= '' then
      self._ghost_entity = radiant.entities.create_entity(data.full_sized_mod_url)
      self._ghost_entity:add_component('render_info')
                           :set_material('materials/ghost_item.xml')

      self._ghost_item_rendered = _radiant.client.create_render_entity(self._parent_node, self._ghost_entity)

      self._render_tracker = ConstructionRenderTracker(self._entity)
                                 :set_visible_ui_modes('hud')
                                 :set_visible_changed_cb(function(visible)
                                       self._ghost_item_rendered:set_visible_override(visible)
                                    end)                                 

   end
end

function GhostItemRenderer:destroy()
   --Note: when we finally get an effect in here, destroying the main entity will nuke the effect too
   if self._ghost_entity then
      _radiant.client.destroy_authoring_entity(self._ghost_entity:get_id())
   end
   if self._promise then
      self._promise:destroy()
      self._promise = nil
   end
   if self._render_tracker then
      self._render_tracker:destroy()
      self._render_tracker = nil
   end
end

return GhostItemRenderer