require 'checks'

-- These methods and tables define a 'radiant.class', which looks functions similarly in an object-oriented fashion
-- to unclass(), but has far less overhead (and far fewer features--no inheritence, for example).

local RADIANT_CLASS_MT = {}

RADIANT_CLASS_MT.__init = function(cls)
   local obj = {}
   setmetatable(obj, cls)
   return obj
end

function RADIANT_CLASS_MT:__call(...)
   local obj = {}
   setmetatable(obj, self)
   if obj.__init then
      obj:__init(...)
   end
   return obj
end

function radiant.class()
   local c = {}
   c.__index = c
   c.__class = c
   setmetatable(c, RADIANT_CLASS_MT)
   return c
end

-- used to add all the methods of `mixin` to `class`.  useful for sharing
-- implementation across common classes.
--
function radiant.mixin(cls, mixin)
   for k, v in pairs(mixin) do
      if type(v) == 'function' then
         cls[k] = v
      end
   end
end

function radiant.keys(t)
   local n, keys = 1, {}
   for k, _ in pairs(t) do
      keys[n] = k
      n = n + 1
   end
   return keys
end

function radiant.size(t)
   local c = 0
   for _, _ in pairs(t) do
      c = c + 1
   end
   return c
end

function radiant.empty(t)
   return not next(t)
end

function radiant.report_traceback(err)
   local traceback = debug.traceback()
   _host:report_error(err, traceback)
   return err
end

function radiant.alloc_region3()
   if _radiant.client then
      return _radiant.client.alloc_region3()
   end
   return _radiant.sim.alloc_region3()
end

function radiant.shallow_copy(t)
   local copy = {}
   for k, v in pairs(t) do
      copy[k] = v
   end
   return copy
end

function radiant.map_to_array(t, xform)
   local n = 1
   local array = {}
   for k, v in pairs(t) do
      if not xform then
         array[n] = v
         n = n + 1
      else
         local nv = xform(k, v)
         if nv == false then
            -- skip it
         elseif nv == nil then
            -- use v
            array[n] = n
            n = n + 1            
         else
            -- use nv
            array[n] = nv
            n = n + 1            
         end
      end
   end
   return array
end

function radiant.not_yet_implemented(fmt, ...)
   local info = debug.getinfo(2, 'Sfl')
   local tail = fmt and (': ' .. string.format(fmt, ...)) or ''
   error(string.format('NOT YET IMPLEMENTED (%s:%d)', info.source, info.currentline) .. tail)
end

function radiant.is_controller(c)
   return radiant.util.is_instance(c) and
          radiant.util.is_datastore(c.__saved_variables)
end

-- a nop function.  useful for creating bindings which do nothing.
function radiant.nop()
end

-- bind an invocation of a method or function and its parameters to an object which can later be
-- called with radiant.invoke.  the function form of bind must pass a string which can be used
-- to lookup the function from the global namespace (e.g. radiant.nop).  the method invocation
-- form must pass a radiant controller for the first argument and the name of the method for
-- the second.  all other arguments will be passed to the function or method being bound.
--
-- function bindings are savable, meaning they survive the trip across a save/load.
--
-- examples:
--
--    radiant.bind('radiand.nop')  -- do nothing
--
--    local binding = radiant.bind(stonehearth.town, 'get_citizens')
--    ...
--    local n = radiant.invoke(binding)
--
function radiant.bind(...)
   local obj
   local args = {...}
   if #args < 1 then
      radiant.error('expected at least one argument to radiant.bind')
   end
   if radiant.is_controller(args[1]) then
      if #args < 2 then
         radiant.error('expected at least two arguments for method invocation in radiant.bind')
      end
      obj = table.remove(args, 1)
   elseif type(args[1]) ~= 'string' then
      radiant.error('expected controller or function name for first arg in radiant.bind.  got "%s"', tostring(args[1]))
   end

   local fn = table.remove(args, 1)
   if type(fn) ~= 'string' then
      radiant.error('expected string type for function name in radiant.bind.  got "%s"', tostring(fn))
   end

   local binding = {
      type = 'binding',
      obj    = obj,
      fn     = fn,
      args   = args,
   }

   return binding
end

-- invoke a bound function (see radiant.bind).  the arguments pass to radiant.invoke will be
-- appended to the call. 
--
-- example:
--
--    function add(a, b) return a + b end
--    local add_two = radiant.bind('add', 2)
--    local five = radiant.invoke(add_two, 3)

function radiant.invoke(binding, ...)
   checks('binding')

   local args, c = {}, table.maxn(binding.args)
   for i=1,c do
      args[i] = binding.args[i]
   end
   local passed_args = { ... }
   for i=1,table.maxn(passed_args) do
      args[i + c] = passed_args[i]
   end

   local results 
   if binding.obj then
      results = { binding.obj[binding.fn](binding.obj, unpack(args)) }
   else
      local parts = string.split(binding.fn, '.')
      local current = _G
      for _, part in pairs(parts) do
         current = rawget(current, part)
         if not current then
            radiant.error('failed to look up function binding name "%s"', binding.fn)
         end
      end
      local fn = current
      results = { fn(unpack(args)) }
   end
   return unpack(results)
end

-- check to make sure `binding` is a binding.  see radiant.bind for what a binding is.
function checkers.binding(binding)
   return type(binding) == 'table' and
          binding.type == 'binding'
end


function radiant.error(...)
   error(string.format(...), 2)
end

function radiant.create_controller(...)
   local args = { ... }
   local name = args[1]
   args[1] = nil
   local i = 2
   while i <= table.maxn(args) do
      args[i - 1] = args[i]
      args[i] = nil
      i = i + 1
   end

   local datastore = radiant.create_datastore()
   local controller = datastore:create_controller(name)
   if not controller then
      return
   end

   if controller.initialize then
      controller:initialize(unpack(args, 1, table.maxn(args)))
   end
   if controller.activate then
      controller:activate()
   end
   return controller
end

-- augment checks with the some radiant native types.

local NATIVE_CHECKS = {
   Entity         = _radiant.om.Entity,
   Region3Boxed   = _radiant.om.Region3Boxed,
   Point2         = _radiant.csg.Point2,
   Point3         = _radiant.csg.Point3,
   Rect2          = _radiant.csg.Rect2,
   Cube3          = _radiant.csg.Cube3,
   Region3        = _radiant.csg.Region3,
   Region2        = _radiant.csg.Region2,
}

for name, expected_type in pairs(NATIVE_CHECKS) do
   assert(not checkers[name])
   checkers[name] = function(x)
      return radiant.util.is_a(x, expected_type)
   end
end

-- add a 'self' check to improve readiblity in methods.
function checkers.self(t)
   return type(t) == 'table'
end

-- check for radiant controllers.
function checkers.controller(c)
   return radiant.is_controller(c)
end
