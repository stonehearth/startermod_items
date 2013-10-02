local resources = {}

function resources.__init()
end

function resources.load_animation(uri)
   return _radiant.res.load_animation(uri)
end

function resources.load_json(uri)
   return _radiant.res.load_json(uri)
end

function resources.load_manifest(uri)
   return _radiant.res.load_manifest(uri)
end

resources.__init()
return resources
