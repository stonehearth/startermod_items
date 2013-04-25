#ifndef _RADIANT_HTTPD_NAMESPACE_H
#define _RADIANT_HTTPD_NAMESPACE_H

#define BEGIN_RADIANT_HTTPD_NAMESPACE  namespace radiant { namespace httpd {
#define END_RADIANT_HTTPD_NAMESPACE    } }

#define RADIANT_HTTPD_NAMESPACE ::radiant::httpd

#define IN_RADIANT_HTTPD_NAMESPACE(x)      \
   BEGIN_RADIANT_HTTPD_NAMESPACE        \
   x                                    \
   END_RADIANT_HTTPD_NAMESPACE

#endif //  _RADIANT_HTTPD_NAMESPACE_H
