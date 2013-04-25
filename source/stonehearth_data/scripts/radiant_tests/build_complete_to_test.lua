local gm = require 'radiant.core.gm'
local om = require 'radiant.core.om'
local MicroWorld = require 'radiant_tests.micro_world'

local CompleteToTest = class(MicroWorld)
gm:register_scenario('radiant.tests.complete_to', CompleteToTest)

CompleteToTest['radiant.md.create'] = function(self, bounds)
   self:create_world()
   
   local blueprint = self:create_room_cmd(-10, -10, 10, 10)
   local house = self:start_project_cmd(blueprint)
   local ec = om:get_component(house, 'entity_container')
 
   local complete_to = function(catalog)
      ec:iterate_children(function(id, child)
         for name, value in pairs(catalog) do
            local bo = om:get_component(child, name)
            if bo then
               bo:complete_to(value)
            end
         end
      end)
   end
   
   --self:at(10, function() complete_to({ peaked_roof = 100}) end)
   self:at(3000, function() complete_to({ post = 50 }) end)
   self:at(4000, function() complete_to({ post = 75 }) end)
   self:at(5000, function() complete_to({ post = 100 }) end)
   self:at(6000, function() complete_to({ wall = 25 }) end)
   self:at(7000, function() complete_to({ wall = 50 }) end)
   self:at(8000, function() complete_to({ wall = 75 }) end)
   self:at(8000, function() complete_to({ wall = 100 }) end)
   self:at(9000, function() complete_to({ peaked_roof = 100, fixture = 100 }) end)
end
