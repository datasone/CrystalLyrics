//
// Created by datasone.
// This file is part of CrystalLyrics.
//

#include "CLyricProvider.h"
#include "CLyricUtils.h"
#include "Base64.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <numeric>
#include <regex>

using nlohmann::json;

using namespace cLyric;

CLyricProvider::CLyricProvider() {
    curlHandle = curl_easy_init();

    curl_easy_setopt(curlHandle, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curlHandle, CURLOPT_TIMEOUT, 10);
    curl_easy_setopt(curlHandle, CURLOPT_CONNECTTIMEOUT, 10);
    curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, storeCURLResponse);
    curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curlHandle, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curlHandle, CURLOPT_ACCEPT_ENCODING, "br, gzip, deflate");
    curl_easy_setopt(curlHandle, CURLOPT_USERAGENT, "CrystalLyrics/0.0.1");
    curl_easy_setopt(curlHandle, CURLOPT_COOKIEFILE, "");
}

CLyricProvider::~CLyricProvider() {
    curl_easy_cleanup(curlHandle);
}

size_t CLyricProvider::storeCURLResponse(void *buffer, size_t size, size_t nmemb, void *userp) {
    auto *targetString = static_cast<std::string *>(userp);
    auto *responseBuffer = static_cast<char *>(buffer);
    targetString->append(responseBuffer, size * nmemb);
    return size * nmemb;
}

void CLyricProvider::normalizeName(std::string &str, bool isHttpParam, bool noSpecialChars) {
    if (noSpecialChars) {
        for (auto &c: str) {
            if (static_cast<unsigned char>(c) > 127) {
                // Non-Ascii char
                continue;
            }
            switch (c) {
                case '!':
                case '"':
                case '#':
                case '$':
                case '%':
                case '&':
                case '\'':
                case '(':
                case ')':
                case '*':
                case '+':
                case ',':
                case '-':
                case '.':
                case '/':
                case ':':
                case ';':
                case '<':
                case '=':
                case '>':
                case '?':
                case '@':
                case '[':
                case '\\':
                case ']':
                case '^':
                case '_':
                case '`':
                case '{':
                case '|':
                case '}':
                case '~':
                    c = ' ';
            }
        }
    }

    if (isHttpParam) {
        char *escaped = curl_easy_escape(curlHandle, str.c_str(), 0);
        str = escaped;
        curl_free(escaped);
    }
}

std::string CLyricProvider::normalizeName(const std::string &str, bool isHttpParam, bool noSpecialChars) {
    std::string s = str;
    normalizeName(s, isHttpParam, noSpecialChars);
    return s;
}

