local constants = require 'constants'
local csg_lib = require 'lib.csg.csg_lib'
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local TraceCategories = _radiant.dm.TraceCategories
local log = radiant.log.create_logger('subterranean_view')

SubterraneanViewService = class()

local EPSILON = 0.000001
local MAX_CLIP_HEIGHT = 1000000000

local function aligned_floor(value, align)
   return math.floor(value / align) * align
end

local function aligned_ceil(value, align)
   return math.ceil(value / align) * align
end

local function is_terrain(location)
   return _physics:is_terrain(location)
end

local function point_to_key(point)
   -- don't assume tostring(point) will return the same string for point3f and point3i
   local key = string.format('%d,%d,%d', point.x, point.y, point.z)
   return key
end

function SubterraneanViewService:initialize()
   self._finished_initialization = false
   self._sv = self.__saved_variables:get_data()

   if not self._sv.initialized then
      self._sv.xray_mode = nil
      self._sv.clip_enabled = false
      self._sv.clip_height = 24
      self._sv.clip_height_initialized = false
      self._sv.initialized = true
   else
   end

   self._interior_region_traces = {}
   self._interior_region_tiles = {}
   self._dirty_xray_tiles = {}
   self._visible_volume_dirty = true

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

function SubterraneanViewService:get_clip_enabled()
   return self._sv.clip_enabled
end

function SubterraneanViewService:set_clip_enabled(enabled)
   if enabled ~= self._sv.clip_enabled then
      self._sv.clip_enabled = enabled
      self.__saved_variables:mark_changed()
      self:_update_clip_height()
      self:_update_all_entities_visibility()
      self._visible_volume_dirty = true
   end
end

function SubterraneanViewService:set_clip_height(height)
   if height ~= self._sv.clip_height then
      self._sv.clip_height = height
      self.__saved_variables:mark_changed()
      self:_update_clip_height()
      self:_update_all_entities_visibility()
      self._visible_volume_dirty = true
   end
end

function SubterraneanViewService:move_clip_height_up()
   self:set_clip_height(self._sv.clip_height + constants.mining.Y_CELL_SIZE);
end

function SubterraneanViewService:move_clip_height_down()
   self:set_clip_height(self._sv.clip_height - constants.mining.Y_CELL_SIZE);
end

function SubterraneanViewService:initialize_clip_height(height)
   self:set_clip_height(height)
   self._sv.clip_height_initialized = true
end

function SubterraneanViewService:clip_height_initialized()
   return self._sv.clip_height_initialized
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

function SubterraneanViewService:toggle_xray_mode(mode)
   if self._sv.xray_mode == mode then
      self._sv.xray_mode = nil
   else
      self._sv.xray_mode = mode
      self:_mark_all_dirty()
   end
   self.__saved_variables:mark_changed()

   self:_update_xray_mode()
   self:_update_all_entities_visibility()
   self:_update_dirty_tiles()
   self._visible_volume_dirty = true
end

function SubterraneanViewService:toggle_xray_mode_command(sessions, response, mode)
   self:toggle_xray_mode(mode)
   return {}
end

function SubterraneanViewService:intersect_region_with_visible_volume(region)
   local clipped_region

   if self._sv.clip_enabled then
      local bounds = region:get_bounds()

      if bounds.min.y >= self._sv.clip_height then
         -- everything clipped
         return Region3()
      end

      if bounds.max.y <= self._sv.clip_height then
         -- nothing clipped
         clipped_region = Region3(region)
      else
         -- retain the region below clip_height
         bounds.max.y = self._sv.clip_height
         clipped_region = region:intersect_cube(bounds)
      end
   else
      clipped_region = Region3(region)
   end

   if self._sv.xray_mode then
      clipped_region = self._xray_tiles:intersect_region(clipped_region)
   end

   return clipped_region
end

-- Remember that the terrain tiles most likely have not been added at this point
function SubterraneanViewService:_deferred_initialize()
   self._interior_tiles = self:_get_terrain_component():get_interior_tiles()
   self._xray_tiles = _radiant.renderer.get_xray_tiles()
   self._tile_size = self._interior_tiles:get_tile_size()

   self:_create_render_frame_trace()
   self:_create_interior_region_traces()
   self:_create_entity_traces()

   self._finished_initialization = true

   self:_update_xray_mode()
   self:_update_clip_height()
   self:_update_all_entities_visibility()
