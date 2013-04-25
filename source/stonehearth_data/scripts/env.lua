--local ProFi = require 'ProFi'
require 'unclasslib'
require 'copas'
local log = require 'radiant.core.log'

Environment = class()
function Environment:__init()
   self._running = false
   self._lastProfileReport = 0
end

function Environment:init(host)  
   native = host
   log:info('initializing environment...')

   require 'debug_web_socket'
   
   --[[
   check = Check()
   util = Util()
   ch = CommandHandler()
   md = MsgDispatcher()
   om = ObjectModel()
   gm = GameMaster()
   sh = Stonehearth()
   ai = AI()
   
   -- xxx: load these based on some "enabled modules" flag.  we don't
   -- want any of the teragen stuff in unit tests, for example.
   tg = TeraGen()
   ]]
   self._md = require 'radiant.core.md'
   self._om = require 'radiant.core.om'
   self._sh = require 'radiant.core.sh'
   self._ch = require 'radiant.core.ch'
   self._gm = require 'radiant.core.gm'
   self._ai_mgr = require 'radiant.ai.ai_mgr'

   host:log('finished initializing environment.')
end

function Environment:register_action(msg, ctor)
   self._ai_mgr:register_action(msg, ctor)
end

function Environment:register_msg_handler(msg, handler)
   print('registering msg handler ' .. msg)
   self._md:register_msg(msg)
   self._md:register_msg_handler(msg, handler)
end

function Environment:start_new_game()
   -- lock down the environment
   require 'strict'
   self._running = true
   --ProFi:start();
   
   self._om:create_root_objects()
   self._sh:start_new_game()
   
   local test
   --test = 'radiant.tests.stockpile'
   test = 'radiant.tests.harvest'
   --test = 'radiant.tests.simple_room'
   --test = 'radiant.tests.simple_room_and_door'
   --test = 'radiant.tests.combat_test'
   --test = 'radiant.tests.complete_to'
   if test then
      self._gm:run_test(test)
   else
      self._gm:start_new_game()
   end
end

function Environment:update(interval)
   local now = self._om:increment_clock(interval)
   self._md:update(now)
   --[[
   if now > self._lastProfileReport + 5000 then
      ProFi:stop();
      ProFi:writeReport();
      ProFi:reset();
      ProFi:start();
      self._lastProfileReport = now
   end
   ]]
   copas.step(0)
   return now
end

function Environment:execute_command(cmd, ...)
   return self._ch:call(cmd, ...)
end

function Environment:is_running()
   return self._running
end

function Environment:now()
   return self._md and self._md:now()
end

function Environment:send_msg(...)
   return self._md and self._md:send_msg(...)
end

-- boo... not global!!
env = Environment()
