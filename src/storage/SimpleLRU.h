#ifndef AFINA_STORAGE_SIMPLE_LRU_H
#define AFINA_STORAGE_SIMPLE_LRU_H

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <iostream>
#include <chrono>
#include <thread>

#include <afina/Storage.h>

namespace Afina {
namespace Backend {

/**
 * # Map based implementation
 * That is NOT thread safe implementaiton!!
 */
class SimpleLRU : public Afina::Storage {

public:
    SimpleLRU(size_t max_size = 1024) : _max_size(max_size) {}
    SimpleLRU(const SimpleLRU& other) : _max_size(other._max_size) {}

    ~SimpleLRU() {
        _lru_index.clear();
        //_lru_head.reset(); // TODO: Here is stack overflow
        while(_lru_head.get() != nullptr){
            Delete_node(*_lru_head.get());
        }
    }

    // Implements Afina::Storage interface
    bool Put(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool PutIfAbsent(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Set(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Delete(const std::string &key) override;

    // Implements Afina::Storage interface
    bool Get(const std::string &key, std::string &value) override;


private:

    // LRU cache node
    using lru_node = struct lru_node {
        const std::string key;
        std::string value;
        lru_node* prev;
        std::unique_ptr<lru_node> next;
    };

    // Maximum number of bytes could be stored in this cache.
    // i.e all (keys+values) must be not greater than the _max_size
    std::size_t _max_size;
    std::size_t _using_size = 0;
    // Main storage of lru_nodes, elements in this list ordered descending by "freshness": in the head
    // element that wasn't used for longest time.
    //
    // List owns all nodes
    std::unique_ptr<lru_node> _lru_head= std::unique_ptr<lru_node>(nullptr);
    lru_node* _lru_tail = nullptr;

    // Index of nodes from list above, allows fast random access to elements by lru_node#key
    std::map<std::reference_wrapper<const std::string>, std::reference_wrapper<lru_node>, std::less<std::string>> _lru_index;

private:

    //Delete node from list
    void Delete_node(lru_node& node);
    //Adds node to head(tail) of list
    void Add_node(const std::string &key, const std::string &value);
    //Move existed node to tail
    void PushNodeToTail(lru_node& node);

    //Deleting nodes from head, until req_memory is not available
    void Delete_old_node(size_t req_memory);

    bool Set_and_push_tail(lru_node& node, const std::string &value);

};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_SIMPLE_LRU_H
