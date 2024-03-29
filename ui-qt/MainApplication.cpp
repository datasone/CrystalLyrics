//
// Created by datasone.
// This file is part of CrystalLyrics.
//

#include "MainApplication.h"
#include "utils.h"

#include <CLyric/CLyricSearch.h>

#include <thread>
#include <QApplication>
#include <QScreen>
#include <QDir>
#include <QMessageBox>
#include <QLocalSocket>
#include <QStandardPaths>
#include <QFileDialog>
#include <QLineEdit>

#ifdef Q_OS_MACOS

#include <CoreFoundation/CoreFoundation.h>

#endif

Q_DECLARE_METATYPE(CLyric)

Q_DECLARE_METATYPE(std::vector<CLyric>)

#ifdef Q_OS_MACOS
CFURLRef url = (CFURLRef) CFAutorelease((CFURLRef) CFBundleCopyBundleURL(CFBundleGetMainBundle()));
QString path = QUrl::fromCFURL(url).path() + "Contents/Resources/";
#else
QString path = QCoreApplication::applicationDirPath();
#endif

using cLyric::CLyricSearch;
using cLyric::LyricStyle;

opencc::SimpleConverter MainApplication::openCCSimpleConverter =
        opencc::SimpleConverter(path.toStdString() + "opencc-files/t2s.json");

MainApplication::MainApplication() {
    appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    if (!QDir().exists(appDataPath))
        QDir().mkpath(appDataPath);

    qRegisterMetaType<CLyric>("CLyric");
    qRegisterMetaType<std::vector<CLyric>>("std::vector<CLyric>");

    eTimer = new QElapsedTimer();

    timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, &MainApplication::timerTimeout);

    trayIcon = new QSystemTrayIcon(QIcon(":/image/icon.png"));
    mainMenu = new QMenu();

    searchLyricAction = new QAction("Search Lyrics", nullptr);
    markTrackAsInstrumentalAction = new QAction("Mark Current Track as Instrumental", nullptr);
    markAlbumAsInstrumentalAction = new QAction("Mark Current Album as Instrumental", nullptr);
    loadLocalLyricFileAction = new QAction("Load Local Lyric File", nullptr);
    markAsWrongAction = new QAction("Current Lyric is Wrong", nullptr);
    lyricsWindowAction = new QAction("Show Lyrics Window", nullptr);
    desktopLyricsWindowAction = new QAction("Reshow Desktop Lyrics Window", nullptr);
    editLyricsAction = new QAction("Edit Lyric", nullptr);
    offsetAction = new QAction("Adjust Lyric Offset", nullptr);
    settingsAction = new QAction("Settings", nullptr);
    exitAction = new QAction("Exit", nullptr);

    createMenu(true);

    trayIcon->setContextMenu(mainMenu);
    trayIcon->setVisible(true);
    trayIcon->show();
    trayIcon->setToolTip("CrystalLyrics");

    QList<QScreen *> screens = QApplication::screens();
    foreach(QScreen *screen, screens) {
        connect(screen, &QScreen::geometryChanged, [this](const QRect &) { geometryChanged(); });
    }

    connect(qobject_cast<QGuiApplication *>(QApplication::instance()), &QApplication::screenRemoved, this,
            &MainApplication::screenChanged);
    connect(qobject_cast<QGuiApplication *>(QApplication::instance()), &QApplication::screenAdded, this,
            &MainApplication::screenChanged);

    connect(lyricsWindowAction, &QAction::triggered, this, &MainApplication::showLyricsWindow);
    connect(desktopLyricsWindowAction, &QAction::triggered, this, [this]() { reshowDesktopLyricsWindow(); });
    connect(markTrackAsInstrumentalAction, &QAction::triggered, this, &MainApplication::setTrackInstrumental);
    connect(markAlbumAsInstrumentalAction, &QAction::triggered, this, &MainApplication::setAlbumInstrumental);
    connect(searchLyricAction, &QAction::triggered, this, &MainApplication::showSearchWindow);
    connect(loadLocalLyricFileAction, &QAction::triggered, this, &MainApplication::loadLyricFile);
    connect(markAsWrongAction, &QAction::triggered, this, &MainApplication::wrongLyric);
    connect(editLyricsAction, &QAction::triggered, this, &MainApplication::showEditLyricsWindow);
    connect(offsetAction, &QAction::triggered, this, &MainApplication::showOffsetWindow);
    connect(settingsAction, &QAction::triggered, this, &MainApplication::showSettingsWindow);
    connect(exitAction, &QAction::triggered, QApplication::instance(), &QApplication::quit);
    connect(QApplication::instance(), &QApplication::aboutToQuit, this, &MainApplication::cleanupOnQuit);

    connect(this, &MainApplication::lyricFound, this, &MainApplication::updateLyric,
            Qt::ConnectionType::QueuedConnection);
    connect(this, &MainApplication::clearLyricsSignal, this, &MainApplication::clearLyrics,
            Qt::ConnectionType::QueuedConnection);

    desktopLyrics = settings.value("desktopLyrics", false).toBool();
    if (desktopLyrics) {
        desktopLyricsWindow = new DesktopLyricsWindow(this, pcLyric, nullptr);
        desktopLyricsWindow->setAttribute(Qt::WA_DeleteOnClose);
        desktopLyricsWindow->show();
    }

    const auto ipcSocketName = settings.value("ipcSocketName", "clyricsocket").toString();

    server = new QLocalServer(this);
    QLocalServer::removeServer(ipcSocketName);
    QFile::remove("/tmp/" + ipcSocketName);
    if (!server->listen(
        #ifdef Q_OS_MACOS
            "/tmp/" + ipcSocketName
        #else
            ipcSocketName
        #endif
            )) {
        QMessageBox msgBox(QMessageBox::Critical, "CrystalLyrics", "Unable to setup IPC socket.\nProgram will exit.",
                           QMessageBox::Ok);
        QObject::connect(&msgBox, &QMessageBox::finished, [=]([[maybe_unused]] int i) { QApplication::quit(); });
        msgBox.show();
    }

    connect(server, &QLocalServer::newConnection, this, &MainApplication::handleConnection);

    conversionTCSC = settings.value("conversionTCSC", false).toBool();
}

