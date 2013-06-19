#include "radiant.h"
#include "radiant_json.h"
#include "radiant_luabind.h"

using namespace ::radiant;
using namespace ::radiant::json;

std::string ConstJsonObject::ToString() const
{
   return node_.write_formatted();
}

#if 0
template <class T>
void setSpecialNewIndex(lua_State* L, lua_CFunction onNewIndex) 
{ 
   using namespace luabind;

   detail::class_registry* reg  = detail::class_registry::get_registry(L); 
   detail::class_rep* rep = reg->find_class(typeid(T)); 
   //detail::class_rep* rep_audio = reg->find_class(typeid(audio::SAudioParameters)); 
   // LOG(INFO) << get_class_name( typeid(irr::SIrrlichtCreationParameters) ); 

   ASSERT(rep);

   LOG(WARNING) << "found rep for class '" << rep->name() << "'"; 
   LOG(WARNING) << "registry: cpp_class" << reg->cpp_class(); 
   LOG(WARNING) << "registry: cpp_instance" << reg->cpp_instance(); 
   LOG(WARNING) << "registry: lua_instance" << reg->lua_instance(); 
   LOG(WARNING) << "registry: lua_class" << reg->lua_class(); 

   int index = rep->metatable_ref(); 
   LOG(WARNING) << "class_rep: metatable_ref" << rep->metatable_ref(); 

   lua_pushinteger(L, index); 

   //Pushes onto the stack the value t[k], where t is the value at  the given valid
   // index and k is the value at the top of the stack. 
   // Pushes on top of stack the metatable 
   lua_gettable(L, LUA_REGISTRYINDEX); 
   // Returns 1 if the value at the given acceptable index is a table, 
   // and 0 otherwise. 
   LOG(WARNING) << "Is table (should be table )" << lua_istable( L, -1); 

   lua_pushstring( L , "__index"); 

   //    //class_rep::lua_settable_dispatcher 
   lua_pushcclosure(L, onNewIndex, 0); 
   // set new index 
   lua_rawset( L, -3); 

   // pop le tableau /metatable 
   lua_pop(L, 1); 
} 
#endif

/*

A for statement like

     for var_1, ···, var_n in explist do block end
is equivalent to the code:

     do
       local f, s, var = explist
       while true do
         local var_1, ···, var_n = f(s, var)
         var = var_1
         if var == nil then break end
         block
       end
     end
Note the following:

explist is evaluated only once. Its results are an iterator function, a state, and an initial value for the first iterator variable.
f, s, and var are invisible variables. The names are here for explanatory purposes only.
You can use break to exit a for loop.
The loop variables var_i are local to the loop; you cannot use their values after the for ends. If you need these values, then assign them to other variables before breaking or exiting the loop.

*/

static void PushNode(lua_State* L, JSONNode const& node)
{
   int type = node.type();
   if (type == JSON_NODE || type == JSON_ARRAY) {
      luabind::object(L, ConstJsonObject(node)).push(L);
   } else if (type == JSON_NUMBER) {
      lua_pushnumber(L, node.as_float());
   } else if (type == JSON_STRING) {
      lua_pushstring(L, node.as_string().c_str());
   } else if (type == JSON_BOOL) {
      lua_pushboolean(L, node.as_bool());
   }
}

class Iterator {
public:
   Iterator(JSONNode const& node) : node_(node), i_(node.begin()) {
      if (node.type() == JSON_ARRAY) {
         index_ = 0;
      }
   }

   virtual ~Iterator() {
      LOG(WARNING) << "destroying iterator...";
   }

   int NextIteration(lua_State *L) const {
      if (i_ != node_.end()) {
         if (node_.type() == JSON_ARRAY) {
            luabind::object(L, index_++).push(L);
         } else {
            luabind::object(L, i_->name()).push(L);
         }
         //luabind::object(L, std::string("happy fun times")).push(L);
         LOG(WARNING) << "node: " << i_->write_formatted();
         PushNode(L, *i_);
         ++i_;
         return 2;
      }
      return 0;
   }

private:
   NO_COPY_CONSTRUCTOR(Iterator);

private:
   JSONNode const& node_;
   mutable int index_;
   mutable JSONNode::const_iterator i_;
};

static int ConstJsonObject_IteratorNext(lua_State *L) {
   using namespace luabind;

   try {
      Iterator* i = object_cast<Iterator*>(object(from_stack(L, -2)));
      return i->NextIteration(L);
   } catch (cast_failed&) {
   }
   return 0;
}

static void ConstJsonObject_IteratorStart(lua_State *L, ConstJsonObject const& o)
{
   using namespace luabind;

   Iterator *iter = new Iterator(o.GetNode());
   lua_pushcfunction(L, &ConstJsonObject_IteratorNext); // f
   object(L, iter).push(L); // s
   object(L, 1).push(L);    // var (ignored)
}

void ConstJsonObject_Get(lua_State *L, ConstJsonObject const& o, luabind::object key)
{
   using namespace luabind;

   JSONNode const& node = o.GetNode();
  
   if (type(key) == LUA_TSTRING) {
      const char *name = object_cast<const char*>(key);
      auto i = node.find(name);
      if (i != node.end()) {
         PushNode(L, *i);
      }
   } else if (type(key) == LUA_TNUMBER) {
      int i = object_cast<int>(key) - 1;
      if (i >= 0 && i < (int)node.size()) {
         PushNode(L, node.at(i));
      }
   } else {
      lua_pushnil(L);
   }
}

void ConstJsonObject_Len(lua_State *L, ConstJsonObject const& o)
{
   lua_pushnumber(L, o.GetNode().size());
}

void ConstJsonObject::RegisterLuaType(struct lua_State* L)
{
   using namespace luabind;

   module(L) [
      class_<ConstJsonObject>("ConstJsonObject")
         .def(tostring(self))
         .def("contents",        &ConstJsonObject_IteratorStart)
         .def("get",             &ConstJsonObject_Get)
         .def("len",             &ConstJsonObject_Len)
      ,
      class_<Iterator, Iterator*>("ConstJsonObjectIterator")
   ];
}
