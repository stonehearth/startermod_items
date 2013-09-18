local Timer = class()

Timer.WALL_CLOCK = 1
Timer.CPU_TIME = 2

function Timer:__init(mode)
   assert(mode ~= nil)
   self.mode = mode
   self:reset()
end

function Timer:reset()
   self._accumulated_seconds = 0
   self._start_time = nil
   self._stop_time = nil
end

function Timer:start()
   self:reset()
   self:resume()
end

function Timer:stop()
   self:pause()
end

function Timer:resume()
   assert(self._start_time == nil)
   self._start_time = self:_get_time()
end

function Timer:pause()
   self._stop_time = self:_get_time()

   assert(self._start_time)
   local current_interval = self._stop_time - self._start_time
   self._accumulated_seconds = self._accumulated_seconds + current_interval

   self._start_time = nil
   self._stop_time = nil
end

function Timer:seconds()
   local current_interval = 0

   if self._start_time then
      -- still running
      current_interval = self:_get_time() - self._start_time
   end

   return self._accumulated_seconds + current_interval
end

function Timer:_get_time()
   if self.mode == Timer.CPU_TIME then
      return os.clock()
   else
      return os.time()
   end
end

return Timer
