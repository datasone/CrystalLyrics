//
// Created by datasone.
// This file is part of CrystalLyrics.
//

#include "CLyricProvider.h"
#include "CLyricUtils.h"
#include "Base64.h"
#include <nlohmann/json.hpp>
#include <regex>
#include <sstream>
#include <iomanip>
#include <tinyxml2.h>
#include <algorithm>
#include <numeric>

using nlohmann::json;

CLyricProvider::CLyricProvider() {
    curlHandle = curl_easy_init();

    curl_easy_setopt(curlHandle, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curlHandle, CURLOPT_TIMEOUT, 10);
    curl_easy_setopt(curlHandle, CURLOPT_CONNECTTIMEOUT, 10);
    curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, storeCURLResponse);
    curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curlHandle, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curlHandle, CURLOPT_USERAGENT, "CrystalLyrics/0.0.1");
}

CLyricProvider::~CLyricProvider() {
    curl_easy_cleanup(curlHandle);
}

size_t CLyricProvider::storeCURLResponse(void* buffer, size_t size, size_t nmemb, void* userp) {
    auto* targetString = static_cast<std::string*>(userp);
    auto* responseBuffer = static_cast<char*>(buffer);
    targetString->append(responseBuffer, size * nmemb);
    return size * nmemb;
}

