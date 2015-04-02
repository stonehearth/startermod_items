local Entity = _radiant.om.Entity
local Point3 = _radiant.om.Point3

-- if these variables don't exist yet, they never will.  put them in the global
-- namespace so we can check them before using (otherwise strict.lua will kick
-- us in the teeth just for checking against nil)
if not decoda_break_on_error then
   decoda_break_on_error = false
end
if not decoda_output then
   decoda_output = false
end

-- register all threads with the backend
if false then
   local cocreate = coroutine.create
   coroutine.create = function(f, ...)
    local co = cocreate(function(...)
      return f(...)
    end, ...)
    _host:register_thread(co)end
end
         
-- this function is only valid in very specific circumstances!  specfically, the
-- caller must be called DIRECTLY from a 3rd party module source file.
__get_current_module_name = function(depth)
   local info = debug.getinfo(depth, 'S')

   if not info.source then
      _host:log('env', 1, 'could not determine module file in radiant "require"')
      return nil
   end
   if info.source:sub(1, 1) ~= '@' then
      _host:log('env', 1, 'lua generated from loadstring() is not allowed to require.')
      return nil
   end
   local modname = info.source:match('@([^/\\]*)')
   if not modname then
      modname = info.source:match('@\\.[/\\]([^/\\]*)')
   end
   if not modname then
      _host:log('env', 1, string.format('could not determine modname from source "%s"', info.source))
      return nil
   end
   return modname
end

local old_require = require   -- hide the lua implementation
require = function(s)
   local result
   
   -- check standard lua stuff..
   local o = package.loaded[s]
   if o then
      return o
   end

   -- level 1 would be the __get_current_module_name function in env.lua...
   -- level 2 would be the caller of this function (e.g. require in env.lua...)
   -- level 3 is the caller of the caller, which is in the module we're looking for!
   local mod
   local modname = __get_current_module_name(3)
   if modname then
      mod = _host:require(modname .. '.' .. s)
   end

   if not mod then
      -- try the full path...
      mod =  _host:require(s)
   end

   if not mod then
      -- try radiant.lib
      mod = _host:require('radiant.lib.' .. s)
   end
   
   return mod
end

-- We need to redefine 'next' and 'pairs' in order to allow for traversal of tables that have 
-- metamethod overrides for '__newindex' and '__index'.  FIXME: if we move to Lua 5.2, we can
-- get rid of this and just implement __pairs on the meta table.

local rawnext = next
function next(t, k)
  local m = getmetatable(t)
  local n = m and m.__next or rawnext
  return n(t, k)
end

function pairs(t)
  return next, t, nil
end

-- pythonesque string split taken from the lua manual
--
function string:split(sSeparator, nMax, bRegexp)
   assert(sSeparator ~= '')
   assert(nMax == nil or nMax >= 1)

   local aRecord = {}

   if self:len() > 0 then
      local bPlain = not bRegexp
      nMax = nMax or -1

      local nField=1
      local nStart=1
      local nFirst, nLast = self:find(sSeparator, nStart, bPlain)
      while nFirst and nMax ~= 0 do
         aRecord[nField] = self:sub(nStart, nFirst-1)
         nField = nField+1
         nStart = nLast+1
         nFirst,nLast = self:find(sSeparator, nStart, bPlain)
         nMax = nMax-1
      end
      aRecord[nField] = self:sub(nStart)
   end

   return aRecord
end

-- strip whitespace from the lua manual
function string:strip()
  return (self:gsub("^%s*(.-)%s*$", "%1"))
end

function string:starts_with(prefix)
   return self:sub(1, prefix:len()) == prefix
end

function string:ends_with(suffix)
   return suffix == '' or self:sub(-suffix:len()) == suffix
end

require 'lualibs.unclasslib'

-- augment checks with the some radiant native types.
require 'lualibs.checks'

local NATIVE_CHECKS = {
   Entity         = _radiant.om.Entity,
   Region3Boxed   = _radiant.om.Region3Boxed,
   Point3         = _radiant.csg.Point3,
   Region3        = _radiant.csg.Region3,
}
for name, expected_type in pairs(NATIVE_CHECKS) do
   assert(not checkers[name])
   checkers[name] = function(x)
      return radiant.util.is_a(x, expected_type)
   end
end

-- add a 'self' check to improve readiblity in methods.
function checkers.self(x)
   return type(x) == 'table'
end

-- check for radiant controllers.
function checkers.controller(x)
   return radiant.util.is_instance(x) and
          radiant.util.is_a(x.__saved_variables, _radiant.om.DataStore)
end
