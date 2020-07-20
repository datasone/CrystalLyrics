//
// Created by datasone.
// This file is part of CrystalLyrics.
//

#ifndef CRYSTALLYRICS_CLYRICPROVIDER_H
#define CRYSTALLYRICS_CLYRICPROVIDER_H

#ifdef Q_OS_WIN
#define NOMINMAX // Eliminate Win32 min and max
#endif

#include "CLyric.h"
#include <curl/curl.h>
#include <cstdint>
#include <utility>
#include <map>
#include <regex>

namespace cLyric {

    class CLyricProvider {
    protected:
        CURL *curlHandle;
        std::string response;

        CLyricProvider();

        static size_t storeCURLResponse(void *buffer, size_t size, size_t nmemb, void *userp);

    public:
        virtual ~CLyricProvider();

        void normalizeName(std::string &str, bool isHttpParam = false);

        std::string normalizeName(const std::string &str, bool isHttpParam = false);

        virtual void
        searchLyrics(const Track &track, std::function<void(std::vector<CLyric>)> appendResultCallback) = 0;
    };

    class Gecimi : public CLyricProvider {

    public:
        void searchLyrics(const Track &track, std::function<void(std::vector<CLyric>)> appendResultCallback) override;
    };

    class Xiami : public CLyricProvider {
        struct XiamiResult {
            XiamiResult(std::string title, std::string album, std::string artist, std::string coverImageUrl,
                        std::string lyricUrl) : title(std::move(title)),
                                                album(std::move(album)), artist(std::move(artist)),
                                                coverImageUrl(std::move(coverImageUrl)),
                                                lyricUrl(std::move(lyricUrl)), distance(INT_MAX) {}

            XiamiResult(std::string title, std::string album, std::string artist, std::string coverImageUrl,
                        std::string lyricUrl,
                        const std::string &targetTitle, const std::string &targetArtist);

            std::string title, album, artist, coverImageUrl, lyricUrl;
            int distance;
        };

    public:
        void searchLyrics(const Track &track, std::function<void(std::vector<CLyric>)> appendResultCallback) override;
    };

    class Kugou : public CLyricProvider {
        struct KugouResult {
            KugouResult(std::string title, std::string artist, std::string id, std::string accessKey,
                        int duration) : title(std::move(title)), artist(std::move(artist)), id(std::move(id)),
                                        accessKey(std::move(accessKey)), duration(duration), distance(INT_MAX) {}

            KugouResult(std::string title, std::string artist, std::string id, std::string accessKey,
                        int duration, const std::string &targetTitle, const std::string &targetArtist);

            std::string title, artist, id, accessKey;
            int duration, distance;
        };

        static std::string decryptKrc(const std::string &krcString, bool base64Parse = false);

    public:
        void searchLyrics(const Track &track, std::function<void(std::vector<CLyric>)> appendResultCallback) override;
    };

    class QQMusic : public CLyricProvider {
        struct QQMusicResult {
            QQMusicResult(std::string title, std::string artist, std::string album, std::string songmid,
                          int albumid, int interval) : title(std::move(title)), artist(std::move(artist)),
                                                       album(std::move(album)), songmid(std::move(songmid)),
                                                       albumid(albumid), duration(interval), distance(INT_MAX) {}

            QQMusicResult(std::string title, std::string artist, std::string album, std::string songmid, int albumid,
                          int interval, const std::string &targetTitle, const std::string &targetArtist);

            std::string title, artist, album, songmid;
            int albumid, duration, distance;
        };

        std::map<std::string, std::string> xmlSpecialChars = {
                {"&amp;",  "&"},
                {"&lt;",   "<"},
                {"&gt;",   ">"},
                {"&quot;", "\""},
                {"&apos;", "'"}
        };

        void unescapeXmlSpeChars(std::string &str) {
            for (auto const&[find, replace]: xmlSpecialChars)
                str = std::regex_replace(str, std::regex(find), replace);
        }

//    opencc::SimpleConverter& converter;

    public:
//    QQMusic(opencc::SimpleConverter& converter) : converter(converter) {}

        void searchLyrics(const Track &track, std::function<void(std::vector<CLyric>)> appendResultCallback) override;
    };

    class Netease : public CLyricProvider {
        struct NeteaseResult {
            NeteaseResult(std::string title, std::string artist, std::string album,
                          std::string coverImageUrl, int id, int duration) : title(std::move(title)),
                                                                             artist(std::move(artist)),
                                                                             album(std::move(album)),
                                                                             coverImageUrl(std::move(coverImageUrl)),
                                                                             id(id), duration(duration / 1000),
                                                                             distance(INT_MAX) {}

            NeteaseResult(std::string title, std::string artist, std::string album, std::string coverImageUrl, int id,
                          int duration,
                          const std::string &targetTitle, const std::string &targetArtist);

            std::string title, artist, album, coverImageUrl;
            int id, duration, distance;
        };

    public:
        void searchLyrics(const Track &track, std::function<void(std::vector<CLyric>)> appendResultCallback) override;
    };

}

#endif //CRYSTALLYRICS_CLYRICPROVIDER_H
