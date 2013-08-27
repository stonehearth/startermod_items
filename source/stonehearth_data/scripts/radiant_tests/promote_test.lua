local MicroWorld = require 'radiant_tests.micro_world'

local PromoteTest = class(MicroWorld)

local gm = require 'radiant.core.gm'
function PromoteTest:start()
   self:create_world()
   local citizen = self:place_citizen(0, 10)
   
   local profession = om:add_component(citizen, 'profession')
   profession:learn_recipe('wooden_practice_sword')
   
   om:turn_to_face(citizen, Point3(0, 1, 0))
   local saw = self:place_item('carpenter-saw', 0, 0)
   
   self:at(10,  function()  ch:call('radiant.commands.promote_carpenter', saw, citizen) end)
end

gm:register_scenario('radiant.tests.promote', PromoteTest)