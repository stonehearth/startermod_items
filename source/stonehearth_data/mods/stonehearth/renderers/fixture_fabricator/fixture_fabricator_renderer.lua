local ConstructionRenderTracker = require 'services.client.renderer.construction_render_tracker'
local FixtureFabricatorRenderer = class()

-- the FixtureFabricatorRenderer renders fixtures attached to structure blueprints.
-- The should only be visible in "hud mode", but otherwise should behave exactly
-- like other entities

-- Initializes a new renderer
--
function FixtureFabricatorRenderer:initialize(render_entity, fixture_fabricator)
   self._ui_mode_visible = false
   self._fixture_visible = false
   self._render_entity = render_entity
   
   self._render_tracker = ConstructionRenderTracker(render_entity:get_entity())
                           :set_visible_ui_modes('hud')
                           :set_visible_changed_cb(function(visible)
                                 self._ui_mode_visible = visible
                                 self:_update_render_state()
                              end)

   local entity = self._render_entity:get_entity()
   local parent = entity:get_component('mob'):get_parent()
   local cd = parent:get_component('stonehearth:construction_data')
   if cd then
      self._render_tracker:set_normal(cd:get_normal())
   end
   self._render_tracker:push_object_state()

   self._ff_trace = fixture_fabricator:trace_data('render trace')
                     :on_changed(function()
                           self._fixture_visible = not fixture_fabricator:get_data().finished
                           self:_update_render_state()
                        end)
                     :push_object_state()
end

-- Destroy the renderer
--
function FixtureFabricatorRenderer:destroy()
   if self._ff_trace then
      self._ff_trace:destroy()
      self._ff_trace = nil
   end
   if self._render_tracker then
      self._render_tracker:destroy()
      self._render_tracker = nil
   end
end

-- Called whenever the UI view mode changes.  Hides the entity if we're not in hud mode.
--
function FixtureFabricatorRenderer:_update_render_state()
   local visible = self._fixture_visible and self._ui_mode_visible
   self._render_entity:set_visible_override(visible)
end

return FixtureFabricatorRenderer
