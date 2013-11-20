import ridl

class string(ridl.Type):
   name = 'std::string'

class weak_ptr(ridl.Type):
   def __init__(self, type):
      self.name = "std::weak_ptr<%s>" % type.name

class shared_ptr(ridl.Type):
   def __init__(self, type):
      self.name = "std::shared_ptr<%s>" % type.name
