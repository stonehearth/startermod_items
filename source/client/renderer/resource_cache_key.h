#ifndef _RADIANT_CLIENT_RENDERER_RESOURCE_CACHE_KEY_H
#define _RADIANT_CLIENT_RENDERER_RESOURCE_CACHE_KEY_H

#include <functional>
#include "namespace.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class ResourceCacheKey {
   public:
      ResourceCacheKey() {
         _hash = std::hash<std::string>()(_key);
      }
      
      template <typename T> void AddElement(std::string const& name, T const& value) {
         _key += BUILD_STRING(name << "=" << value << ",");
         _hash = std::hash<std::string>()(_key);
      }

      std::string const& GetDescription() const {
         return _key;
      }

      size_t GetHash() const {
         return _hash;
      }

      bool operator==(ResourceCacheKey const& o) const {
         return _hash == o._hash;
      }

      struct Hash { 
         size_t operator()(const ResourceCacheKey& o) const {
            return o.GetHash();
         }
      };
      
   private:
      std::string       _key;
      size_t            _hash;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDERER_RESOURCE_CACHE_KEY_H
