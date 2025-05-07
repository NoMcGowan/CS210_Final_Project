#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <map>
#include <list>
#include <queue>
#include <random>
#include <algorithm>

using namespace std;

struct CityKey {
    string city, country;
    bool operator==(const CityKey& other) const {
        return city == other.city && country == other.country;
    }
};

struct CityKeyHasher {
    size_t operator()(const CityKey& k) const {
        return hash<string>()(k.city) ^ (hash<string>()(k.country) << 1);
    }
};

// Abstract Cache Interface
class Cache {
public:
    virtual bool get(const CityKey& key, string& population) = 0;
    virtual void put(const CityKey& key, const string& population) = 0;
    virtual void printCache() const = 0;
    virtual ~Cache() {}
};

// FIFO Cache
class FIFOCache : public Cache {
    size_t maxSize;
    queue<CityKey> order;
    unordered_map<CityKey, string, CityKeyHasher> cache;

public:
    FIFOCache(size_t size) : maxSize(size) {}

    bool get(const CityKey& key, string& population) override {
        auto it = cache.find(key);
        if (it == cache.end()) return false;
        population = it->second;
        return true;
    }

    void put(const CityKey& key, const string& population) override {
        if (cache.find(key) != cache.end()) return;

        if (cache.size() == maxSize) {
            CityKey old = order.front();
            order.pop();
            cache.erase(old);
        }
        order.push(key);
        cache[key] = population;
    }

    void printCache() const override {
        cout << "\n[Cache - FIFO]\n";
        for (const auto& [k, v] : cache)
            cout << k.city << ", " << k.country << " -> " << v << "\n";
    }
};

// LFU Cache
class LFUCache : public Cache {
    size_t maxSize;
    unordered_map<CityKey, pair<string, int>, CityKeyHasher> cache;

public:
    LFUCache(size_t size) : maxSize(size) {}

    bool get(const CityKey& key, string& population) override {
        auto it = cache.find(key);
        if (it == cache.end()) return false;
        it->second.second++;  // Increase frequency
        population = it->second.first;
        return true;
    }

    void put(const CityKey& key, const string& population) override {
        if (cache.find(key) != cache.end()) {
            cache[key].first = population;
            cache[key].second++;
            return;
        }
        if (cache.size() == maxSize) {
            auto victim = min_element(cache.begin(), cache.end(),
                [](const auto& a, const auto& b) {
                    return a.second.second < b.second.second;
                });
            cache.erase(victim);
        }
        cache[key] = {population, 1};
    }

    void printCache() const override {
        cout << "\n[Cache - LFU]\n";
        for (const auto& [k, v] : cache)
            cout << k.city << ", " << k.country << " -> " << v.first << " (Freq: " << v.second << ")\n";
    }
};

// Random Replacement Cache
class RandomCache : public Cache {
    size_t maxSize;
    unordered_map<CityKey, string, CityKeyHasher> cache;
    vector<CityKey> keys;
    random_device rd;
    mt19937 gen;

public:
    RandomCache(size_t size) : maxSize(size), gen(rd()) {}

    bool get(const CityKey& key, string& population) override {
        auto it = cache.find(key);
        if (it == cache.end()) return false;
        population = it->second;
        return true;
    }

    void put(const CityKey& key, const string& population) override {
        if (cache.find(key) != cache.end()) return;

        if (cache.size() == maxSize) {
            uniform_int_distribution<> dis(0, keys.size() - 1);
            int index = dis(gen);
            cache.erase(keys[index]);
            keys.erase(keys.begin() + index);
        }
        cache[key] = population;
        keys.push_back(key);
    }

    void printCache() const override {
        cout << "\n[Cache - Random]\n";
        for (const auto& [k, v] : cache)
            cout << k.city << ", " << k.country << " -> " << v << "\n";
    }
};

// Trie Node
struct TrieNode {
    unordered_map<char, TrieNode*> children;
    unordered_map<string, string> countryToPop; // country code -> population
    bool isEnd = false;
};

class Trie {
    TrieNode* root;

public:
    Trie() { root = new TrieNode(); }

    void insert(const string& city, const string& country, const string& population) {
        TrieNode* node = root;
        for (char c : city)
            node = node->children[c] = node->children[c] ? node->children[c] : new TrieNode();
        node->isEnd = true;
        node->countryToPop[country] = population;
    }

    bool search(const string& city, const string& country, string& population) {
        TrieNode* node = root;
        for (char c : city) {
            if (!node->children.count(c)) return false;
            node = node->children[c];
        }
        if (node->isEnd && node->countryToPop.count(country)) {
            population = node->countryToPop[country];
            return true;
        }
        return false;
    }
};

string toLower(const string& s) {
    string res = s;
    transform(res.begin(), res.end(), res.begin(), ::tolower);
    return res;
}

void loadCSVIntoTrie(const string& filename, Trie& trie) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Failed to open file.\n";
        return;
    }

    string line;
    getline(file, line); // header
    while (getline(file, line)) {
        stringstream ss(line);
        string country, city, population;
        getline(ss, country, ',');
        getline(ss, city, ',');
        getline(ss, population);
        trie.insert(toLower(city), toLower(country), population);
    }

    cout << "[Loaded Trie from file]\n";
}

int main() {
    string filename = "world_cities.csv";
    Trie trie;
    loadCSVIntoTrie(filename, trie);

    cout << "Select caching strategy (1=LFU, 2=FIFO, 3=Random): ";
    int choice;
    cin >> choice;
    cin.ignore();

    Cache* cache;
    if (choice == 1)
        cache = new LFUCache(10);
    else if (choice == 2)
        cache = new FIFOCache(10);
    else
        cache = new RandomCache(10);

    while (true) {
        string city, country;
        cout << "\nEnter city name (or 'exit'): ";
        getline(cin, city);
        if (toLower(city) == "exit") break;

        cout << "Enter country code: ";
        getline(cin, country);

        CityKey key{toLower(city), toLower(country)};
        string population;

        if (cache->get(key, population)) {
            cout << "[From Cache] Population: " << population << endl;
        } else if (trie.search(key.city, key.country, population)) {
            cache->put(key, population);
            cout << "[From Trie] Population: " << population << endl;
        } else {
            cout << "City not found.\n";
        }

        cache->printCache();
    }

    delete cache;
    return 0;
}
