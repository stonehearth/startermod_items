local constants = require 'constants'
local mining_lib = require 'lib.mining.mining_lib'
local build_util = require 'lib.build_util'
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local Color4 = _radiant.csg.Color4
local Vertex = _radiant.csg.Vertex
local Mesh = _radiant.csg.Mesh
local log = radiant.log.create_logger('mining')

local MiningCallHandler = class()

function aligned_floor(value, align)
   return math.floor(value / align) * align
end

function aligned_ceil(value, align)
   return math.ceil(value / align) * align
end

function get_cell_min(value, cell_size)
   return aligned_floor(value, cell_size)
end

function get_cell_max(value, cell_size)
   return get_cell_min(value, cell_size) + cell_size-1
end

function MiningCallHandler:designate_mining_zone(session, response)
   local edge_color = Color4(255, 255, 0, 128)
   local face_color = Color4(255, 255, 0, 16)
   local xz_cell_size = constants.mining.XZ_CELL_SIZE
   local y_cell_size = constants.mining.Y_CELL_SIZE
   local clip_enabled = stonehearth.subterranean_view:clip_enabled()

   local get_proposed_points = function(p0, p1, normal)
      if not self:_valid_endpoints(p0, p1) then
         return nil, nil
      end
      
      local mode = self:_get_mode(p0, normal)
      local y_offset = self:_get_y_offset(mode)
      local y = get_cell_max(p0.y, y_cell_size) + y_offset
      local q0 = Point3(p0.x, y, p0.z)
      local q1 = Point3(p1.x, y, p1.z)

      -- Expand q0 and q1 so they span the the quantized region
      for _, d in ipairs({ 'x', 'z' }) do
         if q0[d] <= q1[d] then
            q0[d] = get_cell_min(q0[d], xz_cell_size)
            q1[d] = get_cell_max(q1[d], xz_cell_size)
         else
            q0[d] = get_cell_max(q0[d], xz_cell_size)
            q1[d] = get_cell_min(q1[d], xz_cell_size)
         end
      end
      log:spam('proposed point transform: %s, %s -> %s, %s', p0, p1, q0, q1)

      return q0, q1
   end

   local get_resolved_points = function(p0, p1, normal)
      if not self:_valid_endpoints(p0, p1) then
         return nil, nil
      end
      
      assert(p0.y == p1.y)
      local q0 = Point3(p0)
      local q1 = Point3(p1)

      -- Contract q1 to the largest quantized region that fits inside the validated region.
      -- q0's final location is the same as the proposed location, which must be valid
      -- or we wouldn't be asked to resolve.
      for _, d in ipairs({ 'x', 'z' }) do
         if q0[d] <= q1[d] then
            assert(q0[d] == get_cell_min(q0[d], xz_cell_size))
            q1[d] = aligned_floor(q1[d]+1, xz_cell_size) - 1
            if q1[d] < q0[d] then
               return nil, nil
            end
         else
            assert(q0[d] == get_cell_max(q0[d], xz_cell_size))
            q1[d] = aligned_ceil(q1[d], xz_cell_size)
            if q0[d] < q1[d] then
               return nil, nil
            end
         end
      end
      log:spam('resolved point transform: %s, %s -> %s, %s', p0, p1, q0, q1)

      return q0, q1
   end

   local terrain_support_filter = function(selected, selector)
      if selected.entity:get_component('terrain') then
         return true
      end
      -- otherwise, keep looking!
      return stonehearth.selection.FILTER_IGNORE
   end

   local contain_entity_filter = function(entity)
      -- allow mining zones to overlap when dragging out the region
      if entity:get_uri() == 'stonehearth:mining_zone_designation' then
         return stonehearth.selection.FILTER_IGNORE
      end

      -- reject other designations
      if radiant.entities.get_entity_data(entity, 'stonehearth:designation') then
         return false
      end

      -- reject entities under construction
      -- TODO: make this more general
      local ncz = entity:get_component('stonehearth:no_construction_zone')
      if ncz then
         return false
      end

      -- reject solid entities that are not terrain
      local rcs = entity:get_component('region_collision_shape')
      if rcs and rcs:get_region_collision_type() ~= _radiant.om.RegionCollisionShape.NONE then
         return false
      end

      return true
   end

   local draw_region_outline_marquee = function(selector, box)
      local mode = self:_infer_mode(box)
      local region = self:_get_dig_region(box, mode)
      local render_node = _radiant.client.create_region_outline_node(1, region, edge_color, face_color)
      return render_node
   end

   local select_cursor = function(box, normal)
      if not box then
         return 'stonehearth:cursors:invalid_hover'
      end

      return 'stonehearth:cursors:mine'
   end

   local ghost_ignored_entity_filter = function(entity)
      if not clip_enabled then
         return false
      end

      local collision_shape_component = entity:get_component('region_collision_shape')
      if not collision_shape_component then
         return false
      end
      
      local region = collision_shape_component:get_region():get()
      return not region:empty()
   end

   stonehearth.selection:select_xz_region()
      :select_front_brick(false)
      :set_validation_offset(Point3.unit_y)
      :set_cursor_fn(select_cursor)
      :set_end_point_transforms(get_proposed_points, get_resolved_points)
      :set_find_support_filter(terrain_support_filter)
      :set_can_contain_entity_filter(contain_entity_filter)
      :set_ghost_ignored_entity_filter(ghost_ignored_entity_filter)
      :use_manual_marquee(draw_region_outline_marquee)

      :done(function(selector, box)
            local mode = self:_infer_mode(box)
            local region = self:_get_dig_region(box, mode)
            _radiant.call('stonehearth:add_mining_zone', region, mode)
               :done(function(r)
                     response:resolve({ mining_zone = r.mining_zone })
                  end
               )
               :always(function()
                     selector:destroy()
                  end
               )
         end
      )

      :fail(function(selector)
            selector:destroy()
            response:reject('no region')
         end
      )

      :go()