end

function SubterraneanViewService:_destroy_initialize_listener()
   if self._initialize_listener then
      self._initialize_listener:destroy()
      self._initialize_listener = nil
   end
end

function SubterraneanViewService:_create_render_frame_trace()
   self._render_frame_trace = _radiant.client.trace_render_frame()
      :on_frame_start('subterranean view', function()
            self:_update_dirty_tiles()
            self:_notify_if_visible_volume_changed()
         end)
end

function SubterraneanViewService:_destroy_render_frame_trace()
   if self._render_frame_trace then
      self._render_frame_trace:destroy()
      self._render_frame_trace = nil
   end
end

function SubterraneanViewService:_create_interior_region_traces()
   local terrain_component = self:_get_terrain_component()

   self._interior_region_map_trace = terrain_component:trace_interior_tiles('subterranean view', TraceCategories.SYNC_TRACE)
      :on_added(function(index, region3i_boxed)
            local trace = region3i_boxed:trace('subterranean view', TraceCategories.SYNC_TRACE)
               :on_changed(function()
                     self:_mark_dirty(index:to_float())
                  end)
            local key = point_to_key(index)
            self._interior_region_traces[key] = trace
            self._interior_region_tiles[key] = index:to_float()
         end)
      :on_removed(function(index)
            local key = point_to_key(index)
            self._interior_region_traces[key] = nil
            self._interior_region_tiles[key] = nil
            self._dirty_xray_tiles[key] = nil
         end)
      :push_object_state()
end

function SubterraneanViewService:_destroy_interior_region_traces()
   if self._interior_region_map_trace then
      self._interior_region_map_trace:destroy()
      self._interior_region_map_trace = nil
   end

   for key, trace in pairs(self._interior_region_traces) do
      trace:destroy()
   end
   self._interior_region_traces = {}
end

function SubterraneanViewService:_create_entity_traces()
   local entity_container = self:_get_root_entity_container()

   self._entity_traces = {}
   self._entity_visibility_handles = {}

   self._entity_container_trace = entity_container:trace_children('subterranean view')
      :on_added(function(id, entity)
            if self._entity_traces[id] then
               -- trace already exists
               return
            end

            local location_trace = radiant.entities.trace_grid_location(entity, 'subterranean view')
               :on_changed(function()
                     if self._sv.xray_mode or self._sv.clip_enabled then
                        self:_update_visiblity(entity)
                     end
                  end)
               :push_object_state()

            local parent_trace = entity:add_component('mob'):trace_parent('subterranean view')
               :on_changed(function()
                     if self._sv.xray_mode or self._sv.clip_enabled then
                        self:_update_visiblity(entity)
                     end
                  end)

            local destroyed_trace = radiant.events.listen(radiant, 'radiant:entity:post_destroy', function(e)
                  self:_destroy_entity_traces(e.entity_id)
               end)

            self._entity_traces[id] = {
               location = location_trace,
               parent = parent_trace,
               destroyed = destroyed_trace
            }
         end)
      :push_object_state()
end

function SubterraneanViewService:_destroy_all_entity_traces()
   if self._entity_container_trace then
      self._entity_container_trace:destroy()
      self._entity_container_trace = nil
   end

   for id, traces in pairs(self._entity_traces) do
      self:_destroy_entity_traces(id)
   end
end

function SubterraneanViewService:_destroy_entity_traces(id)
   local traces = self._entity_traces[id]
   if traces then
      for name, trace in pairs(traces) do
         trace:destroy()
      end
      self._entity_traces[id] = nil
   end

   local visibility_handle = self._entity_visibility_handles[id]
   if visibility_handle then
      visibility_handle:destroy()
      self._entity_visibility_handles[id] = nil
   end
end

function SubterraneanViewService:destroy()
   self:_destroy_render_frame_trace()
   self:_destroy_interior_region_traces()
   self:_destroy_all_entity_traces()
   self:_destroy_initialize_listener()
   self._input_capture:destroy()
end

