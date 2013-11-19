from ridl.om import *
from ridl.ridl import *

class CarryBlock(Component):
   carry_block = dm.Boxed(Ref(Entity))
   