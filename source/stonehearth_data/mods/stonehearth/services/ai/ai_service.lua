local AiService = class()
local AiInjector = require 'services.ai.ai_injector'
local CompoundActionFactory = require 'services.ai.compound_action_factory'
local placeholders = require 'services.ai.placeholders'
local log = radiant.log.create_logger('ai.service')

function AiService:__init()
   -- SUSPEND_THREAD is a unique, non-integer token which indicates the thread
   -- should suspend.  It must be non-intenger, as yielding an int means "wait
   -- until this time".  By creating a table, we guarantee the value of
   -- SUSPEND_THREAD is unique when compared with ==.  The name is for
   -- cosmetic purposes and aids in debugging.
   self.SUSPEND_THREAD = { name = "SUSPEND_THREAD" }
   self.KILL_THREAD = { name = "KILL_THREAD" }
   self.RESERVATION_LEASE_NAME = 'ai_reservation'

   for name, value in pairs(placeholders) do
      AiService[name] = value
   end
   AiService.ANY = { ANY = 'Any lua value will do' }
   AiService.NIL = { NIL = 'The nil value' }
end

function AiService:load(savestate)   
end

-- injecting entity may be null
function AiService:inject_ai(entity, injecting_entity, ai) 
   return AiInjector(entity, injecting_entity, ai)
end

function AiService:format_activity(activity)
   return activity.name .. '(' .. self:format_args(activity.args) .. ')'
end

function AiService:format_args(args)
   local msg = ''
   if args then
      assert(type(args) == 'table', string.format('invalid activity arguments: %s', radiant.util.tostring(args)))
      for name, value in pairs(args) do
         if #msg > 0 then
            msg = msg .. ', '
         end
         msg = msg .. string.format('%s = %s ', name, radiant.util.tostring(value))
      end
   end
   return '{ ' .. msg .. '}'
end

function AiService:create_compound_action(action_ctor)
   assert(action_ctor)
   -- cannot implement anything here.  it gets really confusing (where does start_thinking forward
   -- its args to?  the next action in the chain or the calling action?)
   assert(not action_ctor.start_thinking, 'compound actions must not contain implementation')
   assert(not action_ctor.stop_thinking, 'compound actions must not contain implementation')
   
   return CompoundActionFactory(action_ctor)
end

function AiService:create_activity(name, args)   
   if args == nil then
      args = {}
   end
   assert(type(name) == 'string', 'activity name must be a string')
   assert(type(args) == 'table', 'activity arguments must be an associative array')
   assert(not args[1], 'activity arguments contains numeric elements (invalid!)')

   -- common error... trying to pass a class instance (e.g. 'foo', { my_instance })
   -- this won't catch them all, but will catch all uses of unclasslib clases
   assert(not args.__class, 'attempt to pass class for activity args (not using associative array?)')
   assert(not args.__type,  'attempt to pass instance for activity args (not using associative array?)')

   return {
      name = name,
      args = args
   }
end

local LAST_ID = 0
function AiService:get_next_object_id()
   LAST_ID = LAST_ID + 1
   return LAST_ID
end

return AiService
