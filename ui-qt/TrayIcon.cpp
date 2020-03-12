//
// Created by datasone.
// This file is part of CrystalLyrics.
//

#include "TrayIcon.h"
#include "utils.h"

#include <thread>
#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include <QLocalSocket>
#include <QStandardPaths>
#include <QFileDialog>

Q_DECLARE_METATYPE(CLyric)

Q_DECLARE_METATYPE(std::vector<CLyric>)

opencc::SimpleConverter TrayIcon::openCCSimpleConverter = opencc::SimpleConverter("opencc-files/t2s.json");

TrayIcon::TrayIcon() {
    appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    if (!QDir().exists(appDataPath))
        QDir().mkpath(appDataPath);

    qRegisterMetaType<CLyric>("CLyric");
    qRegisterMetaType<std::vector<CLyric>>("std::vector<CLyric>");

    eTimer = new QElapsedTimer();

    timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, &TrayIcon::timerTimeout);

    trayIcon = new QSystemTrayIcon(QIcon(":/image/icon.png"));
    menu = new QMenu();

    searchLyricAction = new QAction("Search Lyrics", nullptr);
    markAsInstrumentalAction = new QAction("Mark Current Track as Instrumental", nullptr);
    loadLocalLyricFileAction = new QAction("Load Local Lyric File", nullptr);
    markAsWrongAction = new QAction("Current Lyric is Wrong", nullptr);
    lyricsWindowAction = new QAction("Show Lyrics Window", nullptr);
    editLyricsAction = new QAction("Edit Lyrics", nullptr);
    settingsAction = new QAction("Settings", nullptr);
    exitAction = new QAction("Exit", nullptr);

    menu->addAction(searchLyricAction);
    menu->addAction(markAsInstrumentalAction);
    menu->addAction(loadLocalLyricFileAction);
    menu->addAction(markAsWrongAction);
    menu->addAction(editLyricsAction);
    menu->addSeparator();
    menu->addAction(lyricsWindowAction);
    menu->addSeparator();
    menu->addAction(settingsAction);
    menu->addAction(exitAction);

    trayIcon->setContextMenu(menu);
    trayIcon->setVisible(true);
    trayIcon->show();
    trayIcon->setToolTip("CrystalLyrics");

    connect(lyricsWindowAction, &QAction::triggered, this, &TrayIcon::showLyricsWindow);
    connect(markAsInstrumentalAction, &QAction::triggered, this, &TrayIcon::setInstrumental);
    connect(searchLyricAction, &QAction::triggered, this, &TrayIcon::showSearchWindow);
    connect(loadLocalLyricFileAction, &QAction::triggered, this, &TrayIcon::loadLyricFile);
    connect(markAsWrongAction, &QAction::triggered, this, &TrayIcon::wrongLyric);
    connect(editLyricsAction, &QAction::triggered, this, &TrayIcon::showEditLyricsWindow);
    connect(settingsAction, &QAction::triggered, this, &TrayIcon::showSettingsWindow);
    connect(exitAction, &QAction::triggered, QApplication::instance(), &QApplication::quit);
    connect(QApplication::instance(), &QApplication::aboutToQuit, this, &TrayIcon::cleanupOnQuit);

    connect(this, &TrayIcon::lyricFound, this, &TrayIcon::updateLyric, Qt::ConnectionType::QueuedConnection);

    const auto desktopLyrics = settings.value("desktopLyrics", false).toBool();
    if (desktopLyrics) {
        desktopLyricsWindow = new DesktopLyricsWindow(nullptr, pcLyric, this);
        desktopLyricsWindow->setAttribute(Qt::WA_DeleteOnClose);
        desktopLyricsWindow->show();
    }

    const auto ipcSocketName = settings.value("ipcSocketName", "clyricsocket").toString();

    server = new QLocalServer(this);
    QLocalServer::removeServer(ipcSocketName);
    if (!server->listen(ipcSocketName)) {
        QMessageBox msgBox(QMessageBox::Critical, "CrystalLyrics", "Unable to setup IPC socket.\nProgram will exit.",
                           QMessageBox::Ok);
        QObject::connect(&msgBox, &QMessageBox::finished, [=]([[maybe_unused]] int i) { QApplication::quit(); });
        msgBox.show();
    }

    connect(server, &QLocalServer::newConnection, this, &TrayIcon::handleConnection);

    conversionTCSC = settings.value("conversionTCSC", false).toBool();
}