-- index is the index of the dirty interior tile
function SubterraneanViewService:_mark_dirty(index)
   -- create a 3x3x3 cube centered about index
   local cube = Cube3(index, index + Point3.one):inflated(Point3.one)

   -- dirty all 3 space adjacents including diagonals
   for point in cube:each_point() do
      self._dirty_xray_tiles[point_to_key(point)] = point
   end
end

function SubterraneanViewService:_mark_all_dirty()
   for _, index in pairs(self._interior_region_tiles) do
      self:_mark_dirty(index)
   end
end

function SubterraneanViewService:_update_dirty_tiles()
   local xray_mode = self._sv.xray_mode

   if not self._finished_initialization or
      xray_mode == nil or xray_mode == '' then
      return
   end

   if not next(self._dirty_xray_tiles) then
      return
   end

   self._visible_volume_dirty = true

   for _, index in pairs(self._dirty_xray_tiles) do
      self._xray_tiles:clear_tile(index)
   end

   -- TODO: don't change all these tiles when not necessary
   local world_floor = self:_get_world_floor()
   self._xray_tiles:add_cube(world_floor)
   self._xray_tiles:clear_changed_set()

   for _, index in pairs(self._dirty_xray_tiles) do
      local interior_region3i_boxed = self._interior_tiles:find_tile(index)

      if interior_region3i_boxed then
         local interior_region3i = interior_region3i_boxed:get()

         if xray_mode == 'full' then
            self:_update_full_xray_view(interior_region3i)
         elseif xray_mode == 'flat' then
            self:_update_flat_xray_view(interior_region3i)
         else
            log:error('unknown xray_mode: %s', xray_mode)
            assert(false)
         end

         -- restore any portion of the world floor that was cleared
         -- local bounds = self:_get_tile_bounds(index)
         -- local clipped_floor = world_floor:intersected(bounds)
         -- self._xray_tiles:add_cube(clipped_floor)
      end
   end

   for index in self._xray_tiles:each_changed_index() do
      _radiant.renderer.mark_dirty_index(index:to_float())
   end

   self._xray_tiles:optimize_changed_tiles('optimizing xray region')
   self._xray_tiles:clear_changed_set()
   self._dirty_xray_tiles = {}
end

function SubterraneanViewService:_get_tile_bounds(index)
   local min = Point3(
      index.x * self._tile_size.x,
      index.y * self._tile_size.y,
      index.z * self._tile_size.z
   )
   local max = min + self._tile_size
   local bounds = Cube3(min, max)
   return bounds
end

function SubterraneanViewService:_get_visible_cube(interior_cube)
   local visible_cube = interior_cube:inflated(Point3(1, 0, 1))
   visible_cube.min.y = interior_cube.min.y - 1
   return visible_cube
end

function SubterraneanViewService:_update_full_xray_view(interior_region3i)
   local region = Region3()

   -- assumes interior_region is optimized
   for cube3i in interior_region3i:each_cube() do
      local cube = cube3i:to_float()
      local visible_cube = self:_get_visible_cube(cube)
      region:add_cube(visible_cube)
   end

   self._xray_tiles:add_region(region)
end

function SubterraneanViewService:_update_flat_xray_view(interior_region3i)
   local region = Region3()
   local rpg_min_height = 4
   local down = -Point3.unit_y

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

   -- assumes interior_region is optimized
   for cube3i in interior_region3i:each_cube() do
      local cube = cube3i:to_float()
      local check_height = cube.max.y - 1
      local bottom_face = csg_lib.get_face(cube, down)
      for point in bottom_face:each_point() do
         if show_adjacent(point, check_height) then
            local visible_cube = self:_get_visible_cube(Cube3(point, point + Point3.one))
            region:add_cube(visible_cube)
         end
      end
   end

   self._xray_tiles:add_region(region)
end

function SubterraneanViewService:_on_keyboard_event(e)
   if e.down then
      -- if e.key == _radiant.client.KeyboardInput.KEY_X then
      --    self:toggle_xray_mode('full')
      --    return true
      -- end
      -- if e.key == _radiant.client.KeyboardInput.KEY_Z then
      --    self:toggle_xray_mode('flat')
      --    return true
      -- end
   end
   return false
end

