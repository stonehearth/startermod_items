print 'requiring foo'
print (package.path)
print (package.path .. 'CORRUPTED')

local foo = require 'foo'
print 'requiring microworld'
local MicroWorld = require 'lib.micro_world'
local HarvestTest = class(MicroWorld)
local gm = require 'radiant.core.gm'
local om = require 'radiant.core.om'

HarvestTest['radiant.md.create'] = function(self, bounds)
   self:create_world()

   local tree = self:place_tree(-12, -12)
   self:place_citizen(12, 12)
   self:at(10,  function()
         self:place_stockpile_cmd(4, 12)
      end)
   self:at(100, function()
         self:harvest_cmd(tree)
      end)
end

gm:register_scenario('radiant.tests.harvest', HarvestTest)
