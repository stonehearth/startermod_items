local FixtureFabricatorRenderer = class()

-- the FixtureFabricatorRenderer renders fixtures attached to structure blueprints.
-- The should only be visible in "hud mode", but otherwise should behave exactly
-- like other entities

-- Initializes a new renderer
--
function FixtureFabricatorRenderer:initialize(render_entity, fixture_fabricator)
   self._render_entity = render_entity
   
   radiant.events.listen(radiant, 'stonehearth:ui_mode_changed', self, self._update_render_state)

   self._trace = fixture_fabricator:trace_data('render trace')
                     :on_changed(function()
                           self._visible = not fixture_fabricator:get_data().finished
                           self:_update_render_state()
                        end)
                     :push_object_state()
end

-- Destroy the renderer
--
function FixtureFabricatorRenderer:destroy()
   if self._trace then
      self._trace:destroy()
      self._trace = nil
   end
   radiant.events.unlisten(radiant, 'stonehearth:ui_mode_changed', self, self._update_render_state)
end

-- Called whenever the UI view mode changes.  Hides the entity if we're not in hud mode.
--
function FixtureFabricatorRenderer:_update_render_state()
   local visible = self._visible and stonehearth.renderer:get_ui_mode() == 'hud'
   self._render_entity:set_visible_override(visible)
end

return FixtureFabricatorRenderer
