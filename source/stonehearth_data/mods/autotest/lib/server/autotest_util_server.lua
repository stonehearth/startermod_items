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
      radiant.events.listen(radiant.events, 'stonehearth:entity:post_destroy', function()
            if not entity:is_valid() then
               autotest.success()
               return radiant.events.UNLISTEN
            end
         end)      
   end
end

function util.fail_if_expired(timeout, format, ...)
   autotest.sleep(timeout)
   autotest.fail(format, ...)
end

return util
