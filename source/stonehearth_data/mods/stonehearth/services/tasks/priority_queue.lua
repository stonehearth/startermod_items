local PriorityQueue = class()

function PriorityQueue:__init(name, get_priority_cb)
   self._name = name

   -- the items table contains items lists bucketed by priority
   self._buckets = {}
   self._priorities = {}
   self._item_priorities = {}
   self._get_priority_cb = get_priority_cb

   -- for the round robin scheduler
   self._rr_pri = nil
   self._rr_tsk = nil

   self._count = 0
end

function PriorityQueue:remove(id)
   local pri = self._item_priorities[id]
   if pri then
      self._item_priorities[id] = nil
      local items = self:_get_bucket(pri)
      for i, item in ipairs(items) do
         if item:get_id() == id then
            self._log:debug('removing item %d from %s queue', id, self._name)
            table.remove(items, i)
            self._count = self._count - 1

            -- make sure we don't skip a task when the list currently running through
            -- the round robin iterator gets modified
            if pri == self._rr_pri and i <= self._rr_tsk then
               self._rr_tsk = self._rr_tsk - 1
            end
            assert(self._rr_tsk <= #self._buckets[self._rr_pri])

            return
         end
      end
   end
   self._log:error('cannot remove unknown item %d from %s queue', id, self._name)
end

function PriorityQueue:add(id, item)
   if not self._item_priorities[id] then
      local pri = self._get_priority_cb(item)
      self._item_priorities[id] = pri
      local items = self:_get_bucket(pri)
      self._log:debug('adding item %d to %s queue', id, self._name)
      table.insert(items, item)
      self._count = self._count + 1
      return
   end
   self._log:error('cannot add duplicate item %d to %s queue', id, self._name)
end

function PriorityQueue:contains(id)
   return self._item_priorities[id] ~= nil
end

function PriorityQueue:count()
   return self._count
end

function PriorityQueue:round_robin()
   if not self._rr_pri or self._count == 0 then
      return nil
   end
   
   local count = 0
   local get_next_task = function()
      if count >= self._count then
         return nil
      end

      local pri = self._priorities[self._rr_pri]
      local pri_len = #self._priorities
      local bucket = self._buckets[pri]
      local bucket_len = #bucket

      assert(self._rr_tsk <= bucket_len)
      local result = bucket[self._rr_tsk]

      self._rr_tsk = self._rr_tsk + 1
      if self._rr_tsk > bucket_len then
         repeat
            self._rr_pri = self._rr_pri + 1
            if self._rr_pri > pri_len then
               self._rr_pri = 1
            end
            bucket_len = self._buckets[self._rr_pri]
         until bucket_len > 0
         self._rr_tsk = 1
      end
   end
   return get_next_task
end

function PriorityQueue:_get_bucket(pri)
   local bucket = self._buckets[pri]
   if not bucket then
      bucket = {}
      self._buckets[pri] = bucket
      table.insert(self._priorities, pri)
      table.sort(self._priorities, function (a, b) return a > b end)
      if not self._rr_pri then
         self._rr_pri = 1
         self._rr_tsk = 1
      end
   end
   return bucket
end

return PriorityQueue

