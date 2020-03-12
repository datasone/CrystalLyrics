//
// Created by datasone.
// This file is part of CrystalLyrics.
//

#include <QApplication>
#include <QtGui/QScreen>
#include <QtCore/QPropertyAnimation>
#include <QtGui/QPainter>
#include <QMouseEvent>
#include "DesktopLyricsWindow.h"
#include "TrayIcon.h"

const int roundCornerRadius = 36;

DesktopLyricsWindow::DesktopLyricsWindow(QWidget* parent, CLyric* lyric, TrayIcon* trayIcon)
        : QWidget(parent), cLyric(lyric), trayIcon(trayIcon) {
    QScreen* screen = QApplication::primaryScreen();
    const int screenWidth = screen->geometry().width();
    const int screenHeight = screen->geometry().height();

    doubleLineDisplay = settings.value("doubleLineDisplay", false).toBool();
    bgColor = QColor(settings.value("bgColor", "#99000000").toString());
    lyricsTextColor = QColor(settings.value("lyricsTextColor", "#FFFFFF").toString());
    lyricsTextPlayedColor = QColor(settings.value("lyricsTextPlayedColor", "#AAFFFF").toString());

    auto desktopFontFamily = settings.value("desktopFont/family");
    if (desktopFontFamily.isNull()) {
        desktopFont = desktopFont.defaultFamily();
    } else {
        auto fontSize = settings.value("desktopFont/size", 16);
        auto fontWeight = settings.value("desktopFont/weight", QFont::Normal);
        auto fontItalic = settings.value("desktopFont/italic", false);
        desktopFont = QFont(desktopFontFamily.toString(), fontSize.toInt(), fontWeight.toInt(), fontItalic.toBool());
    }

    conversionTCSC = settings.value("conversionTCSC", false).toBool();

    int fontHeight = QFontMetrics(desktopFont).height();

    this->setAttribute(Qt::WA_TransparentForMouseEvents);
    this->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setFixedSize(screenWidth * 0.8, doubleLineDisplay ? fontHeight * 3 : fontHeight * 2);
    this->move(screenWidth * 0.1, screenHeight * 0.75);
    this->setContentsMargins(50, 0, 50, 0);

    layout = new QVBoxLayout(this);

    Qt::Alignment firstLineAlignment = Qt::AlignVCenter;
    Qt::Alignment secondLineAlignment = Qt::AlignVCenter;

    switch (settings.value("firstLineAlignment", 1).toInt()) {
        case 0:
            firstLineAlignment |= Qt::AlignLeft;
            break;
        case 1:
            firstLineAlignment |= Qt::AlignHCenter;
            break;
        case 2:
            firstLineAlignment |= Qt::AlignRight;
            break;
    }

    switch (settings.value("secondLineAlignment", 1).toInt()) {
        case 0:
            secondLineAlignment |= Qt::AlignLeft;
            break;
        case 1:
            secondLineAlignment |= Qt::AlignHCenter;
            break;
        case 2:
            secondLineAlignment |= Qt::AlignRight;
            break;
    }

    auto* startupItem = new CLyricItem("CrystalLyrics", 0);
    auto* emptyItem = new CLyricItem("", 0);

    firstLine = new CLyricLabel(this, desktopFont, lyricsTextColor, lyricsTextPlayedColor, startupItem, true);

    firstLine->setAlignment(firstLineAlignment);
    firstLine->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    layout->addWidget(firstLine);

    if (doubleLineDisplay) {
        secondLine = new CLyricLabel(this, desktopFont, lyricsTextColor, lyricsTextPlayedColor, emptyItem, false);
        secondLine->setAlignment(secondLineAlignment);
        secondLine->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

        layout->addWidget(secondLine);
        connect(this, &DesktopLyricsWindow::updateSecondLineLyrics, secondLine, &CLyricLabel::updateLyric);
    }

    connect(this, &DesktopLyricsWindow::updateFirstLineLyrics, firstLine, &CLyricLabel::updateLyric);
    connect(this, &DesktopLyricsWindow::updateFirstLineTime, firstLine, &CLyricLabel::updateTime);
}

void DesktopLyricsWindow::setLine(int lineNum, int timeInLine) {
    if (currentLine == lineNum)
        return;

    currentLine = lineNum;

    if (currentLine < cLyric->lyrics.size() - 1) {
        CLyricItem* currentLyric = &cLyric->lyrics[currentLine], * nextLyric = &cLyric->lyrics[currentLine + 1];

        emit updateFirstLineLyrics(currentLyric, true, contentConversionTCSC);
        if (currentLyric->isDoubleLine()) {
            emit updateSecondLineLyrics(currentLyric, false, translationConversionTCSC);
        } else {
            emit updateSecondLineLyrics(nextLyric, true, contentConversionTCSC);
        }
    } else {
        CLyricItem* currentLyric = &cLyric->lyrics[currentLine];
        emit updateFirstLineLyrics(currentLyric, true, contentConversionTCSC);
        if (currentLyric->isDoubleLine()) {
            emit updateSecondLineLyrics(currentLyric, false, translationConversionTCSC);
        } else {
            emit updateSecondLineLyrics(new CLyricItem("", 0), true, false);
        }
    }

    if (timeInLine != 0) {
        emit updateFirstLineTime(timeInLine);
    }

}

void DesktopLyricsWindow::updateLyric(CLyric* lyric) {
    if (lyric != nullptr) {
        cLyric = lyric;
    }

    contentConversionTCSC = conversionTCSC && (trayIcon->currentTrack.contentLanguage == Track::Language::zh);
    translationConversionTCSC = conversionTCSC && (trayIcon->currentTrack.translateLanguage == Track::Language::zh);

    this->resume();
    if (cLyric && cLyric->track.instrumental) {
        this->pause();
    } else setLine(0);
}

void DesktopLyricsWindow::paintEvent([[maybe_unused]] QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addRoundedRect(this->rect(), roundCornerRadius, roundCornerRadius);
    painter.fillPath(path, bgColor);
}

void DesktopLyricsWindow::pause() {
    auto* animation = new QPropertyAnimation(this, "windowOpacity");
    animation->setEndValue(0);
    animation->setDuration(200);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void DesktopLyricsWindow::resume() {
    auto* animation = new QPropertyAnimation(this, "windowOpacity");
    animation->setEndValue(1);
    animation->setDuration(200);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}
