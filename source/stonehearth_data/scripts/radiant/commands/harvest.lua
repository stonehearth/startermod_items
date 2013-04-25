local ch = require 'radiant.core.ch'
local sh = require 'radiant.core.sh'

ch:register_cmd("radiant.commands.harvest", function(entity)
   sh:harvest(entity)
   return {}
end)
