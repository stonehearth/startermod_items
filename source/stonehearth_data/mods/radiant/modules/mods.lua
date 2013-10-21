local mods = {}

function mods.__init()
end

function mods.require(s)
   return _host:require(s)
end

-- ideally only this form of requiring would survive!
function mods.load(s)
   return _host:require(string.format('%s.api', s))
end

function mods.load_script(s)
   return _host:require_script(s)
end

function mods.create_object(arg1, arg2)
   local ctor = mods.require(arg1, arg2)
   return ctor()
end

mods.__init()
return mods
