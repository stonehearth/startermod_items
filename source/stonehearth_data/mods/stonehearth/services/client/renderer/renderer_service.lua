local Renderer = class()

function Renderer:initialize()
   self._sv = self.__saved_variables:get_data()
   self._building_vision_mode = 'normal'
   
   if self._sv.visible_region_uri then
      radiant.events.listen_once(radiant, 'radiant:game_loaded', self, self._install_regions)
   end
end

function Renderer:get_ui_mode()
   return self._ui_mode
end

function Renderer:get_building_vision_mode()
   return self._building_vision_mode
end

function Renderer:set_ui_mode(ui_mode)
   if self._ui_mode ~= ui_mode then
      self._ui_mode = ui_mode
      if ui_mode == 'normal' then
         h3dSetGlobalShaderFlag("DRAW_GRIDLINES", false)
      elseif ui_mode == 'hud' then
         h3dSetGlobalShaderFlag("DRAW_GRIDLINES", true)
      end
      radiant.events.trigger_async(radiant, 'stonehearth:ui_mode_changed')
   end
end

function Renderer:set_building_vision_mode(mode)
   if self._building_vision_mode ~= mode then
      self._building_vision_mode = mode
      radiant.events.trigger_async(radiant, 'stonehearth:building_vision_mode_changed')
   end
end

function Renderer:set_visibility_regions(visible_region_uri, explored_region_uri)
   self._sv.visible_region_uri = visible_region_uri
   self._sv.explored_region_uri = explored_region_uri

   self._visible_region_installed = false
   self._explored_region_installed = false
   self:_install_regions()
end

function Renderer:_install_regions()
   -- when starting a new game or loading, the uri's for the visible and explored regions
   -- may arrive slightly ahead of the actual regions themselves.  keep trying to 
   -- register them with the renderer, until it finally succeeds.
   if not self._installing_regions then
      self._installing_regions = true
      self._timer = radiant.set_realtime_interval(100, function()
            self._installing_regions = true
            if not self._visible_region_installed then
               self._visible_region_installed = _radiant.renderer.visibility.set_visible_region(self._sv.visible_region_uri)
            end
            if not self._explored_region_installed then
               self._explored_region_installed = _radiant.renderer.visibility.set_explored_region(self._sv.explored_region_uri)
            end

            -- both registered!  time to die....
            if self._visible_region_installed and self._explored_region_installed then
               self._installing_regions = false
               self._timer:destroy()
               self._timer = nil
            end
         end)
   end
end

return Renderer
