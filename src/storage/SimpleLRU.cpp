#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    auto node_it = _lru_index.find(cref(key));
    size_t curr_size = key.size() + value.size(); //сколько хотим добавить

    if(curr_size > _max_size){//если пара не влезет
        return false;
    }
    if(node_it != _lru_index.end()){//если ключ уже есть, удалим старый
        lru_node& node = node_it->second.get();
        //удаляем из мапа
        _lru_index.erase(node_it);
        //удаляем из списка
        Delete_node(node);
    }
    Add_node(key, value);
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    auto node_it = _lru_index.find(cref(key));

    size_t curr_size = key.size() + value.size(); //сколько хотим добавить

    if(curr_size > _max_size){//если пара не влезет
        return false;
    }
    if(node_it != _lru_index.end()){
        return false;
    }
    Add_node(key, value);
    return true;
}

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
    _lru_index.erase(node_it);

    if(_using_size - node.value.size() + value.size() > _max_size){//если новое значение не влезет
        return false;
    }

    Delete_node(node);
    Add_node(key, value);

    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
    auto node_it = _lru_index.find(cref(key));
    if(node_it == _lru_index.end()){
        return false;
    }
    lru_node& node = node_it->second.get();
    //удаляем из мапа
    _lru_index.erase(node_it);
    //удаляем ноду
    Delete_node(node);
    return true;
}

void SimpleLRU::Delete_node(lru_node& node){
    //освободим доступную память
    _using_size -= node.key.size() + node.value.size();
    //всего 1 элемент
    if(node.next.get() == nullptr && node.prev==nullptr){
        _lru_head = nullptr;
        _lru_tail = nullptr;
    //хвост
    }else if(node.next.get() == nullptr){
        _lru_tail = node.prev;
        _lru_tail->next = nullptr;
    //голова
    }else if(node.prev == nullptr){
        _lru_head = std::move(node.next);
        _lru_head->prev = nullptr;
    }else{
        node.next->prev = node.prev;
        node.prev->next = std::move(node.next);
    }
}

void SimpleLRU::Add_node(const std::string &key, const std::string &value){
     lru_node* new_node = new lru_node {key,
                                        value,
                                        nullptr,
                                        nullptr};

    std::unique_ptr<lru_node> temp_node(new_node);

    size_t curr_size = key.size() + value.size();
    //теперь будем удалять из головы(?), пока места не освободится
    while(_using_size + curr_size > _max_size){
        _lru_index.erase(_lru_head->key);
        Delete_node(*_lru_head.get());
    }

    if(_lru_head.get() == nullptr && _lru_tail == nullptr){
        _lru_head = std::move(temp_node);
    }else if(_lru_tail != nullptr){
        _lru_tail->next = std::move(temp_node);
    }
    new_node->prev = _lru_tail;
    _lru_tail = new_node; //добавили в список

    _using_size += _lru_tail->key.size() + _lru_tail->value.size();

     //добавим в map
     _lru_index.emplace(std::ref(new_node->key), std::ref(*new_node));
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) {

    auto node_it = _lru_index.find(cref(key));
    if(node_it == _lru_index.end()){
        return false;
    }

    lru_node& node = node_it->second.get();
    value = node.value;
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
    return true;
}

} // namespace Backend
} // namespace Afina
