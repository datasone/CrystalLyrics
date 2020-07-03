//
// Created by datasone.
// This file is part of CrystalLyrics.
//

#include <QApplication>
#include <QScreen>
#include <QPropertyAnimation>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include "DesktopLyricsWindow.h"
#include "TrayIcon.h"

DesktopLyricsWindow::DesktopLyricsWindow(QWidget* parent, CLyric* lyric, TrayIcon* trayIcon)
        : QWidget(parent), cLyric(lyric), trayIcon(trayIcon) {

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

    this->setAttribute(Qt::WA_TransparentForMouseEvents);
    this->setWindowFlags(
        #ifdef Q_OS_MACOS
            Qt::Window |
        #else
            Qt::Tool |
        #endif
            Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    this->setAttribute(Qt::WA_TranslucentBackground);

    layout = new QVBoxLayout(this);

    this->setContentsMargins(10, 10, 10, 10);
    layout->setContentsMargins(0, 0, 0, 0);

    Qt::Alignment firstLineAlignment = Qt::AlignVCenter | Qt::AlignHCenter;
    Qt::Alignment secondLineAlignment = Qt::AlignVCenter | Qt::AlignHCenter;

    auto* startupItem = new CLyricItem("CrystalLyrics", 0);

    firstLine = new CLyricLabel(this, desktopFont, lyricsTextColor, lyricsTextPlayedColor, startupItem, true);

    firstLine->setAlignment(firstLineAlignment);
    firstLine->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    layout->addWidget(firstLine);

    if (doubleLineDisplay) {
        secondLine = new CLyricLabel(this, desktopFont, lyricsTextColor, lyricsTextPlayedColor, &cLyric::emptyCLyricItem, false);
        secondLine->setAlignment(secondLineAlignment);
        secondLine->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

        layout->addWidget(secondLine);
    }

    this->resize(trayIcon->currentScreenIndex());
}

void DesktopLyricsWindow::setLine(int lineNum, int timeInLine) {
    if (currentLine == lineNum)
        return;

    currentLine = lineNum;

    if (cLyric->lyrics.empty()) {
        pause();
        return;
    }

#ifdef Q_OS_MACOS
    QWidget* focusWidget = QApplication::focusWidget();
    hide();
    show();
    if (focusWidget) {
        focusWidget->activateWindow();
        focusWidget->raise();
    }
    // A strange problem, some text won't be cleared (displayed in darker color, maybe caused by transparency)
    // when the app is not in focus in macOS. A simple hide and show fixes it.
#endif

    if (currentLine < cLyric->lyrics.size() - 1) {
        CLyricItem* currentLyric = &cLyric->lyrics[currentLine], * nextLyric = &cLyric->lyrics[currentLine + 1];

        firstLine->updateLyric(currentLyric, true, contentConversionTCSC);
        if (doubleLineDisplay && currentLyric->isDoubleLine()) {
            secondLine->updateLyric(currentLyric, false, translationConversionTCSC);
        } else {
            secondLine->updateLyric(nextLyric, true, contentConversionTCSC);
        }
    } else {
        CLyricItem* currentLyric = &cLyric->lyrics[currentLine];
        firstLine->updateLyric(currentLyric, true, contentConversionTCSC);
        if (doubleLineDisplay && currentLyric->isDoubleLine()) {
            secondLine->updateLyric(currentLyric, false, translationConversionTCSC);
        } else {
            secondLine->updateLyric(new CLyricItem("", 0), true, false);
        }
    }

    resize(trayIcon->currentScreenIndex());

    if (timeInLine != 0) {
        firstLine->updateTime(timeInLine);
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

    QFontMetricsF metrics(desktopFont);

    QPainterPath path;
    path.addRoundedRect(this->rect(), metrics.height() / 2, metrics.height() / 2);
    painter.fillPath(path, bgColor);
}

void DesktopLyricsWindow::pause() {
    auto* animation = new QPropertyAnimation(this, "windowOpacity");
    animation->setEndValue(0);
    animation->setDuration(200);
    connect(animation, &QPropertyAnimation::finished, [this]() {
        this->hide();
        sender()->deleteLater();
    });
    animation->start();
}

void DesktopLyricsWindow::resume() {
    this->show();
    auto* animation = new QPropertyAnimation(this, "windowOpacity");
    animation->setEndValue(1);
    animation->setDuration(200);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void DesktopLyricsWindow::resize(int screenIndex) {
    QList<QScreen*> screens = QApplication::screens();
    QScreen* screen = screens[screenIndex];
    const int screenWidth = screen->availableGeometry().width();
    const int screenHeight = screen->availableGeometry().height();

    this->resume();
    if (firstLine->text.trimmed().isEmpty() && (doubleLineDisplay ? secondLine->text.trimmed().isEmpty() : true))
        this->pause();

    QFontMetricsF metrics(desktopFont);

    int fontHeight = metrics.height();
    int fontWidth;

    if (doubleLineDisplay) {
        fontWidth = std::max(metrics.horizontalAdvance(firstLine->text), metrics.horizontalAdvance(secondLine->text));
    } else {
        fontWidth = metrics.horizontalAdvance(firstLine->text);
    }

    this->setFixedSize(fontWidth + metrics.averageCharWidth() * 4 + layout->contentsMargins().left() * 2,
            (doubleLineDisplay ? fontHeight * 3 : fontHeight * 2) + layout->contentsMargins().top() * 2);
    this->move(screen->geometry().x() + (screenWidth - this->width()) / 2, screen->geometry().y() + screenHeight - this->height() - 20);
}

void DesktopLyricsWindow::clearLyrics() {
    firstLine->updateLyric(&cLyric::emptyCLyricItem, false, false);
    if (doubleLineDisplay)
        secondLine->updateLyric(&cLyric::emptyCLyricItem, false, false);
}