function SubterraneanViewService:_update_all_entities_visibility()
   local entity_container = self:_get_root_entity_container()

   if self._sv.xray_mode or self._sv.clip_enabled then
      self:_each_contained_entity(entity_container, function(child)
            self:_update_visiblity(child)
         end)
   else
      self:_each_contained_entity(entity_container, function(child)
            self:_update_visiblity(child, true)
         end)
   end
end

function SubterraneanViewService:_update_visiblity(entity, visible)
   if not entity:is_valid() then
      return
   end

   local custom_clip = radiant.entities.get_entity_data(entity, 'stonehearth:hud:custom_clip', true)
   if custom_clip == true then
      return
   end

   local id = entity:get_id()

   if visible == nil then
      visible = self:_is_visible(entity)
   end

   self:_set_entity_visible(entity, visible)
end

function SubterraneanViewService:_is_visible(entity)
   local parent = entity:add_component('mob'):get_parent()
   if not parent then
      -- not in the world, just default to true
      return true
   end

   if not parent:get_component('terrain') then
      -- not a child of the terrain, let our ancestors decide visibility
      return true
   end

   -- our parent is the terrain entity, test to see if we're on a visible block
   local visible = true

   if self._sv.xray_mode then
      visible = self:_is_xray_visible(entity)
   end

   if visible and self._sv.clip_enabled then
      visible = self:_is_clip_mode_visible(entity)
   end

   return visible
end

function SubterraneanViewService:_is_xray_visible(entity)
   local location = radiant.entities.get_world_grid_location(entity)
   local visible = location and self._interior_tiles:contains_point(location)
   return visible
end

function SubterraneanViewService:_is_clip_mode_visible(entity)
   local location = radiant.entities.get_world_grid_location(entity)
   local visible = location and location.y <= self._sv.clip_height
   return visible
end

function SubterraneanViewService:_set_entity_visible(entity, visible)
   local visibility_handle = self:_get_visibility_handle(entity)

   if visibility_handle then
      visibility_handle:set_visible(visible)
   end
end

function SubterraneanViewService:_get_visibility_handle(entity)
   local id = entity:get_id()
   local visibility_handle = self._entity_visibility_handles[id]

   if not visibility_handle then
      local render_entity = _radiant.client.get_render_entity(entity)
      if render_entity then
         visibility_handle = render_entity:get_visibility_override_handle()
         self._entity_visibility_handles[id] = visibility_handle
      end
   end

   return visibility_handle
end

function SubterraneanViewService:_update_xray_mode()
   -- explicitly check against nil to coerce to boolean type
   local enabled = self._sv.xray_mode ~= nil and self._sv.xray_mode ~= ''
   _radiant.renderer.enable_xray_mode(enabled)
end

function SubterraneanViewService:_update_clip_height()
   if self._sv.clip_enabled then
      _radiant.renderer.set_clip_height(self._sv.clip_height)
   else
      _radiant.renderer.set_clip_height(MAX_CLIP_HEIGHT)
   end
end

function SubterraneanViewService:_get_world_floor()
   local world_floor = self:_get_terrain_bounds()
   world_floor.min.y = -2
   world_floor.max.y = 0
   return world_floor
end

function SubterraneanViewService:_get_terrain_bounds()
   local bounds = self:_get_terrain_component():get_bounds()
   return bounds
end

function SubterraneanViewService:_get_terrain_component()
   local root_entity = _radiant.client.get_object(1)
   return root_entity:add_component('terrain')
end

function SubterraneanViewService:_get_root_entity_container()
   local root_entity = _radiant.client.get_object(1)
   return root_entity:add_component('entity_container')
end

-- visits all the children and attached items
function SubterraneanViewService:_each_contained_entity(entity_container, cb)
   if not entity_container then
      return
   end

   for id, entity in entity_container:each_child() do
      cb(entity)
   end

   for id, entity in entity_container:each_attached_item() do
      cb(entity)
   end
end

function SubterraneanViewService:_notify_if_visible_volume_changed()
   if self._visible_volume_dirty then
      radiant.events.trigger_async(self, 'stonehearth:visible_volume_changed')
      self._visible_volume_dirty = false
   end
end

return SubterraneanViewService
