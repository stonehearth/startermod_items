local api = {}

function api.chop(tree)
   radiant.log.info('chopping down tree %s', tostring(tree))
   --radiant.mods.load("mod://stonehearth_worker_class/").chop(tree)
end

return api
