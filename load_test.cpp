#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <chrono>
#include <random>
#include <algorithm>
#include "main.cpp" 

using namespace std;
using namespace std::chrono;

struct Query {
    string city, country;
};

// Load all entries from CSV
vector<Query> loadAllCities(const string& filename) {
    ifstream file(filename);
    vector<Query> all;
    string line;

    getline(file, line); // Skip header
    while (getline(file, line)) {
        stringstream ss(line);
        string country, city, population;
        getline(ss, country, ',');
        getline(ss, city, ',');
        getline(ss, population);

        if (!city.empty() && !country.empty())
            all.push_back({toLower(city), toLower(country)});
    }

    return all;
}

// Generate 1000 random queries from existing city data
vector<Query> generateTestQueries(const vector<Query>& all) {
    vector<Query> queries;
    mt19937 rng(random_device{}());
    uniform_int_distribution<> dist(0, all.size() - 1);

    for (int i = 0; i < 1000; ++i)
        queries.push_back(all[dist(rng)]);

    return queries;
}

void runTest(Cache* cache, Trie& trie, const vector<Query>& queries, const string& strategyName) {
    int hits = 0;
    auto start = high_resolution_clock::now();

    for (const auto& q : queries) {
        CityKey key{q.city, q.country};
        string population;

        if (cache->get(key, population)) {
            ++hits;
        } else if (trie.search(key.city, key.country, population)) {
            cache->put(key, population);
        }
    }

    auto end = high_resolution_clock::now();
    double totalTime = duration<double, milli>(end - start).count();
    double avgTime = totalTime / queries.size();
    double hitRate = (hits * 100.0) / queries.size();

    cout << "\n==== [" << strategyName << " Cache Results] ====\n";
    cout << "Total Queries     : " << queries.size() << "\n";
    cout << "Cache Hits        : " << hits << "\n";
    cout << "Cache Hit Rate    : " << hitRate << "%\n";
    cout << "Average Time (ms) : " << avgTime << "\n";
}

int main() {
    string filename = "world_cities.csv";
    Trie trie;
    loadCSVIntoTrie(filename, trie);

    auto allCities = loadAllCities(filename);
    auto testQueries = generateTestQueries(allCities);

    // Test LFU
    LFUCache lfu(10);
    runTest(&lfu, trie, testQueries, "LFU");

    // Test FIFO
    FIFOCache fifo(10);
    runTest(&fifo, trie, testQueries, "FIFO");

    // Test Random
    RandomCache random(10);
    runTest(&random, trie, testQueries, "Random");

    return 0;
}
