//
// Created by datasone.
// This file is part of CrystalLyrics.
//

#include "CLyric.h"
#include "CLyricUtils.h"

#include <cctype>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <regex>
#include <map>
#include <utility>
#include <algorithm>

CLyricItem::CLyricItem(const std::vector<string>& lyricLines, LyricStyle style) {
    for (const string& lyricLine : lyricLines) {
        std::regex linePattern(R"((\[.*?\])+(.*))");
        std::regex timeTagPattern(R"(\[(\d{2})?:?(\d{2,3}):(\d{2})\.?(\d{1,3})?\])");
        std::regex wordTagPattern(R"(\[(\w+)\])");

        string lineContent;
        std::smatch lineMatch;
        if (std::regex_match(lyricLine, lineMatch, linePattern)) {
            lineContent = lineMatch.str(2);
        } else continue;

        std::smatch timeTagMatch;
        if (style != LyricStyle::KugouStyle) {
            if (std::regex_search(lyricLine, timeTagMatch, timeTagPattern)) {
                int hourTime = 0;
                try {
                    hourTime = std::stoi(timeTagMatch.str(1));
                } catch (std::invalid_argument& e) {}
                int minuteTime = std::stoi(timeTagMatch.str(2));
                int secondTime = std::stoi(timeTagMatch.str(3));
                int millisecondTime = 0;
                const string& millisecondString = timeTagMatch.str(4);
                try {
                    millisecondTime = std::stoi(millisecondString);
                } catch (std::invalid_argument& e) {}

                switch (millisecondString.length()) {
                    case 1:
                        millisecondTime *= 100;
                        break;
                    case 2:
                        millisecondTime *= 10;
                        break;
                    default:;
                }

                startTime = hourTime * 60 * 60 * 1000 + minuteTime * 60 * 1000 + secondTime * 1000 + millisecondTime;

                if (secondTime >= 60) { // some mm:ss:ms format
                    startTime += secondTime;
                }

            } else continue;
        } else {
            std::regex KugouTimeTagPattern(R"(\[(\d+?),(\d+?)\])");
            std::smatch KugouTimeTagMatch;
            if (std::regex_search(lyricLine, KugouTimeTagMatch, KugouTimeTagPattern)) {
                startTime = std::stoi(KugouTimeTagMatch.str(1));
            } else continue;
        }

        std::smatch wordTagMatch;
        if (std::regex_search(lyricLine, wordTagMatch, wordTagPattern)) {
            if (wordTagMatch.str(1) == "tr") {
                translation = lineContent;
            } else if (wordTagMatch.str(1) == "tc") {
                std::vector<string> lineTimeCodes = split_string(lineContent, "|");
                for (const string& timecode : lineTimeCodes) {
                    std::vector<string> codes = split_string(timecode, ",");
                    timecodes.emplace_back(std::stoi(codes[0]), std::stoi(codes[1]));
                }
            }
        } else if (style == LyricStyle::XiamiStyle) {
            std::regex timeCodePairRegex(R"((\d+)>(.*?)<)");
            std::regex lastPairRegex(R"(^.*<(\d+)>(.*?)$)");

            std::smatch lastPairMatch;
            if (std::regex_match(lineContent, lastPairMatch, lastPairRegex)) {
                int lastTimeCode = std::stoi(lastPairMatch.str(1));
                string lastWord = lastPairMatch.str(2);

                int totalTime = 0, charNum = 0; // Number of character "char", not the byte "char"
                timecodes.emplace_back(totalTime, charNum);
                content.clear();
                std::smatch timeCodePairMatch;
                while (std::regex_search(lineContent, timeCodePairMatch, timeCodePairRegex)) {
                    string word = timeCodePairMatch.str(2);
                    totalTime += std::stoi(timeCodePairMatch.str(1));
                    charNum += utf8StringChars(word);
                    timecodes.emplace_back(totalTime, charNum);
                    content.append(word);
                    lineContent = timeCodePairMatch.suffix();
                }

                totalTime += lastTimeCode;
                charNum += utf8StringChars(lastWord);
                timecodes.emplace_back(totalTime, charNum);
                content.append(lastWord);
            }
        } else if (style == LyricStyle::KugouStyle) {
            std::regex timeCodePairRegex(R"((\d+?),(\d+?),\d+?>(.*?)<)");
            std::regex lastPairRegex(R"(.*<(\d+?),(\d+?),\d+?>(.*?)$)");

            std::smatch lastPairMatch;
            if (std::regex_match(lineContent, lastPairMatch, lastPairRegex)) {
                int lastTimeCodeStart = std::stoi(lastPairMatch.str(1));
                int lastTimeCodeLength = std::stoi(lastPairMatch.str(2));
                string lastWord = lastPairMatch.str(3);

                int totalTime = 0, charNum = 0; // Number of character "char", not the byte "char"
                timecodes.emplace_back(totalTime, charNum);
                content.clear();
                std::smatch timeCodePairMatch;
                while (std::regex_search(lineContent, timeCodePairMatch, timeCodePairRegex)) {
                    string word = timeCodePairMatch.str(3);
                    // totalTime = start + length;
                    totalTime = std::stoi(timeCodePairMatch.str(1)) + std::stoi(timeCodePairMatch.str(2));
                    charNum += utf8StringChars(word);
                    timecodes.emplace_back(totalTime, charNum);
                    content.append(word);
                    lineContent = timeCodePairMatch.suffix();
                }

                totalTime = lastTimeCodeStart + lastTimeCodeLength;
                charNum += utf8StringChars(lastWord);
                timecodes.emplace_back(totalTime, charNum);
                content.append(lastWord);
            }
        }

        if (content.empty()) {
            content = lineContent;
        }
    }
}