void CLyricProvider::normalizeName(string& str, bool isHttpParam) {
    for (auto& c: str) {
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

    if (isHttpParam) {
        char* escaped = curl_easy_escape(curlHandle, str.c_str(), 0);
        str = escaped;
        curl_free(escaped);
    }
}

string CLyricProvider::normalizeName(const string& str, bool isHttpParam) {
    string s = str;
    normalizeName(s, isHttpParam);
    return s;
}

void Gecimi::searchLyrics(const Track& track, std::function<void(std::vector<CLyric>)> appendResultCallback) {
    string url = "http://gecimi.com/api/lyric/" + track.title;

    if (track.artist.empty()) {
        curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
    } else {
        url.append("/");
        url.append(track.artist);
        curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
    }

    response.clear();
    curl_easy_perform(curlHandle);

    try {
        auto searchResult = json::parse(response);
        std::vector<int> distances(searchResult["result"].size());
        std::vector<CLyric> lyrics;
        std::map<int, string> artistMap, coverMap;

        for (int i = 0; i < searchResult["result"].size(); ++i) {
            auto item = searchResult["result"][i];

            // Get artist name
            string artistName = track.artist;
            int artistId = item["artist_id"];
            int retryCount, maxTries = 3;
            while (true) {
                try {
                    if (artistMap.find(artistId) == artistMap.end()) {
                        string artistInfoUrl = "http://gecimi.com/api/artist/";
                        artistInfoUrl.append(std::to_string(artistId));
                        curl_easy_setopt(curlHandle, CURLOPT_URL, artistInfoUrl.c_str());
                        response.clear();
                        curl_easy_perform(curlHandle);
                        auto artistInfo = json::parse(response);
                        artistMap[artistId] = artistInfo["result"]["name"];
                    }
                    break;
                } catch (json::exception& e) {
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

            string coverImageUrl;
            // Get album cover image
            int albumId = item["aid"];
            try {
                if (coverMap.find(albumId) == coverMap.end()) {
                    string albumInfoUrl = "http://gecimi.com/api/cover/";
                    albumInfoUrl.append(std::to_string(albumId));
                    curl_easy_setopt(curlHandle, CURLOPT_URL, albumInfoUrl.c_str());
                    response.clear();
                    curl_easy_perform(curlHandle);
                    auto albumInfo = json::parse(response);
                    coverMap[albumId] = albumInfo["result"]["cover"];
                }
            } catch (json::exception& e) {
                // Leave image empty
            }
            coverImageUrl = coverMap[albumId];

            curl_easy_setopt(curlHandle, CURLOPT_URL, item["lrc"].get<string>().c_str());
            response.clear();
            if (curl_easy_perform(curlHandle) != CURLE_OK)
                continue;

            if (response.empty() || response[0] != '[')
                continue;

            lyrics.emplace_back(response, Track(item["song"], track.album, item["artist"], coverImageUrl, "Gecimi",
                                                track.duration));
            if (++count > 5) break;
        }

        if (!lyrics.empty())
            appendResultCallback(std::move(lyrics));

    } catch (json::exception& e) {
        return;
    }
}

TTPlayer::TTPlayerResult::TTPlayerResult(string title, string artist, int id, const string& targetTitle,
                                         const string& targetArtist) : title(std::move(title)),
                                                                       artist(std::move(artist)), id(id) {
    distance = stringDistance(this->title, targetTitle) + stringDistance(this->artist, targetArtist) / 2;
}

void TTPlayer::searchLyrics(const Track& track, std::function<void(std::vector<CLyric>)> appendResultCallback) {
    string searchUrl = "http://ttlrcct.qianqian.com/dll/lyricsvr.dll?sh";
    searchUrl.append("?Artist=").append(toUTF16LEHexString(normalizeName(converter.Convert(track.artist))));
    searchUrl.append("&Title=").append(toUTF16LEHexString(normalizeName(converter.Convert(track.title))));
    searchUrl.append("&Flags=0");

    curl_easy_setopt(curlHandle, CURLOPT_URL, searchUrl.c_str());
    response.clear();
    curl_easy_perform(curlHandle);

    std::vector<CLyric> lyrics;
    tinyxml2::XMLDocument songXML;
    if (songXML.Parse(response.c_str()) == tinyxml2::XML_SUCCESS) {
        std::vector<TTPlayerResult> results;
        tinyxml2::XMLNode* lrcNode = songXML.FirstChildElement("result")->FirstChildElement("lrc");
        if (!lrcNode)
            return;
        do {
            const auto* id = lrcNode->ToElement()->Attribute("id");
            const auto* artist = lrcNode->ToElement()->Attribute("artist");
            const auto* title = lrcNode->ToElement()->Attribute("title");
            if (!(id && artist && title)) {
                continue;
            }

            int numId = 0;

            try {
                numId = std::stoi(id);
            } catch (std::exception& e) {
                continue;
            }

            results.emplace_back(title, artist, numId, track.title, track.artist);
        } while ((lrcNode = lrcNode->NextSibling()));

        std::stable_sort(results.begin(), results.end(),
                         [](const TTPlayerResult& res1, const TTPlayerResult& res2) {
                             return res1.distance < res2.distance;
                         });

        int count = 0;
        for (const auto& result : results) {
            // Download lyric
            string lyricUrl = "http://ttlrcct.qianqian.com/dll/lyricsvr.dll?dl";
            lyricUrl.append("?Id=").append(std::to_string(result.id));
            lyricUrl.append("&Code=").append(std::to_string(TTPlayerMagicCode(result.artist, result.title, result.id)));
            curl_easy_setopt(curlHandle, CURLOPT_URL, lyricUrl.c_str());
            response.clear();
            if (curl_easy_perform(curlHandle) != CURLE_OK)
                continue;

            if (response.empty() || response[0] != '[')
                continue;

            lyrics.emplace_back(response,
                                Track(result.title, track.album, result.artist, "", "TTPlayer", track.duration));
            if (++count > 5) break;
        }

        if (!lyrics.empty())
            appendResultCallback(std::move(lyrics));

    }
}

string TTPlayer::toUTF16LEHexString(const string& str) {
    // Requires str encoded in UTF-8
    std::ostringstream ss;
    int i = 0;
    std::vector<unsigned int> unicodes;
    while (i < str.length()) {
        auto ch = static_cast<unsigned char>(str[i++]);
        if (ch < 0x80u) {
            // ASCII (UTF-8 single byte)
            unicodes.push_back(ch);
        } else if (ch < 0xE0u) {
            // UTF-8 double byte
            auto ch2 = static_cast<unsigned char>(str[i++]);
            unicodes.push_back(((ch - 0xC0u) << 6u) + (ch2 - 0x80u));
        } else if (ch < 0xF0u) {
            // UTF-8 triple byte
            auto ch2 = static_cast<unsigned char>(str[i++]);
            auto ch3 = static_cast<unsigned char>(str[i++]);
            unicodes.push_back(((ch - 0xE0u) << 12u) + ((ch2 - 0x80u) << 6u) + (ch3 - 0x80u));
        } else {
            // UTF-8 quadruple byte
            auto ch2 = static_cast<unsigned char>(str[i++]);
            auto ch3 = static_cast<unsigned char>(str[i++]);
            auto ch4 = static_cast<unsigned char>(str[i++]);
            unicodes.push_back(((ch - 0xE0u) << 18u) + ((ch2 - 0x80u) << 12u) + ((ch3 - 0x80u) << 6u) + (ch4 - 0x80u));
        }
    }
    for (const unsigned int code: unicodes) {
        if (code < 0x100u) {
            ss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << code << "00";
        } else if (code < 0x10000u) {
            unsigned int bl = code & 0xFFu, bh = code >> 8u;
            ss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << bl;
            ss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << bh;
        } else {
            unsigned int be = ((code - 0x10000u) >> 10u) + 0xD800u;
            unsigned int b = (code & 0x3FFu) + 0xDC00u;
            unsigned int bel = be & 0xFFu, beh = be >> 8u;
            unsigned int bl = b & 0xFFu, bh = b >> 8u;
            ss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << bel;
            ss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << beh;
            ss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << bl;
            ss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << bh;
        }
    }
    return ss.str();
}

int TTPlayer::TTPlayerMagicCode(const string& artist, const string& title, unsigned int id) {
    // Requires strings encoded in UTF-8
    string infoString = artist + title;
    std::vector<unsigned char> code;
    code.reserve(infoString.length());

    for (auto& c: artist + title) {
        code.push_back(static_cast<unsigned char>(c));
    }

    // Magic
    uint32_t t1, t2, t3, t4, t5;
    t1 = (id & 0x0000FF00u) >> 8u;
    t2 = 0;
    if ((id & 0x00FF0000u) == 0)
        t3 = 0x000000FFu & ~t1;
    else
        t3 = 0x000000FFu & ((id & 0x00FF0000u) >> 16u);
    t3 = t3 | ((0x000000FFu & id) << 8u);
    t3 = t3 << 8u;
    t3 = t3 | (0x000000FFu & t1);
    t3 = t3 << 8u;
    if ((id & 0xFF000000u) == 0)
        t3 = t3 | (0x000000FFu & (~id));
    else
        t3 = t3 | (0x000000FFu & (id >> 24u));

    int j = infoString.length() - 1;
    while (j >= 0) {
        int c = code[j];
        if (c >= 0x80)
            c = c - 0x100;
        t1 = c + t2;
        t2 = t2 << (j % 2 + 4);
        t2 = t1 + t2;
        --j;
    }

    j = 0;
    t1 = 0;
    while (j < infoString.length()) {
        int c = code[j];
        if (c >= 0x80)
            c = c - 0x100;
        t4 = c + t1;
        t1 = t1 << (j % 2 + 3);
        t1 = t1 + t4;
        ++j;
    }

    t5 = t2 ^ t3;
    t5 = t5 + (t1 | id);
    t5 = t5 * (t1 | t3);
    t5 = t5 * (t2 ^ id);

    return static_cast<int>(t5);
}

void Xiami::searchLyrics(const Track& track, std::function<void(std::vector<CLyric>)> appendResultCallback) {
    curl_easy_setopt(curlHandle, CURLOPT_POST, 1);
    curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDS, "");
    curl_easy_setopt(curlHandle, CURLOPT_REFERER, "http://h.xiami.com/");

    string url = "http://api.xiami.com/web?v=2.0&r=search%2Fsongs&limit=10";
    url.append("&key=").append(normalizeName(track.title + " " + track.artist, true));
    url.append("&app_key=1");

    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
    response.clear();
    curl_easy_perform(curlHandle);

    try {
        auto searchResult = json::parse(response);
        std::vector<XiamiResult> results;
        std::vector<CLyric> lyrics;

        for (const auto& songItem : searchResult["data"]["songs"]) {
            results.emplace_back(songItem["song_name"], songItem["album_name"], songItem["artist_name"],
                                 songItem["album_logo"], songItem["lyric"], track.title, track.artist);
        }

        std::stable_sort(results.begin(), results.end(),
                         [](const XiamiResult& res1, const XiamiResult& res2) {
                             return res1.distance < res2.distance;
                         });

        curl_easy_setopt(curlHandle, CURLOPT_POST, 0);

        int count = 0;
        for (const auto& result: results) {
            curl_easy_setopt(curlHandle, CURLOPT_URL, result.lyricUrl.c_str());
            response.clear();
            if (curl_easy_perform(curlHandle) != CURLE_OK)
                continue;

            lyrics.emplace_back(response,
                                Track(result.title, result.album, result.artist, result.coverImageUrl, "Xiami",
                                      track.duration),
                                LyricStyle::XiamiStyle);

            if (++count > 5) break;
        }

        if (!lyrics.empty())
            appendResultCallback(std::move(lyrics));

    } catch (json::exception& e) {
        return;
    }
}

Xiami::XiamiResult::XiamiResult(string title, string album, string artist, string coverImageUrl, string lyricUrl,
                                const string& targetTitle, const string& targetArtist) : title(std::move(title)),
                                                                                         album(std::move(album)),
                                                                                         artist(std::move(artist)),
                                                                                         coverImageUrl(std::move(
                                                                                                 coverImageUrl)),
                                                                                         lyricUrl(std::move(lyricUrl)) {
    distance = stringDistance(this->title, targetTitle) + stringDistance(this->artist, targetArtist) / 2;
}

void Kugou::searchLyrics(const Track& track, std::function<void(std::vector<CLyric>)> appendResultCallback) {
    string url = "http://lyrics.kugou.com/search";
    url.append("?keyword=").append(normalizeName(track.title + " " + track.artist, true));
    url.append("&duration=").append(std::to_string(track.duration * 1000));
    url.append("&client=pc&ver=1&man=yes");

    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
    response.clear();
    curl_easy_perform(curlHandle);

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
                         [](const KugouResult& res1, const KugouResult& res2) {
                             return res1.distance < res2.distance;
                         });

        int count = 0;
        for (const auto& result: results) {
            string lyricUrl = "http://lyrics.kugou.com/download";
            lyricUrl.append("?id=").append(result.id);
            lyricUrl.append("&accesskey=").append(result.accessKey);
            lyricUrl.append("&fmt=krc&charset=utf8&client=pc&var=1");

            curl_easy_setopt(curlHandle, CURLOPT_URL, lyricUrl.c_str());
            response.clear();
            if (curl_easy_perform(curlHandle) != CURLE_OK)
                continue;

            auto lyricResult = json::parse(response);

            string lyricText = decryptKrc(lyricResult["content"], true);
            lyrics.emplace_back(lyricText, Track(result.title, "", result.artist, "", "Kugou", track.duration),
                                LyricStyle::KugouStyle);

            if (++count > 5) break;
        }

        if (!lyrics.empty())
            appendResultCallback(std::move(lyrics));

    } catch (json::exception& e) {
        return;
    }

}

