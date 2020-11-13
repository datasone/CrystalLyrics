//
// Created by datasone.
// This file is part of CrystalLyrics.
//

#include "../CLyric.h"
#include "../CLyricProvider.h"

#include <gtest/gtest.h>

using namespace cLyric;

TEST(CLyricTests, CLyricItemBasicLrcParseTest) {
    std::vector<std::string> testContent;
    testContent.emplace_back("[00:56.34]测试1");
    testContent.emplace_back("[00:56.34][tr]测试1翻译");
    testContent.emplace_back("[00:56.34][tc]0,0|128,1|256,2|384,3");

    CLyricItem cLyricItem(testContent, LyricStyle::CLrcStyle);

    std::vector<std::pair<int, int>> timecodes;
    timecodes.emplace_back(0, 0);
    timecodes.emplace_back(128, 1);
    timecodes.emplace_back(256, 2);
    timecodes.emplace_back(384, 3);

    EXPECT_STREQ(cLyricItem.content.c_str(), "测试1") << "Lyric Item Content Test Failed";
    EXPECT_STREQ(cLyricItem.translation.c_str(), "测试1翻译") << "Lyric Item Translation Test Failed";
    EXPECT_EQ(cLyricItem.startTime, 56340) << "Lyric Item Start Time Test Failed";
    EXPECT_EQ(cLyricItem.timecodes, timecodes) << "Lyric Item Time Codes Test Failed";

    testContent.clear();
    testContent.emplace_back("[00:56.343]测试1");

    cLyricItem = CLyricItem(testContent);

    EXPECT_EQ(cLyricItem.startTime, 56343) << "Lyric Item Three Digits Milliseconds Start Time Test Failed";
}

TEST(CLyricTests, CLyricItemXiamiParseTest) {
    std::vector<std::string> testContent;
    testContent.emplace_back("[00:04.057]<100>作<100>詞<309>：<602>cittan*");
    testContent.emplace_back("[00:04.057][tr]作词：cittan*");

    CLyricItem cLyricItem(testContent, LyricStyle::XiamiStyle);

    std::vector<std::pair<int, int>> timecodes;
    timecodes.emplace_back(0, 0);
    timecodes.emplace_back(100, 1);
    timecodes.emplace_back(200, 2);
    timecodes.emplace_back(509, 3);
    timecodes.emplace_back(1111, 10);

    EXPECT_STREQ(cLyricItem.content.c_str(), "作詞：cittan*") << "Xiami Lyric Item Content Test Failed";
    EXPECT_STREQ(cLyricItem.translation.c_str(), "作词：cittan*") << "Xiami Lyric Item Translation Test Failed";
    EXPECT_EQ(cLyricItem.startTime, 4057) << "Xiami Lyric Item Start Time Test Failed";
    EXPECT_EQ(cLyricItem.timecodes, timecodes) << "Xiami Lyric Item Time Codes Test Failed";
}

TEST(CLyricTests, CLyricItemKugouParseTest) {
    std::vector<std::string> testContent;
    testContent.emplace_back(
            "[30889,5860]<0,210,0>見<210,220,0>え<430,340,0>な<770,230,0>い<1000,1040,0>の <2040,320,0>見<2360,240,0>つ<2600,230,0>か<2830,460,0>ら<3290,990,0>な<4280,460,0>い<4740,1120,0>の");

    CLyricItem cLyricItem(testContent, LyricStyle::KugouStyle);

    std::vector<std::pair<int, int>> timecodes;
    timecodes.emplace_back(0, 0);
    timecodes.emplace_back(210, 1);
    timecodes.emplace_back(430, 2);
    timecodes.emplace_back(770, 3);
    timecodes.emplace_back(1000, 4);
    timecodes.emplace_back(2040, 6);
    timecodes.emplace_back(2360, 7);
    timecodes.emplace_back(2600, 8);
    timecodes.emplace_back(2830, 9);
    timecodes.emplace_back(3290, 10);
    timecodes.emplace_back(4280, 11);
    timecodes.emplace_back(4740, 12);
    timecodes.emplace_back(5860, 13);

    EXPECT_STREQ(cLyricItem.content.c_str(), "見えないの 見つからないの") << "Kugou Lyric Item Content Test Failed";
    EXPECT_EQ(cLyricItem.startTime, 30889) << "Kugou Lyric Item Start Time Test Failed";
    EXPECT_EQ(cLyricItem.timecodes, timecodes) << "Kugou Lyric Item Time Codes Test Failed";
}

TEST(CLyricTests, CLyricBasicLrcParseTest) {
    std::string contentText = R"([ti]海阔天空
[al]乐与怒
[ar]Beyond
[offset]+40
[03:57.70][03:20.00][02:08.00][01:09.00]原谅我这一生不羁放纵爱自由
[04:04.50][03:27.00][02:15.00][01:16.00]也会怕有一天会跌倒
[04:10.85][03:46.00][03:33.00][02:21.00][01:22.00]被弃了理想谁人都可以
[03:52.00][03:39.60][02:28.00][01:28.00]哪会怕有一天只你共我
)";

    CLyric lyric(contentText, LyricStyle::CLrcStyle);

    EXPECT_STREQ(lyric.track.title.c_str(), "海阔天空") << "Lyric Title Test Failed";
    EXPECT_STREQ(lyric.track.album.c_str(), "乐与怒") << "Lyric Album Test Failed";
    EXPECT_STREQ(lyric.track.artist.c_str(), "Beyond") << "Lyric Artist Test Failed";
    EXPECT_EQ(lyric.offset, 40) << "Lyric Offset Test Failed";

    std::vector<int> startTimes;
    for (auto &item : lyric.lyrics) {
        startTimes.push_back(item.startTime);
    }
    std::sort(startTimes.begin(), startTimes.end());

    std::vector<int> baseStartTimes;
    baseStartTimes.insert(baseStartTimes.end(),
                          {69000, 76000, 82000, 88000, 128000, 135000, 141000, 148000, 200000, 207000, 213000, 219600,
                           226000, 232000, 237700, 244500, 250850});

    EXPECT_EQ(lyric.lyrics.size(), 17) << "Lyric Item Count Test Failed";
    EXPECT_EQ(startTimes, baseStartTimes) << "Lyric Item Start Times Test Failed";
}

TEST(CLyricTests, CLyricXiamiParseTest) {
    std::string contextTest = R"([00:04.057]<100>作<100>詞<309>：<602>cittan*
[x-trans]作词：cittan*)";

    CLyric lyric(contextTest, LyricStyle::XiamiStyle);

    EXPECT_STREQ(lyric.lyrics[0].translation.c_str(), "作词：cittan*") << "Xiami Lyric Translation Test Failed";
}