void Gecimi::searchLyrics(const Track &track, std::function<void(std::vector<CLyric>)> appendResultCallback) {
    std::string url = "http://gecimi.com/api/lyric/" + track.title;

    if (track.artist.empty()) {
        curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
    } else {
        url.append("/");
        url.append(track.artist);
        curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
    }

    response.clear();
    CURLcode curlResult = curl_easy_perform(curlHandle);

    if (curlResult != CURLE_OK)
        return;

    try {
        auto searchResult = json::parse(response);
        std::vector<int> distances(searchResult["result"].size());
        std::vector<CLyric> lyrics;
        std::map<int, std::string> artistMap, coverMap;

        for (size_t i = 0; i < searchResult["result"].size(); ++i) {
            auto item = searchResult["result"][i];

            // Get artist name
            std::string artistName = track.artist;
            int artistId = item["artist_id"];
            int retryCount, maxTries = 3;
            while (true) {
                try {
                    if (artistMap.find(artistId) == artistMap.end()) {
                        std::string artistInfoUrl = "http://gecimi.com/api/artist/";
                        artistInfoUrl.append(std::to_string(artistId));
                        curl_easy_setopt(curlHandle, CURLOPT_URL, artistInfoUrl.c_str());
                        response.clear();
                        curl_easy_perform(curlHandle);
                        auto artistInfo = json::parse(response);
                        artistMap[artistId] = artistInfo["result"]["name"];
                    }
                    break;
                } catch (json::exception &e) {
                    if (++retryCount == maxTries) break;
                }
            }
            artistName = artistMap[artistId];
            item["artist"] = artistName;

            distances[i] = stringDistance(item["song"], track.title) + stringDistance(artistName, track.artist) / 2;
        }

        std::vector<size_t> sorted_indexes = sort_indexes(distances);

        int count = 0;
        for (size_t sorted_index : sorted_indexes) {
            auto item = searchResult["result"][sorted_index];

            std::string coverImageUrl;
            // Get album cover image
            int albumId = item["aid"];
            try {
                if (coverMap.find(albumId) == coverMap.end()) {
                    std::string albumInfoUrl = "http://gecimi.com/api/cover/";
                    albumInfoUrl.append(std::to_string(albumId));
                    curl_easy_setopt(curlHandle, CURLOPT_URL, albumInfoUrl.c_str());
                    response.clear();
                    curl_easy_perform(curlHandle);
                    auto albumInfo = json::parse(response);
                    coverMap[albumId] = albumInfo["result"]["cover"];
                }
            } catch (json::exception &e) {
                // Leave image empty
            }
            coverImageUrl = coverMap[albumId];

            curl_easy_setopt(curlHandle, CURLOPT_URL, item["lrc"].get<std::string>().c_str());
            response.clear();
            if (curl_easy_perform(curlHandle) != CURLE_OK)
                continue;

            if (response.empty() || response[0] != '[')
                continue;

            lyrics.emplace_back(response,
                                Track(item["song"], track.album, item["artist"],
                                      coverImageUrl, "Gecimi", track.duration));
            if (++count > 5) break;
        }

        if (!lyrics.empty())
            appendResultCallback(std::move(lyrics));

    } catch (json::exception &e) {
        return;
    }
}

void Xiami::searchLyrics(const Track &track, std::function<void(std::vector<CLyric>)> appendResultCallback) {
    curl_easy_setopt(curlHandle, CURLOPT_POST, 1);
    curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDS, "");
    curl_easy_setopt(curlHandle, CURLOPT_REFERER, "http://h.xiami.com/");

    std::string url = "http://api.xiami.com/web?v=2.0&r=search%2Fsongs&limit=10";
    url.append("&key=").append(normalizeName(track.title + " " + track.artist, true));
    url.append("&app_key=1");

    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
    response.clear();
    CURLcode curlResult = curl_easy_perform(curlHandle);

    if (curlResult != CURLE_OK)
        return;

    try {
        auto searchResult = json::parse(response);
        std::vector<XiamiResult> results;
        std::vector<CLyric> lyrics;

        for (const auto &songItem : searchResult["data"]["songs"]) {
            results.emplace_back(songItem["song_name"], songItem["album_name"], songItem["artist_name"],
                                 songItem["album_logo"], songItem["lyric"], track.title, track.artist);
        }

        std::stable_sort(results.begin(), results.end(),
                         [](const XiamiResult &res1, const XiamiResult &res2) {
                             return res1.distance < res2.distance;
                         });

        curl_easy_setopt(curlHandle, CURLOPT_POST, 0);

        int count = 0;
        for (const auto &result: results) {
            curl_easy_setopt(curlHandle, CURLOPT_URL, result.lyricUrl.c_str());
            response.clear();
            if (curl_easy_perform(curlHandle) != CURLE_OK)
                continue;

            lyrics.emplace_back(response,
                                Track(result.title, result.album, result.artist,
                                      result.coverImageUrl, "Xiami", track.duration), LyricStyle::XiamiStyle);

            if (++count > 5) break;
        }

        if (!lyrics.empty())
            appendResultCallback(std::move(lyrics));

    } catch (json::exception &e) {
        return;
    }
}

Xiami::XiamiResult::XiamiResult(std::string title, std::string album, std::string artist, std::string coverImageUrl,
                                std::string lyricUrl, const std::string &targetTitle, const std::string &targetArtist)
        : title(std::move(title)), album(std::move(album)), artist(std::move(artist)),
          coverImageUrl(std::move(coverImageUrl)), lyricUrl(std::move(lyricUrl)) {
    distance = stringDistance(this->title, targetTitle) + stringDistance(this->artist, targetArtist) / 2;
}

