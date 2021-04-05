#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) { return false; }

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) { return false; }

// See MapBasedGlobalLockImpl.h

bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    auto node_it = _lru_index.find(cref(key));

    size_t curr_size = key.size() + value.size(); //сколько хотим добавить

    if(curr_size > _max_size){//если пара не влезет
        return false;
    }
    if(node_it == _lru_index.end()){//если ключа нет
        return false;
    }

    lru_node& node = node_it->second.get();

    if(_using_size - node.value.size() + value.size() > _max_size){//если новое значение не влезет
        return false;
    }
    node.value = value;
    PushNodeToTail(node);

    return true;
}


// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) { return false; }

// See MapBasedGlobalLockImpl.h

bool SimpleLRU::Get(const std::string &key, std::string &value) {
//    std::this_thread::sleep_for(std::chrono::milliseconds(10000));

    auto node_it = _lru_index.find(cref(key));
    if(node_it == _lru_index.end()){
        return false;
    }

    lru_node& node = node_it->second.get();
    value = node.value;
    PushNodeToTail(node);
    return true;
}

void SimpleLRU::PushNodeToTail(lru_node& node){
    if(_lru_tail != &node){
        if(node.prev != nullptr){
            _lru_tail->next = std::move(node.prev->next);
            node.prev->next = std::move(node.next);
            node.prev->next->prev = node.prev;
        }else{
            _lru_tail->next = std::move(_lru_head);
            _lru_head = std::move(node.next);
            _lru_head->prev = nullptr;
        }
        node.prev = _lru_tail;
        _lru_tail = &node;
        _lru_tail->next.reset(nullptr);
    }
}


} // namespace Backend
} // namespace Afina
