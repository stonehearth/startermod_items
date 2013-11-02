local constants = {
   -- Priorities
   priorities = {

      -- Priorities for the worker task scheduler.  Tasks with higher priorities
      -- tend to get executed before the lower ones.  The priorities are only
      -- valid within the worker task scheduler, so feel free to use arbitrary
      -- numbers (xxx: this comment will be true once Tony implements the action
      -- priority system next week).
      worker_task = {
         DEFAULT            = 2,
         RESTOCK_STOCKPILE  = 2,
         CHOP_TREE          = 4,
         CONSTRUCT_BUILDING = 7,
         PLACE_ITEM         = 8,
         LIGHT_FIRE         = 10,
      }
   },

   -- Constants related to constructing buildings
   construction = {
      STOREY_HEIGHT = 6,
      MAX_WALL_SPAN = 8,
   },

   -- Constants for the worker scheduler tasks
   worker_task = {
      -- The number of ticks to wait before picking the best solution to dispatch
      DISPATCHER_WAIT_TIME = 3,
   }
}

return constants
