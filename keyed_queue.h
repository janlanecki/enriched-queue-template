#ifndef KEYED_QUEUE_H
#define KEYED_QUEUE_H

#include <map>
#include <list>
#include <memory>
#include <cstddef>
#include <utility>
#include <exception>

class lookup_error: public std::exception {
public:
  const char *what() {
    return "lookup error";
  }
};

template <class K, class V>
class keyed_queue {  
private:
  using CKey_Value = std::pair<K const &, V &>;
  using CKey_CValue = std::pair<K const &, V const &>;
  
  class base_queue {
  private:
    using queue_t = std::list<std::pair<const K*, V>>;
    using nodes_t = std::map<K, std::list<typename queue_t::iterator>>;
    using nodes_it_t = typename nodes_t::const_iterator;
    
    nodes_t nodes;
    queue_t queue;
    bool unshareable;
    
  public:
    base_queue() : unshareable(false) {
    }
    
    base_queue(base_queue const &b) : queue(b.queue), unshareable(false) {
      for (auto queue_it = queue.begin(); queue_it != queue.end(); ++queue_it)
        nodes[*(queue_it->first)].push_back(queue_it);
      for (auto nodes_it = nodes.begin(); nodes_it != nodes.end(); ++nodes_it)
        for (auto queue_it : nodes_it->second)
          queue_it->first = &(nodes_it->first);
    }
    
    bool get_unshareable() {
      return unshareable;
    }
    
    void set_unshareable() {
      unshareable = true;
    }
          
    void check_empty() const {
      if (queue.empty())
        throw lookup_error();
    }
  
    void check_no_key(K const& k) const {
      if (nodes.find(k) == nodes.end())
        throw lookup_error();
    }
    
    void check_nodes_iterator(nodes_it_t const &it) const {
      if (it == nodes.end())
        throw lookup_error();
    }
    
    void push(K const &, V const &);
    void pop();
    void pop(K const &);
    void move_to_back(K const &);
    
    CKey_Value front() {
      return CKey_Value(*(queue.front().first), queue.front().second);
    }
    
    CKey_Value back() {
      return CKey_Value(*(queue.back().first), queue.back().second);
    }
    
    CKey_CValue front() const {
      return CKey_CValue(*(queue.front().first), queue.front().second);
    }
    
    CKey_CValue back() const {
      return CKey_CValue(*(queue.back().first), queue.back().second);
    }
    
    CKey_Value first(K const &k) {
      auto nodes_it = nodes.find(k);
      return CKey_Value(*(nodes_it->second.front()->first), nodes_it->second.front()->second);
    }
    
    CKey_Value last(K const &k) {
      auto nodes_it = nodes.find(k);
      return CKey_Value(*(nodes_it->second.back()->first), nodes_it->second.back()->second);
    }
    
    CKey_CValue first(K const &k) const {
      auto nodes_it = nodes.begin();
      return CKey_Value(*(nodes_it->second.front()->first), nodes_it->second.front()->second);
    }
    
    CKey_CValue last(K const &k) const {
      auto nodes_it = --nodes.end();
      return CKey_Value(*(nodes_it->second.back()->first), nodes_it->second.back()->second);
    }
    
    size_t size() const noexcept {
      return queue.size();
    }
    
    bool empty() const noexcept {
      return queue.empty();
    }
    
    void clear() noexcept {
      nodes.clear();
      queue.clear();
    }
    
    size_t count(K const &k) const {
      auto nodes_it = nodes.find(k);
      if (nodes_it == nodes.end())
        return 0;
      return nodes_it->second.size();
    }
    
    class k_iterator {
    friend class base_queue;
    
    private:
      nodes_it_t iterator;
      
      k_iterator(nodes_it_t it) : iterator(it) {}
      
    public:
      k_iterator() {
      }
      
      k_iterator(const k_iterator &k) : iterator(k.iterator) {
      }
      
      k_iterator& operator++() noexcept {
        ++iterator;
        return *this;
      }
      
      bool operator==(k_iterator const &k) const noexcept {
        return iterator == k.iterator;
      }
      
      bool operator!=(k_iterator const &k) const noexcept {
        return !(*this == k);
      }
      
      const K& operator*() const noexcept {
        return iterator->first;
      }

    };
    
    k_iterator k_begin() const {
      return k_iterator(nodes.begin());
    }
    
    k_iterator k_end() const {
      return k_iterator(nodes.end());
    }
  
  };

  std::shared_ptr<base_queue> queue_ptr;
  
