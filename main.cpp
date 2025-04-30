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

