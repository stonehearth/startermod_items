local Color4 = _radiant.csg.Color4

local Renderer = class()
local log = radiant.log.create_logger('renderer')

function Renderer:initialize()
   self._sv = self.__saved_variables:get_data()
   self._ui_mode = 'normal'
   self._building_vision_mode = 'normal'
   self._installing_regions = false
   self._visible_region_installed = false
   self._explored_region_installed = false
   self._debug_shapes = {}
   self._debug_shape_traces = {}

   if self._sv.visible_region_uri then
      radiant.events.listen_once(radiant, 'radiant:game_loaded', self, self._install_regions)
   end

   if radiant.util.get_config('show_scaffolding_regions', false) then
      self._promise = _radiant.call_obj('stonehearth.build', 'get_scaffolding_manager_command')
                                 :done(function(o)
                                       local scaffolding_manager = o.scaffolding_manager
                                       self._sm_trace = scaffolding_manager:trace('debug sm')
                                          :on_changed(function()
                                                self:_update_sm_debug_shapes(scaffolding_manager:get_data())
                                             end)
                                          :push_object_state()
                                    end)
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

   self:_install_regions()
end

function Renderer:_install_regions()
   log:debug('installing regions')
   -- when starting a new game or loading, the uri's for the visible and explored regions
   -- may arrive slightly ahead of the actual regions themselves.  keep trying to 
   -- register them with the renderer, until it finally succeeds.
   if not self._installing_regions then
      self._installing_regions = true

      self._timer = radiant.set_realtime_interval(100, function()
            log:debug('attempting to install regions')
            self._installing_regions = true

            if not self._visible_region_installed then
               self._visible_region_installed = _radiant.renderer.visibility.set_visible_region(self._sv.visible_region_uri)
            end

            if not self._explored_region_installed then
               self._explored_region_installed = _radiant.renderer.visibility.set_explored_region(self._sv.explored_region_uri)
            end

            -- both registered!  time to die....
            if self._visible_region_installed and self._explored_region_installed then
               log:debug('suceeded installing regions')
               self._installing_regions = false
               self._timer:destroy()
               self._timer = nil
            end
         end)
   end
end

function Renderer:_update_region_debug_shape(id, origin, region, r, g, b)
   local edge_color = Color4(r, g, b, 255)
   local face_color = Color4(r, b, b, 128)

   local update_shape = function()
      if self._debug_shapes[id] then
         self._debug_shapes[id]:destroy()
      end
      local model = region:get():translated(origin)
      self._debug_shapes[id] = _radiant.client.create_region_outline_node(H3DRootNode, model, edge_color, face_color, 'materials/transparent.material.json')   
   end

   if self._debug_shape_traces[id] then
      self._debug_shape_traces[id]:destroy()
   end
   self._debug_shape_traces[id] = region:trace('drawing debug shape')
                                       :on_changed(update_shape)
                                       :push_object_state()
end

function Renderer:_update_sm_debug_shapes(smd)
   for rid, rblock in pairs(smd.regions) do
      local r = _radiant.csg.get_default_rng():get_int(1, 255)
      local g = _radiant.csg.get_default_rng():get_int(1, 255)
      local b = _radiant.csg.get_default_rng():get_int(1, 255)
      self:_update_region_debug_shape(rid, rblock.origin, rblock.region, r, g, b)
   end
   for sid, sblock in pairs(smd.scaffolding) do
      local r = _radiant.csg.get_default_rng():get_int(1, 255)
      local g = _radiant.csg.get_default_rng():get_int(1, 255)
      local b = _radiant.csg.get_default_rng():get_int(1, 255)
   end
   for id, shape in pairs(self._debug_shapes) do
      if not smd.regions[id] and not smd.scaffolding[id] then
         self._debug_shapes[id]:destroy()
         self._debug_shapes[id] = nil
         self._debug_shape_traces[id]:destroy()
         self._debug_shape_traces[id] = nil
      end
   end
end

return Renderer