  std::shared_ptr<base_queue> get_base_queue_ptr() {
    if (queue_ptr.use_count() > 1) {
      return std::make_shared<base_queue>(*queue_ptr);
    }
    else {
      queue_ptr->set_unshareable();
      return queue_ptr;
    }
  }
  
public:
  using k_iterator = typename base_queue::k_iterator;
  
  keyed_queue() : queue_ptr(std::make_shared<base_queue>()) {
  }

  keyed_queue(keyed_queue const &k) {
    if (queue_ptr->get_unshareable()) {
      queue_ptr = std::make_shared<base_queue>(*(k.queue_ptr));
    } else {
      queue_ptr = k.queue_ptr;
    }
  }
  
  keyed_queue(keyed_queue &&k) noexcept : queue_ptr(k.queue_ptr) {
  }
  
  keyed_queue &operator=(keyed_queue o) {
    queue_ptr.swap(o.queue_ptr);
    return *this;
  }
  
  void push(K const &k, V const &v) {
    auto ptr = get_base_queue_ptr();
    ptr->push(k, v);
    queue_ptr.swap(ptr);
  }

  void pop() {
    auto ptr = get_base_queue_ptr();
    ptr->pop();
    queue_ptr.swap(ptr);
  }

  void pop(K const &k) {
    auto ptr = get_base_queue_ptr();
    ptr->pop(k);
    queue_ptr.swap(ptr);
  }

  void move_to_back(K const &k) {
    auto ptr = get_base_queue_ptr();
    ptr->move_to_back(k);
    queue_ptr.swap(ptr);
  }

  CKey_Value front() {
    queue_ptr->check_empty();
    queue_ptr = get_base_queue_ptr();
    return queue_ptr->front();
  }

  CKey_Value back() {
    queue_ptr->check_empty();
    queue_ptr = get_base_queue_ptr();
    return queue_ptr->back();
  }

  CKey_CValue front() const {
    queue_ptr->check_empty();
    return queue_ptr->front();
  }

  CKey_CValue back() const {
    queue_ptr->check_empty();
    return queue_ptr->back();
  }

  CKey_Value first(K const &k) {
    queue_ptr->check_no_key(k);
    queue_ptr = get_base_queue_ptr();
    return queue_ptr->first(k);
  }

  CKey_Value last(K const &k) {
    queue_ptr->check_no_key(k);
    queue_ptr = get_base_queue_ptr();
    return queue_ptr->last(k);
  }

  CKey_CValue first(K const &k) const {
    queue_ptr->check_no_key(k);
    return queue_ptr->first(k);
  }

  CKey_CValue last(K const &k) const {
    queue_ptr->check_no_key(k);
    return queue_ptr->last(k);
  }

  size_t size() const noexcept {
    return queue_ptr->size();
  }

  bool empty() const noexcept {
    return queue_ptr->empty();
  }

  void clear() {
    if (queue_ptr.use_count() > 1)
      queue_ptr = std::make_shared<base_queue>();
    else
      queue_ptr->clear();
  }

  size_t count(K const &k) const {
    return queue_ptr->count(k);
  }

  k_iterator k_begin() const noexcept {
    return queue_ptr->k_begin();
  }
  
  k_iterator k_end() const noexcept {
    return queue_ptr->k_end();
  }
  
};

template<class K, class V>
void keyed_queue<K, V>::base_queue::push(K const &k, V const &v) {
  queue.emplace_back(nullptr, v);
  auto queue_it = --queue.end();
  auto nodes_it = nodes.find(k);
  
  try {
    nodes[k].push_back(queue_it);
  }
  catch (...) {
    queue.pop_back();
    throw;
  }
  
  queue_it->first = &(nodes_it->first);
}

template<class K, class V>
void keyed_queue<K, V>::base_queue::pop() {
  auto nodes_it = nodes.find(*(queue.back().first));
  check_nodes_iterator(nodes_it);
  
  queue.pop_back();
  
  if (nodes_it->second.size() == 1)
    nodes.erase(nodes_it);
  else
    nodes_it->second.pop_back();
}

template<class K, class V>
void keyed_queue<K, V>::base_queue::pop(K const &k) {
  auto nodes_it = nodes.find(k);
  check_nodes_iterator(nodes_it);
  
  queue.erase(nodes_it->second.back());
  
  if (nodes_it->second.size() == 1)
    nodes.erase(nodes_it);
  else
    nodes_it->second.pop_back();
}

template<class K, class V>
void keyed_queue<K, V>::base_queue::move_to_back(K const &k) {
  auto nodes_it = nodes.find(k);
  check_nodes_iterator(nodes_it);
  
  for (auto queue_it : nodes_it->second)
    queue.splice(queue.cend(), queue, queue_it);
}

#endif /* KEYED_QUEUE_H */