bool CLyricItem::operator<(const CLyricItem& item) const {
    return this->startTime < item.startTime;
}

bool CLyricItem::operator>(const CLyricItem& item) const {
    return this->startTime > item.startTime;
}

string CLyricItem::fileSaveString() {
    std::ostringstream stringStream;
    stringStream << '[' << std::setw(2) << std::setfill('0') << startTime / 60000 << ':' << std::setw(2)
                 << std::setfill('0') << startTime / 1000 % 60 << '.' << std::left << std::setw(3) << std::setfill('0')
                 << startTime % 1000 << ']';
    string timeTag = stringStream.str();
    stringStream.str("");
    stringStream.clear();
    stringStream << timeTag << content << '\n';
    if (!translation.empty()) {
        stringStream << timeTag << "[tr]" << translation << '\n';
    }
    if (!timecodes.empty()) {
        std::ostringstream timeCodeStringStream;
        for (int i = 0; i < timecodes.size() - 1; ++i) {
            timeCodeStringStream << timecodes[i].first << ',' << timecodes[i].second << '|';
        }
        timeCodeStringStream << timecodes.back().first << ',' << timecodes.back().second;
        string timeCodeString = timeCodeStringStream.str();
        stringStream << timeTag << "[tc]" << timeCodeString << '\n';
    }
    return stringStream.str();
}

