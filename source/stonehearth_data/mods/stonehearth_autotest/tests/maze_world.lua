local Point3  = _radiant.csg.Point3
local Cube3   = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

local MAZE = 
[[
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
|                       |                   |                       |           |
|  S                 S  |                 S |  S                    |           |
|                       |                   |                       |           |
+---+   +---+   +---+---+   +---+   +---+---+   +---+   +---+---+   +   +---+   +
|       |   |   |           |   |           |       |   |   |       |       |   |
|       | S |   |           |   |           |       |   |   |       |    E  |   |
|       |   |   |           |   |           |       |   |   |       |       |   |
+   +---+   +   +   +---+---+   +---+---+   +   +---+   +   +   +---+---+---+   +
|       |       |   |                   |       |       |       |               |
|       |       |   |                   |       |       |       |               |
|       |       |   |                   |       |       |       |               |
+   +   +   +---+   +---+   +   +---+   +---+---+   +---+---+   +---+   +---+   +
|   |   |   |   |       |   |   |       |       |   |       |           |       |
| S |   |   |  S|       | S |   |       |       |   |       |           |       |
|   |   |   |   |       |   |   |       |       |   |       |           |       |
+---+   +   +   +---+   +---+   +   +---+---+   +   +   +   +---+---+---+   +---+
|       |       |       |       |               |       |   |       |       | S |
|       |       |       |       |               |  S    |   |       |       |   |
|       |       |       |       |               |       |   |       |       |   |
+   +   +---+---+   +---+   +---+---+---+---+   +---+---+   +---+   +   +---+   +
|   |   |       |   |           |           |   |       |       |   |   |       |
|   |   |       |   |           |           |   |       |       |   |   |       |
|   | S |       |   |           |           |   |       |       |   |   |       |
+   +---+   +   +   +---+---+   +---+   +   +   +   +   +---+   +   +   +   +   +
|   |       |       |       |       |   |   |   |   |       |   |   |       |   |
|   |       |       |       |  S    |   |   |   |   |       |   |   |       |   |
|   |       |       |       |       |   |   |   |   |       |   |   |       |   |
+   +   +---+---+---+   +   +---+   +   +   +   +---+---+   +   +   +---+---+   +
|   |   |               |           |   |   |               |   |               |
|   |   |               |           |   |   |               |   |               |
|   |   |               |           |   |   |               |   |               |
+   +   +---+---+---+   +---+---+---+   +   +---+---+---+   +   +---+---+   +---+
|       |               |   |           |           |       |   |       |       |
|       |               | S |           |           |       |   |       |       |
|       |               |   |           |           |       |   |       |       |
+   +---+   +---+---+---+   +   +---+---+---+---+   +   +---+   +   +   +---+   +
|   |       |           |   |   |       |       |   |   |   |       |   |       |
|   |       |           |   |   |       |       |   |   |   |       |   |       |
|   |       |           |   |   |       |       |   |   |   |       |   |       |
+   +   +   +   +---+   +   +   +   +   +   +   +   +   +   +---+---+   +   +---+
|       |   |       |   |           |   |   |   |   |   |           |   |       |
|       |   |       |   |           |   |   |   |   |   |           |   |       |
|       |   |       |   |           |   |   |   |   |   |           |   |       |
+   +---+---+---+   +   +---+---+---+   +   +   +   +   +   +---+   +   +---+   +
|   |               |           |   |       |   |           |   |   |       |   |
|   |               |           |   |       |   |     S     |   |   |       |   |
| S |               |           |   |       |   |           |   |   |       |   |
+---+   +---+---+---+---+---+   +   +---+   +---+---+---+---+   +   +---+   +   +
|   |   |       |           |   |       |   |           |       |       |   |   |
| S |   |     S |           |   |       |   |           |       |       |   |   |
|   |   |       |           |   |       |   |           |       |       |   |   |
+   +   +   +---+   +---+   +   +---+   +   +   +---+   +---+   +---+   +   +   +
|   |   |           |       |       |       |   |           |           |   |   |
|   |   |           |       |       |       |   |           |           |   |   |
|   |   |           |       |       |       |   |           |           |   | S |
+   +   +   +---+---+   +---+---+   +   +---+   +---+---+   +---+---+   +   +---+
|   |   |   |   |       |           |       |       |   |           |   |       |
|   |   |   | S |       |           |       |       | S |           |   |       |
|   |   |   |   |       |           |       |       |   |           |   |       |
+   +   +   +   +   +---+   +---+---+---+---+---+   +   +---+---+   +   +---+   +
|       |   |   |           |                       |               |       |   |
|       |   |   |           |                       |               |       |   |
|       |   |   |           |                       |               |       |   |
+---+---+   +   +---+---+---+---+   +   +---+---+---+   +---+---+---+---+   +   +
|       |       |               |   |       |       |           |           |   |
|       |       | S             |   |       |       |           |           |   |
|       |       |               |   |       |       |           |           |   |
+   +   +---+   +---+---+   +   +   +---+   +   +   +---+---+   +---+---+---+   +
|   |       |       |       |   |       |   |   |   |       |           |       |
|   |     S |       |       |   |     S |   | S |   |       |           |       |
|   |       |       |       |   |       |   |   |   |       |           |       |
+   +   +---+---+   +   +---+   +   +---+   +---+   +   +   +---+---+   +   +---+
|   |           |   |   |       |   |       |       |   |           |   |       |
|   |           |   |   |       | S |       |       |   |           |   |       |
|   |           |   |   |       |   |       |       |   |           |   |       |
+   +---+   +---+   +   +   +---+---+   +---+   +---+   +---+---+   +   +---+   +
|       |               |               |                       |         S     |
|     S |  S            |         S     | S                  S  |               |
|       |               |               |                       |               |
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
]]


