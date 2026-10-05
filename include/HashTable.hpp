#ifndef HASHTABLE_HASHTABLE_HPP
#define HASHTABLE_HASHTABLE_HPP

#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <stdexcept>
#include <utility>
#include <vector>

enum class CollisionResolution {
    CHAINING,
    LINEAR_PROBING,
    QUADRATIC_PROBING,
    DOUBLE_HASHING
};

template <typename Key, typename Value, typename Hash = std::hash<Key>>
class HashTable {
public:
    explicit HashTable(CollisionResolution method = CollisionResolution::CHAINING,
                        std::size_t initialCapacity = 11);

    void insert(const Key& key, const Value& value);
    bool remove(const Key& key);
    bool contains(const Key& key) const;
    Value* find(const Key& key);
    const Value* find(const Key& key) const;
    Value& operator[](const Key& key);

    std::size_t size() const;
    std::size_t capacity() const;
    double loadFactor() const;
    bool empty() const;
    void clear();
    CollisionResolution method() const;
    std::vector<std::pair<Key, Value>> items() const;

private:
    enum class SlotState : std::uint8_t { EMPTY_SLOT, OCCUPIED, DELETED };

    struct OpenEntry {
        Key key{};
        Value value{};
        SlotState state = SlotState::EMPTY_SLOT;
    };

    CollisionResolution resolutionMethod;
    std::vector<std::list<std::pair<Key, Value>>> chainTable;
    std::vector<OpenEntry> openTable;
    std::size_t elementCount;
    std::size_t tableCapacity;
    Hash hasher;

    static constexpr double CHAINING_MAX_LOAD_FACTOR = 0.75;
    static constexpr double OPEN_ADDRESSING_MAX_LOAD_FACTOR = 0.5;
    static constexpr std::size_t MIN_CAPACITY = 7;

    static bool isPrime(std::size_t n);
    static std::size_t nextPrime(std::size_t n);

    std::size_t primaryHash(const Key& key, std::size_t cap) const;
    std::size_t secondaryHash(const Key& key, std::size_t cap) const;
    std::size_t probeIndex(const Key& key, std::size_t attempt, std::size_t cap) const;

    double maxLoadFactor() const;
    bool usesOpenAddressing() const;
    void rehash(std::size_t newCapacity);
    void growIfNeeded();

    void insertChaining(const Key& key, const Value& value);
    void insertOpenAddressing(const Key& key, const Value& value);
    bool removeChaining(const Key& key);
    bool removeOpenAddressing(const Key& key);
    Value* findChaining(const Key& key);
    const Value* findChaining(const Key& key) const;
    Value* findOpenAddressing(const Key& key);
    const Value* findOpenAddressing(const Key& key) const;
};

template <typename Key, typename Value, typename Hash>
HashTable<Key, Value, Hash>::HashTable(CollisionResolution method, std::size_t initialCapacity)
    : resolutionMethod(method), elementCount(0), tableCapacity(nextPrime(initialCapacity < MIN_CAPACITY ? MIN_CAPACITY : initialCapacity)) {
    if (usesOpenAddressing()) {
        openTable.resize(tableCapacity);
    } else {
        chainTable.resize(tableCapacity);
    }
}

template <typename Key, typename Value, typename Hash>
bool HashTable<Key, Value, Hash>::isPrime(std::size_t n) {
    if (n < 2) return false;
    if (n == 2 || n == 3) return true;
    if (n % 2 == 0) return false;
    for (std::size_t i = 3; i * i <= n; i += 2) {
        if (n % i == 0) return false;
    }
    return true;
}

template <typename Key, typename Value, typename Hash>
std::size_t HashTable<Key, Value, Hash>::nextPrime(std::size_t n) {
    if (n <= 2) return 2;
    std::size_t candidate = (n % 2 == 0) ? n + 1 : n;
    while (!isPrime(candidate)) {
        candidate += 2;
    }
    return candidate;
}

template <typename Key, typename Value, typename Hash>
std::size_t HashTable<Key, Value, Hash>::primaryHash(const Key& key, std::size_t cap) const {
    return hasher(key) % cap;
}

template <typename Key, typename Value, typename Hash>
std::size_t HashTable<Key, Value, Hash>::secondaryHash(const Key& key, std::size_t cap) const {
    std::size_t h = hasher(key);
    return 1 + (h % (cap - 1));
}

template <typename Key, typename Value, typename Hash>
std::size_t HashTable<Key, Value, Hash>::probeIndex(const Key& key, std::size_t attempt, std::size_t cap) const {
    std::size_t h1 = primaryHash(key, cap);
    switch (resolutionMethod) {
        case CollisionResolution::LINEAR_PROBING:
            return (h1 + attempt) % cap;
        case CollisionResolution::QUADRATIC_PROBING:
            return (h1 + attempt * attempt) % cap;
        case CollisionResolution::DOUBLE_HASHING: {
            std::size_t h2 = secondaryHash(key, cap);
            return (h1 + attempt * h2) % cap;
        }
        default:
            return h1;
    }
}

