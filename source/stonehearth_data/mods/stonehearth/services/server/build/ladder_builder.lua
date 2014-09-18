local priorities = require('constants').priorities.simple_labor

local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3

local LadderBuilder = class()

function LadderBuilder:initialize(manager, base, normal, removeable)
   self._sv.climb_to = {}

   local ladder = radiant.entities.create_entity('stonehearth:wooden_ladder')
   if removeable then
      ladder:add_component('stonehearth:commands')
                  :add_command('/stonehearth/data/commands/remove_ladder')
   end

   self._sv.manager = manager
   self._sv.ladder = ladder
   self._sv.base = base
   self.__saved_variables:mark_changed()

   radiant.terrain.place_entity_at_exact_location(ladder, base)
   self._ladder_component = ladder:add_component('stonehearth:ladder')
                                       :set_normal(normal)

   self._vpr_component = ladder:add_component('vertical_pathing_region')
   self._vpr_component:set_region(_radiant.sim.alloc_region3())
   
   self:_install_traces()  
end

function LadderBuilder:restore()
   radiant.events.listen(radiant, 'radiant:game_loaded', function(e)
         local ladder = self._sv.ladder
         self._ladder_component = ladder:get_component('stonehearth:ladder')
         self._vpr_component = ladder:get_component('vertical_pathing_region')
         self:_install_traces()
         self:_update_ladder_tasks()
      end)
end

function LadderBuilder:_install_traces()
   self._vpr_trace = self._vpr_component:trace_region('ladder builder')
                                             :on_changed(function()
                                                   self:_update_ladder_tasks()
                                                end)
end

function LadderBuilder:destroy()
   self:_stop_teardown_task()   
   self:_stop_build_tasks()

   if self._vpr_trace then
      self._vpr_trace:destroy()
      self._vpr_trace = nil
   end   
   if self._sv.ladder then
      radiant.entities.destroy_entity(self._sv.ladder)
      self._vpr_component = nil
      self._sv.ladder = nil
      self.__saved_variables:mark_changed()
   end
end

function LadderBuilder:add_point(to)
   self._sv.climb_to[to:key_value()] = to
   self.__saved_variables:mark_changed()   
   self:_update_ladder_tasks()
end

function LadderBuilder:remove_point(to)
   self._sv.climb_to[to:key_value()] = nil
   self.__saved_variables:mark_changed()   
   self:_update_ladder_tasks()
end

function LadderBuilder:clear_all_points()
   self._sv.climb_to = {}
   self.__saved_variables:mark_changed()   
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
   local top
   for _, pt in pairs(self._sv.climb_to) do
      assert(not top or (pt.x == top.x and pt.z == top.z))
      if not top or pt.y > top.y then
         top = pt
      end
   end
   return top and top - origin or nil
end

function LadderBuilder:_get_completed_height()
   local region = self._vpr_component:get_region():get()
   local completed = Point3(0, 0, 0)
   while region:contains(completed) do
      completed.y = completed.y + 1
   end
   return completed.y
end

function LadderBuilder:_update_ladder_tasks()
   local climb_to = self:_get_climb_to()
   local completed_height = self:_get_completed_height()
   
   --local desired_height = climb_to.y > 0 and climb_to.y + 1 or 0
   local desired_height = climb_to and climb_to.y + 1 or 0
   self._ladder_component:set_desired_height(desired_height)

   local delta = desired_height - completed_height
   if delta > 0 then
      self:_stop_teardown_task()
      self:_start_build_tasks()
   elseif delta < 0 then
      self:_stop_build_tasks()
      self:_start_teardown_task()
   else
      self:_stop_teardown_task()
      self:_stop_build_tasks()
      self:_check_if_valid()
   end
end

function LadderBuilder:is_ladder_finished()
   if not self._vpr_component then
      return true
   end
   local bounds = self._vpr_component:get_region():get():get_bounds()
   local height = bounds.max.y - bounds.min.y
   return height == self._ladder_component:get_desired_height()
end

function LadderBuilder:grow_ladder(direction)
   self._vpr_component:get_region():modify(function(r)
         local rung
         local top = self._ladder_component:get_desired_height()

         -- look for the next empty point in the 'up' or 'down' direction
         -- and add that point to our shape
         if direction == 'up' then            
            rung = Point3(0, 0, 0)
            while rung.y < top and r:contains(rung) do
               rung.y = rung.y + 1
            end
         else
            rung = Point3(0, top - 1, 0)
            while rung.y > 0 and r:contains(rung) do
               rung.y = rung.y - 1
            end
         end
         r:add_point(rung)
      end)
end

function LadderBuilder:shrink_ladder()
   self._vpr_component:get_region():modify(function(r)
         local bounds = r:get_bounds()
         r:subtract_point(Point3(bounds.min.x, bounds.max.y, bounds.min.z) - Point3.unit_y)
      end)
end

function LadderBuilder:_stop_build_tasks()
   if self._build_up_task then
      self._build_up_task:destroy()
      self._build_up_task = nil
   end
   if self._build_down_task then
      self._build_down_task:destroy()
      self._build_down_task = nil
   end
end

function LadderBuilder:_stop_teardown_task()
   if self._teardown_task then
      self._teardown_task:destroy()
      self._teardown_task = nil
   end
end

function LadderBuilder:_start_build_tasks()
   local town = stonehearth.town:get_town('player_1') -- xxx: ug!  need 1 build service per player >_<
   if not self._build_up_task then
      self._build_up_task = town:create_task_for_group('stonehearth:task_group:build', 'stonehearth:build_ladder', {
                                                ladder = self._sv.ladder,
                                                builder = self,
                                             })

      self._build_up_task:set_name('build ladder')
                         :set_source(self._sv.ladder)
                         :set_priority(priorities.BUILD_LADDER)
                         :set_max_workers(1)
                         :start()
      end
   if not self._build_down_task then
      self._build_down_task = town:create_task_for_group('stonehearth:task_group:build', 'stonehearth:build_ladder_down', {
                                                ladder = self._sv.ladder,
                                                builder = self,
                                             })

      self._build_down_task:set_name('build ladder')
                           :set_source(self._sv.ladder)
                           :set_priority(priorities.BUILD_LADDER)
                           :set_max_workers(1)
                           :start()
   end
end

function LadderBuilder:_start_teardown_task()
   if not self._teardown_task then
      local town = stonehearth.town:get_town('player_1') -- xxx: ug!  need 1 build service per player >_<
      if town then
         self._teardown_task = town:create_task_for_group('stonehearth:task_group:build', 'stonehearth:teardown_ladder', {
                                                   ladder = self._sv.ladder,
                                                   builder = self,
                                                })

         self._teardown_task:set_name('teardown ladder')
                            :set_source(self._sv.ladder)
                            :set_priority(priorities.BUILD_LADDER)
                            :set_max_workers(1)
                            :start()
      end
   end
end

function LadderBuilder:_check_if_valid()
   -- if no one wants the ladder around anymore, destroy it
   if self:is_empty() then
      local ladder_top = self:_get_ladder_top()
      if ladder_top.y == 0 then
         self._sv.manager:_destroy_builder(self._sv.base, self)
      end
   end
end

return LadderBuilder