void MainApplication::showLyricsWindow() {
    if (lyricsWindow == nullptr) {
        lyricsWindow = new LyricsWindow(this, pcLyric, nullptr);
        lyricsWindow->setAttribute(Qt::WA_DeleteOnClose);
        lyricsWindow->show();
        lyricsWindow->activateWindow();
        lyricsWindow->raise();
    } else {
        lyricsWindow->activateWindow();
        searchWindow->raise();
    }
}

void MainApplication::showSearchWindow() {
    if (searchWindow == nullptr) {
        searchWindow = new SearchWindow(currentTrack.title, currentTrack.artist, currentTrack.duration, this,
                                        nullptr);
        searchWindow->setAttribute(Qt::WA_DeleteOnClose);
        searchWindow->show();
        searchWindow->activateWindow();
        searchWindow->raise();
    } else {
        searchWindow->setTrack(currentTrack);
        searchWindow->activateWindow();
        searchWindow->raise();
    }
}

void MainApplication::showSettingsWindow() {
    if (settingsWindow == nullptr) {
        settingsWindow = new SettingsWindow(this, nullptr);
        settingsWindow->setAttribute(Qt::WA_DeleteOnClose);
        settingsWindow->show();
        settingsWindow->activateWindow();
        settingsWindow->raise();
    } else {
        settingsWindow->activateWindow();
        searchWindow->raise();
    }
}

void MainApplication::showEditLyricsWindow() {
    if (editLyricsWindow == nullptr) {
        editLyricsWindow = new EditLyricsWindow(this, nullptr);
        editLyricsWindow->setAttribute(Qt::WA_DeleteOnClose);
        editLyricsWindow->show();
        editLyricsWindow->activateWindow();
        editLyricsWindow->raise();
    } else {
        editLyricsWindow->activateWindow();
        searchWindow->raise();
    }
}

