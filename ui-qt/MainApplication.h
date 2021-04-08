//
// Created by datasone.
// This file is part of CrystalLyrics.
//

#ifndef CRYSTALLYRICS_MAINAPPLICATION_H
#define CRYSTALLYRICS_MAINAPPLICATION_H

#include "LyricsWindow.h"
#include "SearchWindow.h"
#include "SettingsWindow.h"
#include "DesktopLyricsWindow.h"
#include "EditLyricsWindow.h"
#include "OffsetWindow.h"

#include <CLyric/CLyric.h>
#include <QtWidgets/QSystemTrayIcon>
#include <QtWidgets/QMenu>
#include <QtGui/QAction>
#include <QtGui/QActionGroup>
#include <QtCore/QPointer>
#include <QtNetwork/QLocalServer>
#include <QtCore/QHash>
#include <QtCore/QElapsedTimer>

#include <opencc/opencc.h>
#include <QWidgetAction>

using cLyric::CLyric;
using cLyric::Track;

class MainApplication : public QObject {
Q_OBJECT
public:
    MainApplication();

    bool isPlaying = false;
    Track currentTrack;
    int currentLine = 0;

    int currentScreenIndex();

    CLyric *pcLyric = nullptr;

    static opencc::SimpleConverter openCCSimpleConverter;

private slots:

    void screenChanged(QScreen *screen);

    void geometryChanged();

    void setDesktopLyricScreen();

    void showLyricsWindow();

    void showSearchWindow();

    void showSettingsWindow();

    void showOffsetWindow();

    void setTrackInstrumental();

    void setAlbumInstrumental();

    void wrongLyric();

    void loadLyricFile();

    void showEditLyricsWindow();

    void handleConnection();

    void handleSocketRead();

    void timerTimeout();

    void cleanupOnQuit();

    void clearLyrics();

public slots:

    void reshowDesktopLyricsWindow(bool changed = false, bool enabled = false);

    void reshowLyricsWindow();

    void updateLyric(const CLyric &lyric, bool manualSearch);

    void updateTime(int position = -1, bool playing = true);

    void updateLyricOffset(int offset);

    void clearLyricOffset();

    void saveLyricOffset(int offset);

signals:

    void lyricFound(const CLyric &lyric, bool manualSearch);

    void clearLyricsSignal();

private:
    QSystemTrayIcon *trayIcon;
    QMenu *mainMenu, *screenSubMenu;

    QAction
            *lyricsWindowAction,
            *desktopLyricsWindowAction,
            *searchLyricAction,
            *markAsWrongAction,
            *markTrackAsInstrumentalAction,
            *markAlbumAsInstrumentalAction,
            *loadLocalLyricFileAction,
            *editLyricsAction,
            *offsetAction,
            *settingsAction,
            *exitAction;

    QActionGroup *actionGroup;
    QList<QAction *> screenActions;
    QString chosenScreenName;

    QLocalServer *server;
    QHash<QLocalSocket *, QString> socketMap;
    QSettings settings;

    QPointer<DesktopLyricsWindow> desktopLyricsWindow = nullptr;
    QPointer<LyricsWindow> lyricsWindow = nullptr;
    QPointer<SearchWindow> searchWindow = nullptr;
    QPointer<SettingsWindow> settingsWindow = nullptr;
    QPointer<EditLyricsWindow> editLyricsWindow = nullptr;
    QPointer<OffsetWindow> offsetWindow = nullptr;

    CLyric cLyric;
    CLyricItem *currentLyric = nullptr, *nextLyric = nullptr;
    QElapsedTimer *eTimer;
    QTimer *timer;
    int elapsedTime = -1;
    int offset = 0;

    bool conversionTCSC;

    QString appDataPath;

    bool desktopLyrics;

    void createMenu(bool firstTime = false);

    void parseSocketResult(QLocalSocket *socket);

    void closeSocket(QLocalSocket *socket, const char *message = "");

    void pause();

    void resume();

    void findLyric(const std::string &title, const std::string &album, const std::string &artist, int duration);
};


#endif //CRYSTALLYRICS_MAINAPPLICATION_H