template <typename Key, typename Value, typename Hash>
double HashTable<Key, Value, Hash>::maxLoadFactor() const {
    return usesOpenAddressing() ? OPEN_ADDRESSING_MAX_LOAD_FACTOR : CHAINING_MAX_LOAD_FACTOR;
}

template <typename Key, typename Value, typename Hash>
bool HashTable<Key, Value, Hash>::usesOpenAddressing() const {
    return resolutionMethod != CollisionResolution::CHAINING;
}

template <typename Key, typename Value, typename Hash>
void HashTable<Key, Value, Hash>::rehash(std::size_t newCapacity) {
    std::size_t primeCapacity = nextPrime(newCapacity);

    if (usesOpenAddressing()) {
        std::vector<OpenEntry> oldTable = std::move(openTable);
        openTable.assign(primeCapacity, OpenEntry{});
        tableCapacity = primeCapacity;
        elementCount = 0;
        for (const OpenEntry& entry : oldTable) {
            if (entry.state == SlotState::OCCUPIED) {
                insertOpenAddressing(entry.key, entry.value);
            }
        }
    } else {
        std::vector<std::list<std::pair<Key, Value>>> oldTable = std::move(chainTable);
        chainTable.assign(primeCapacity, std::list<std::pair<Key, Value>>{});
        tableCapacity = primeCapacity;
        elementCount = 0;
        for (const auto& bucket : oldTable) {
            for (const auto& entry : bucket) {
                insertChaining(entry.first, entry.second);
            }
        }
    }
}

template <typename Key, typename Value, typename Hash>
void HashTable<Key, Value, Hash>::growIfNeeded() {
    if (loadFactor() >= maxLoadFactor()) {
        rehash(tableCapacity * 2);
    }
}

template <typename Key, typename Value, typename Hash>
void HashTable<Key, Value, Hash>::insertChaining(const Key& key, const Value& value) {
    std::size_t index = primaryHash(key, tableCapacity);
    for (auto& entry : chainTable[index]) {
        if (entry.first == key) {
            entry.second = value;
            return;
        }
    }
    chainTable[index].emplace_back(key, value);
    ++elementCount;
}

template <typename Key, typename Value, typename Hash>
void HashTable<Key, Value, Hash>::insertOpenAddressing(const Key& key, const Value& value) {
    std::size_t firstDeleted = tableCapacity;
    for (std::size_t attempt = 0; attempt < tableCapacity; ++attempt) {
        std::size_t index = probeIndex(key, attempt, tableCapacity);
        SlotState state = openTable[index].state;

        if (state == SlotState::OCCUPIED && openTable[index].key == key) {
            openTable[index].value = value;
            return;
        }
        if (state == SlotState::DELETED && firstDeleted == tableCapacity) {
            firstDeleted = index;
        }
        if (state == SlotState::EMPTY_SLOT) {
            std::size_t target = (firstDeleted != tableCapacity) ? firstDeleted : index;
            openTable[target].key = key;
            openTable[target].value = value;
            openTable[target].state = SlotState::OCCUPIED;
            ++elementCount;
            return;
        }
    }

    if (firstDeleted != tableCapacity) {
        openTable[firstDeleted].key = key;
        openTable[firstDeleted].value = value;
        openTable[firstDeleted].state = SlotState::OCCUPIED;
        ++elementCount;
        return;
    }

    throw std::overflow_error("HashTable: no free slot found for open addressing insert");
}

template <typename Key, typename Value, typename Hash>
void HashTable<Key, Value, Hash>::insert(const Key& key, const Value& value) {
    growIfNeeded();
    if (usesOpenAddressing()) {
        insertOpenAddressing(key, value);
    } else {
        insertChaining(key, value);
    }
}

template <typename Key, typename Value, typename Hash>
bool HashTable<Key, Value, Hash>::removeChaining(const Key& key) {
    std::size_t index = primaryHash(key, tableCapacity);
    auto& bucket = chainTable[index];
    for (auto it = bucket.begin(); it != bucket.end(); ++it) {
        if (it->first == key) {
            bucket.erase(it);
            --elementCount;
            return true;
        }
    }
    return false;
}

template <typename Key, typename Value, typename Hash>
bool HashTable<Key, Value, Hash>::removeOpenAddressing(const Key& key) {
    for (std::size_t attempt = 0; attempt < tableCapacity; ++attempt) {
        std::size_t index = probeIndex(key, attempt, tableCapacity);
        SlotState state = openTable[index].state;

        if (state == SlotState::EMPTY_SLOT) {
            return false;
        }
        if (state == SlotState::OCCUPIED && openTable[index].key == key) {
            openTable[index].state = SlotState::DELETED;
            --elementCount;
            return true;
        }
    }
    return false;
}

