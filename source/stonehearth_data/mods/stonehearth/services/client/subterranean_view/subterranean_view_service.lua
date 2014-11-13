local constants = require 'constants'
local mining_lib = require 'lib.mining.mining_lib'
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local TraceCategories = _radiant.dm.TraceCategories
local log = radiant.log.create_logger('subterranean_view')

SubterraneanViewService = class()

local EPSILON = 0.000001
local MAX_CLIP_HEIGHT = 1000000000

local function is_terrain(location)
   return _physics:is_terrain(location)
end

function SubterraneanViewService:initialize()
   local enable_mining = radiant.util.get_config('enable_mining', false)
   if not enable_mining then
      return
   end

   self._sv = self.__saved_variables:get_data()

   if not self._sv.initialized then
      self._sv.xray_mode = ''
      self._sv.clip_enabled = false
      self._sv.clip_height = 25
      self._sv.initialized = true
   else
   end

   self._input_capture = stonehearth.input:capture_input()
   self._input_capture:on_keyboard_event(function(e)
         return self:_on_keyboard_event(e)
      end)

   self._initialize_listener = radiant.events.listen(radiant, 'stonehearth:gameloop', function()
         if _radiant.renderer.render_terrain_is_available() then
            self:_deferred_initialize()
            self:_destroy_initialize_listener()
         end
      end)
end

function SubterraneanViewService:_deferred_initialize()
   self._flat_xray_region_boxed = _radiant.client.alloc_region3()
   _radiant.renderer.set_flat_xray_region(self._flat_xray_region_boxed) -- TODO: set this on load
   self._full_xray_region_boxed = _radiant.client.alloc_region3()
   _radiant.renderer.set_full_xray_region(self._full_xray_region_boxed) -- TODO: set this on load
   self:_initialize_traces() -- TODO: destroy the traces
   self:_update()
end

function SubterraneanViewService:_initialize_traces()
   local root_entity = _radiant.client.get_object(1)
   local terrain_component = root_entity:add_component('terrain')
   
   self._interior_region_boxed = terrain_component:get_interior_region()

   self._interior_region_trace = terrain_component:trace_interior_region('subterranean view', TraceCategories.SYNC_TRACE)
      :on_changed(function()
            self:_on_interior_region_changed()
         end)
      :push_object_state()
end

function SubterraneanViewService:_on_interior_region_changed()
   self:_update_full_xray_view()
   self:_update_flat_xray_view()
end

function SubterraneanViewService:_get_visible_cube(interior_cube)
   local visible_cube = interior_cube:inflated(Point3(1, 0, 1))
   visible_cube.min.y = interior_cube.min.y - 1
   return visible_cube
end

function SubterraneanViewService:_update_full_xray_view()
   local interior_region = self._interior_region_boxed:get()

   self._full_xray_region_boxed:modify(function(cursor)
         cursor:clear()

         -- assumes interior_region is optimized
         for cube in interior_region:each_cube() do
            local visible_cube = self:_get_visible_cube(cube)
            cursor:add_cube(visible_cube)
         end

         local world_floor = self:_get_world_floor()
         cursor:add_cube(world_floor)

         cursor:optimize_by_merge()
      end)
end

function SubterraneanViewService:_update_flat_xray_view()
   local rpg_min_height = 4
   local down = -Point3.unit_y
   local interior_region = self._interior_region_boxed:get()

   local show_adjacent = function(point, check_height)
      if not is_terrain(point + down) then
         return false
      end

      local test_point = Point3(point)
      for y = check_height+1, point.y+rpg_min_height-1 do
         test_point.y = y
         if is_terrain(test_point) then
            return false
         end
      end

      return true
   end

   self._flat_xray_region_boxed:modify(function(cursor)
         cursor:clear()

         -- assumes interior_region is optimized
         for cube in interior_region:each_cube() do
            local check_height = cube.max.y - 1
            local bottom_face = mining_lib.get_face(cube, down)
            for point in bottom_face:each_point() do
               if show_adjacent(point, check_height) then
                  local visible_cube = self:_get_visible_cube(Cube3(point, point + Point3.one))
                  cursor:add_cube(visible_cube)
               end
            end
         end

         local world_floor = self:_get_world_floor()
         cursor:add_cube(world_floor)

         cursor:optimize_by_merge()
      end)
end

function SubterraneanViewService:_on_keyboard_event(e)
   if e.down then
      if e.key == _radiant.client.KeyboardInput.KEY_X then
         self:toggle_xray_mode('full')
         return true
      end
      if e.key == _radiant.client.KeyboardInput.KEY_Z then
         self:toggle_xray_mode('flat')
         return true
      end
   end
   return false
end

function SubterraneanViewService:toggle_xray_mode(mode)
   if self._sv.xray_mode == mode then
      self._sv.xray_mode = ''
   else
      self._sv.xray_mode = mode
   end
   self.__saved_variables:mark_changed()
   _radiant.renderer.set_xray_mode(self._sv.xray_mode)
end

function SubterraneanViewService:destroy()
   self:_destroy_initialize_listener()
   self._input_capture:destroy()
end

function SubterraneanViewService:_destroy_initialize_listener()
   if self._initialize_listener then
      self._initialize_listener:destroy()
      self._initialize_listener = nil
   end
end

function SubterraneanViewService:set_clip_enabled(enabled)
   self._sv.clip_enabled = enabled
   self.__saved_variables:mark_changed()
   self:_update()
end

function SubterraneanViewService:set_clip_height(height)
   self._sv.clip_height = height
   self.__saved_variables:mark_changed()
   self:_update()
end

function SubterraneanViewService:move_clip_height_up()
   self:set_clip_height(self._sv.clip_height + constants.mining.Y_CELL_SIZE);
end

function SubterraneanViewService:move_clip_height_down()
   self:set_clip_height(self._sv.clip_height - constants.mining.Y_CELL_SIZE);
end

function SubterraneanViewService:set_clip_enabled_command(sessions, response, enabled)
   self:set_clip_enabled(enabled)
   return {}
end

function SubterraneanViewService:move_clip_height_up_command(session, response)
   self:move_clip_height_up()
   return {}
end

function SubterraneanViewService:move_clip_height_down_command(session, response)
   self:move_clip_height_down()
   return {}
end

function SubterraneanViewService:_update()
   if self._sv.clip_enabled then
      -- tweak the clip_height for some feathering in the renderer
      local tweaked_clip_height = self._sv.clip_height * (1+10*EPSILON)
      -- -1 to remove the ceiling
      _radiant.renderer.set_clip_height(tweaked_clip_height-1)
      h3dSetVerticalClipMax(tweaked_clip_height)
   else
      _radiant.renderer.set_clip_height(MAX_CLIP_HEIGHT)
      h3dClearVerticalClipMax()
   end
end

function SubterraneanViewService:_get_world_floor()
   local world_floor = self:_get_terrain_bounds()
   world_floor.min.y = -1
   world_floor.max.y = 0
   return world_floor
end

function SubterraneanViewService:_get_terrain_bounds()
   local root_entity = _radiant.client.get_object(1)
   local terrain_component = root_entity:add_component('terrain')
   local bounds = terrain_component:get_bounds()
   return bounds
end

return SubterraneanViewService
