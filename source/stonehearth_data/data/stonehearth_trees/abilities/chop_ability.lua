local api = {}

function api.chop(tree)
   radiant.log.info('chopping down tree %s', tostring(tree))
   local api = radiant.mods.require("mod://stonehearth_worker_class/")
   api.chop(tree)
end

return api
