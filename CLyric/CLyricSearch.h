//
// Created by datasone on 8/6/2020.
//

#ifndef CRYSTALLYRICS_CLYRICSEARCH_H
#define CRYSTALLYRICS_CLYRICSEARCH_H

#include "CLyric.h"
#include "CLyricProvider.h"

class CLyricSearch {
    std::vector<CLyric> results;

    void appendResultCallback(std::vector<CLyric> lyrics);

    std::unique_ptr<CLyricProvider> providerList[6];

public:
    explicit CLyricSearch(opencc::SimpleConverter& converter);

    CLyric fetchCLyric(const string& title, const string& album, const string& artist, int duration,
                       const string& saveDirectoryPath);

    std::vector<CLyric> searchCLyric(const string& title, const string& artist, int duration);
};


#endif //CRYSTALLYRICS_CLYRICSEARCH_H
