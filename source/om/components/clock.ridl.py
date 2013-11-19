from ridl.om import *
from ridl.ridl import *

class Clock(Component):
   _generate_construct_object = True

   time = dm.Boxed(int)
