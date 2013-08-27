local ResourceFactory = class()

local md = require 'radiant.core.md'
local om = require 'radiant.core.om'
local log = require 'radiant.core.log'
local check = require 'radiant.core.check'

md:register_msg("radiant.resource_node.spawn_resource", Harvest)
md:register_msg_handler("radiant.msg_handlers.resource_factory", ResourceFactory)

ResourceFactory['radiant.md.create'] = function(self, entity)
   self.entity = entity
   md:listen(entity, 'radiant.resource_node.spawn_resource', self)
end

ResourceFactory['radiant.resource_node.spawn_resource'] = function(self, location)
   check:is_a(location, Point3)
   
   log:info("%s spawing resource near %s.", om:get_display_name(self.entity), tostring(location))

   local node = om:get_component(self.entity, 'resource_node')
   local resource = om:create_entity(node:get_resource())
   local item = om:get_component(resource, 'item')
   om:place_on_terrain(resource, location + Point3(math.random(-3, 3), 0, math.random(-3, 3)))

   local durability = node:get_durability() - 1
   node:set_durability(durability)
   log:info('resource node %s durability now at %d.', tostring(self.entity), durability)
   if durability <= 0 then
      native:destroy_entity(self.entity)
   end
end

