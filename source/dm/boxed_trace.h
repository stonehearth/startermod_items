#ifndef _RADIANT_DM_BOXED_TRACE_H
#define _RADIANT_DM_BOXED_TRACE_H

#include "dm.h"
#include "trace_impl.h"


BEGIN_RADIANT_DM_NAMESPACE

template <typename BoxedType>
class BoxedTrace : public TraceImpl<BoxedTrace<BoxedType>>
{
public:
   typedef typename BoxedType::Value   Value;
   typedef std::function<void(Value const& v)> ChangedCb;

public:
   BoxedTrace(const char* reason, BoxedType const& b);
   std::shared_ptr<BoxedTrace> OnChanged(ChangedCb changed);

protected:
   void SignalChanged(Value const& value);
   void SignalObjectState() override;
   ObjectId GetObjectId() const;
   Store const& GetStore() const;

protected:
   friend Store;
   virtual void NotifyChanged(Value const& value) = 0;

private:
   ChangedCb         changed_;
   BoxedType const&  boxed_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_BOXED_TRACE_H

