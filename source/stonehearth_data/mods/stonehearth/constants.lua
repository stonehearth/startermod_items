local constants = {
   -- Priorities
   priorities = {
      -- Priorities for the worker task scheduler.  Tasks with higher priorities
      -- tend to get executed before the lower ones.  The priorities are only
      -- valid within the worker task scheduler, so feel free to use arbitrary
      -- numbers (xxx: this comment will be true once Tony implements the action
      -- priority system next week).
      -- Note: use these when the task is created, not in the action header
      worker_task = {
         DEFAULT            = 2,
         RESTOCK_STOCKPILE  = 2,
         HARVEST            = 4,
         CONSTRUCT_BUILDING = 5,
         TEARDOWN_BUILDING  = 6,
         PLACE_ITEM         = 8,
         PLACE_WORKSHOP     = 8,
         GATHER_RESOURCE    = 8,
         GATHER_FOOD        = 9,
         LIGHT_FIRE         = 10,
      },

      farmer_task = {
         RESTOCK_STOCKPILE  = 2,
         PLANT              = 3,
         TILL               = 4,
         HARVEST            = 5,
      },

      -- Priorities for basic needs
      top = {
         IDLE = 1,
         FREE_TIME = 2,
         --ADMIRE_FIRE = 2,
         WORK = 10,
         AMBIENT_PET_BEHAVIOR = 10,
         CRAFT = 10,
         BASIC_NEEDS = 20,
         SLEEP = 40, -- todo: factor into basic_needs (just like eat...)
         COMBAT = 100,
         UNIT_CONTROL = 1000,
         COMPELLED_BEHAVIOR = 9999999,
         DIE = 10000000,
      },

      -- basic needs
      basic_needs = {
         EAT = 20,
      },

      free_time = {
         ADMIRE_FIRE = 2
      },

      combat = {
         IDLE = 1,
         ACTIVE = 10,
      },

      -- Priorites of commands issued by the player to a unit
      unit_control = {
         MOVE = 2,
         DEFAULT = 10,
         CAST_SPELL = 100
      },

      compelled_behavior = {
         STRUGGLE = 4,
         HIT_STUN = 100
      }
   },
   
   -- Constants related to constructing buildings
   construction = {
      STOREY_HEIGHT = 6,
      MAX_WALL_SPAN = 8,
   },

   input = {
      mouse = {
         dead_zone_size = 4
      }
   }, 

   soil_fertility = {
      MIN  =  1,
      POOR = 10,
      FAIR = 25,
      GOOD = 35,
      MAX  = 40,
      VARIATION = 10,
   },

   food = {
      MAX_ENERGY = 75, 
      MALNOURISHED = 0, 
      MIN_ENERGY = -25,
      HOURLY_ENERGY_LOSS = 1,
      HOURLY_HP_LOSS = 5,
      HOURLY_HP_GAIN = 10, 
      MEALTIME_START = 10
   },

   think_priorities = {
      HUNGRY = 100,
      SLEEPY = 50,
   },
}

return constants
   