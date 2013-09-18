local stonehearth_trees = require 'stonehearth_trees'

local Chop = class()

function Chop:chop(session, response, tree)
   if not tree then
      response:fail('invalid tree')
      return
   end
   radiant.log.info('chopping down tree %s', tostring(tree))
   stonehearth_trees.harvest_tree(session.faction, tree)
   return true
end

return Chop