function create_world(env)
   local block_types = radiant.terrain.get_block_types()
   local people = {}
   local endpoint

   local r = Region3()
   local x, y = 0, 0
   for i = 1, #MAZE do
      local c = MAZE:sub(i, i)
      local skip = false

      if c == '\r' then
         skip = true
      elseif c == '\n' then
         x = 0
         y = y + 1
         skip = true
      end

      if not skip then
         local p0 = Point3(x, 0, y)
         local p1 = Point3(x, 10, y)
         r:add_cube(Cube3(p0, p1 + Point3(1, 0, 1), block_types.bedrock))

         local ditch = c == '-' or c == '+' or c == '|'
         if not ditch then
            local p0 = Point3(x, 10, y)
            local p1 = Point3(x, 13, y)
            local p2 = Point3(x, 14, y)
            r:add_cube(Cube3(p0, p1 + Point3(1, 0, 1), block_types.soil_light))
            r:add_cube(Cube3(p1, p2 + Point3(1, 0, 1), block_types.grass))
         end
         if c == 'S' then
            table.insert(people, { x=x, z=y })
         elseif c == 'E' then
            endpoint = { x=x, z=y }
         end
         x = x + 1
      end
   end
   
   local midpoint = r:get_bounds().min:to_int()
   r:translate(-midpoint)
   r:optimize_by_merge()
   local terrain = radiant._root_entity:add_component('terrain')
   terrain:add_tile(r)

   for i, coord in ipairs(people) do
      people[i] = { x = coord.x - midpoint.x, z = coord.z - midpoint.z }
   end
   endpoint = { x = endpoint.x - midpoint.x, z = endpoint.z - midpoint.z }

   local CAMERA_POSITION = Point3(-15.296, 40.0627, 97.4829)
   local CAMERA_LOOK_AT = Point3(19.8147, 14, 61.7923)
   autotest_framework.ui.move_camera(CAMERA_POSITION, CAMERA_LOOK_AT)

   return {
      people = people,
      endpoint = endpoint,
   }
end

return create_world
