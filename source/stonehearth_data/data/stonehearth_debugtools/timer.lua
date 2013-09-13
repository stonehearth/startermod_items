local Timer = class()

Timer.WALL_CLOCK = 1
Timer.CPU_TIME = 2

function Timer:__init(mode)
   assert(mode ~= nil)
   self.mode = mode
   self:reset()
end

function Timer:reset()
   self.start_time = nil
   self.stop_time = nil
end

function Timer:start()
   self:reset()
   self.start_time = self:_get_time()
end

function Timer:stop()
   self.stop_time = self:_get_time()
end

function Timer:seconds()
   if not self.start_time then return 0 end

   if self.stop_time then
      return self.stop_time - self.start_time
   else
      return self:_get_time() - self.start_time
   end
end

function Timer:_get_time()
   if self.mode == Timer.CPU_TIME then
      return os.clock()
   else
      return os.time()
   end
end

return Timer
