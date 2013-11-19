from ridl.om import *
from ridl.ridl import *

class Effect(object): pass

class EffectList(Component):
   effects = dm.Set(Ptr(Effect), name="effect")