string Kugou::decryptKrc(const string& krcString, bool base64Parse) {
    string encodedString;
    if (base64Parse) {
        macaron::Base64::Decode(krcString, encodedString);
    } else {
        encodedString = krcString;
    }

    unsigned char decodeKey[] = {64, 71, 97, 119, 94, 50, 116, 71, 81, 54, 49, 45, 206, 210, 110, 105};
    if (encodedString.rfind("krc1", 0) == 0) {
        for (size_t i = 4; i < encodedString.length(); ++i) {
            char& byte = encodedString[i];
            byte = byte ^ decodeKey[(i - 4) & 0b1111u];
        }
        string decompressed;
        if (zlibInflate(encodedString.substr(4), decompressed))
            return decompressed;
    }
    return string();
}

Kugou::KugouResult::KugouResult(string title, string artist, string id, string accessKey, int duration,
                                const string& targetTitle, const string& targetArtist) : title(std::move(title)),
                                                                                         artist(std::move(artist)),
                                                                                         id(std::move(id)), accessKey(
                std::move(accessKey)), duration(duration) {
    distance = stringDistance(this->title, targetTitle) + stringDistance(this->artist, targetArtist) / 2;
}

void QQMusic::searchLyrics(const Track& track, std::function<void(std::vector<CLyric>)> appendResultCallback) {
    string url = "http://c.y.qq.com/soso/fcgi-bin/client_search_cp";
    url.append("?w=").append(normalizeName(track.title + "+" + track.artist, true));
    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
    response.clear();
    auto result = curl_easy_perform(curlHandle);

    try {
        std::vector<CLyric> lyrics;
        std::vector<QQMusicResult> results;

        response = response.substr(9, response.length() - 10); // remove "callback( {...} )"
        auto searchResult = json::parse(response);

        for (const auto& searchItem: searchResult["data"]["song"]["list"]) {
            results.emplace_back(searchItem["songname"], searchItem["singer"][0]["name"], searchItem["albumname"],
                                 searchItem["songmid"], searchItem["albumid"], searchItem["interval"], track.title,
                                 track.artist);
        }

        std::stable_sort(results.begin(), results.end(),
                         [](const QQMusicResult& res1, const QQMusicResult& res2) {
                             return res1.distance < res2.distance;
                         });

        curl_easy_setopt(curlHandle, CURLOPT_REFERER, "http://y.qq.com/portal/player.html");
        int count = 0;
        for (const auto& result: results) {
            string lyricURL = "http://c.y.qq.com/lyric/fcgi-bin/fcg_query_lyric_new.fcg";
            lyricURL.append("?songmid=").append(result.songmid);
            lyricURL.append("&g_tk=").append("5381");
            curl_easy_setopt(curlHandle, CURLOPT_URL, lyricURL.c_str());
            response.clear();
            curl_easy_perform(curlHandle);

            response = response.substr(18, response.length() - 19); // remove "(MusicJsonCallback {...} )"
            auto lyricResponse = json::parse(response);

            if (lyricResponse["lyric"].is_null())
                continue;

            string lyric = lyricResponse["lyric"];

            if (lyric.empty())
                continue;

            string coverImageUrl = "http://imgcache.qq.com/music/photo/album/";
            coverImageUrl.append(std::to_string(result.albumid % 100)).append("/albumpic_").append(
                    std::to_string(result.albumid)).append("_0.jpg");

            string decodedLyric, decodedTrans;
            macaron::Base64::Decode(lyric, decodedLyric);
            CLyric cLyric = CLyric(decodedLyric,
                                   Track(result.title, result.album, result.artist, coverImageUrl, "QQMusic",
                                         result.duration), LyricStyle::CLrcStyle);

            if (!lyricResponse["trans"].is_null()) {
                string trans = lyricResponse["trans"];
                macaron::Base64::Decode(trans, decodedTrans);
                CLyric transLyric = CLyric(decodedTrans, LyricStyle::CLrcStyle);
                cLyric.mergeTranslation(transLyric);
            }

            lyrics.push_back(cLyric);

            if (++count > 5) break;
        }

        if (!lyrics.empty())
            appendResultCallback(std::move(lyrics));

    } catch (json::exception& e) {
        return;
    }
}

