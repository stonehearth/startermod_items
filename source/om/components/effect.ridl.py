from ridl.om import *
from ridl.ridl import *

class Effect(dm.Record):
   name = dm.Boxed(string, set=None, trace=None)
   start_time = dm.Boxed(int, set=None, trace=None)
   params = dm.Map(string, Selection, get=None, set=None, trace=None)

   _public_methods = """
   void Init(std::string name, int start)
   {
      name_ = name;
      start_time_ = start;
   }

   
   void AddParam(std::string param, luabind::object o);
   const Selection& GetParam(std::string param) const
   {
      static const Selection null;
      auto i = params_.find(param);
      return i != params_.end() ? i->second : null;
   }
   """

   _private_methods = """
   void UpdateDerivedValues();
   void ComputeAdjacentRegion(csg::Region3 const& r);
   """