void Kugou::searchLyrics(const Track &track, std::function<void(std::vector<CLyric>)> appendResultCallback) {
    std::string url = "http://lyrics.kugou.com/search";
    url.append("?keyword=").append(normalizeName(track.title + " " + track.artist, true));
    url.append("&duration=").append(std::to_string(track.duration * 1000));
    url.append("&client=pc&ver=1&man=yes");

    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
    response.clear();
    CURLcode curlResult = curl_easy_perform(curlHandle);

    if (curlResult != CURLE_OK)
        return;

    try {
        auto searchResult = json::parse(response);
        std::vector<KugouResult> results;
        std::vector<CLyric> lyrics;
        for (auto searchItem: searchResult["candidates"]) {
            if (searchItem["score"] < 70) // Too low, basically no relationships
                continue;
            results.emplace_back(searchItem["song"], searchItem["singer"], searchItem["id"], searchItem["accesskey"],
                                 searchItem["duration"], track.title, track.artist);
        }

        std::stable_sort(results.begin(), results.end(),
                         [](const KugouResult &res1, const KugouResult &res2) {
                             return res1.distance < res2.distance;
                         });

        int count = 0;
        for (const auto &result: results) {
            std::string lyricUrl = "http://lyrics.kugou.com/download";
            lyricUrl.append("?id=").append(result.id);
            lyricUrl.append("&accesskey=").append(result.accessKey);
            lyricUrl.append("&fmt=krc&charset=utf8&client=pc&var=1");

            curl_easy_setopt(curlHandle, CURLOPT_URL, lyricUrl.c_str());
            response.clear();
            if (curl_easy_perform(curlHandle) != CURLE_OK)
                continue;

            auto lyricResult = json::parse(response);

            std::string lyricText = decryptKrc(lyricResult["content"], true);
            lyrics.emplace_back(lyricText, Track(result.title, "", result.artist,
                                                 "", "Kugou", track.duration), LyricStyle::KugouStyle);

            if (++count > 5) break;
        }

        if (!lyrics.empty())
            appendResultCallback(std::move(lyrics));

    } catch (json::exception &e) {
        return;
    }

}

std::string Kugou::decryptKrc(const std::string &krcString, bool base64Parse) {
    std::string encodedString;
    if (base64Parse) {
        macaron::Base64::Decode(krcString, encodedString);
    } else {
        encodedString = krcString;
    }

    unsigned char decodeKey[] = {64, 71, 97, 119, 94, 50, 116, 71, 81, 54, 49, 45, 206, 210, 110, 105};
    if (encodedString.rfind("krc1", 0) == 0) {
        for (size_t i = 4; i < encodedString.length(); ++i) {
            char &byte = encodedString[i];
            byte = byte ^ decodeKey[(i - 4) & 0b1111u];
        }
        std::string decompressed;
        if (zlibInflate(encodedString.substr(4), decompressed))
            return decompressed;
    }
    return std::string();
}

Kugou::KugouResult::KugouResult(std::string title, std::string artist, std::string id, std::string accessKey,
                                int duration, const std::string &targetTitle, const std::string &targetArtist)
        : title(std::move(title)), artist(std::move(artist)), id(std::move(id)), accessKey(std::move(accessKey)),
          duration(duration) {
    distance = stringDistance(this->title, targetTitle) + stringDistance(this->artist, targetArtist) / 2;
}

