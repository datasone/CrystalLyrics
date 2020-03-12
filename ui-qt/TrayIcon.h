//
// Created by datasone.
// This file is part of CrystalLyrics.
//

#ifndef CRYSTALLYRICS_TRAYICON_H
#define CRYSTALLYRICS_TRAYICON_H

#include "CLyric.h"
#include "LyricsWindow.h"
#include "SearchWindow.h"
#include "SettingsWindow.h"
#include "DesktopLyricsWindow.h"
#include "EditLyricsWindow.h"

#include <QtWidgets/QSystemTrayIcon>
#include <QtWidgets/QMenu>
#include <QtWidgets/QAction>
#include <QtCore/QPointer>
#include <QtNetwork/QLocalServer>
#include <QtCore/QHash>
#include <QtCore/QElapsedTimer>

#include <opencc/opencc.h>

class TrayIcon : public QObject {
Q_OBJECT
public:
    TrayIcon();

    bool isPlaying = false;
    Track currentTrack;
    int currentLine = 0;

    CLyric* pcLyric = nullptr;

    static opencc::SimpleConverter openCCSimpleConverter;

private slots:

    void showLyricsWindow();

    void showSearchWindow();

    void showSettingsWindow();

    void setInstrumental();

    void wrongLyric();

    void loadLyricFile();

    void showEditLyricsWindow();

    void handleConnection();

    void handleSocketRead();

    void timerTimeout();

    void cleanupOnQuit();

public slots:

    void updateLyric(const CLyric& lyric, bool manualSearch);

    void updateTime(int position = -1, bool playing = true);

signals:

    void lyricFound(const CLyric& lyric, bool manualSearch);

private:
    QSystemTrayIcon* trayIcon;
    QMenu* menu;

    QAction
            * lyricsWindowAction,
            * searchLyricAction,
            * markAsInstrumentalAction,
            * markAsWrongAction,
            * loadLocalLyricFileAction,
            * editLyricsAction,
            * settingsAction,
            * exitAction;

    QLocalServer* server;
    QHash<QLocalSocket*, QString> socketMap;
    QSettings settings;

    QPointer<DesktopLyricsWindow> desktopLyricsWindow = nullptr;
    QPointer<LyricsWindow> lyricsWindow = nullptr;
    QPointer<SearchWindow> searchWindow = nullptr;
    QPointer<SettingsWindow> settingsWindow = nullptr;
    QPointer<EditLyricsWindow> editLyricsWindow = nullptr;

    CLyric cLyric;
    CLyricItem* currentLyric = nullptr, * nextLyric = nullptr;
    QElapsedTimer* eTimer;
    QTimer* timer;
    int elapsedTime = -1;

    bool conversionTCSC;

    QString appDataPath;

    void parseSocketResult(QLocalSocket* socket);

    void closeSocket(QLocalSocket* socket, const char* message = "");

    void pause();

    void resume();

    void findLyric(const string& title, const string& album, const string& artist, int duration);
};


#endif //CRYSTALLYRICS_TRAYICON_H
