//
// Created by datasone.
// This file is part of CrystalLyrics.
//

#ifndef CRYSTALLYRICS_CLYRIC_H
#define CRYSTALLYRICS_CLYRIC_H

#include <string>
#include <utility>
#include <vector>
#include <functional>

#include <opencc/opencc.h>

class CLyricProvider;

using std::string;

enum LyricStyle {
    CLrcStyle,       // Compatible with normal lrc, with support for multiple time tags in one line
    XiamiStyle,      // x-trans tag, timecode style
    KugouStyle       // Unique time tag and timecode style
};

class CLyricItem {
public:

    CLyricItem(string content, const int startTime,
               string translation = "",
               std::vector<std::pair<int, int>> timecodes = std::vector<std::pair<int, int>>()) :
            content(std::move(content)), translation(std::move(translation)),
            startTime(startTime), timecodes(std::move(timecodes)) {}

    explicit CLyricItem(const std::vector<string>& lyricLines, LyricStyle style = CLrcStyle);

    [[nodiscard]] bool isDoubleLine() const { return !translation.empty(); }

    string content, translation;
    int startTime;
    std::vector<std::pair<int, int>> timecodes; // std::vector<std::map<ms, chars>>

    string fileSaveString();

    bool operator<(const CLyricItem& item) const;

    bool operator>(const CLyricItem& item) const;
};

struct Track {
    string title, album, artist;
    string coverImageUrl, source;
    int duration = -1; // in seconds
    bool instrumental = false;

    enum Language { ja, zh, other };

    Language contentLanguage = Language::other, translateLanguage = Language::other;

    Track() = default;

    explicit Track(string title, string album = "", string artist = "", string coverImageUrl = "", string source = "",
                   int duration = -1, bool instrumental = false) : title(std::move(title)), album(std::move(album)),
                                                                   artist(std::move(artist)),
                                                                   coverImageUrl(std::move(coverImageUrl)),
                                                                   source(std::move(source)), duration(duration),
                                                                   instrumental(instrumental) {}
};

class CLyric {
public:

    Track track;
    std::vector<CLyricItem> lyrics;

    CLyric() = default;

    CLyric(Track track, std::vector<CLyricItem> lyrics) : track(std::move(track)), lyrics(std::move(lyrics)) {}

    explicit CLyric(string lyricContent, LyricStyle style = CLrcStyle);

    CLyric(const string& lyricContent, Track track, LyricStyle style = CLrcStyle) : CLyric(lyricContent, style) {
        this->track = std::move(track);
    }

    [[nodiscard]] bool isValid() const {
        return (!track.title.empty() && (track.instrumental || !lyrics.empty()));
    };

    string filename();

    string readableString();

    void saveToFile(const string& saveDirectoryPath);

    void deleteFile(const string& saveDirectoryPath);

    void mergeTranslation(const CLyric& trans);

    static string filename(const string& title, const string& album, const string& artist);
};

class CLyricSearch {
    std::vector<CLyric> results;

    void appendResultCallback(std::vector<CLyric> lyrics);

    CLyricProvider* providerList[6];

public:
    explicit CLyricSearch(opencc::SimpleConverter& converter);

    CLyric fetchCLyric(const string& title, const string& album, const string& artist, int duration,
                       const string& saveDirectoryPath);

    std::vector<CLyric> searchCLyric(const string& title, const string& artist, int duration);
};

#endif //CRYSTALLYRICS_CLYRIC_H
