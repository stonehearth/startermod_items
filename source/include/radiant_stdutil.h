#ifndef _RADIANT_STDUTIL_H
#define _RADIANT_STDUTIL_H

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <hash_map>
#include <set>
#include <list>
#include <memory>
#include <map>
#include <functional>

namespace radiant {

   namespace strutil {
      template <class S1, class S2> inline bool endsWith(S1 const& subject, S2 const& suffix)
      {
         return subject.size() >= suffix.size() && subject.substr(subject.size() - suffix.size()) == suffix;
      }

      static inline std::string replace(std::string const& str, std::string const& oldStr, std::string const& newStr)
      {
         size_t pos = 0;
         std::string result = str;
         while((pos = result.find(oldStr, pos)) != std::string::npos)
         {
            result.replace(pos, oldStr.length(), newStr);
            pos += newStr.length();
         }
         return result;
      }
      static inline std::vector<std::string> split(std::string const& s, std::string const& by) {
         std::vector<std::string> parts;
         boost::split(parts, s, boost::is_any_of(by));
         parts.erase(std::remove(parts.begin(), parts.end(), ""), parts.end());
         return std::move(parts);
      }
      static inline std::string join(std::vector<std::string> const& parts, std::string const& with) {
         return std::move(boost::algorithm::join(parts, with));
      }

