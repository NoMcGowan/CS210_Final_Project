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

