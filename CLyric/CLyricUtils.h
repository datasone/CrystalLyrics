//
// Created by datasone.
// This file is part of CrystalLyrics.
//

#ifndef CRYSTALLYRICS_CLYRICUTILS_H
#define CRYSTALLYRICS_CLYRICUTILS_H

#include <string>
#include <vector>
#include <numeric>
#include <algorithm>

size_t utf8StringChars(const std::string &str);

int stringDistance(const std::string &compareString, const std::string &baseString);

int longestCommonSubsequece(const std::string &str1, const std::string &str2);

int levenshteinDistance(const std::string &str1, const std::string &str2);

std::vector<std::string> split_string(const std::string &str, const std::string &delimiter);

bool zlibInflate(const std::string &compressed, std::string &uncompressed);

template<typename T>
inline std::vector<size_t> sort_indexes(const std::vector<T> &v) {

    // initialize original index locations
    std::vector<size_t> idx(v.size());
    std::iota(idx.begin(), idx.end(), 0);

    // sort indexes based on comparing values in v
    // using std::stable_sort instead of std::sort
    // to avoid unnecessary index re-orderings
    // when v contains elements of equal values
    std::stable_sort(idx.begin(), idx.end(),
                     [&v](size_t i1, size_t i2) { return v[i1] < v[i2]; });

    return idx;
}

inline std::string normalizeFileName(std::string name) {
    for (auto &c: name) {
        if (static_cast<unsigned char>(c) > 127) {
            // Non-Ascii char
            continue;
        }
        switch (c) {
            case '/':
            case '\\':
            case '?':
            case '%':
            case '*':
            case ':':
            case '|':
            case '"':
            case '<':
            case '>':
                c = ' ';
        }
    }
    return name;
}

// trim from left
inline std::string &ltrim(std::string &s, const char *t = " \t\n\r\f\v") {
    s.erase(0, s.find_first_not_of(t));
    return s;
}

// trim from right
inline std::string &rtrim(std::string &s, const char *t = " \t\n\r\f\v") {
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

// trim from left & right
inline std::string &trim(std::string &s, const char *t = " \t\n\r\f\v") {
    return ltrim(rtrim(s, t), t);
}

// copying versions

inline std::string ltrim_copy(std::string s, const char *t = " \t\n\r\f\v") {
    return ltrim(s, t);
}

inline std::string rtrim_copy(std::string s, const char *t = " \t\n\r\f\v") {
    return rtrim(s, t);
}

inline std::string trim_copy(std::string s, const char *t = " \t\n\r\f\v") {
    return trim(s, t);
}

#endif //CRYSTALLYRICS_CLYRICUTILS_H
