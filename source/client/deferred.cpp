#include "pch.h"
#include "deferred.h"

using namespace ::radiant;
using namespace ::radiant::client;

Deferred::Deferred() :
   code_(0)
{
}

Deferred::~Deferred()
{
}

void Deferred::Always(std::function<void()> cb)
{
   if (code_ == 0) {
      alwaysCbs_.push_back(cb);
   } else {
      cb();
   }
}

void Deferred::SetResponse(int code)
{
   SetResponse(code, "", "");
}

void Deferred::SetResponse(int code, std::string const& data, std::string const& mimetype)
{
   code_ = code;
   data_ = data;
   mimeType_ = mimetype;
   Resolve();
}

void Deferred::Resolve()
{
   for (auto &cb : alwaysCbs_) {
      cb();
   }
   alwaysCbs_.clear();
}
