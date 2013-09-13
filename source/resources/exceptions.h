#ifndef _RADIANT_RESOURCES_EXCEPTIONS_H
#define _RADIANT_RESOURCES_EXCEPTIONS_H

#include "namespace.h"

BEGIN_RADIANT_RESOURCES_NAMESPACE

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

class InvalidFilePath : public Exception {
public:
   InvalidFilePath(std::string const& path) {
      std::ostringstream format;
      format << "invalid file path '" << path << "'.";
      SetError(format.str());
   }
};

class InvalidJsonAtPathException : public Exception {
public:
   InvalidJsonAtPathException(std::string const& path) {
      std::ostringstream format;
      format << "invalid json at path '" << path << "'.";
      SetError(format.str());
   }
};

class InvalidResourceException : public Exception {
public:
   InvalidResourceException(std::string const& path, std::string const& why) {
      std::ostringstream format;
      format << "invalid resource '" << path << "'. " << why << ".";
      SetError(format.str());
   }
};

END_RADIANT_RESOURCES_NAMESPACE

#endif // _RADIANT_RESOURCES_EXCEPTIONS_H
