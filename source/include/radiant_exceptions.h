#ifndef _RADIANT_EXCEPTIONS_H
#define _RADIANT_EXCEPTIONS_H

#include <exception>
#include <string>

namespace radiant {
   namespace core {

      class Exception : public std::exception {
      public:
         Exception() { }
         Exception(std::string const& error) : error_(error) {}

         void SetError(std::string const& error) {
            error_ = error;
         }
         const char* what() const throw() override {
            return error_.c_str();
         }

      private:
         std::string       error_;
      };

      typedef Exception InvalidArgumentException;
      typedef Exception InvalidDataException;
   };

   // Does not inherit from 'exception', so that only the top-level handler that catches '...' 
   // will be able to catch this.
   class CrashException {
   };
};
#endif // _RADIANT_EXCEPTIONS_H
