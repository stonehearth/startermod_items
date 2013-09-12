local resources = {}

function resources.__init()
end

function resources.load_animation(uri)
   return native:load_animation(uri)
end

function resources.load_json(uri)
   return native:load_json(uri)
end

resources.__init()
return resources
