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
         COMPELLED_BEHAVIOR = 10000,
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
         CLEAR              = 3,
         CONSTRUCT_BUILDING = 5,
         TEARDOWN_BUILDING  = 6,
         BUILD_LADDER       = 7,
         RESTOCK_FROM_BACKPACK = 7,
         PLACE_ITEM         = 8,
         LOOT_ITEM          = 8,
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
         PARTY_AGGRESSIVE_FORMATION = 1,
         IDLE                       = 5,
         ACTIVE                     = 10,
         PARTY_PASSIVE_FORMATION    = 15,
         PARTY_TASK                 = 16,
         PANIC                      = 20,
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
         RAID_STOCKPILE = 20,
         DEPART_WORLD = 30,

         hold_formation = {
            IDLE_IN_FORMATION = 1,
            RETURN_TO_FORMATION = 10
         }
      },

      compelled_behavior = {
         STRUGGLE = 4,
         HIT_STUN = 100,
         IMPRISONED = 200, 
         DEPART_WORLD = 1000
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
      },
      brushes = radiant.resources.load_json('stonehearth:build:brushes')      
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
      BEDTIME_START = 3,
      BEDTIME_THRESHOLD = 8,  -- if sleepiness >= than this, will go to sleep at bedtime.
      MINS_TO_WALK_TO_BED = 20, -- Number of minutes to account for walking to the bed.
      TIRED = 22,          -- will try to sleep in a bed
      EXHAUSTION = 24,     -- will collapse on the ground
      MAX_SLEEPINESS = 25, 
      HOURLY_SLEEPINESS = 1,
      MIN_SLEEPINESS = 0
   },

   think_priorities = {
      HEALTH = 1000,
      ALERT  = 900,         
      SLEEPY = 100,
      HUNGRY = 50,
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

   combat = {
      ALLY_AGGRO_RATIO = 0.50,
      DEFAULT_PANIC_THRESHOLD = 0.25
   },

   mining = {
      XZ_CELL_SIZE = 4,
      Y_CELL_SIZE = 5,
   },

   hydrology = {
      PRESSURE_TO_FLOW_RATE = 1,        -- constant converting pressure to a flow rate per unit cross section
      STOP_FLOW_THRESHOLD = 0.01,       -- a 'drop' of water. don't flow below this threshold to avoid immaterial calculations
      MIN_FLOW_RATE = 0.1,              -- minimum flow rate through a channel to avoid exponentially long merge times
      WETTING_VOLUME = 0.25,            -- the volume of water consumed to make a block wet
      MERGE_ELEVATION_THRESHOLD = 0.1,  -- how close the water levels have to be before allowing a standard merge
      MERGE_VOLUME_THRESHOLD = 1,       -- how much water has to flow to equalize water levels for a merge
      DEFAULT_EDGE_AREA_LIMIT = 64,     -- how large a water body can grow on a flat plain
      PLAYER_EDGE_AREA_LIMIT = 256,     -- how large a water body can grow in a region mined by the player
   },

   ai = {
      -- the following activities will 'slow start', meaning the execution unit which was chosen
      -- previously will get the first crack at running before the others are allowed to go.  this
      -- applies to every execution unit run by the frame at this level.
      SLOW_START_ACTIVITIES = {
         ['stonehearth:simple_labor'] = true,
         ['stonehearth:mining'] = true,
      }
   },

   shop = {
      RESALE_CONSTANT = 0.6
   },

   attribute_effects = {
      MUSCLE_MELEE_MULTIPLIER = 0.01,   -- Multiply an entity's mucle attribute by this to get multiplier on top of base damage.
      MUSCLE_MELEE_MULTIPLIER_BASE = 1,   -- Add melee multiplier and melee multipler base to get full multiplier
      INVENTIVENESS_CRAFTING_FINE_THRESHOLD = 20,   -- Inventiveness must be greater than or equal to this to get the bonus multiplier.
      INVENTIVENESS_CRAFTING_FINE_MULTIPLIER = 0.5,   -- Multiply an entity's inventiveness attribute by this, and then add that to the percentage fine chance when crafting.
      DILIGENCE_WORK_UNITS_THRESHOLD = 3, -- Craftables with work units above this threshold will get a diligence deduction
      DILIGENCE_WORK_UNITS_REDUCTION_MULTIPLER = 0.006, -- Multiple entity's diligence attribute by this to get work unit deduction percentage
      CURIOSITY_NEW_CRAFTABLE_MULTIPLER = 0.05, -- Multiply entity's curiosity attribute by this to get multiplier for xp earned. Min multiplier is 1,
      STAMINA_SLEEP_ON_GROUND_OKAY_THRESHOLD = 55, -- If your stamina is >= this threshold, then you are not groggy from sleeping on the ground.
      COMPASSION_TRAPPER_TWO_PETS_THRESHOLD = 40, -- If a trapper's compassion is >= this threshold, he/she can have 2 pets
   },

   placement = {
      DEFAULT_ROTATION = 180
   }
}

constants.construction.DEFAULT_WOOD_FLOOR_BRUSH = constants.construction.brushes.voxel['wood resource'][1]
constants.construction.DEFAULT_WOOD_WALL_BRUSH   = constants.construction.brushes.wall['wood resource'][1]
constants.construction.DEFAULT_WOOD_COLUMN_BRUSH = constants.construction.brushes.column['wood resource'][1]
constants.construction.DEFAULT_WOOD_ROOF_BRUSH = constants.construction.brushes.roof['wood resource'][1]
constants.construction.DEFAULT_WOOD_PATTERN_BRUSH = constants.construction.brushes.pattern['wood resource'][1]
constants.construction.DEFAULT_STONE_FLOOR_BRUSH = constants.construction.brushes.voxel['stone resource'][1]
constants.construction.DEFAULT_STONE_WALL_BRUSH   = constants.construction.brushes.wall['stone resource'][1]
constants.construction.DEFAULT_STONE_COLUMN_BRUSH = constants.construction.brushes.column['stone resource'][1]
constants.construction.DEFAULT_STONE_ROOF_BRUSH = constants.construction.brushes.roof['stone resource'][1]
constants.construction.DEFAULT_STONE_PATTERN_BRUSH = constants.construction.brushes.pattern['stone resource'][1]

return constants
   