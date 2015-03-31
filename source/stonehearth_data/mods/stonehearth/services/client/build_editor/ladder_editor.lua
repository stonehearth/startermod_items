local constants = require('constants').construction
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3

local MODEL_OFFSET = Point3(-0.5, 0, -0.5)

local LadderEditor = class()

function LadderEditor:__init(build_service)
   self._build_service = build_service
   self._ladder_height = 0
   self._normal = Point3.unit_x
   self._ladder_uri = 'stonehearth:wooden_ladder'
   self._log = radiant.log.create_logger('builder')
end

function LadderEditor:go(session, response)
   self._cursor = radiant.entities.create_entity(self._ladder_uri)
   self._cursor_render_entity = _radiant.client.create_render_entity(H3DRootNode, self._cursor)
   self._cursor_mob = self._cursor:add_component('mob')
   self._cursor_vpr = self._cursor:add_component('vertical_pathing_region')
   self._cursor_vpr:set_region(_radiant.client.alloc_region3())
   self._cursor_ladder = self._cursor:add_component('stonehearth:ladder')

   stonehearth.selection:select_location()
      :set_cursor('stonehearth:cursors:create_ladder')
      :set_filter_fn(function (result)
            self._log:spam('testing: %s', result.entity)
            if result.entity == self._cursor then
               return stonehearth.selection.FILTER_IGNORE
            end

            -- ignore transparent entities (e.g. the no construction zone at the base of buildings)
            local rcs = result.entity:get_component('region_collision_shape')
            if rcs and rcs:get_region_collision_type() == _radiant.om.RegionCollisionShape.NONE then
               return stonehearth.selection.FILTER_IGNORE
            end

            return self:_update_ladder_cursor(result.brick, result.normal:to_int())
         end)
      :progress(function(selector, location, rotation)
            if not location then
               location = Point3(0, -100000, 0)
            end
            self._cursor_mob:move_to_grid_aligned(location - Point3(0, self._ladder_height, 0))
         end)
      :done(function(selector, location, rotation)
            _radiant.call_obj(self._build_service, 'create_ladder_command', self._ladder_uri, location, self._normal)
               :always(function()
                     response:resolve({})
                     radiant.entities.destroy_entity(self._cursor)
                  end)
         end)
      :fail(function(selector)
            response:reject({})
            radiant.entities.destroy_entity(self._cursor)
         end)
      :always(function(selector)
            selector:destroy()
         end)
      :go()
end

function LadderEditor:_update_ladder_cursor(brick, normal)
   if normal.y ~= 0 then
      self._log:spam('%s normal.y is not zero.  cannot build ladder here.', normal)
      -- have to mouse over the edge of the thing you want to get to
      return false
   end
   self._climb_to = brick
   local base = Point3(brick.x, brick.y, brick.z)

   if radiant.terrain.is_blocked(base) or not radiant.terrain.in_bounds(base) then
      -- blocked can occur when a ladder is built, but the selected location hasn't changed for the next ladder
      -- also, don't build on the edge of the world
      return false
   end

   -- keep looking down until we find stable ground
   self._ladder_height = 0
   while not radiant.terrain.is_standable(base) do
      base.y = base.y - 1
      self._ladder_height = self._ladder_height + 1

      -- don't search for ground past the bottom of the world
      if not radiant.terrain.in_bounds(base) then
         return false
      end
   end
   self._normal = normal
   
   self._cursor_ladder:set_normal(normal)
                      :set_desired_height(self._ladder_height)

   -- fill in the region so it's rendered solid in preview mode
   local shape = Cube3(Point3.zero, Point3(1, self._ladder_height + 1, 1))
   self._cursor_vpr:get_region():modify(function(r)
         r:clear()
         r:add_unique_cube(shape)
         self._log:spam('set ghost ladder region to %s.', r:get_bounds())
      end)

   return true
end

function LadderEditor:destroy()
end

return LadderEditor 