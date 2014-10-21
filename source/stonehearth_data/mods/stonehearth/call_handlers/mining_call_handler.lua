local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local Color4 = _radiant.csg.Color4
local Vertex = _radiant.csg.Vertex
local Mesh = _radiant.csg.Mesh
local log = radiant.log.create_logger('mining')

local MiningCallHandler = class()

-- TODO: move these constants to constants where they can be accessed from both client and server
local XZ_ALIGN = 4
local Y_ALIGN = 5

-- test code
function MiningCallHandler:designate_mining_zone(session, response)
   local enable_mining = radiant.util.get_config('enable_mining', false)
   if not enable_mining then
      response:reject('disabled')
      return
   end

   local aligned_floor = function(value)
      return math.floor(value / XZ_ALIGN) * XZ_ALIGN
   end

   local aligned_ceil = function(value)
      return math.ceil(value / XZ_ALIGN) * XZ_ALIGN
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
            q1[d] = aligned_floor(q1[d]) + XZ_ALIGN-1
         else
            q0[d] = aligned_floor(q0[d]) + XZ_ALIGN-1
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
            q0[d] = aligned_floor(q0[d]) + XZ_ALIGN-1
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
            -- test code
            local ground_box = box:translated(Point3(0, -1, 0))
            ground_box.min.y = ground_box.max.y - Y_ALIGN
            local region = Region3(ground_box)
            local render_node = _radiant.client.create_region_outline_node(1, region, Color4(255, 255, 0, 0))
            return render_node
         end)
      :done(function(selector, box)
            -- test code
            local ground_box = box:translated(Point3(0, -1, 0))
            local region = Region3(ground_box)

            _radiant.call('stonehearth:add_mining_zone', region)
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

-- test code
function MiningCallHandler:add_mining_zone(session, response, region_table)
   local new_region = Region3()
   new_region:load(region_table)
   local inflated_region = new_region:inflated(Point3(1, 1, 1))

   -- using town as a proxy for the eventual player object
   local town = stonehearth.town:get_town(session.player_id)
   local mining_zones = town:get_mining_zones()
   local mergable_zones = {}

   -- find all the existing mining zones that are adjacent or overlap the new region
   for _, zone in pairs(mining_zones) do
      -- move the inflated region to the local space of the zone
      local location = radiant.entities.get_world_grid_location(zone)
      local translated_inflated_region = inflated_region:translated(-location)

      local mining_zone_component = zone:add_component('stonehearth:mining_zone')
      local existing_region = mining_zone_component:get_region():get()

      if existing_region:intersects(translated_inflated_region) then
         mergable_zones[zone:get_id()] = zone
      end
   end

   local _, selected_zone = next(mergable_zones)

   if selected_zone then
      -- remove the surviving zone from the merge list
      mergable_zones[selected_zone:get_id()] = nil
   else
      -- no adjacent or overlapping zone exists, so create a new one
      selected_zone = stonehearth.mining:create_mining_zone(session.player_id, session.faction)
      town:add_mining_zone(selected_zone)
   end

   stonehearth.mining:dig_down(selected_zone, new_region)

   -- merge the other zones into the surviving zone and destroy them
   for _, zone in pairs(mergable_zones) do
      stonehearth.mining:merge_zones(selected_zone, zone)
   end

   return { mining_zone = selected_zone }
end

return MiningCallHandler
