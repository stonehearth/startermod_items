#ifndef _RADIANT_OM_SELECTION_H
#define _RADIANT_OM_SELECTION_H

#include "om.h"
#include "dm/dm.h"
#include "csg/cube.h"
#include "protocols/store.pb.h"
#include "protocols/tesseract.pb.h"
#include "lib/lua/bind.h"

BEGIN_RADIANT_OM_NAMESPACE

class Selection {
public:
   Selection();

   Selection(std::string str) {
      flags_ = HAS_STRING;
      string_ = str;
   }

   Selection(csg::Point3 block) {
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

   void AddBlock(const csg::Point3& location);
   bool HasBlock() const;
   const csg::Point3& GetBlock() const;

   bool HasLocation() const;
   csg::Point3f GetLocation() const;
   void AddLocation(const csg::Point3f& location, const csg::Point3f& normal);
   const csg::Point3f& GetNormal();
   
   bool HasEntities();
   void AddEntity(const dm::ObjectId);
   const std::vector<dm::ObjectId>& GetEntities() { return entities_; }
   
   bool HasBounds() const { return (flags_ & HAS_BOUNDS) != 0; }
   void SetBounds(const csg::Cube3& bounds);
   const csg::Cube3& GetBounds() const { return bounds_; }

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
   csg::Point3   block_;
   csg::Point3f    location_;
   csg::Point3f   normal_;
   csg::Cube3  bounds_;
   std::string       string_;
   std::vector<dm::ObjectId> entities_;

   dm::ObjectType    objectType_;
   dm::ObjectId      objectId_;
};

static std::ostream& operator<<(std::ostream& out, const Selection& source) { return source.Format(out); }

END_RADIANT_OM_NAMESPACE

IMPLEMENT_DM_EXTENSION(::radiant::om::Selection, Protocol::Selection::extension)

#endif //  _RADIANT_OM_SELECTION_H
