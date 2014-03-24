local util = {}

-- stall to simulate user lag and let the javascript have a time to chew
local function _send(...)
   _client:send(...)
   --_client:send(commands.WAIT_REALTIME, 1000)
end

function util.succeed_when_destroyed(entity)
   if not entity:is_valid() then
      autotest.success()
   else
      radiant.events.listen(radiant, 'radiant:entity:post_destroy', function()
            if not entity:is_valid() then
               autotest.success()
               return radiant.events.UNLISTEN
            end
         end)      
   end
end

function util.call_if_buff_added(entity, uri, cb)
   radiant.events.listen(entity, 'stonehearth:buff_added', function(e)
         if e.uri == uri then
            cb()
            return radiant.events.UNLISTEN
         end
      end)
end

function util.succeed_if_buff_added(entity, uri)
   util.call_if_buff_added(entity, uri, function()
         autotest.success()
      end)
end

function util.fail_if_buff_added(entity, uri)
   util.call_if_buff_added(entity, uri, function()
         autotest.fail('explicitly asked not to receive buff "%s"', uri)
      end)
end

function util.call_if_attribute_above(entity, attribute, threshold, cb)
   local current = radiant.entities.get_attribute(entity, attribute)
   if current > threshold then
      cb()
   else
      radiant.events.listen(entity, 'stonehearth:attribute_changed:' .. attribute, function(e)
            if e.value > threshold then
               cb()
            end
         end)
   end
end

function util.succeed_if_attribute_above(entity, attribute, threshold)
   util.call_if_attribute_above(entity, attribute, threshold, function()
         autotest.success()
      end)
end

function util.fail_if_attribute_above(entity, attribute, threshold)
   util.call_if_attribute_above(entity, attribute, threshold, function()
         autotest.fail('"%s" attribute exceeded %d', attribute, threshold)
      end)
end

function util.fail_if_expired(timeout, format, ...)
   autotest.sleep(timeout)
   if format then
      autotest.fail(format, ...)
   else
      autotest.fail(format, "timeout %d ms exceeded", timeout)
   end
end

return util