void QQMusic::searchLyrics(const Track &track, std::function<void(std::vector<CLyric>)> appendResultCallback) {
    std::string url = "http://c.y.qq.com/soso/fcgi-bin/client_search_cp";
    url.append("?w=").append(normalizeName(track.title + "+" + track.artist, true));
    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
    response.clear();
    CURLcode curlResult = curl_easy_perform(curlHandle);

    if (curlResult != CURLE_OK)
        return;

    try {
        std::vector<CLyric> lyrics;
        std::vector<QQMusicResult> results;

        response = response.substr(9, response.length() - 10); // remove "callback( {...} )"
        auto searchResult = json::parse(response);

        for (const auto &searchItem: searchResult["data"]["song"]["list"]) {
            results.emplace_back(searchItem["songname"], searchItem["singer"][0]["name"], searchItem["albumname"],
                                 searchItem["songmid"], searchItem["albumid"], searchItem["interval"], track.title,
                                 track.artist);
        }

        std::stable_sort(results.begin(), results.end(),
                         [](const QQMusicResult &res1, const QQMusicResult &res2) {
                             return res1.distance < res2.distance;
                         });

        curl_easy_setopt(curlHandle, CURLOPT_REFERER, "http://y.qq.com/portal/player.html");
        int count = 0;
        for (const auto &result: results) {
            std::string lyricURL = "http://c.y.qq.com/lyric/fcgi-bin/fcg_query_lyric_new.fcg";
            lyricURL.append("?songmid=").append(result.songmid);
            lyricURL.append("&g_tk=").append("5381");
            curl_easy_setopt(curlHandle, CURLOPT_URL, lyricURL.c_str());
            response.clear();
            curlResult = curl_easy_perform(curlHandle);

            if (curlResult != CURLE_OK)
                continue;

            response = response.substr(18, response.length() - 19); // remove "(MusicJsonCallback {...} )"
            auto lyricResponse = json::parse(response);

            if (lyricResponse["lyric"].is_null())
                continue;

            std::string lyric = lyricResponse["lyric"];

            if (lyric.empty())
                continue;

            std::string coverImageUrl = "http://imgcache.qq.com/music/photo/album/";
            coverImageUrl.append(std::to_string(result.albumid % 100)).append("/albumpic_").append(
                    std::to_string(result.albumid)).append("_0.jpg");

            std::string decodedLyric, decodedTrans;
            macaron::Base64::Decode(lyric, decodedLyric);
            unescapeXmlSpeChars(decodedLyric);
            CLyric cLyric(decodedLyric,
                          Track(
                                  result.title, result.album, result.artist, coverImageUrl,
                                  "QQMusic", result.duration
                                  ),
                          LyricStyle::CLrcStyle);

            if (!lyricResponse["trans"].is_null()) {
                std::string trans = lyricResponse["trans"];
                if (trans.empty())
                    goto emptyTransBreak;
                macaron::Base64::Decode(trans, decodedTrans);
                unescapeXmlSpeChars(decodedTrans);
                CLyric transLyric = CLyric(decodedTrans, LyricStyle::CLrcStyle);
                cLyric.mergeTranslation(transLyric);
            }

emptyTransBreak:
            if (cLyric.lyrics.size() == 1 && cLyric.lyrics[0].content == "此歌曲为没有填词的纯音乐，请您欣赏") {
                cLyric.track.instrumental = true;
                cLyric.lyrics.clear();
            }
            lyrics.push_back(cLyric);

            if (++count > 5) break;
        }

        if (!lyrics.empty())
            appendResultCallback(std::move(lyrics));

    } catch (json::exception &e) {
        return;
    }
}

QQMusic::QQMusicResult::QQMusicResult(std::string title, std::string artist, std::string album, std::string songmid,
                                      int albumid, int interval, const std::string &targetTitle,
                                      const std::string &targetArtist)
        : title(std::move(title)), artist(std::move(artist)), album(std::move(album)), songmid(std::move(songmid)),
          albumid(albumid), duration(interval) {
    distance = stringDistance(this->title, targetTitle) + stringDistance(this->artist, targetArtist) / 2;
}

