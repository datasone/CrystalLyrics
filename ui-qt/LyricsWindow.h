//
// Created by datasone.
// This file is part of CrystalLyrics.
//

#ifndef CRYSTALLYRICS_LYRICSWINDOW_H
#define CRYSTALLYRICS_LYRICSWINDOW_H

#include <QWidget>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QLabel>
#include <QtWidgets/QScrollArea>
#include <QLayout>
#include <QtWidgets/QPushButton>
#include <QSettings>
#include "CLyric.h"

class MainApplication;

using cLyric::CLyric;

class LyricsWindow : public QWidget {
Q_OBJECT
public:
    explicit LyricsWindow(MainApplication *mainApp, CLyric *lyric = nullptr, QWidget *parent = nullptr);

    void clearLyrics();

protected:
    void mousePressEvent(QMouseEvent* event) override;

    void mouseMoveEvent(QMouseEvent* event) override;

private:
    QPoint oldPos;
    QVBoxLayout* textLayout, * rootLayout;
    QScrollArea* scrollArea;
    QWidget* displayArea;
    QList<QLabel*> labels;
    QLabel* activatedLabel = nullptr;
    QPushButton* closeButton;

    CLyric* cLyric;

    QSettings settings;
    QFont windowFont;

    QString windowBackgroundColor, windowLyricsTextColor, windowLyricsTextPlayingColor;

    MainApplication* mainApp;

protected:
    void closeEvent(QCloseEvent* event) override;

public slots:

    void updateLyric(CLyric* lyric = nullptr);

    void activateLine(int lineNum);

};


#endif //CRYSTALLYRICS_LYRICSWINDOW_H
