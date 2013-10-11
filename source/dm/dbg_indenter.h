#pragma once

#include <set>
#include "dm.h"

BEGIN_RADIANT_DM_NAMESPACE

class Indenter : public std::streambuf {
public:
   Indenter(std::ostream& os);
   ~Indenter();

public: // overrids of std::streambuf
   int overflow(int ch) override;
   
private:
   std::string       indent_;
   std::streambuf*   rdbuf_;
   std::ostream&     owner_;
   int               last_ch_;
};

END_RADIANT_DM_NAMESPACE
