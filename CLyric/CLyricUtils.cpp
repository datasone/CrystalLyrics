//
// Created by datasone.
// This file is part of CrystalLyrics.
//

#include "CLyricUtils.h"
#include <algorithm>
#include <zlib.h>
#include <cstring>

size_t utf8StringChars(const std::string &str) {
    size_t count = 0, i = 0;
    while (i < str.length()) {
        auto ch = static_cast<unsigned char>(str[i++]);
        if (ch < 0x80u) {
            // ASCII (UTF-8 single byte)
            ++count;
            continue;
        } else if (ch < 0xE0u) {
            // UTF-8 double byte
            ++count;
            i += 1;
            continue;
        } else if (ch < 0xF0u) {
            // UTF-8 triple byte
            ++count;
            i += 2;
            continue;
        } else {
            // UTF-8 quadruple byte
            ++count;
            i += 3;
            continue;
        }
    }
    return count;
}

int stringDistance(const std::string &compareString, const std::string &baseString) {
    return levenshteinDistance(compareString, baseString) + baseString.length() -
           longestCommonSubsequece(compareString, baseString);
}

int longestCommonSubsequece(const std::string &str1, const std::string &str2) {
    int row = str1.length() + 1, column = str2.length() + 1;
    int i, j;

    std::vector<int> LCS(row * column);

    //LCS[row][column]
    //LCS[i][j]=Length of longest common subsequence of str1[0....(i-1)] and str2[0...(j-1)]

    //initializing
    for (i = 0; i < row; i++)
        LCS[i * column] = 0;    //empty str2

    for (j = 0; j < column; j++)
        LCS[j] = 0;   //empty str1

    //now, start filling the matrix row wise
    for (i = 1; i < row; i++) {
        for (j = 1; j < column; j++) {
            //if current character of both strings match
            if (str1[i - 1] == str2[j - 1]) {
                LCS[i * column + j] = 1 + LCS[(i - 1) * column + (j - 1)];
            } //mismatch
            else {
                LCS[i * column + j] = std::max(LCS[(i - 1) * column + j], LCS[i * column + (j - 1)]);
            }
        }
    }

    //now, return the final value
    return LCS[(row - 1) * column + (column - 1)];
}

int levenshteinDistance(const std::string &str1, const std::string &str2) {
    int cost, row = str1.length() + 1, column = str2.length() + 1;

    std::vector<int> distance(row * column);

    for (int i = 0; i < column; ++i) {
        distance[i * row] = i;
    }
    for (int j = 0; j < row; ++j) {
        distance[j] = j;
    }
    for (int i = 1; i < column; ++i) {
        for (int j = 1; j < row; ++j) {
            if (str2[i - 1] == str1[j - 1]) cost = 0;
            else cost = 1;
            distance[i * row + j] = std::min({
                                                     distance[(i - 1) * row + j] + 1,
                                                     distance[i * row + (j - 1)] + 1,
                                                     distance[(i - 1) * row + (j - 1)] + cost,
                                             });
        }
    }

    return distance[(column - 1) * row + (row - 1)];
}

std::vector<std::string> split_string(const std::string &str, const std::string &delimiter) {
    std::vector<std::string> strings;

    std::string::size_type pos;
    std::string::size_type prev = 0;
    while ((pos = str.find(delimiter, prev)) != std::string::npos) {
        strings.push_back(str.substr(prev, pos - prev));
        prev = pos + 1;
    }

    // To get the last substring (or only, if delimiter is not found)
    strings.push_back(str.substr(prev));

    return strings;
}

bool zlibInflate(const std::string &compressed, std::string &uncompressed) {
    if (compressed.empty()) {
        uncompressed = std::string();
        return true;
    }

    uncompressed.clear();

    unsigned fullLength = compressed.size();
    unsigned halfLength = compressed.size() / 2;

    unsigned uncompLength = fullLength;
    char *uncomp = new char[uncompLength];

    z_stream zStream;
    zStream.next_in = (Bytef *) compressed.c_str();
    zStream.avail_in = compressed.size();
    zStream.total_out = 0;
    zStream.zalloc = Z_NULL;
    zStream.zfree = Z_NULL;

    bool done = false;

    if (inflateInit(&zStream) != Z_OK) {
        delete[] uncomp;
        return false;
    }

    while (!done) {
        // If our output buffer is too small
        if (zStream.total_out >= uncompLength) {
            // Increase size of output buffer
            char *uncomp2 = new char[uncompLength + halfLength];
            std::memcpy(uncomp2, uncomp, uncompLength);
            uncompLength += halfLength;
            delete[] uncomp;
            uncomp = uncomp2;
        }

        zStream.next_out = (Bytef *) (uncomp + zStream.total_out);
        zStream.avail_out = uncompLength - zStream.total_out;

        // Inflate another chunk.
        int err = inflate(&zStream, Z_SYNC_FLUSH);
        if (err == Z_STREAM_END) done = true;
        else if (err != Z_OK) {
            break;
        }
    }

    if (inflateEnd(&zStream) != Z_OK) {
        delete[] uncomp;
        return false;
    }

    for (size_t i = 0; i < zStream.total_out; ++i) {
        uncompressed += uncomp[i];
    }
    delete[] uncomp;
    return true;
}