void MainApplication::handleConnection() {
    QLocalSocket *socket = server->nextPendingConnection();
    connect(socket, &QLocalSocket::readyRead, this, &MainApplication::handleSocketRead);
    connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
}

void MainApplication::handleSocketRead() {
    auto *socket = qobject_cast<QLocalSocket *>(sender());
    const auto content = QString::fromUtf8(socket->readAll());
    if (socketMap.contains(socket)) {
        socketMap[socket] = socketMap[socket] + content;
    } else {
        socketMap[socket] = content;
    }
    parseSocketResult(socket);
}

void MainApplication::parseSocketResult(QLocalSocket *socket) {
    const QString content = socketMap[socket].trimmed();
    if (!content.startsWith('^')) {
        closeSocket(socket, "Bad Request");
    }
    if (!content.endsWith('$')) {
        closeSocket(socket, "Bad Request");
        return;
    }
    const QString task = content.right(content.size() - 2).split(']')[0];
    QMap<QString, QString> parameters;
    foreach(QString parameter, content.mid(3 + task.size(), content.size() - 4 - task.size()).split('(')) {
        if (parameter.right(1) != ')')
            continue;
        parameter = parameter.left(parameter.size() - 1);
        parameters.insert(parameter.split('=')[0], parameter.split('=')[1].replace("\\\\[", "(").replace("\\\\]", ")"));
    }
    if (task == "setTrack") {
        if ((currentTrack.title == parameters["title"].toStdString()) &&
            (currentTrack.album == parameters["album"].toStdString()) &&
            (currentTrack.artist == parameters["artist"].toStdString())) {
            closeSocket(socket);
            return;
        }
        currentTrack.title = parameters["title"].toStdString();
        currentTrack.album = parameters["album"].toStdString();
        currentTrack.artist = parameters["artist"].toStdString();
        currentTrack.duration = parameters["duration"].toInt();

        timer->stop();
        elapsedTime = 0;
        eTimer->start();

        if (desktopLyricsWindow)
            desktopLyricsWindow->clearLyrics();

        if (lyricsWindow)
            lyricsWindow->clearLyrics();

        pcLyric = nullptr;
        std::thread thread([this] {
            findLyric(currentTrack.title, currentTrack.album, currentTrack.artist, currentTrack.duration);
        });
        thread.detach();
    } else if (task == "setState") {
        const int position = parameters["position"].toInt();
        const bool playing = parameters["playing"] == "true";
        updateTime(position, playing);
    } else if (task == "setQuit") {
        const bool quit = parameters["quit"] == "true";
        if (quit) {
            desktopLyricsWindow->hide();
            desktopLyricsWindow->clearLyrics();
            currentTrack = Track();
        }
    } else {
        closeSocket(socket, "Bad Request");
    }
    closeSocket(socket);
}

void MainApplication::closeSocket(QLocalSocket *socket, const char *message) {
    socketMap.remove(socket);
    socket->write(message);
    socket->close();
    socket->disconnectFromServer();
    socket->deleteLater();
}

void MainApplication::timerTimeout() {
    if (!isPlaying || !pcLyric || pcLyric->track.instrumental)
        return;

    try {
        currentLyric = &pcLyric->lyrics.at(++currentLine);
    } catch (const std::out_of_range &e) {
        return;
    }

    if (desktopLyricsWindow)
        desktopLyricsWindow->setLine(currentLine);
    if (lyricsWindow)
        lyricsWindow->activateLine(currentLine);

    if (currentLine < static_cast<int>(pcLyric->lyrics.size()) - 1) {
        nextLyric = &(pcLyric->lyrics[currentLine + 1]);
        timer->setInterval(nextLyric->startTime - currentLyric->startTime);
        timer->start();
    }
}

