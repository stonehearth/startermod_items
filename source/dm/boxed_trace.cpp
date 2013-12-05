#include "radiant.h"
#include "store.h"
#include "boxed.h"
#include "boxed_trace.h"
#include "dm_save_impl.h"
#include "protocols/store.pb.h"

using namespace radiant;
using namespace radiant::dm;

template <typename B>
BoxedTrace<B>::BoxedTrace(const char* reason, B const& b) :
   TraceImpl(reason),
   boxed_(b)
{
}

template <typename B>
ObjectId BoxedTrace<B>::GetObjectId() const
{
   return boxed_.GetObjectId();
}

template <typename B>
Store const& BoxedTrace<B>::GetStore() const
{
   return boxed_.GetStore();
}

template <typename B>
std::shared_ptr<BoxedTrace<B>> BoxedTrace<B>::OnChanged(ChangedCb changed)
{
   changed_ = changed;
   return shared_from_this();
}

template <typename B>
void BoxedTrace<B>::SignalObjectState()
{
   SignalChanged(boxed_.Get());
}

template <typename B>
void BoxedTrace<B>::SignalChanged(Value const& value)
{
   SignalModified();
   if (changed_) {
      changed_(value);
   }
}

#define CREATE_BOXED(B)           template BoxedTrace<B>;
#include "types/all_boxed_types.h"
ALL_DM_BOXED_TYPES