end

function MiningCallHandler:_valid_endpoints(p0, p1)
   if not p0 or not p1 then
      return false
   end

   -- TODO: ask the world generation service for the y level of the world floor
   if p0.y <= 0 or p1.y <= 0 then
      return false
   end

   return true
end

function MiningCallHandler:_get_y_offset(mode)
   return mode == 'down' and 0 or -1
end

function MiningCallHandler:_get_mode(p0, normal)
   -- if the normal of the staring brick is horizontal, the mode is out
   if normal.y == 0 then
      return 'out'
   end

   -- if the top of the starting brick is aligned, then mode is down
   if (p0.y + 1) % constants.mining.Y_CELL_SIZE == 0 then
      return 'down'
   else
      return 'out'
   end
end

function MiningCallHandler:_infer_mode(box)
   if box.max.y % constants.mining.Y_CELL_SIZE == 0 then
      return 'down'
   else
      return 'out'
   end
end

function MiningCallHandler:_get_aligned_cube(cube)
   return mining_lib.get_aligned_cube(cube, constants.mining.XZ_CELL_SIZE, constants.mining.Y_CELL_SIZE)
end

function MiningCallHandler:_get_dig_region(selection_box, mode)
   local cube = nil

   if mode == 'down' then
      cube = self:_get_aligned_cube(selection_box)
   elseif mode == 'out' then
      cube = self:_get_aligned_cube(selection_box)
      cube.max.y = cube.max.y - 1
   end

   if cube then
      return Region3(cube)
   else
      return nil
   end
end

-- test code
function MiningCallHandler:add_mining_zone(session, response, region_table, mode)
   local mining_zone
   local region = Region3()
   region:load(region_table)
   
   if mode == 'down' then
      mining_zone = stonehearth.mining:dig_down(session.player_id, region)
   elseif mode == 'out' then
      mining_zone = stonehearth.mining:dig_out(session.player_id, region)
   elseif mode == 'up' then
      mining_zone = stonehearth.mining:dig_up(session.player_id, region)
   end

   return { mining_zone = mining_zone }
end

return MiningCallHandler
