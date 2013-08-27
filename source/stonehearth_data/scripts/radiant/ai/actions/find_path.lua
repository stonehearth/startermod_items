local FindPath = class()

local om = require 'radiant.core.om'
local ai_mgr = require 'radiant.ai.ai_mgr'
ai_mgr:register_action('radiant.actions.find_path', FindPath)

function FindPath:run(ai, entity, entity_to)
   -- xxx: run "thinking" effect?
   local location = om:get_world_grid_location(entity_to)

   local points = PointList()   
   points:insert(Point3(location.x, location.y, location.z + 1))
   points:insert(Point3(location.x, location.y, location.z - 1))
   points:insert(Point3(location.x + 1, location.y, location.z))
   points:insert(Point3(location.x - 1, location.y, location.z))
   
   local pf = native:create_path_finder('travelling to ' .. om:get_display_name(entity_to), entity)
   pf:add_destination(EntityDestination(entity_to, points))

   ai:wait_until(function()
         return pf:get_solution() ~= nil
      end)

   return pf:get_solution()
end


--[[
local FindPath = class()

local md = require 'radiant.core.md'
local om = require 'radiant.core.om'

md:register_msg_handler('radiant.actions.find_path_to_item', FindPath)

FindPath['radiant.md.create'] = function(self, entity, item, done)
   self.entity = entity
   self.item = item
   self.done = done
end

FindPath['radiant.ai.actions.start'] = function (self)
   local location = om:get_component(self.item, 'mob'):get_grid_location()
   local points = PointList()
   
   points:insert(Point3(location.x, location.y, location.z + 1))
   points:insert(Point3(location.x, location.y, location.z - 1))
   points:insert(Point3(location.x + 1, location.y, location.z))
   points:insert(Point3(location.x - 1, location.y, location.z))
   
   self.pf = native:create_path_finder('travelling to ' .. om:get_display_name(self.item), self.entity)
   self.pf:add_destination(EntityDestination(self.item, points))
end

FindPath['radiant.ai.actions.run'] = function (self)
   if self.pf then
      local solution = self.pf:get_solution()
      if not solution then
         return false
      end
      self.done(solution)
      self.pf = nil
   end
   return true
end

FindPath['radiant.ai.actions.stop'] = function (self)
end
]]