void TrayIcon::showLyricsWindow() {
    if (lyricsWindow == nullptr) {
        lyricsWindow = new LyricsWindow(nullptr, pcLyric, this);
        lyricsWindow->setAttribute(Qt::WA_DeleteOnClose);
        lyricsWindow->show();
    } else {
        lyricsWindow->activateWindow();
    }
}

void TrayIcon::showSearchWindow() {
    if (searchWindow == nullptr) {
        searchWindow = new SearchWindow(currentTrack.title, currentTrack.artist, currentTrack.duration, this,
                                        nullptr);
        searchWindow->setAttribute(Qt::WA_DeleteOnClose);
        searchWindow->show();
    } else {
        searchWindow->activateWindow();
    }
}

void TrayIcon::showSettingsWindow() {
    if (settingsWindow == nullptr) {
        settingsWindow = new SettingsWindow();
        settingsWindow->setAttribute(Qt::WA_DeleteOnClose);
        settingsWindow->show();
    } else {
        settingsWindow->activateWindow();
    }
}

void TrayIcon::showEditLyricsWindow() {
    if (editLyricsWindow == nullptr) {
        editLyricsWindow = new EditLyricsWindow(nullptr, this);
        editLyricsWindow->setAttribute(Qt::WA_DeleteOnClose);
        editLyricsWindow->show();
    } else {
        editLyricsWindow->activateWindow();
    }
}

void TrayIcon::handleConnection() {
    QLocalSocket* socket = server->nextPendingConnection();
    connect(socket, &QLocalSocket::readyRead, this, &TrayIcon::handleSocketRead);
    connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
}

void TrayIcon::handleSocketRead() {
    auto* socket = qobject_cast<QLocalSocket*>(sender());
    const auto content = QString::fromUtf8(socket->readAll());
    if (socketMap.contains(socket)) {
        socketMap[socket] = socketMap[socket] + content;
    } else {
        socketMap[socket] = content;
    }
    parseSocketResult(socket);
}

void TrayIcon::parseSocketResult(QLocalSocket* socket) {
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
            parameters.insert(parameter.split('=')[0], parameter.split('=')[1]);
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

        pcLyric = nullptr;
        std::thread thread([this] {
            findLyric(currentTrack.title, currentTrack.album, currentTrack.artist, currentTrack.duration);
        });
        thread.detach();
    } else if (task == "setState") {
        const int position = parameters["position"].toInt();
        const bool playing = parameters["playing"] == "true";
        updateTime(position, playing);
    } else {
        closeSocket(socket, "Bad Request");
    }
    closeSocket(socket);
}

void TrayIcon::closeSocket(QLocalSocket* socket, const char* message) {
    socketMap.remove(socket);
    socket->write(message);
    socket->close();
    socket->disconnectFromServer();
    socket->deleteLater();
}

void TrayIcon::timerTimeout() {
    if (!isPlaying || pcLyric->track.instrumental)
        return;

    currentLyric = &pcLyric->lyrics[++currentLine];

    if (desktopLyricsWindow)
        desktopLyricsWindow->setLine(currentLine);
    if (lyricsWindow)
        lyricsWindow->activateLine(currentLine);

    if (currentLine < pcLyric->lyrics.size() - 1) {
        nextLyric = &(pcLyric->lyrics[currentLine + 1]);
        timer->setInterval(nextLyric->startTime - currentLyric->startTime);
        timer->start();
    }
}

void TrayIcon::pause() {
    if (desktopLyricsWindow) {
        desktopLyricsWindow->pause();
    }
    if (isPlaying) {
        timer->stop();
        elapsedTime += eTimer->elapsed();
        isPlaying = false;
    }
}