QQMusic::QQMusicResult::QQMusicResult(string title, string artist, string album, string songmid, int albumid,
                                      int interval, const string& targetTitle, const string& targetArtist) : title(
        std::move(title)), artist(std::move(artist)), album(std::move(album)), songmid(std::move(songmid)),
                                                                                                             albumid(albumid),
                                                                                                             duration(
                                                                                                                     interval) {
    distance = stringDistance(this->title, targetTitle) + stringDistance(this->artist, targetArtist) / 2;
}

void Netease::searchLyrics(const Track& track, std::function<void(std::vector<CLyric>)> appendResultCallback) {
    string url = "http://music.163.com/api/search/pc";
    url.append("?s=").append(normalizeName(track.title + " " + track.artist, true));
    url.append("&offset=0").append("&limit=10").append("&type=1");
    curl_easy_setopt(curlHandle, CURLOPT_REFERER, "http://music.163.com/");
    curl_easy_setopt(curlHandle, CURLOPT_URL, url.c_str());
    response.clear();
    curl_easy_perform(curlHandle);

    try {
        std::vector<CLyric> lyrics;
        std::vector<NeteaseResult> results;
        auto searchResult = json::parse(response);

        for (auto& searchItem: searchResult["result"]["songs"]) {
            results.emplace_back(searchItem["name"], searchItem["artists"][0]["name"], searchItem["album"]["name"],
                                 searchItem["album"]["picUrl"], searchItem["id"], searchItem["duration"], track.title,
                                 track.artist);
        }

        std::stable_sort(results.begin(), results.end(),
                         [](const NeteaseResult& res1, const NeteaseResult& res2) {
                             return res1.distance < res2.distance;
                         });

        int count = 0;
        for (const auto& result: results) {
            string lyricURL = "http://music.163.com/api/song/lyric";
            lyricURL.append("?id=").append(std::to_string(result.id));
            lyricURL.append("&lv=1").append("&kv=1").append("&tv=-1");
            curl_easy_setopt(curlHandle, CURLOPT_URL, lyricURL.c_str());
            response.clear();
            curl_easy_perform(curlHandle);

            auto lyricResult = json::parse(response);

            if (result.distance == 0 && !lyricResult["nolyric"].is_null() && lyricResult["nolyric"]) {
                CLyric cLyric = CLyric(
                        Track(result.title, result.album, result.artist, result.coverImageUrl, "Netease",
                              result.duration, true), std::vector<CLyricItem>());
                lyrics.push_back(cLyric);
            }

            if (lyricResult["lrc"]["lyric"].is_null())
                continue;
            CLyric cLyric = CLyric(lyricResult["lrc"]["lyric"],
                                       Track(result.title, result.album, result.artist, result.coverImageUrl,
                                             "Netease", result.duration), LyricStyle::CLrcStyle);

            if (!lyricResult["tlyric"]["lyric"].is_null()) {
                CLyric trans = CLyric(lyricResult["tlyric"]["lyric"], LyricStyle::CLrcStyle);
                cLyric.mergeTranslation(trans);
            }

            lyrics.push_back(cLyric);

            if (++count > 5) break;
        }

        if (!lyrics.empty())
            appendResultCallback(std::move(lyrics));

    } catch (json::exception& e) {
        return;
    }
}

Netease::NeteaseResult::NeteaseResult(string title, string artist, string album, string coverImageUrl, int id,
                                      int duration, const string& targetTitle, const string& targetArtist) : title(
        std::move(title)), artist(std::move(artist)), album(std::move(album)), coverImageUrl(std::move(coverImageUrl)),
                                                                                                             id(id),
                                                                                                             duration(
                                                                                                                     duration /
                                                                                                                     1000) {
    distance = stringDistance(this->title, targetTitle) + stringDistance(this->artist, targetArtist) / 2;
}
