#ifndef _RADIANT_OM_SELECTION_H
#define _RADIANT_OM_SELECTION_H

#include "om.h"
#include "dm/dm.h"
#include "math3d.h"
#include "protocols/store.pb.h"
#include "protocols/tesseract.pb.h"
#include "radiant_luabind.h"

BEGIN_RADIANT_OM_NAMESPACE

class Selection {
public:
   Selection();

   Selection(std::string str) {
      flags_ = HAS_STRING;
      string_ = str;
   }

   Selection(math3d::ipoint3 block) {
      flags_ = HAS_BLOCK;
      block_ = block;
   }

   Selection(int number) {
      flags_ = HAS_NUMBER;
      number_ = number;
   }

   template <class T> Selection(std::shared_ptr<T> obj) {
      flags_ = HAS_OBJECT;
      objectType_ = obj->GetObjectType();
      objectId_ = obj->GetObjectId();
   }   
   
   void Clear();

   void AddBlock(const math3d::ipoint3& location);
   bool HasBlock() const;
   const math3d::ipoint3& GetBlock() const;

   bool HasLocation() const;
   void AddLocation(const math3d::point3& location, const math3d::vector3& normal);
   const math3d::vector3& GetNormal();
   
   bool HasEntities();
   void AddEntity(const om::EntityId);
   const std::vector<om::EntityId>& GetEntities() { return entities_; }
   
   bool HasBounds() const { return (flags_ & HAS_BOUNDS) != 0; }
   void SetBounds(const math3d::ibounds3& bounds);
   const math3d::ibounds3& GetBounds() const { return bounds_; }

   void AddString(std::string s);
   std::string GetString() const { return (flags_ & HAS_STRING) ? string_ : std::string(); }
   void AddNumber(int number);

   //luabind::object ToLuaObject();
   void FromLuaObject(luabind::object o);

   void SaveValue(Protocol::Selection* msg) const;
   void LoadValue(const Protocol::Selection& msg);

   std::ostream& Format(std::ostream& out) const;

private:
   enum Flags {
      HAS_BLOCK      = (1 << 0),
      HAS_LOCATION   = (1 << 2),
      HAS_BOUNDS     = (1 << 3),
      HAS_OBJECT     = (1 << 4),
      HAS_STRING     = (1 << 5),
      HAS_NUMBER     = (1 << 6),
   };

private:  
   int               flags_;
   int               number_;
   math3d::ipoint3   block_;
   math3d::point3    location_;
   math3d::vector3   normal_;
   math3d::ibounds3  bounds_;
   std::string       string_;
   std::vector<om::EntityId> entities_;

   dm::ObjectType    objectType_;
   dm::ObjectId      objectId_;
};

static std::ostream& operator<<(std::ostream& out, const Selection& source) { return source.Format(out); }

END_RADIANT_OM_NAMESPACE

IMPLEMENT_DM_EXTENSION(::radiant::om::Selection, Protocol::Selection::extension)

#endif //  _RADIANT_OM_SELECTION_H
