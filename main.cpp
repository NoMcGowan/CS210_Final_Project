#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <list>
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

class LRUCache {
    size_t maxSize;
    list<pair<CityKey, string>> entries;
    unordered_map<CityKey, list<pair<CityKey, string>>::iterator, CityKeyHasher> cache;

public:
    LRUCache(size_t size) : maxSize(size) {}

    bool get(const CityKey& key, string& population) {
        auto it = cache.find(key);
        if (it == cache.end()) return false;

        entries.splice(entries.begin(), entries, it->second);
        population = it->second->second;
        return true;
    }

    void put(const CityKey& key, const string& population) {
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

    void printCache() const {
        cout << "\n[Cache] Most Recent -> Oldest:\n";
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

int main() {
    LRUCache cache(10);
    string filename = "world_cities.csv";

    while (true) {
        string city, country;
        cout << "\nEnter city name (or 'exit'): ";
        getline(cin, city);
        if (toLower(city) == "exit") break;

        cout << "Enter country code: ";
        getline(cin, country);

        CityKey key{toLower(city), toLower(country)};
        string population;

        if (cache.get(key, population)) {
            cout << "[From Cache] Population: " << population << endl;
        } else if (lookupCity(filename, key, population)) {
            cache.put(key, population);
            cout << "[From File] Population: " << population << endl;
        } else {
            cout << "City not found.\n";
        }

        cache.printCache(); //Finalized
    }

    return 0;
}