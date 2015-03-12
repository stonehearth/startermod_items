
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
