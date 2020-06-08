//
// Created by datasone.
// This file is part of CrystalLyrics.
//

#ifndef CRYSTALLYRICS_CLYRICPROVIDER_H
#define CRYSTALLYRICS_CLYRICPROVIDER_H

#define NOMINMAX

#include "CLyric.h"
#include <curl/curl.h>
#include <opencc/opencc.h>
#include <cstdint>
#include <utility>

class CLyricProvider {
protected:
    CURL* curlHandle;
    string response;

    CLyricProvider();

    static size_t storeCURLResponse(void* buffer, size_t size, size_t nmemb, void* userp);

public:
    virtual ~CLyricProvider();

    void normalizeName(string& str, bool isHttpParam = false);

    string normalizeName(const string& str, bool isHttpParam = false);

    virtual void searchLyrics(const Track& track, std::function<void(std::vector<CLyric>)> appendResultCallback) = 0;
};

class Gecimi : public CLyricProvider {

public:
    void searchLyrics(const Track& track, std::function<void(std::vector<CLyric>)> appendResultCallback) override;
};

class TTPlayer : public CLyricProvider {
    struct TTPlayerResult {
        TTPlayerResult(string title, string artist, int id) : title(std::move(title)), artist(std::move(artist)),
                                                              id(id), distance(INT_MAX) {}

        TTPlayerResult(string title, string artist, int id, const string& targetTitle, const string& targetArtist);

        string title, artist;
        int id = 0, distance;
    };

    static string toUTF16LEHexString(const string& str);

    static int TTPlayerMagicCode(const string& artist, const string& title, unsigned int id);

    opencc::SimpleConverter& converter;

public:
    TTPlayer(opencc::SimpleConverter& converter) : converter(converter) {}

    void searchLyrics(const Track& track, std::function<void(std::vector<CLyric>)> appendResultCallback) override;
};

class Xiami : public CLyricProvider {
    struct XiamiResult {
        XiamiResult(string title, string album, string artist, string coverImageUrl, string lyricUrl) : title(
                std::move(title)), album(std::move(album)), artist(std::move(artist)), coverImageUrl(
                std::move(coverImageUrl)), lyricUrl(std::move(lyricUrl)), distance(INT_MAX) {}

        XiamiResult(string title, string album, string artist, string coverImageUrl, string lyricUrl,
                    const string& targetTitle, const string& targetArtist);

        string title, album, artist, coverImageUrl, lyricUrl;
        int distance;
    };

public:
    void searchLyrics(const Track& track, std::function<void(std::vector<CLyric>)> appendResultCallback) override;
};

class Kugou : public CLyricProvider {
    struct KugouResult {
        KugouResult(string title, string artist, string id, string accessKey, int duration) : title(std::move(title)),
                                                                                              artist(std::move(artist)),
                                                                                              id(std::move(id)),
                                                                                              accessKey(std::move(
                                                                                                      accessKey)),
                                                                                              duration(duration),
                                                                                              distance(INT_MAX) {}

        KugouResult(string title, string artist, string id, string accessKey, int duration, const string& targetTitle,
                    const string& targetArtist);

        string title, artist, id, accessKey;
        int duration, distance;
    };

    static string decryptKrc(const string& krcString, bool base64Parse = false);

public:
    void searchLyrics(const Track& track, std::function<void(std::vector<CLyric>)> appendResultCallback) override;
};

class QQMusic : public CLyricProvider {
    struct QQMusicResult {
        QQMusicResult(string title, string artist, string album, string songmid, int albumid, int interval) : title(
                std::move(title)), artist(std::move(artist)), album(std::move(album)), songmid(std::move(songmid)),
                                                                                                              albumid(albumid),
                                                                                                              duration(
                                                                                                                      interval),
                                                                                                              distance(
                                                                                                                      INT_MAX) {}

        QQMusicResult(string title, string artist, string album, string songmid, int albumid, int interval,
                      const string& targetTitle, const string& targetArtist);

        string title, artist, album, songmid;
        int albumid, duration, distance;
    };

//    opencc::SimpleConverter& converter;

public:
//    QQMusic(opencc::SimpleConverter& converter) : converter(converter) {}

    void searchLyrics(const Track& track, std::function<void(std::vector<CLyric>)> appendResultCallback) override;
};

class Netease : public CLyricProvider {
    struct NeteaseResult {
        NeteaseResult(string title, string artist, string album, string coverImageUrl, int id, int duration) : title(
                std::move(title)), artist(std::move(artist)), album(std::move(album)), coverImageUrl(
                std::move(coverImageUrl)), id(id), duration(duration / 1000), distance(INT_MAX) {}

        NeteaseResult(string title, string artist, string album, string coverImageUrl, int id, int duration,
                      const string& targetTitle, const string& targetArtist);

        string title, artist, album, coverImageUrl;
        int id, duration, distance;
    };

public:
    void searchLyrics(const Track& track, std::function<void(std::vector<CLyric>)> appendResultCallback) override;
};

#endif //CRYSTALLYRICS_CLYRICPROVIDER_H
