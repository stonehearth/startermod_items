local constants = require 'constants'
local mining_lib = require 'lib.mining.mining_lib'
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local Color4 = _radiant.csg.Color4
local Vertex = _radiant.csg.Vertex
local Mesh = _radiant.csg.Mesh
local log = radiant.log.create_logger('mining')

local MiningCallHandler = class()

function MiningCallHandler:designate_mining_zone(session, response, mode)
   local enable_mining = radiant.util.get_config('enable_mining', false)
   if not enable_mining then
      response:reject('disabled')
      return
   end

   local edge_color = Color4(255, 255, 0, 128)
   local face_color = Color4(255, 255, 0, 16)
   local xz_cell_size = constants.mining.XZ_CELL_SIZE
   local y_cell_size = constants.mining.Y_CELL_SIZE

   local aligned_floor = function(value, align)
      return math.floor(value / align) * align
   end

   local aligned_ceil = function(value, align)
      return math.ceil(value / align) * align
   end

   local get_cell_min = aligned_floor

   local get_cell_max = function(value, cell_size)
      return get_cell_min(value, cell_size) + cell_size-1
   end

   local get_proposed_points = function(p0, p1)
      if not p0 or not p1 then
         return nil, nil
      end
      
      -- Set the y level to the top of the zone
      local y_offset = mode == 'out' and -1 or 0
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

   local get_resolved_points = function(p0, p1)
      if not p0 or not p1 then
         return nil, nil
      end
      
      assert(p0.y == p1.y)
      local q0 = Point3(p0.x, p0.y, p0.z)
      local q1 = Point3(p1.x, p1.y, p1.z)

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

   stonehearth.selection:select_xz_region()
      :set_cursor('stonehearth:cursors:harvest')
      :set_end_point_transforms(get_proposed_points, get_resolved_points)
      :select_front_brick(false)
      :set_find_support_filter(function(selected, selector)
            -- fast check for 'is terrain'
            if selected.entity:get_id() == 1 then
               return true
            end
            -- otherwise, keep looking!
            return stonehearth.selection.FILTER_IGNORE
         end)
      :set_can_contain_entity_filter(function(entity)
            -- TODO
            return true
         end)
      :use_manual_marquee(function(selector, box)
            local region = self:_get_dig_region(box, mode)
            local render_node = _radiant.client.create_region_outline_node(1, region, edge_color, face_color)
            return render_node
         end)
      :done(function(selector, box)
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

function MiningCallHandler:_get_aligned_cube(cube)
   return mining_lib.get_aligned_cube(cube, constants.mining.XZ_CELL_SIZE, constants.mining.Y_CELL_SIZE)
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

function MiningCallHandler:get_mining_zone_enabled(session, response, mining_zone)
   local mining_zone_component = mining_zone:add_component('stonehearth:mining_zone')
   local enabled = mining_zone_component:get_mining_zone_enabled()
   return { enabled = enabled }
end

function MiningCallHandler:set_mining_zone_enabled(session, response, mining_zone, enabled)
   local mining_zone_component = mining_zone:add_component('stonehearth:mining_zone')
   mining_zone_component:set_mining_zone_enabled(enabled)
   return {}   
end

return MiningCallHandler
