#ifndef CUSTOM_UNORDERED_MAP_H
#define CUSTOM_UNORDERED_MAP_H

#include <cstddef>
#include <functional>
#include <iostream>
#include <utility>

// Hash map with separate chaining (each bucket is a singly linked list).
//
//  - Templated: works for any key/value type (int -> Order, string -> double, ...)
//  - Resizes automatically when load factor would exceed 0.75, so chains stay
//    short and insert/find/remove stay O(1) on average.
//  - Copy constructor / assignment make deep copies (no double free).
//  - Nodes are never moved in memory (rehash only re-links them), so a pointer
//    returned by find() stays valid until that key is removed.
template <typename K, typename V, typename Hash = std::hash<K>>
class CustomUnorderedMap
{
private:
    struct Node
    {
        K key;
        V value;
        Node* next;
        Node(const K& k, const V& v) : key(k), value(v), next(nullptr) {}
    };

    Node** buckets_;
    std::size_t capacity_;
    std::size_t size_;
    Hash hasher_;

    static constexpr double kMaxLoadFactor = 0.75;

    std::size_t indexFor(const K& key, std::size_t cap) const
    {
        return hasher_(key) % cap;
    }

    static Node** makeBuckets(std::size_t cap)
    {
        Node** b = new Node*[cap];
        for (std::size_t i = 0; i < cap; ++i)
        {
            b[i] = nullptr;
        }
        return b;
    }

    Node* findNode(const K& key) const
    {
        Node* cur = buckets_[indexFor(key, capacity_)];
        while (cur != nullptr)
        {
            if (cur->key == key)
            {
                return cur;
            }
            cur = cur->next;
        }
        return nullptr;
    }

    // Re-link every existing node into a bigger bucket array (no new nodes).
    void rehash(std::size_t newCapacity)
    {
        Node** newBuckets = makeBuckets(newCapacity);
        for (std::size_t i = 0; i < capacity_; ++i)
        {
            Node* cur = buckets_[i];
            while (cur != nullptr)
            {
                Node* next = cur->next;
                std::size_t idx = indexFor(cur->key, newCapacity);
                cur->next = newBuckets[idx];
                newBuckets[idx] = cur;
                cur = next;
            }
        }
        delete[] buckets_;
        buckets_ = newBuckets;
        capacity_ = newCapacity;
    }

    void clear()
    {
        for (std::size_t i = 0; i < capacity_; ++i)
        {
            Node* cur = buckets_[i];
            while (cur != nullptr)
            {
                Node* toDelete = cur;
                cur = cur->next;
                delete toDelete;
            }
            buckets_[i] = nullptr;
        }
        size_ = 0;
    }

public:
    explicit CustomUnorderedMap(std::size_t initialCapacity = 16)
        : buckets_(nullptr),
          capacity_(initialCapacity == 0 ? 1 : initialCapacity),
          size_(0),
          hasher_()
    {
        buckets_ = makeBuckets(capacity_);
    }

    // Deep copy
    CustomUnorderedMap(const CustomUnorderedMap& other)
        : buckets_(nullptr),
          capacity_(other.capacity_),
          size_(0),
          hasher_(other.hasher_)
    {
        buckets_ = makeBuckets(capacity_);
        for (std::size_t i = 0; i < other.capacity_; ++i)
        {
            for (Node* cur = other.buckets_[i]; cur != nullptr; cur = cur->next)
            {
                Node* n = new Node(cur->key, cur->value);
                n->next = buckets_[i];
                buckets_[i] = n;
                ++size_;
            }
        }
    }

    void swap(CustomUnorderedMap& other) noexcept
    {
        std::swap(buckets_, other.buckets_);
        std::swap(capacity_, other.capacity_);
        std::swap(size_, other.size_);
        std::swap(hasher_, other.hasher_);
    }

    // Copy-and-swap: parameter is already a copy, so we just swap with it.
    CustomUnorderedMap& operator=(CustomUnorderedMap other)
    {
        swap(other);
        return *this;
    }

    ~CustomUnorderedMap()
    {
        clear();
        delete[] buckets_;
    }

    // Inserts a new key, or updates the value if the key already exists.
    // Returns true if a new key was added, false if an existing one was updated.
    bool insert(const K& key, const V& value)
    {
        Node* existing = findNode(key);
        if (existing != nullptr)
        {
            existing->value = value;
            return false;
        }

        if (static_cast<double>(size_ + 1) >
            kMaxLoadFactor * static_cast<double>(capacity_))
        {
            rehash(capacity_ * 2);
        }

        std::size_t idx = indexFor(key, capacity_);
        Node* n = new Node(key, value);
        n->next = buckets_[idx]; // insert at head: O(1)
        buckets_[idx] = n;
        ++size_;
        return true;
    }

    // Returns pointer to the stored value, or nullptr if the key is absent.
    V* find(const K& key)
    {
        Node* n = findNode(key);
        return n ? &(n->value) : nullptr;
    }

    const V* find(const K& key) const
    {
        const Node* n = findNode(key);
        return n ? &(n->value) : nullptr;
    }

    bool remove(const K& key)
    {
        std::size_t idx = indexFor(key, capacity_);
        Node* cur = buckets_[idx];
        Node* prev = nullptr;
        while (cur != nullptr)
        {
            if (cur->key == key)
            {
                if (prev == nullptr)
                {
                    buckets_[idx] = cur->next;
                }
                else
                {
                    prev->next = cur->next;
                }
                delete cur;
                --size_;
                return true;
            }
            prev = cur;
            cur = cur->next;
        }
        return false;
    }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    std::size_t bucketCount() const { return capacity_; }
    double loadFactor() const
    {
        return static_cast<double>(size_) / static_cast<double>(capacity_);
    }

    // Only usable when K can be printed with operator<<.
    void display(std::ostream& os = std::cout) const
    {
        for (std::size_t i = 0; i < capacity_; ++i)
        {
            os << "Bucket " << i << ": ";
            for (Node* cur = buckets_[i]; cur != nullptr; cur = cur->next)
            {
                os << "[" << cur->key << "] -> ";
            }
            os << "NULL\n";
        }
    }
};

#endif
