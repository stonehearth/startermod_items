local ProtectStockpile = class()

local md = require 'radiant.core.md'
local om = require 'radiant.core.om'
local ai_mgr = require 'radiant.ai.ai_mgr'
local ani_mgr = require 'radiant.core.animation'

-- xxx: merge this with follow path?
ai_mgr:register_action('radiant.actions.protect_stockpile', ProtectStockpile)

function ProtectStockpile:run(ai, entity, stockpile)
   -- compute the stockpile bounds
   local position = om:get_world_grid_location(stockpile)
   local bounds = om:get_component(stockpile, 'stockpile_designation'):get_bounds()
   bounds = bounds + position

   -- grow the bounds by a good radius...
   bounds.min.x = bounds.min.x - 3
   bounds.min.z = bounds.min.z - 3
   bounds.max.x = bounds.max.x + 3
   bounds.max.z = bounds.max.z + 3

   -- compute the patrol corners
   self._patrol = {
      bounds.min,
      RadiantIPoint3(bounds.max.x, bounds.min.y, bounds.min.z),
      RadiantIPoint3(bounds.max.x, bounds.min.y, bounds.max.z),
      RadiantIPoint3(bounds.min.x, bounds.min.y, bounds.max.z),
   }

   -- run to the first corner and patrol the rest   
   self._next_patrol = 2
   ai:execute('radiant.actions.goto_location', RadiantPoint3(self._patrol[1]))
   while true do
      self:_patrol_perimeter(ai)
   end
end

function ProtectStockpile:_patrol_perimeter(ai)
   local dst = RadiantPoint3(self._patrol[self._next_patrol])
   self._next_patrol = (self._next_patrol + 1)
   if self._next_patrol > #self._patrol then
      self._next_patrol = 1
   end
   ai:execute('radiant.actions.goto_location', dst, 'patrol')
end

function ProtectStockpile:stop(ai, entity, path)
end

--[[
local ProtectStockpile = class()

local md = require 'radiant.core.md'
local check = require 'radiant.core.check'

md:register_msg_handler('radiant.actions.ProtectStockpile', ProtectStockpile)

ProtectStockpile['radiant.md.create'] = function(self, action, ...)
   check:is_string(action)

   self.action = action
   self.args = {...}
   self.finished = false
end

-- Action messages 

ProtectStockpile['radiant.ai.actions.start'] = function (self, entity)
   check:is_entity(entity)
   
   self.entity = entity
   md:listen(self.entity, 'radiant.animation', self)
   md:send_msg(self.entity, 'radiant.animation.start_action', self.action, unpack(self.args))
end

ProtectStockpile['radiant.ai.actions.run'] = function (self)
   return self.finished
end

ProtectStockpile['radiant.ai.actions.stop'] = function (self)
   md:send_msg(self.entity, 'radiant.animation.stop_action', self.action)
   md:unlisten(self.entity, 'radiant.animation', self)
end

ProtectStockpile['radiant.animation.on_stop'] = function (self, action)
   if self.action == action then
      self.finished = true
   end
end
]]