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

   local xz_align = constants.mining.XZ_ALIGN

   local aligned_floor = function(value)
      return math.floor(value / xz_align) * xz_align
   end

   local aligned_ceil = function(value)
      return math.ceil(value / xz_align) * xz_align
   end

   local get_proposed_points = function(p0, p1)
      if not p0 or not p1 then
         return nil, nil
      end
      
      local q0 = Point3(p0.x, p0.y, p0.z)
      local q1 = Point3(p1.x, p1.y, p1.z)

      -- expand q0 and q1 so they span the the quantized region
      for _, d in ipairs({ 'x', 'z' }) do
         if q0[d] <= q1[d] then
            q0[d] = aligned_floor(q0[d])
            q1[d] = aligned_floor(q1[d]) + xz_align-1
         else
            q0[d] = aligned_floor(q0[d]) + xz_align-1
            q1[d] = aligned_floor(q1[d])
         end
      end
      log:spam('proposed point transform: %s, %s -> %s, %s', p0, p1, q0, q1)

      return q0, q1
   end

   local get_resolved_points = function(p0, p1)
      if not p0 or not p1 then
         return nil, nil
      end
      
      local q0 = Point3(p0.x, p0.y, p0.z)
      local q1 = Point3(p1.x, p1.y, p1.z)

      -- contract q0 and q1 to the largest quantized region that fits inside the validated region
      -- this code is ugly. find a clearer way to do this
      for _, d in ipairs({ 'x', 'z' }) do
         if q0[d] <= q1[d] then
            q0[d] = aligned_floor(q0[d])
            q1[d] = aligned_floor(q1[d]+1) - 1
            if q1[d] < q0[d] then
               return nil, nil
            end
         else
            q0[d] = aligned_floor(q0[d]) + xz_align-1
            q1[d] = aligned_ceil(q1[d])
            if q0[d] < q1[d] then
               return nil, nil
            end
         end
      end
      log:spam('resolved point transform: %s, %s -> %s, %s', p0, p1, q0, q1)

      return q0, q1
   end

   stonehearth.selection:select_xz_region()
      :set_end_point_transforms(get_proposed_points, get_resolved_points)
      :set_can_contain_entity_filter(function(entity)
            -- TODO
            return true
         end)
      :set_cursor('stonehearth:cursors:harvest')
      :use_manual_marquee(function(selector, box)
            local region = self:_get_dig_region(box, mode)
            local render_node = _radiant.client.create_region_outline_node(1, region, Color4(255, 255, 0, 0))
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
      local ground_box = selection_box:translated(-Point3.unit_y)
      cube = self:_get_aligned_cube(ground_box)
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
   return mining_lib.get_aligned_cube(cube, constants.mining.XZ_ALIGN, constants.mining.Y_ALIGN)
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
