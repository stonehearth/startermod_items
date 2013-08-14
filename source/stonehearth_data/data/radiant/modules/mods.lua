local mods = {}

function mods.__init()
end

function mods.load_api(uri)
   return native:lua_require(uri)
end

function mods.require(uri)
   return native:lua_require(uri)
end

function mods.create_data_blob(obj)
   return native:create_data_blob(obj)
end

function mods.publish_data_blob(uri, obj)
   return native:publish_data_blob(uri, obj)
end

mods.__init()
return mods