CLyric::CLyric(string lyricContent, LyricStyle style) {
    while (lyricContent.find('\r') != string::npos) {
        lyricContent.erase(lyricContent.find('\r'), 1);
    }

    std::vector<string> lines = split_string(lyricContent, "\n");
    std::multimap<string, string> linesMap;

    for (int i = 0; i < lines.size(); ++i) {
        string line = lines[i];

        std::regex linePattern(R"((\[.*?\])+(.*))");
        std::smatch lineMatch;
        string content;
        if (std::regex_match(line, lineMatch, linePattern)) {
            content = lineMatch.str(2);
        }

        std::regex tagsPattern(R"(\[(.*?)\])");
        std::smatch tagsMatch;
        std::vector<string> tags;
        while (std::regex_search(line, tagsMatch, tagsPattern)) {
            tags.push_back(tagsMatch.str(1));
            line = tagsMatch.suffix();
        }

        if (tags.size() == 1) {
            const string& tag = tags[0];
            if (std::isdigit(static_cast<unsigned char>(tag[0]))) { // Signed char UB
                linesMap.insert(std::pair<string, string>(tag, "[" + tag + "]" + line));
            } else {
                if (tag == "ti") {
                    track.title = content;
                } else if (tag == "al") {
                    track.album = content;
                } else if (tag == "ar") {
                    track.artist = content;
                } else if (tag == "du") {
                    track.duration = std::stoi(content);
                } else if (tag == "instrumental") {
                    track.instrumental = true;
                } else if (style == LyricStyle::XiamiStyle && tag == "x-trans") {
                    string previousLine = lines[i - 1];

                    string previousContent;
                    if (std::regex_match(previousLine, lineMatch, linePattern)) {
                        previousContent = lineMatch.str(2);
                    }

                    std::vector<string> previousTags;
                    while (std::regex_search(previousLine, tagsMatch, tagsPattern)) {
                        previousTags.push_back(tagsMatch.str(1));
                        previousLine = tagsMatch.suffix();
                    }

                    if (content.empty()) { // Empty trans
                        content = previousContent;
                    }

                    for (const string& previousTag: previousTags) {
                        if (std::isdigit(static_cast<unsigned char>(previousTag[0])))
                            linesMap.insert(
                                    std::pair<string, string>(previousTag, "[" + previousTag + "]" + "[tr]" + content));
                    }
                }
            }
        } else {
            for (auto& tag: tags) {
                if (!std::isdigit(static_cast<unsigned char>(tag[0]))) {
                    string prepend = "[";
                    prepend.append(tag).append("]").append(content);
                    content = prepend;
                }
            }
            for (auto& tag: tags) {
                if (std::isdigit(static_cast<unsigned char>(tag[0]))) {
                    std::ostringstream lineStream;
                    lineStream << "[" << tag << "]" << content;
                    linesMap.insert(std::pair<string, string>(tag, lineStream.str()));
                }
            }
        }
    }

    for (auto itr = linesMap.begin(); itr != linesMap.end(); itr = linesMap.upper_bound(itr->first)) {
        auto range = linesMap.equal_range(itr->first);
        std::vector<string> linesWithSameTag;
        while (range.first != range.second) {
            linesWithSameTag.push_back(range.first->second);
            ++range.first;
        }
        lyrics.emplace_back(linesWithSameTag, style);
    }

    std::stable_sort(lyrics.begin(), lyrics.end(),
                     [](const CLyricItem& item1, const CLyricItem& item2) {
                         return item1.startTime < item2.startTime;
                     });
}

string CLyric::filename() const {
    return filename(track.title, track.album, track.artist);
}

string CLyric::filename(const string& title, const string& album, const string& artist) {
    std::ostringstream stringStream;
    stringStream << title << " - " << artist << " - " << album << ".clrc";
    string filename = stringStream.str();
    return normalizeFileName(filename);
}

void CLyric::saveToFile(const string& saveDirectoryPath) {
    std::ofstream saveFile(std::filesystem::u8path(saveDirectoryPath + "/" + filename()), std::ios::out);
    saveFile << readableString();
    saveFile.close();
}

void CLyric::mergeTranslation(const CLyric& trans) {
    for (int i = 0, j = 0; i < this->lyrics.size() && j < trans.lyrics.size();) {
        if (this->lyrics[i++].startTime == trans.lyrics[j].startTime)
            this->lyrics[i - 1].translation = trans.lyrics[j++].content;
    }
}

string CLyric::readableString() {
    std::ostringstream stringStream;
    stringStream << "[ti]" << track.title << '\n';
    if (!track.album.empty()) {
        stringStream << "[al]" << track.album << '\n';
    }
    if (!track.artist.empty()) {
        stringStream << "[ar]" << track.artist << '\n';
    }
    if (track.duration > 0) {
        stringStream << "[du]" << track.duration << '\n';
    }
    if (track.instrumental) {
        stringStream << "[instrumental]\n";
    } else for (auto item: lyrics) {
        stringStream << item.fileSaveString();
    }
    return stringStream.str();
}

void CLyric::deleteFile(const string& saveDirectoryPath) {
    remove(std::filesystem::u8path(saveDirectoryPath + "/" + filename()));
}

