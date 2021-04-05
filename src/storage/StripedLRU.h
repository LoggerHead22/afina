#ifndef AFINA_STORAGE_STRIPED_SIMPLE_LRU_H
#define AFINA_STORAGE_STRIPED_SIMPLE_LRU_H

#include <map>
#include <mutex>
#include <string>
#include <functional>

#include "ThreadSafeSimpleLRU.h"

namespace Afina {
namespace Backend {

/**
 * # StrippedLRU thread safe + no high contention version
 *
 *
 */
class StripedLRU: public Afina::Storage{

private:
    std::vector<ThreadSafeSimplLRU> _caches;
    std::hash<std::string> hasher;

public:

    static StripedLRU create_cache(size_t memory_limit = 64 * 1024 * 1024 , size_t shards_count = 8){
        if(memory_limit / shards_count < 1 * 1024 * 1024UL ){
            throw std::runtime_error("Error: Shards count is too large!");
        }
        return StripedLRU(memory_limit / shards_count, shards_count);
    }

    StripedLRU(size_t memory_limit_per_shards = 64 * 1024 * 1024, size_t shards_count = 8){
        for(size_t i = 0; i < shards_count; i++){
            //_caches.push_back(*(new ThreadSafeSimplLRU(memory_limit_per_shards)));
            _caches.emplace_back(memory_limit_per_shards);
        }
    }

    ~StripedLRU() {}

    // see ThreadSafeSimpleLRU.h
    bool Put(const std::string &key, const std::string &value) override{
        return _caches[hasher(key) % _caches.size()].Put(key, value);
    }

    // see ThreadSafeSimpleLRU.h
    bool PutIfAbsent(const std::string &key, const std::string &value) override{
        return _caches[hasher(key) % _caches.size()].PutIfAbsent(key, value);
    }

    // see ThreadSafeSimpleLRU.h
    bool Set(const std::string &key, const std::string &value) override{
        return _caches[hasher(key) % _caches.size()].Set(key, value);
    }

    // see ThreadSafeSimpleLRU.h
    bool Delete(const std::string &key) override{
        return _caches[hasher(key) % _caches.size()].Delete(key);
    }

    // see ThreadSafeSimpleLRU.h
    bool Get(const std::string &key, std::string &value) override{
        return _caches[hasher(key) % _caches.size()].Get(key, value);
    }

};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_STRIPED_SIMPLE_LRU_H
