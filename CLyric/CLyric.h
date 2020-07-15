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

namespace cLyric {

    enum LyricStyle {
        CLrcStyle,       // Compatible with normal lrc, with support for multiple time tags in one line
        XiamiStyle,      // x-trans tag, timecode style
        KugouStyle       // Unique time tag and timecode style
    };

    class CLyricItem {
    public:

        CLyricItem(std::string content, const int startTime,
                   std::string translation = "",
                   std::vector<std::pair<int, int>> timecodes = std::vector<std::pair<int, int>>()) :
                content(std::move(content)), translation(std::move(translation)),
                startTime(startTime), timecodes(std::move(timecodes)) {}

        explicit CLyricItem(const std::vector<std::string> &lyricLines, LyricStyle style = CLrcStyle);

        [[nodiscard]] bool isDoubleLine() const { return !translation.empty(); }

        std::string content, translation;
        int startTime;
        std::vector<std::pair<int, int>> timecodes; // std::vector<std::map<ms, chars>>

        std::string fileSaveString();

        bool operator<(const CLyricItem &item) const;

        bool operator>(const CLyricItem &item) const;
    };

    static CLyricItem emptyCLyricItem("", 0);

    struct Track {
        std::string title, album, artist;
        std::string coverImageUrl, source;
        int duration = -1; // in seconds
        bool instrumental = false;

        enum Language {
            ja, zh, other
        };

        Language contentLanguage = Language::other, translateLanguage = Language::other;

        Track() = default;

        explicit Track(std::string title, std::string album = "", std::string artist = "",
                       std::string coverImageUrl = "", std::string source = "",
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

        explicit CLyric(std::string lyricContent, LyricStyle style = CLrcStyle);

        CLyric(const std::string &lyricContent, Track track, LyricStyle style = CLrcStyle) : CLyric(lyricContent,
                                                                                                    style) {
            this->track = std::move(track);
        }

        [[nodiscard]] bool isValid() const {
            return (!track.title.empty() && (track.instrumental || !lyrics.empty()));
        };

        [[nodiscard]] std::string filename() const;

        std::string readableString();

        void saveToFile(const std::string &saveDirectoryPath);

        void deleteFile(const std::string &saveDirectoryPath);

        void mergeTranslation(const CLyric &trans);

        static std::string filename(const std::string &title, const std::string &album, const std::string &artist);
    };

}

#endif //CRYSTALLYRICS_CLYRIC_H