void TrayIcon::resume() {
    if (!isPlaying) {
        eTimer->start();
        if (desktopLyricsWindow) {
            desktopLyricsWindow->resume();
        }
        isPlaying = true;
    }
}

void TrayIcon::updateLyric(const CLyric& lyric, bool manualSearch) {
    if (!manualSearch && pcLyric)
        return;

    cLyric = lyric;
    pcLyric = &cLyric;

    if (cLyric.track.source != "LocalFile") {
        currentTrack.instrumental = cLyric.track.instrumental;
        cLyric.track = currentTrack;
        cLyric.saveToFile(appDataPath.toStdString());
    }

    if (conversionTCSC) {
        for (auto& item: pcLyric->lyrics) {
            if (stringContainsKana(item.content)) {
                currentTrack.contentLanguage = Track::Language::ja;
                break;
            }
            if (stringContainsCJKCharacter(item.content)) {
                currentTrack.contentLanguage = Track::Language::zh;
            }
        }
        for (auto& item: pcLyric->lyrics) {
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

    if (isPlaying)
        updateTime();
}

void TrayIcon::updateTime(int position, bool playing) {
    if (playing)
        resume();
    else pause();

    if (pcLyric && pcLyric->track.instrumental) {
        if (desktopLyricsWindow)
            desktopLyricsWindow->pause();
        return;
    }

    if (!pcLyric) {
        return;
    }

    if (position == -1) {
        position = elapsedTime + eTimer->elapsed();
    } else {
        elapsedTime = position;
        eTimer->start();
    }


    for (int i = 0; i < pcLyric->lyrics.size() - 2;) {
        if (pcLyric->lyrics[++i].startTime > position) {
            currentLine = i - 1;
            timer->setInterval(pcLyric->lyrics[i].startTime - position);
            timer->start();
            if (desktopLyricsWindow)
                desktopLyricsWindow->setLine(currentLine, position - pcLyric->lyrics[currentLine].startTime);
            if (lyricsWindow)
                lyricsWindow->activateLine(currentLine);
            break;
        }
    }

}

void TrayIcon::findLyric(const string& title, const string& album, const string& artist, int duration) {
    if (appDataPath.isEmpty())
        return;
    CLyric lyric = CLyricSearch(TrayIcon::openCCSimpleConverter).fetchCLyric(title, album, artist, duration, appDataPath.toStdString());
    if (lyric.isValid()) {
        emit lyricFound(lyric, false);
    }
}

void TrayIcon::cleanupOnQuit() {
    trayIcon->hide();
}

void TrayIcon::setInstrumental() {
    if (currentTrack.duration > 0) {
        currentTrack.instrumental = true;
        CLyric instrumentalLyric(currentTrack, std::vector<CLyricItem>());
        updateLyric(instrumentalLyric, true);
    }
}

void TrayIcon::loadLyricFile() {
    QString fileName = QFileDialog::getOpenFileName(nullptr, "Select Lyric File", QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    QFile lyricFile(fileName);
    if (!lyricFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox(QMessageBox::Critical, "CrystalLyrics", "Invalid Lyric File", QMessageBox::Ok).show();
        return;
    }
    QTextStream lyricIn(&lyricFile);
    lyricIn.setCodec("UTF-8");
    QString fileContent = lyricIn.readAll();
    CLyric lyric(fileContent.toStdString(), currentTrack, LyricStyle::CLrcStyle);
    if (lyric.isValid())
        updateLyric(lyric, true);
    else
        QMessageBox(QMessageBox::Critical, "CrystalLyrics", "Invalid Lyric File", QMessageBox::Ok).show();
}

void TrayIcon::wrongLyric() {
    if (desktopLyricsWindow)
        desktopLyricsWindow->pause();
    if (lyricsWindow)
        lyricsWindow->clearLyrics();
    pcLyric->deleteFile(appDataPath.toStdString());
}
