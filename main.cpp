#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <list>
#include <vector>
#include <algorithm>
#include <memory>
#include <cstdlib>
#include <ctime>

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

class TrieNode {
    public:
        unordered_map<char, TrieNode*> children;
        unordered_map<string, string> countryToPopulation;
        bool isEnd = false;
    
        ~TrieNode() {
            for (auto& pair : children)
                delete pair.second;
        }
    };
    class Trie {
        TrieNode* root;

    public:
        Trie() {
            root = new TrieNode();
        }
        ~Trie() {
            delete root;
        }
        void insert(const string& city, const string& country, const string& population) {
            TrieNode* node = root;
            for (char c : city) {
                c = tolower(c);
                if (!node->children.count(c))
                    node->children[c] = new TrieNode();
                node = node->children[c];
            }
            node->isEnd = true;
            node->countryToPopulation[country] = population;
        }
        bool search(const string& city, const string& country, string& population) {
            TrieNode* node = root;
            for (char c : city) {
                c = tolower(c);
                if (!node->children.count(c))
                    return false;
                node = node->children[c];
            }
            if (node->isEnd && node->countryToPopulation.count(country)) {
                population = node->countryToPopulation[country];
                return true;
            }
            return false;
        }
    };
    
class ICache {
    public:
        virtual bool get(const CityKey& key, string& population) = 0;
        virtual void put(const CityKey& key, const string& population) = 0;
        virtual void printCache() const = 0;
        virtual ~ICache() = default;
    };
    
// FIFO Cache (was LRU)
class FIFOCache : public ICache {
    size_t maxSize;
    list<pair<CityKey, string>> entries;
    unordered_map<CityKey, list<pair<CityKey, string>>::iterator, CityKeyHasher> cache;

public:
    FIFOCache(size_t size) : maxSize(size) {}

    bool get(const CityKey& key, string& population) override {
        auto it = cache.find(key);
        if (it == cache.end()) return false;

        population = it->second->second;
        return true;
    }

    void put(const CityKey& key, const string& population) override {
        if (cache.find(key) != cache.end()) {
            entries.erase(cache[key]);
            cache.erase(key);
        } else if (entries.size() == maxSize) {
            cache.erase(entries.back().first);
            entries.pop_back();
        }
        entries.push_front({key, population});
        cache[key] = entries.begin();
    }

    void printCache() const override {
        cout << "\n[FIFO Cache] Most Recent -> Oldest:\n";
        for (const auto& [k, pop] : entries)
            cout << k.city << ", " << k.country << " -> " << pop << endl;
    }
};

class LFUCache : public ICache {
    size_t maxSize;
    unordered_map<CityKey, pair<string, int>, CityKeyHasher> values;
    list<CityKey> order;

public:
    LFUCache(size_t size) : maxSize(size) {}

    bool get(const CityKey& key, string& population) override {
        auto it = values.find(key);
        if (it == values.end()) return false;
        it->second.second++;
        population = it->second.first;
        return true;
    }

    void put(const CityKey& key, const string& population) override {
        if (values.find(key) != values.end()) {
            values[key].first = population;
            values[key].second++;
            return;
        }

        if (values.size() == maxSize) {
            CityKey evictKey = order.front();
            int minFreq = values[evictKey].second;
            for (const auto& k : order) {
                if (values[k].second < minFreq) {
                    evictKey = k;
                    minFreq = values[k].second;
                }
            }
            values.erase(evictKey);
            order.remove(evictKey);
        }

        values[key] = {population, 1};
        order.push_back(key);
    }

    void printCache() const override {
        cout << "\n[LFU Cache] Key -> Population (Freq):\n";
        for (const auto& [k, v] : values)
            cout << k.city << ", " << k.country << " -> " << v.first << " (" << v.second << ")" << endl;
    }
};

class RandomCache : public ICache {
    size_t maxSize;
    unordered_map<CityKey, string, CityKeyHasher> values;
    vector<CityKey> keys;

public:
    RandomCache(size_t size) : maxSize(size) { srand(static_cast<unsigned>(time(nullptr))); }

    bool get(const CityKey& key, string& population) override {
        auto it = values.find(key);
        if (it == values.end()) return false;
        population = it->second;
        return true;
    }

    void put(const CityKey& key, const string& population) override {
        if (values.find(key) != values.end()) {
            values[key] = population;
            return;
        }

        if (values.size() == maxSize) {
            int idx = rand() % keys.size();
            values.erase(keys[idx]);
            keys.erase(keys.begin() + idx);
        }

        values[key] = population;
        keys.push_back(key);
    }

    void printCache() const override {
        cout << "\n[Random Cache] Key -> Population:\n";
        for (const auto& key : keys)
            cout << key.city << ", " << key.country << " -> " << values.at(key) << endl;
    }
};

string toLower(const string& s) {
    string res = s;
    transform(res.begin(), res.end(), res.begin(), ::tolower);
    return res;
}

bool lookupCity(const string& filename, const CityKey& key, string& population) {
    ifstream file(filename);
    if (!file.is_open()) return false;

    string line;
    getline(file, line);
    while (getline(file, line)) {
        stringstream ss(line);
        string country, city, pop;
        getline(ss, country, ',');
        getline(ss, city, ',');
        getline(ss, pop);

        if (toLower(city) == key.city && toLower(country) == key.country) {
            population = pop;
            return true;
        }
    }
    return false;
}

int main() {
    unique_ptr<ICache> cache;
    string filename = "world_cities.csv";

    cout << "Select caching strategy:\n1. LFU\n2. FIFO\n3. Random\nChoice: ";
    int choice;
    cin >> choice;
    cin.ignore(); // ignore newline

    switch (choice) {
        case 1: cache = make_unique<LFUCache>(10); break;
        case 2: cache = make_unique<FIFOCache>(10); break;
        case 3: cache = make_unique<RandomCache>(10); break;
        default: cout << "Invalid choice.\n"; return 1;
    }

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
        } else if (lookupCity(filename, key, population)) {
            cache->put(key, population);
            cout << "[From File] Population: " << population << endl;
        } else {
            cout << "City not found.\n";
        }

        cache->printCache();
    }

    return 0;
}
