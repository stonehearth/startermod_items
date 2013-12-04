#include "radiant.h"
#include "object.h"
#include "dbg_indenter.h"

using namespace ::radiant;
using namespace ::radiant::dm;

Indenter::Indenter(std::ostream& os) : 
   rdbuf_(os.rdbuf()),
   indent_(3, ' '),
   owner_(os),
   last_ch_('\n')
{      
   owner_.rdbuf(this);
}

Indenter::~Indenter()
{
   owner_.rdbuf(rdbuf_);
}

int Indenter::overflow(int ch)
{
   if (ch != '\n' && last_ch_ == '\n') {
      rdbuf_->sputn(indent_.data(), indent_.size());
   }
   last_ch_ = ch;
   return rdbuf_->sputc(ch);
}