void Netease::searchLyrics(const Track &track, std::function<void(std::vector<CLyric>)> appendResultCallback) {
    bool firstTry = true;

    std::string url = "http://music.163.com/api/search/pc";
    url.append("?s=").append(normalizeName(track.title + " " + track.artist, true));
    url.append("&offset=0").append("&limit=10").append("&type=1");
    curl_easy_setopt(curlHandle, CURLOPT_REFERER, "http://music.163.com/");
    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
retry:
    response.clear();
    CURLcode curlResult = curl_easy_perform(curlHandle);

    if (curlResult != CURLE_OK)
        return;

    try {
        std::vector<CLyric> lyrics;
        std::vector<NeteaseResult> results;
        auto searchResult = json::parse(response);

        if (firstTry && searchResult["code"].is_number() && searchResult["code"] != 200) {
            firstTry = false;
            goto retry;
        }

        for (auto &searchItem: searchResult["result"]["songs"]) {
            results.emplace_back(searchItem["name"], searchItem["artists"][0]["name"], searchItem["album"]["name"],
                                 searchItem["album"]["picUrl"], searchItem["id"], searchItem["duration"], track.title,
                                 track.artist);
        }

        std::stable_sort(results.begin(), results.end(),
                         [](const NeteaseResult &res1, const NeteaseResult &res2) {
                             return res1.distance < res2.distance;
                         });

        int count = 0;
        for (const auto &result: results) {
            std::string lyricURL = "http://music.163.com/api/song/lyric";
            lyricURL.append("?id=").append(std::to_string(result.id));
            lyricURL.append("&lv=1").append("&kv=1").append("&tv=-1");
            curl_easy_setopt(curlHandle, CURLOPT_URL, lyricURL.c_str());
            response.clear();
            curlResult = curl_easy_perform(curlHandle);

            if (curlResult != CURLE_OK)
                continue;

            auto lyricResult = json::parse(response);

            if (result.distance == 0 && !lyricResult["nolyric"].is_null() && lyricResult["nolyric"]) {
                CLyric cLyric = CLyric(
                        Track(result.title, result.album, result.artist,
                              result.coverImageUrl, "Netease", result.duration, true), std::vector<CLyricItem>());
                lyrics.push_back(cLyric);
            }

            if (lyricResult["lrc"]["lyric"].is_null())
                continue;
            CLyric cLyric = CLyric(lyricResult["lrc"]["lyric"],
                                   Track(result.title, result.album, result.artist,
                                         result.coverImageUrl, "Netease", result.duration), LyricStyle::CLrcStyle);

            if (!lyricResult["tlyric"]["lyric"].is_null()) {
                CLyric trans = CLyric(lyricResult["tlyric"]["lyric"], LyricStyle::CLrcStyle);
                cLyric.mergeTranslation(trans);
            }

            lyrics.push_back(cLyric);

            if (++count > 5) break;
        }

        if (!lyrics.empty())
            appendResultCallback(std::move(lyrics));

    } catch (json::exception &e) {
        return;
    }
}

Netease::NeteaseResult::NeteaseResult(std::string title, std::string artist, std::string album,
                                      std::string coverImageUrl, int id, int duration, const std::string &targetTitle,
                                      const std::string &targetArtist)
        : title(std::move(title)), artist(std::move(artist)), album(std::move(album)),
          coverImageUrl(std::move(coverImageUrl)), id(id), duration(duration / 1000) {
    distance = stringDistance(this->title, targetTitle) + stringDistance(this->artist, targetArtist) / 2;
}

void THBWiki::searchLyrics(const Track &track, std::function<void(std::vector<CLyric>)> appendResultCallback) {
    long responseCode;

    std::string url = "https://cd.thwiki.cc/lyrics/";
    url.append(normalizeName(trim_copy(track.title), true)).append(".all.lrc");
    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
    response.clear();
    CURLcode curlResult = curl_easy_perform(curlHandle);

    if (curlResult != CURLE_OK)
        return;
    curl_easy_getinfo(curlHandle, CURLINFO_RESPONSE_CODE, &responseCode);

    if (responseCode != 200)
        return;

    std::regex transPattern(R"((\[.*\])(.*) *\/\/ *(.*))");
    std::string lyric = std::regex_replace(response, transPattern, "$1$2\n$1[tr]$3");

    std::regex tagPattern(R"(\[(ti|ar|al):(.*)\])");
    std::string lyricContent = std::regex_replace(lyric, tagPattern, "[$1]$2");

    CLyric cLyric(lyricContent, CLrcStyle);
    cLyric.track.source = "THBWiki";

    std::vector<CLyric> lyrics = {cLyric};

    appendResultCallback(lyrics);
}
