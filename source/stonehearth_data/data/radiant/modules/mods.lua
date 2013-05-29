local mods = {}

function mods.__init()
end

function mods.load_api(uri)
   return native:lua_require(uri)
end

function mods.require(uri)
   return native:lua_require(uri)
end

mods.__init()
return mods
