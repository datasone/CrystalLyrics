//
// Created by datasone.
// This file is part of CrystalLyrics.
//

#ifndef CRYSTALLYRICS_DESKTOPLYRICSWINDOW_H
#define CRYSTALLYRICS_DESKTOPLYRICSWINDOW_H

#include <QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>
#include <QtCore/QSettings>
#include "CLyric.h"
#include "CLyricLabel.h"

class TrayIcon;

using cLyric::CLyric;

class DesktopLyricsWindow : public QWidget {
Q_OBJECT
public:
    explicit DesktopLyricsWindow(QWidget* parent = nullptr, CLyric* lyric = nullptr, TrayIcon* trayIcon = nullptr);

    void resize(int screenIndex = 0);

    void clearLyrics();

private:
    QVBoxLayout* layout;
    CLyricLabel* firstLine, * secondLine;
    CLyric* cLyric;
    int currentLine = 0;

    TrayIcon* trayIcon;

    QSettings settings;
    QColor bgColor, lyricsTextColor, lyricsTextPlayedColor;
    bool doubleLineDisplay, conversionTCSC, contentConversionTCSC = false, translationConversionTCSC = false;
    QFont desktopFont;

public slots:

    void updateLyric(CLyric* lyric = nullptr);

    void setLine(int lineNum, int timeInLine = 0);

    void pause();

    void resume();

protected:
    void paintEvent(QPaintEvent* event) override;
};


#endif //CRYSTALLYRICS_DESKTOPLYRICSWINDOW_H
