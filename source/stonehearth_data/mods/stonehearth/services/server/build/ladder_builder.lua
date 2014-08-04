local priorities = require('constants').priorities.simple_labor

local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3

local LadderBuilder = class()

function LadderBuilder:initialize(base, normal)
   self._sv.climb_to = {}

   local ladder = radiant.entities.create_entity('stonehearth:wooden_ladder')
   self._sv.ladder = ladder
   self.__saved_variables:mark_changed()

   radiant.terrain.place_entity(ladder, base)
   ladder:add_component('stonehearth:construction_data')
               :set_normal(normal)

   self._ladder_component = ladder:add_component('stonehearth:ladder')
   self._vpr_component = ladder:add_component('vertical_pathing_region')
   self._vpr_component:set_region(_radiant.sim.alloc_region())

   self._vpr_trace = self._vpr_component:trace_region('ladder builder')
                                             :on_changed(function()
                                                   self:_update_ladder_tasks()
                                                end)
                                             :push_object_state()
end

function LadderBuilder:destroy()
   if self._vpr_trace then
      self._vpr_trace:destroy()
      self._vpr_trace = nil
   end
end

function LadderBuilder:add_point(to)
   self._sv.climb_to[to] = 1
   self:_update_ladder_tasks()
end

function LadderBuilder:remove_point(to)
   self._sv.climb_to[to] = nil
   self:_update_ladder_tasks()
end

function LadderBuilder:get_material()
   return 'wood resource'
end

function LadderBuilder:is_empty()
   return next(self._sv.climb_to) == nil
end

function LadderBuilder:_get_climb_to()
   local origin = radiant.entities.get_world_grid_location(self._sv.ladder)
   local top = origin
   for pt, _ in pairs(self._sv.climb_to) do
      assert(pt.x == top.x and pt.z == top.z)
      if pt.y > top.y then
         top = pt
      end
   end
   return top - origin
end

function LadderBuilder:_get_ladder_top()
   local bounds = self._vpr_component:get_region():get():get_bounds()
   return Point3(0, bounds.max.y, 0)
end

function LadderBuilder:_update_ladder_tasks()
   local climb_to = self:_get_climb_to()
   local ladder_top = self:_get_ladder_top()
   
   local desired_height = climb_to.y + 1
   self._ladder_component:set_desired_height(desired_height)

   local delta = desired_height - ladder_top.y
   if delta > 0 then
      self:_start_build_task()
   elseif delta < 0 then
      assert(false)
   else
      self:_stop_build_task()
   end
end

function LadderBuilder:is_ladder_finished()
   local bounds = self._vpr_component:get_region():get():get_bounds()
   local height = bounds.max.y - bounds.min.y
   return height >= self._ladder_component:get_desired_height()
end

function LadderBuilder:grow_ladder()
   self._vpr_component:get_region():modify(function(r)
         local bounds = r:get_bounds()
         r:add_unique_point(Point3(bounds.min.x, bounds.max.y, bounds.min.z))
      end)
end

function LadderBuilder:_stop_build_task()
   if self._build_task then
      self._build_task:destroy()
      self._build_task = nil
   end
end

function LadderBuilder:_start_build_task()
   if not self._build_task then
      local town = stonehearth.town:get_town('player_1') -- xxx: ug!  need 1 build service per player >_<
      if town then
         self._build_task = town:create_task_for_group('stonehearth:task_group:build', 'stonehearth:build_ladder', {
                                                   ladder = self._sv.ladder,
                                                   builder = self,
                                                })

         self._build_task:set_priority(stonehearth.constants.priorities.worker_task.PLACE_ITEM)                    
                           :set_name('build ladder')
                           :set_source(self._sv.ladder)
                           :set_priority(priorities.CONSTRUCT_BUILDING)
                           :set_max_workers(1)
                           :start()
      end
   end
end


return LadderBuilder