void MainApplication::pause() {
    if (desktopLyricsWindow) {
        desktopLyricsWindow->hide();
    }
    if (isPlaying) {
        timer->stop();
        elapsedTime += eTimer->elapsed();
        isPlaying = false;
    }
}

void MainApplication::resume() {
    if (!isPlaying) {
        eTimer->start();
        if (desktopLyricsWindow) {
            if (!currentTrack.instrumental && pcLyric && pcLyric->isValid())
                desktopLyricsWindow->show();
            // Sometimes Windows just ignores the window's always on top
        } else if (desktopLyrics) {
            desktopLyricsWindow = new DesktopLyricsWindow(this, pcLyric, nullptr);
            desktopLyricsWindow->setAttribute(Qt::WA_DeleteOnClose);
            desktopLyricsWindow->show();
        }
        isPlaying = true;
    }
}

void MainApplication::updateLyric(const CLyric &lyric, bool manualSearch) {
    if (!manualSearch && pcLyric)
        return;

    cLyric = lyric;
    pcLyric = &cLyric;

    currentTrack.instrumental = cLyric.track.instrumental;

    offset = cLyric.offset;

    if (cLyric.track.source != "LocalFile") {
        cLyric.track = currentTrack;
        cLyric.saveToFile(appDataPath.toStdString());
    }

    if (conversionTCSC) {
        for (auto &item: pcLyric->lyrics) {
            if (stringContainsKana(item.content)) {
                currentTrack.contentLanguage = Track::Language::ja;
                break;
            }
            if (stringContainsCJKCharacter(item.content)) {
                currentTrack.contentLanguage = Track::Language::zh;
            }
        }
        for (auto &item: pcLyric->lyrics) {
            if (stringContainsKana(item.translation)) {
                currentTrack.translateLanguage = Track::Language::ja;
                break;
            }
            if (stringContainsCJKCharacter(item.translation)) {
                currentTrack.translateLanguage = Track::Language::zh;
            }
        }
    }

    if (desktopLyricsWindow)
        desktopLyricsWindow->updateLyric(pcLyric);
    if (lyricsWindow)
        lyricsWindow->updateLyric(pcLyric);
    if (offsetWindow)
        offsetWindow->close();

    if (isPlaying)
        updateTime();
}

void MainApplication::updateTime(int position, bool playing) {
    if (playing)
        resume();
    else pause();

    if (pcLyric && pcLyric->track.instrumental) {
        if (desktopLyricsWindow)
            desktopLyricsWindow->hide();
        return;
    }

    if (!pcLyric || !pcLyric->isValid()) {
        return;
    }

    if (position == -1) {
        position = elapsedTime + eTimer->elapsed();
    }

    elapsedTime = position;
    eTimer->start();

    if (pcLyric->lyrics.size() == 1) {
        currentLine = 0;
        if (desktopLyricsWindow)
            desktopLyricsWindow->setLine(currentLine, position - pcLyric->lyrics[currentLine].startTime - offset);
        if (lyricsWindow)
            lyricsWindow->activateLine(currentLine);
        return;
    }

    for (size_t i = 0; i < pcLyric->lyrics.size() - 2;) {
        if (pcLyric->lyrics[++i].startTime + offset > position) {
            currentLine = i - 1;
            timer->setInterval(pcLyric->lyrics[i].startTime + offset - position);
            timer->start();
            if (desktopLyricsWindow)
                desktopLyricsWindow->setLine(currentLine, position - pcLyric->lyrics[currentLine].startTime - offset);
            if (lyricsWindow)
                lyricsWindow->activateLine(currentLine);
            break;
        }
    }

}

void MainApplication::findLyric(const std::string &title, const std::string &album, const std::string &artist,
                                int duration) {
    if (appDataPath.isEmpty())
        return;
    CLyric lyric = CLyricSearch().fetchCLyric(title, album, artist, duration, appDataPath.toStdString());
    if (lyric.isValid()) {
        emit lyricFound(lyric, false);
    } else {
        emit clearLyricsSignal();
    }
}

