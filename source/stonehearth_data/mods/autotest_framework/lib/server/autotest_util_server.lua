local AutotestUtil = class()

function AutotestUtil:__init(autotest)
	self._autotest = autotest
end

function AutotestUtil:succeed_when_destroyed(entity, callback)
   if not entity:is_valid() then
      self._autotest:success()
   else
      radiant.events.listen(radiant, 'radiant:entity:post_destroy', function()
            if not entity:is_valid() then
               if (callback) then
                  callback()
               end
               self._autotest:success()
               return radiant.events.UNLISTEN
            end
         end)      
   end
end

function AutotestUtil:call_if_buff_added(entity, uri, cb)
   radiant.events.listen(entity, 'stonehearth:buff_added', function(e)
         if e.uri == uri then
            return cb()
         end
      end)
end

-- note: only works with the functional form of listen
function AutotestUtil:listen_while_running(obj, msg, cb)
   radiant.events.listen(obj, msg, function(...)
         if self._autotest:is_finished() then
            return radiant.events.UNLISTEN
         end
         cb(...)
      end)   
end

function AutotestUtil:succeed_if_buff_added(entity, uri)
   self:call_if_buff_added(entity, uri, function()
         self._autotest:success()
         return radiant.events.UNLISTEN
      end)
end

function AutotestUtil:fail_if_buff_added(entity, uri)
   self:call_if_buff_added(entity, uri, function()
         self._autotest:fail('explicitly asked not to receive buff "%s"', uri)
         return radiant.events.UNLISTEN
      end)
end

function AutotestUtil:fail_if_created(uri)
   self:listen_while_running(radiant, 'radiant:entity:post_create', function(e)
         if e.entity:get_uri() == uri then
            self._autotest:fail('entity with uri "%s" created.  failing', uri)
         end
      end)
end

function AutotestUtil:call_if_attribute_above(entity, attribute, threshold, cb)
   local current = radiant.entities.get_attribute(entity, attribute)
   if current > threshold then
      cb()
   else
      radiant.events.listen(entity, 'stonehearth:attribute_changed:' .. attribute, function(e)
      		local value = radiant.entities.get_attribute(entity, attribute)
            if value > threshold then
               return cb()
            end
         end)
   end
end

function AutotestUtil:succeed_if_attribute_above(entity, attribute, threshold)
   self:call_if_attribute_above(entity, attribute, threshold, function()
         self._autotest:success()
         return radiant.events.UNLISTEN
      end)
end

function AutotestUtil:fail_if_attribute_above(entity, attribute, threshold)
   self:call_if_attribute_above(entity, attribute, threshold, function()
         self._autotest:fail('"%s" attribute exceeded %d', attribute, threshold)
         return radiant.events.UNLISTEN
      end)
end

function AutotestUtil:fail_if_expired(timeout, format, ...)
   self._autotest:sleep(timeout)
   if format then
      self._autotest:fail(format, ...)
   else
      self._autotest:fail(format, "timeout %d ms exceeded", timeout)
   end
end

function AutotestUtil:fail_if_table_mismatch(expected, actual, format, ...)
   local success, error = self:check_table(expected, actual)
   if not success then
      self._autotest:log('%s', error)
      self._autotest:fail(format, ...);
   end
end

function AutotestUtil:fail_if(v, ...)
   if v then
      self._autotest:fail(...)
   end
end

function AutotestUtil:assert(condition, format, ...)
	if not condition then
		self._autotest:fail(format, ...)
	end
end

function AutotestUtil:check_table(expected, actual, route)
   local parent_route = route or ''
   if #parent_route > 0 then
      parent_route = parent_route .. '.'
   end
   for name, value in pairs(expected) do
      local route = parent_route .. name
      local val = actual[name]
      if not val then
         return false, string.format('expected "%s" value "%s" not in table', route, tostring(name), tostring(value))
      end
      if type(val) ~= type(value) then
         return false, string.format('expected "%s" type "%s" does not match actual type "%s"', route, type(value), type(val))
      end
      if type(val) == 'table' then
         local success, err = self:check_table(value, val, route)
         if not success then
            return false, err
         end
      else
         if val ~= value then
            return false, string.format('expected "%s" value "%s" does not match actual value "%s"', route, value, val)
         end      
      end
   end
   return true
end

return AutotestUtil
