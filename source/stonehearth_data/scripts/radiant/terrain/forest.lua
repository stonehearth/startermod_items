require 'unclasslib'
local om = require('radiant.core.om')
local tg = require('radiant.core.tg')
local log = require 'radiant.core.log'

local Forest = class()

function Forest:__init()
   log:info('initializing forest...')
end

function Forest:_add_tree(x, y, z)
   log:info('adding tree at %d, %d, %d', x, y, z)
   local trees = {
      'tree-v1',
      'tree-v2',
      'tree-v3'
   }
   local tree = om:create_entity(trees[math.random(1, #trees)])
   om:place_on_terrain(tree, RadiantIPoint3(x, y, z))
   om:get_component(tree, 'mob'):turn_to(90*math.random(0, 3))
end

function Forest:_random_point(region)
   local rect = region:get_rect(math.random(0, region:get_num_rects() - 1))
   local x = math.random(rect.min.x, rect.max.x - 1);
   local y = math.random(rect.min.y, rect.max.y - 1);
   return x, y
end

function Forest:create_layer(region, height)
   local half_size = 5
   local box_size = (half_size * 2 + 1);
   log:info("-- box size is %d.", box_size)
   
   local covered = Region2()
   while not region:empty() do
      local x, y = self:_random_point(region)      
      local rect = Rect2(Point2(x - half_size, y - half_size), Point2(x + half_size, y + half_size))
      
      log:info('grabbed rect(%d, %d, %d, %d)', rect.min.x, rect.min.y, rect:width(), rect:height())
      if not covered:intersects(rect) then
         self:_add_tree(x, height, y)
      end
      covered:add(rect)
      region:subtract(rect)
   end
end

tg:register_terrain(Forest, {
   name = 'forest',
})
