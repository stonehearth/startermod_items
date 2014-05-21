local FixtureFabricatorRenderer = class()

-- the FixtureFabricatorRenderer renders fixtures attached to structure blueprints.
-- The should only be visible in "hud mode", but otherwise should behave exactly
-- like other entities

-- Initializes a new renderer
--
function FixtureFabricatorRenderer:initialize(render_entity, fixture_fabricator)
   self._render_entity = render_entity

   radiant.events.listen(radiant, 'stonehearth:ui_mode_changed', self, self._update_ui_mode)
   self:_update_ui_mode()
end

-- Destroy the renderer
--
function FixtureFabricatorRenderer:destroy()
   radiant.events.unlisten(radiant, 'stonehearth:ui_mode_changed', self, self._update_ui_mode)
end

-- Called whenever the UI view mode changes.  Hides the entity if we're not in hud mode.
--
function FixtureFabricatorRenderer:_update_ui_mode()
   local mode = stonehearth.renderer:get_ui_mode()
   if self._ui_view_mode ~= mode then
      self._ui_view_mode = mode
      local visible = self._ui_view_mode == 'hud'
      self._render_entity:set_visible(visible)
   end
end

return FixtureFabricatorRenderer