template <typename Key, typename Value, typename Hash>
bool HashTable<Key, Value, Hash>::remove(const Key& key) {
    return usesOpenAddressing() ? removeOpenAddressing(key) : removeChaining(key);
}

template <typename Key, typename Value, typename Hash>
Value* HashTable<Key, Value, Hash>::findChaining(const Key& key) {
    std::size_t index = primaryHash(key, tableCapacity);
    for (auto& entry : chainTable[index]) {
        if (entry.first == key) {
            return &entry.second;
        }
    }
    return nullptr;
}

template <typename Key, typename Value, typename Hash>
const Value* HashTable<Key, Value, Hash>::findChaining(const Key& key) const {
    std::size_t index = primaryHash(key, tableCapacity);
    for (const auto& entry : chainTable[index]) {
        if (entry.first == key) {
            return &entry.second;
        }
    }
    return nullptr;
}

template <typename Key, typename Value, typename Hash>
Value* HashTable<Key, Value, Hash>::findOpenAddressing(const Key& key) {
    for (std::size_t attempt = 0; attempt < tableCapacity; ++attempt) {
        std::size_t index = probeIndex(key, attempt, tableCapacity);
        SlotState state = openTable[index].state;

        if (state == SlotState::EMPTY_SLOT) {
            return nullptr;
        }
        if (state == SlotState::OCCUPIED && openTable[index].key == key) {
            return &openTable[index].value;
        }
    }
    return nullptr;
}

template <typename Key, typename Value, typename Hash>
const Value* HashTable<Key, Value, Hash>::findOpenAddressing(const Key& key) const {
    for (std::size_t attempt = 0; attempt < tableCapacity; ++attempt) {
        std::size_t index = probeIndex(key, attempt, tableCapacity);
        SlotState state = openTable[index].state;

        if (state == SlotState::EMPTY_SLOT) {
            return nullptr;
        }
        if (state == SlotState::OCCUPIED && openTable[index].key == key) {
            return &openTable[index].value;
        }
    }
    return nullptr;
}

template <typename Key, typename Value, typename Hash>
Value* HashTable<Key, Value, Hash>::find(const Key& key) {
    return usesOpenAddressing() ? findOpenAddressing(key) : findChaining(key);
}

template <typename Key, typename Value, typename Hash>
const Value* HashTable<Key, Value, Hash>::find(const Key& key) const {
    return usesOpenAddressing() ? findOpenAddressing(key) : findChaining(key);
}

template <typename Key, typename Value, typename Hash>
bool HashTable<Key, Value, Hash>::contains(const Key& key) const {
    return find(key) != nullptr;
}

template <typename Key, typename Value, typename Hash>
Value& HashTable<Key, Value, Hash>::operator[](const Key& key) {
    Value* existing = find(key);
    if (existing != nullptr) {
        return *existing;
    }
    insert(key, Value{});
    return *find(key);
}

template <typename Key, typename Value, typename Hash>
std::size_t HashTable<Key, Value, Hash>::size() const {
    return elementCount;
}

template <typename Key, typename Value, typename Hash>
std::size_t HashTable<Key, Value, Hash>::capacity() const {
    return tableCapacity;
}

template <typename Key, typename Value, typename Hash>
double HashTable<Key, Value, Hash>::loadFactor() const {
    return static_cast<double>(elementCount) / static_cast<double>(tableCapacity);
}

template <typename Key, typename Value, typename Hash>
bool HashTable<Key, Value, Hash>::empty() const {
    return elementCount == 0;
}

template <typename Key, typename Value, typename Hash>
void HashTable<Key, Value, Hash>::clear() {
    if (usesOpenAddressing()) {
        openTable.assign(tableCapacity, OpenEntry{});
    } else {
        chainTable.assign(tableCapacity, std::list<std::pair<Key, Value>>{});
    }
    elementCount = 0;
}

template <typename Key, typename Value, typename Hash>
CollisionResolution HashTable<Key, Value, Hash>::method() const {
    return resolutionMethod;
}

template <typename Key, typename Value, typename Hash>
std::vector<std::pair<Key, Value>> HashTable<Key, Value, Hash>::items() const {
    std::vector<std::pair<Key, Value>> result;
    result.reserve(elementCount);

    if (usesOpenAddressing()) {
        for (const OpenEntry& entry : openTable) {
            if (entry.state == SlotState::OCCUPIED) {
                result.emplace_back(entry.key, entry.value);
            }
        }
    } else {
        for (const auto& bucket : chainTable) {
            for (const auto& entry : bucket) {
                result.push_back(entry);
            }
        }
    }

    return result;
}

#endif
