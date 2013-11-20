from ridl.om_types import *
import ridl.dm_types as dm
import ridl.std_types as std
import ridl.ridl as ridl

class Paperdoll(Component):
   slot_type = ridl.Enum('Slot',
      MAIN_HAND         = 0,
      WEAPON_SCABBARD   = 1,
      NUM_SLOTS         = 2,
   )
   slots = dm.Array(EntityRef(), slot_type.NUM_SLOTS, singular_name='slot')
