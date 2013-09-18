#ifndef _RADIANT_RES_EXCEPTIONS_H
#define _RADIANT_RES_EXCEPTIONS_H

#include "namespace.h"
#include <boost/network/uri/uri.hpp>

BEGIN_RADIANT_RES_NAMESPACE

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

class InvalidUriException : public Exception {
public:
   InvalidUriException(boost::network::uri::uri const& uri) {
      std::ostringstream format;
      format << "invalid uri '" << uri.string() << "'.";
      SetError(format.str());
   }
};

class InvalidFilePath : public Exception {
public:
   InvalidFilePath(std::string const& path) {
      std::ostringstream format;
      format << "invalid file path '" << path << "'.";
      SetError(format.str());
   }
};

class InvalidJsonAtUriException : public Exception {
public:
   InvalidJsonAtUriException(boost::network::uri::uri const& uri) {
      std::ostringstream format;
      format << "json at uri '" << uri.string() << "' is not valid.";
      SetError(format.str());
   }
};

class InvalidResourceException : public Exception {
public:
   InvalidResourceException(boost::network::uri::uri const& uri, std::string const& why) {
      std::ostringstream format;
      format << "invalid resource '" << uri.string() << "'. " << why << ".";
      SetError(format.str());
   }
};

END_RADIANT_RES_NAMESPACE

#endif // _RADIANT_RES_EXCEPTIONS_H
