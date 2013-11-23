#ifndef _RADIANT_DM_OBJECT_MACROS_H
#define _RADIANT_DM_OBJECT_MACROS_H


#define DEFINE_DM_OBJECT_TYPE(Cls, lower)   \
   enum { DmType = Cls ## ObjectType }; \
   ObjectType GetObjectType() const override { return Cls::DmType; } \
   const char *GetObjectClassNameLower() const override { return #lower; } \
   std::shared_ptr<Cls ## Trace<Cls>> TraceChanges(const char* reason, int category) const ;

#endif //  _RADIANT_DM_OBJECT_MACROS_H