      static inline std::wstring utf8_to_unicode(std::string const& s) {
         int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
         std::wstring unicode;
         unicode.resize(len);
         MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, (LPWSTR)unicode.data(), unicode.capacity());
         return unicode;
      }
   }

   namespace stdutil {
     
      template <class Other, class Iterator> class IteratorWrapper {
      public:
         IteratorWrapper(typename Iterator i) : i_(i) { }

         bool operator !=(const IteratorWrapper &other) const {
            return i_ != other.i_;
         }

         Other operator *() const {
            return Other(*i_);
         }

         const IteratorWrapper & operator++() {
            ++i_;
            return *this;
         }
      protected:
         typename Iterator     i_;
      };

      template <class Other, class Container, class Iterator> class RangeBasedForLoopHelper {
      public:
         RangeBasedForLoopHelper(typename Container &obj) : obj_(obj) { }

         IteratorWrapper<Other, Iterator> begin() { return IteratorWrapper<Other, Iterator>(obj_.begin()); }
         IteratorWrapper<Other, Iterator> end() { return IteratorWrapper<Other, Iterator>(obj_.end()); }

      protected:
         typename Container &      obj_;
      };

      template <class T, class K> bool UniqueInsert(std::vector<T> &container, const K& element) {
         if (std::find(container.begin(), container.end(), element) == container.end()) {
            container.push_back(element);
            return true;
         }
         return false;
      }

      template <class T> bool UniqueInsert(std::vector<std::weak_ptr<T>> &container, std::shared_ptr<T> const& element) {
         bool found = false;
         uint i = 0, c = container.size();
         while (i < c) {
            std::shared_ptr<T> entry = container[i].lock();
            if (entry) {
               if (entry == element) {
                  found = true;
                  break;
               }
               i++;
            } else {
               container[i] = container[--c];
            }
         }
         container.resize(c);
         if (!found) {
            container.emplace_back(element);
         }
         return found;
      }

      template <class T, class K> bool UniqueRemove(std::vector<T> &container, const K& element) {
         ASSERT(std::count(container.begin(), container.end(), element) <= 1);

         int size = container.size();
         for (int i = 0; i < size; i++) {
            if (container[i] == element) {
               container[i] = container[size - 1];
               container.resize(size - 1);
               return true;
            }
         }
         return false;
      }

      // does not compress...
      template <class T, class K> bool UniqueRemove(std::vector<T> &container, const std::weak_ptr<K>& element) {
         std::shared_ptr<K> e = element.lock();
         if (e) {
            int size = container.size();
            for (int i = 0; i < size; i++) {
               auto ith = container[i].lock();
               if (ith == e) {
                  container[i] = container[size - 1];
                  container.resize(size - 1);
                  return true;
               }
            }
         }
         return false;
      }

      template <class T, class K> int FastRemove(std::vector<T> &container, K const& element) {
         int c = 0;
         int size = container.size();
         for (int i = 0; i < size - c; ) {
            if (container[i] == element) {
               container[i] = container[size - 1];
               c++;
            } else {
               i++;
            }
         }
         container.resize(size - c);
         return c;
      }

      // also compresses nulls out of the std::vector...
      template <class K> int FastRemove(std::vector<std::weak_ptr<K>> &container, std::shared_ptr<K> const& e) {
         int c = container.size();
         if (e) {
            for (int i = 0; i < c; ) {
               auto ith = container[i].lock();
               if (!ith || ith == e) {
                  container[i] = container[--c];
               } else {
                  i++;
               }
            }
            container.resize(c);
         }
         return c;
      }

      template <class K> int FastRemove(std::vector<std::weak_ptr<K>> &container, std::weak_ptr<K> const& element) {
         return FastRemove<K>(container, element.lock());
      }

      

      template <class K, class V> void RemoveValue(std::map<K, V> &container, const K &item) {
         for (auto i = container.begin(); i != container.end(); ++i) {
            if (i->second == item) {
               container.erase(i);
            }
         }
      }

      template <class K, class V> bool contains(const std::map<K, V> &container, const K &item) {
         return container.find(item) != container.end();
      }

      template <class K, class V> bool contains(const std::unordered_map<K, V> &container, const K &item) {
         return container.find(item) != container.end();
      }

      template <class K, class V, typename H> bool contains(const std::hash_map<K, V, H> &container, const K &item) {
         return container.find(item) != container.end();
      }

      template <class V> bool contains(const std::set<V> &container, const V &item) {
         return container.find(item) != container.end();
      }

      template <class V> bool contains(const std::vector<V> &container, const V &item) {
         return std::find(container.begin(), container.end(), item) != container.end();
      }

      template <class V> bool contains(const std::vector<std::weak_ptr<V>> &container, const std::weak_ptr<V> &item) {
         auto e = item.lock();
         if (!e) {
            return false;
         }
         for (auto i : container) {
            auto ith = i.lock();
            if (ith == e) {
               return true;
            }
         }
         return false;
      }

      template <typename V, typename I> bool contains(const V &container, const I &item) {
         return container.find(item) != container.end();
      }

      template <class C> typename C::value_type pop_front(C &container) {
         auto value = container.front();
         container.pop_front();
         return value;
      }
      

      template <class K> void ForEachPrune(std::list<std::weak_ptr<K>> &container, std::function <void (std::shared_ptr<K>)> fn) {
         for (auto i = container.begin(); i != container.end(); ) {
            std::shared_ptr<K> p = i->lock();
            if (!p) {
               i = container.erase(i);
            } else {
               fn(p);
               ++i;
            }
         }
      }

      template <class K> void ForEachPrune(std::vector<std::weak_ptr<K>> &container, std::function <void (std::shared_ptr<K>)> fn) {
         int i = 0, size = container.size();
         for (int i = 0; i < size; ) {
            std::shared_ptr<K> p = container[i].lock();
            if (!p) {
               container[i] = container[size - 1];
               size--;
            } else {
               fn(p);
               ++i;
            }
         }
         container.resize(size);
      }

      template <class K> void ForEachPrune(std::set<std::weak_ptr<K>> &container, std::function <void (std::shared_ptr<K>)> fn) {
         for (auto i = container.begin(); i != container.end(); ) {
            std::shared_ptr<K> p = i->lock();
            if (!p) {
               i = container.erase(i);
            } else {
               fn(p);
               i++;
            }
         }
      }

      template <class K, class V> void ForEachPrune(std::unordered_multimap<K, std::weak_ptr<V>> &container, std::function <void (K const&, std::shared_ptr<V>)> fn) {
         for (auto i = container.begin(); i != container.end(); ) {
            std::shared_ptr<V> p = i->second.lock();
            if (!p) {
               i = container.erase(i);
            } else {
               fn(i->first, p);
               ++i;
            }
         }
      }

      template <class T> std::string ToString(const T& t) {
         std::stringstream ss;
         ss << t;
         return ss.str();
      }
   }
};

#endif // _RADIANT_STDUTIL_H
