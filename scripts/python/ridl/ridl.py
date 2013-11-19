
declare = 'declare'

class Ref(object):
   def __init__(self, type):
      self.type = type
      self.__name__ = "std::weak_ptr<%s>" % self.type.__name__

class Ptr(object):
   def __init__(self, type):
      self.type = type
      self.__name__ = "std::shared_ptr<%s>" % self.type.__name__

class string(object):
   pass
string.__name__ = 'std::string'