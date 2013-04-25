local gm = require 'radiant.core.gm'
local MicroWorld = require 'radiant_tests.micro_world'

local MultiWorkerRoom = class(MicroWorld)
gm:register_scenario('radiant.tests.multi_worker_room', MultiWorkerRoom)

function MultiWorkerRoom:start()
   self:create_world()
   self:place_citizen(11, 12)
   self:place_citizen(12, 12)
   self:place_citizen(12, 13)
   self:place_citizen(13, 12)
   self:place_citizen(13, 13)
   self:place_citizen(14, 12)
   self:place_citizen(14, 13)
   
   self:place_item_cluster('oak-log', 4, 4, 10, 10)
   self:place_stockpile_cmd(12, -12)
   
   local house = self:create_room_cmd(-12, -12, 10, 10)
   
   self:at(100, function() self:start_project_cmd(house) end)
end
