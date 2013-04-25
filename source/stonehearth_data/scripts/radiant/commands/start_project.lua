local ch = require 'radiant.core.ch'
local om = require 'radiant.core.om'
local check = require 'radiant.core.check'

ch:register_cmd("radiant.commands.start_project", function(blueprint)
   check:is_entity(blueprint)
   local project = om:start_project(blueprint)
   return { entity_id = project:get_id() }
end)
