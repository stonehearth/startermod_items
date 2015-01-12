local constants = {
   -- Priorities
   priorities = {
      -- Priorities for the worker task scheduler.  Tasks with higher priorities
      -- tend to get executed before the lower ones.  The priorities are only
      -- valid within the worker task scheduler, so feel free to use arbitrary
      -- numbers (xxx: this comment will be true once Tony implements the action
      -- priority system next week).
      -- Note: use these when the task is created, not in the action header
      --TODO: move all priorities from here to the ones under top, remove worker_task and farmer_task
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

      -- Priorities for the top level of action categories
      top = {
         IDLE = 1,
         FREE_TIME = 2,
         AMBIENT_PET_BEHAVIOR = 10,
         CRAFT = 15,
         URGENT_ACTIONS = 80,
         COMBAT = 100,
         UNIT_CONTROL = 1000,
         COMPELLED_BEHAVIOR = 9999999,
         DIE = 10000000,
      },

      work = {    --Categories of work. Represented as dispatchers.
         SIMPLE_LABOR      = 1,    --basic tasks traditionally the domain of workers
         PATROLLING        = 2,
         FARMING           = 2,
         TRAPPING          = 2,
         MINING            = 2,
         HERDING           = 2,
         CRAFTING          = 10,
         UPGRADE_EQUIPMENT = 20,
      },

      simple_labor = {
         DEFAULT            = 2,
         RESTOCK_STOCKPILE  = 2,
         CONSTRUCT_BUILDING = 5,
         TEARDOWN_BUILDING  = 6,
         BUILD_LADDER       = 7,
         PLACE_ITEM         = 8,
         LIGHT_FIRE         = 10,
      },

      farming = {
         PLANT              = 3,
         TILL               = 4,
         HARVEST            = 5,
      },

      trapping = {
         SURVEY_TRAPPING_GROUNDS = 1,
         SET_TRAPS = 2,
         UNLOAD_BACKPACK = 3,
         CHECK_TRAPS = 3,
         PICK_UP_LOOT = 10,
      },

      shepherding_animals = {
         GATHER_ANIMALS = 3, 
         RETURN_TO_PASTURE = 2, 
         HARVEST = 1,
      },

      mining = {
         MINE = 1,
      },

      crafting = {
         DEFAULT = 2,
      },

      -- basic needs
      basic_needs = {
         EAT = 20,
         SLEEP = 40
      },

      free_time = {
         ADMIRE_FIRE = 2
      },

      combat = {
         IDLE = 1,
         ACTIVE = 10,
         PANIC = 20,
      },

      urgent = {
         TOWN_DEFENSE = 1,
         RALLY = 2,
      },

      town_defense = {
         IDLE = 1,
      },

      -- Priorites of commands issued by the player to a unit
      unit_control = {
         MOVE = 2,
         DEFAULT = 10,
         CAST_SPELL = 100
      },

      party = {
         IDLE = 1,
         HOLD_FORMATION = 10,

         hold_formation = {
            IDLE_IN_FORMATION = 1,
            RETURN_TO_FORMATION = 10
         }
      },

      compelled_behavior = {
         STRUGGLE = 4,
         HIT_STUN = 100,
      }, 

      goblins = {
         HOARD = 5, 
         RUN_TOWARDS_SETTLEMENT = 4
      },
   },
   
   -- Constants related to constructing buildings
   construction = {
      STOREY_HEIGHT = 6,
      MAX_WALL_SPAN = 16,
      floor_category = {
         FLOOR = 'floor',
         ROAD = 'road',
         SLAB = 'slab',
         CURB = 'curb'
      }
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
      MEALTIME_START = 14
   },

   sleep = {
      BEDTIME_START = 1,
      EXHAUSTION = 24, 
      MAX_SLEEPINESS = 25, 
      HOURLY_SLEEPINESS = 1, 
      MIN_SLEEPINESS = 0
   },

   think_priorities = {
      ALERT  = 900,
      HEALTH = 1000,
      SLEEPY = 100,
      HUNGRY = 50
   },

   --Constants that contribute to the scoare
   score = {
      DEFAULT_MIN = 0,
      DEFAULT_MAX = 100,
      DEFAULT_VALUE = 50,
      NET_WORTH_INTERVAL = '10m',
      DEFAULT_CIV_WORTH = 10, 

      net_worth_categories = {  --TODO: edit these values! Just first cuts
         'camp',                --also, if you change these, update en.json too :)
         'settlement', 
         'hamlet', 
         'village', 
         'town', 
         'county', 
         'kingdom', 
         'empire'
      },

      category_threshholds = {
         camp = 50, 
         settlement = 100, 
         hamlet = 300, 
         village = 800, 
         town = 1000, 
         city = 4000, 
         metropolis = 6000
      },

      nutrition = {
         EAT_ONCE_TODAY = 10,
         SIMPLE_EATING_SCORE_CAP = 50,   --if score is already > than this, don't apply the eat once bonus
         HOMOGENEITY_THRESHHOLD = 3,     --apply eat same food penalty if we eat more than this number of foods in a row
         EAT_SAME_FOODS = -10,
         EAT_DIFFERENT_FOODS = 10,       --bonus if what we just at != what we ate right before that
         NO_FOOD_TODAY = -10,            --penalty if we've eaten nothing today, applied at midnight
         EAT_NUTRITIOUS_FOOD = 10,       --applied if the food is really satisfying
         NUTRITION_THRESHHOLD = 30,      --satisfaction has to be this or greater to get bonus
         MALNOURISHMENT_PENALTY = -10    --if malnourished, extra penalty once per day
      }, 
      shelter = {
         SLEEP_ON_GROUND = -10,  -- sleeping on the ground penalty
         SLEEP_IN_BED = 10,      -- sleeping in bed bonus
         BED_SCORE_CAP = 50,     -- up to a score of 50
      }, 
      safety = {
         PANIC_PENALTY = -10,
         PEACEFUL_DAY_BONUS = 5, 
         NEAR_DEATH_PENALTY = -10,
         FOOTMAN_BONUS = 5,        --this is per-footman
         TOWN_DEATH = -20,
      }
   },

   mining = {
      XZ_CELL_SIZE = 4,
      Y_CELL_SIZE = 5,
   },
}

return constants
   