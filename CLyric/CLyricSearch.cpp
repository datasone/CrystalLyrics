//
// Created by datasone on 8/6/2020.
//

#include "CLyricSearch.h"

#include <filesystem>
#include <fstream>

#include "CLyricUtils.h"

using namespace cLyric;

CLyric CLyricSearch::fetchCLyric(const std::string &title, const std::string &album, const std::string &artist,
                                 int duration, const std::string &saveDirectoryPath) {
    auto name = saveDirectoryPath + "/" + CLyric::filename(title, album, artist);
    std::ifstream localFile(std::filesystem::u8path(saveDirectoryPath + "/" + CLyric::filename(title, album, artist)));
    std::string lineContent, localFileContents;
    if (localFile.is_open()) {
        while (std::getline(localFile, lineContent)) {
            localFileContents += lineContent;
            localFileContents.push_back('\n');
        }
        localFile.close();
        CLyric lyric(localFileContents, LyricStyle::CLrcStyle);
        if (lyric.isValid()) {
            lyric.track.source = "LocalFile";
            return lyric;
        }
    }

    // Check Album Instrumental
    std::ifstream albumFlagFile(
            std::filesystem::u8path(saveDirectoryPath + "/" + normalizeFileName(album + ".instrumental")));
    if (albumFlagFile.is_open()) {
        CLyric instrumentalLyric(Track(title, album, artist, "", "", duration, true), std::vector<CLyricItem>());
        return instrumentalLyric;
    }

    std::function < void(std::vector<CLyric>) > callback = [this](std::vector<CLyric> lyrics) {
        this->appendResultCallback(std::move(lyrics));
    };
    for (std::unique_ptr<CLyricProvider>& provider: providerList) {
        provider->searchLyrics(Track(title, album, artist, "", "", duration), callback);
    }

    std::stable_sort(results.begin(), results.end(),
                     [title, artist](const CLyric& res1, const CLyric& res2) {
                         int length = title.size() + artist.size() / 2;
                         int distance1 =
                                 stringDistance(res1.track.title, title) + stringDistance(res1.track.artist, artist) / 2;
                         int distance2 =
                                 stringDistance(res2.track.title, title) + stringDistance(res2.track.artist, artist) / 2;
                         double score1 = 1 - double(distance1) / length;
                         double score2 = 1 - double(distance2) / length;
                         if (std::any_of(res1.lyrics.begin(), res1.lyrics.end(),
                                         [](CLyricItem item) { return !item.translation.empty(); }))
                             score1 += 0.2;
                         if (std::any_of(res2.lyrics.begin(), res2.lyrics.end(),
                                         [](CLyricItem item) { return !item.translation.empty(); }))
                             score2 += 0.2;
                         if (std::any_of(res1.lyrics.begin(), res1.lyrics.end(),
                                         [](CLyricItem item) { return !item.timecodes.empty(); }))
                             score1 += 0.1;
                         if (std::any_of(res2.lyrics.begin(), res2.lyrics.end(),
                                         [](CLyricItem item) { return !item.timecodes.empty(); }))
                             score2 += 0.1;
                         if (!res1.isValid())
                             score1 -= 1;
                         if (!res2.isValid())
                             score2 -= 1;
                         return score1 > score2;
                     });

    if (results.empty())
        return CLyric();
    CLyric resultLyric = results.front();

    return resultLyric;
}

void CLyricSearch::appendResultCallback(std::vector<CLyric> lyrics) {
    if (this->results.empty()) {
        this->results = std::move(lyrics);
    } else {
        this->results.insert(std::end(this->results), std::make_move_iterator(std::begin(lyrics)),
                             std::make_move_iterator(std::end(lyrics)));
    }
}

std::vector<CLyric> CLyricSearch::searchCLyric(const std::string &title, const std::string &artist, int duration) {
    std::function<void(std::vector<CLyric>)> callback = [this](std::vector<CLyric> lyrics) {
        this->appendResultCallback(std::move(lyrics));
    };
    for (std::unique_ptr<CLyricProvider> &provider: providerList) {
        provider->searchLyrics(Track(title, "", artist, "", "", duration), callback);
    }
    return this->results;
}

CLyricSearch::CLyricSearch() {
    providerList[0] = std::make_unique<Xiami>();
    providerList[1] = std::make_unique<Netease>();
    providerList[2] = std::make_unique<QQMusic>();
    providerList[3] = std::make_unique<Kugou>();
    providerList[4] = std::make_unique<Gecimi>();
    providerList[5] = std::make_unique<THBWiki>();
}