void MainApplication::cleanupOnQuit() {
    trayIcon->hide();
}

void MainApplication::loadLyricFile() {
    QString fileName = QFileDialog::getOpenFileName(nullptr, "Select Lyric File",
                                                    QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    QFile lyricFile(fileName);
    if (!lyricFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox(QMessageBox::Critical, "CrystalLyrics", "Invalid Lyric File", QMessageBox::Ok).show();
        return;
    }
    QTextStream lyricIn(&lyricFile);
    lyricIn.setEncoding(QStringConverter::Utf8);
    QString fileContent = lyricIn.readAll();
    CLyric lyric(fileContent.toStdString(), currentTrack, LyricStyle::CLrcStyle);
    if (lyric.isValid())
        updateLyric(lyric, true);
    else
        QMessageBox(QMessageBox::Critical, "CrystalLyrics", "Invalid Lyric File", QMessageBox::Ok).show();
}

void MainApplication::wrongLyric() {
    if (desktopLyricsWindow)
        desktopLyricsWindow->hide();
    if (lyricsWindow)
        lyricsWindow->clearLyrics();
    if (pcLyric)
        pcLyric->deleteFile(appDataPath.toStdString());
    pcLyric = nullptr;
}

namespace {
    inline std::string normalizeFileName(std::string name) {
        for (auto &c: name) {
            if (static_cast<unsigned char>(c) > 127) {
                // Non-Ascii char
                continue;
            }
            switch (c) {
                case '/':
                case '\\':
                case '?':
                case '%':
                case '*':
                case ':':
                case '|':
                case '"':
                case '<':
                case '>':
                    c = ' ';
            }
        }
        return name;
    }
}

void MainApplication::setTrackInstrumental() {
    if (currentTrack.duration > 0) {
        currentTrack.instrumental = true;
        CLyric instrumentalLyric(currentTrack, std::vector<CLyricItem>());
        updateLyric(instrumentalLyric, true);
    }
}

void MainApplication::setAlbumInstrumental() {
    if (!currentTrack.album.empty()) {
        QFile albumFile(
                appDataPath + "/" + QString::fromStdString(normalizeFileName(currentTrack.album + ".instrumental")));
        albumFile.open(QIODevice::WriteOnly);
        albumFile.close();
    }
    setTrackInstrumental();
}

void MainApplication::clearLyrics() {
    if (desktopLyricsWindow)
        desktopLyricsWindow->hide();
    if (lyricsWindow)
        lyricsWindow->clearLyrics();
}

void MainApplication::reshowDesktopLyricsWindow(bool changed, bool enabled) {
    if (changed) {
        if (enabled) {
            desktopLyricsWindow = new DesktopLyricsWindow(this, pcLyric, nullptr);
            desktopLyricsWindow->setAttribute(Qt::WA_DeleteOnClose);
            desktopLyricsWindow->show();
        } else if (desktopLyricsWindow) desktopLyricsWindow->close();
    } else if (desktopLyricsWindow) {
        desktopLyricsWindow->close();
        desktopLyricsWindow = new DesktopLyricsWindow(this, pcLyric, nullptr);
        desktopLyricsWindow->setAttribute(Qt::WA_DeleteOnClose);
        desktopLyricsWindow->show();
    }
    if (offsetWindow)
        offsetWindow->close();
}

void MainApplication::reshowLyricsWindow() {
    if (lyricsWindow) {
        lyricsWindow->close();
        lyricsWindow = new LyricsWindow(this, pcLyric, nullptr);
        lyricsWindow->setAttribute(Qt::WA_DeleteOnClose);
        lyricsWindow->show();
    }
    if (offsetWindow)
        offsetWindow->close();
}

void MainApplication::createMenu(bool firstTime) {
    if (!firstTime) {
        screenActions.clear();
        actionGroup->deleteLater();
        screenSubMenu->deleteLater();
        mainMenu->deleteLater();
    }

    QList<QScreen *> screens = QApplication::screens();
    int screenIndex = 0;

    if (!chosenScreenName.isEmpty()) {
        for (int i = 0; i < screens.size(); ++i) {
            if (screens[i]->name() == chosenScreenName) {
                screenIndex = i;
                break;
            }
        }
    } else {
        chosenScreenName = screens[screenIndex]->name();
    }

    actionGroup = new QActionGroup(this);
    screenSubMenu = new QMenu("Desktop Lyrics Screen");

    for (int i = 0; i < screens.size(); ++i) {
        auto *screenAction = new QAction(
                QString("%1 %2x%3").arg(screens[i]->name()).arg(screens[i]->size().width()).arg(
                        screens[i]->size().height()));
        screenAction->setCheckable(true);
        screenAction->setData(i);
        connect(screenAction, &QAction::triggered, this, &MainApplication::setDesktopLyricScreen);
        screenActions.append(screenAction);
        screenSubMenu->addAction(screenAction);
        actionGroup->addAction(screenAction);
    }

    screenActions[screenIndex]->setChecked(true);

    mainMenu = new QMenu();
    mainMenu->addAction(searchLyricAction);
    mainMenu->addAction(loadLocalLyricFileAction);
    mainMenu->addAction(markAsWrongAction);
    mainMenu->addAction(editLyricsAction);
    mainMenu->addAction(offsetAction);
    mainMenu->addSeparator();
    mainMenu->addAction(markTrackAsInstrumentalAction);
    mainMenu->addAction(markAlbumAsInstrumentalAction);
    mainMenu->addSeparator();
    mainMenu->addAction(lyricsWindowAction);
    mainMenu->addSeparator();
    mainMenu->addMenu(screenSubMenu);
    mainMenu->addAction(desktopLyricsWindowAction);
    mainMenu->addSeparator();
    mainMenu->addAction(settingsAction);
    mainMenu->addAction(exitAction);
}

void MainApplication::setDesktopLyricScreen() {
    auto *action = qobject_cast<QAction *>(sender());
    int screenIndex = action->data().toInt();
    QList<QScreen *> screens = QApplication::screens();
    chosenScreenName = screens[screenIndex]->name();
    if (desktopLyricsWindow)
        desktopLyricsWindow->resize(screenIndex);
}

void MainApplication::screenChanged([[maybe_unused]] QScreen *screen) {
    QList<QScreen *> screens = QApplication::screens();
    foreach(QScreen *screen, screens) {
        screen->disconnect();
        connect(screen, &QScreen::geometryChanged, [this](const QRect &) { geometryChanged(); });
    }
    geometryChanged();
}

void MainApplication::geometryChanged() {
    createMenu();
    trayIcon->setContextMenu(mainMenu);
    if (desktopLyricsWindow) {
        desktopLyricsWindow->resize(currentScreenIndex());
    }
}

int MainApplication::currentScreenIndex() {
    QList<QScreen *> screens = QApplication::screens();

    for (int i = 0; i < screens.size(); ++i) {
        if (screens[i]->name() == chosenScreenName) {
            return i;
        }
    }
    return 0;
}

void MainApplication::updateLyricOffset(int offset) {
    this->offset = int(offset);
    updateTime(-1, isPlaying);
}

void MainApplication::clearLyricOffset() {
    updateLyricOffset(cLyric.offset);
}

void MainApplication::saveLyricOffset(int offset) {
    updateLyricOffset(offset);
    if (cLyric.isValid()) {
        cLyric.offset = offset;
        cLyric.saveToFile(appDataPath.toStdString());
    }
}

void MainApplication::showOffsetWindow() {
    if (offsetWindow == nullptr) {
        offsetWindow = new OffsetWindow(this, pcLyric, nullptr);
        offsetWindow->setAttribute(Qt::WA_DeleteOnClose);
        offsetWindow->show();
        offsetWindow->activateWindow();
        offsetWindow->raise();
    } else {
        offsetWindow->activateWindow();
    }
}
