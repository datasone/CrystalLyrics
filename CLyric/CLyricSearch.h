//
// Created by datasone on 8/6/2020.
//

#ifndef CRYSTALLYRICS_CLYRICSEARCH_H
#define CRYSTALLYRICS_CLYRICSEARCH_H

#include "CLyric.h"
#include "CLyricProvider.h"

namespace cLyric {

    class CLyricSearch {
        std::vector<CLyric> results;

        void appendResultCallback(std::vector<CLyric> lyrics);

        std::unique_ptr<CLyricProvider> providerList[6];

    public:
        explicit CLyricSearch();

        CLyric fetchCLyric(const std::string &title, const std::string &album, const std::string &artist, int duration,
                           const std::string &saveDirectoryPath);

        std::vector<CLyric> searchCLyric(const std::string &title, const std::string &artist, int duration);
    };

}

#endif //CRYSTALLYRICS_CLYRICSEARCH_H
