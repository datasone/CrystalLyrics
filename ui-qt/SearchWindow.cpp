//
// Created by datasone.
// This file is part of CrystalLyrics.
//

#include <thread>
#include <QTimer>
#include <QNetworkReply>
#include <CLyric/CLyricUtils.h>

#include "CLyric.h"
#include "TrayIcon.h"
#include "SearchWindow.h"
#include "ui_SearchWindow.h"

SearchWindow::SearchWindow(const string& title, const string& artist, int duration, TrayIcon* trayIcon, QWidget* parent)
        : QMainWindow(parent), ui(new Ui::SearchWindow), duration(duration) {
    ui->setupUi(this);

    nam = new QNetworkAccessManager(this);
    timer = new QTimer();
    timer->setSingleShot(true);

    ui->titleEdit->setText(QString::fromStdString(title));
    ui->artistEdit->setText(QString::fromStdString(artist));

    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setColumnCount(3);
    ui->tableWidget->setHorizontalHeaderLabels(QStringList() << "Title" << "Artist" << "Source");
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setHighlightSections(false);
    ui->tableWidget->verticalHeader()->setVisible(false);

    connect(ui->searchButton, &QPushButton::clicked, this, &SearchWindow::searchLyrics);
    connect(ui->tableWidget, &QTableWidget::itemClicked, this, &SearchWindow::itemClicked);
    connect(ui->tableWidget, &QTableWidget::itemDoubleClicked, this, &SearchWindow::itemDoubleClicked);

    connect(timer, &QTimer::timeout, this, &SearchWindow::timeout);
    connect(nam, &QNetworkAccessManager::finished, this, &SearchWindow::coverDownloadfinished);
    connect(this, &SearchWindow::searchResultSignal, this, &SearchWindow::appendLyrics,
            Qt::ConnectionType::QueuedConnection);
    connect(this, &SearchWindow::updateLyricSignal, trayIcon, &TrayIcon::updateLyric);
}

SearchWindow::~SearchWindow() {
    delete ui;
}

void SearchWindow::searchLyrics() {
    lyricList.clear();
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(0);

    ui->progressBar->reset();
    ui->progressBar->setMaximum(0);

    std::thread thread([this] {
        auto lyrics = CLyricSearch(TrayIcon::openCCSimpleConverter).searchCLyric(ui->titleEdit->text().toStdString(),
                                                                                 ui->artistEdit->text().toStdString(),
                                                                                 duration);

        std::stable_sort(lyrics.begin(), lyrics.end(),
                         [this](const CLyric& res1, const CLyric& res2) {
                             string title = this->ui->titleEdit->text().toStdString();
                             string artist = this->ui->artistEdit->text().toStdString();
                             int length = title.size() + artist.size() / 2;
                             int distance1 =
                                     stringDistance(res1.track.title, title) + stringDistance(res1.track.artist, artist) / 2;
                             int distance2 =
                                     stringDistance(res2.track.title, title) + stringDistance(res2.track.artist, artist) / 2;
                             double score1 = 1 - double(distance1) / length;
                             double score2 = 1 - double(distance2) / length;
                             if (std::any_of(res1.lyrics.begin(), res1.lyrics.end(), [](CLyricItem item){return !item.translation.empty();}))
                                 score1 += 0.2;
                             if (std::any_of(res2.lyrics.begin(), res2.lyrics.end(), [](CLyricItem item){return !item.translation.empty();}))
                                 score2 += 0.2;
                             if (std::any_of(res1.lyrics.begin(), res1.lyrics.end(), [](CLyricItem item){return !item.timecodes.empty();}))
                                 score1 += 0.1;
                             if (std::any_of(res2.lyrics.begin(), res2.lyrics.end(), [](CLyricItem item){return !item.timecodes.empty();}))
                                 score2 += 0.1;
                             return score1 > score2;
                         });

        emit searchResultSignal(std::move(lyrics));
    });
    thread.detach();
}

void SearchWindow::appendLyrics(std::vector<CLyric> lyrics) {
    ui->progressBar->setMaximum(100);
    ui->progressBar->setValue(100);
    for (CLyric& lyric: lyrics) {
        int currentRow = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(currentRow);
        ui->tableWidget->setItem(currentRow, 0, new QTableWidgetItem(QString::fromStdString(lyric.track.title)));
        ui->tableWidget->setItem(currentRow, 1, new QTableWidgetItem(QString::fromStdString(lyric.track.artist)));
        ui->tableWidget->setItem(currentRow, 2, new QTableWidgetItem(QString::fromStdString(lyric.track.source)));
    }
    if (lyricList.empty()) {
        lyricList = std::move(lyrics);
    } else {
        lyricList.insert(std::end(lyricList), std::make_move_iterator(std::begin(lyrics)),
                         std::make_move_iterator(std::end(lyrics)));
    }
}

void SearchWindow::itemClicked(QTableWidgetItem* item) {
    int row = item->row();
    if (selectedRow != row)
        selectedRow = row;
    else return;
    CLyric& lyric = lyricList[row];
    ui->lyricText->setText(QString::fromStdString(lyric.readableString()));
    if (coverImages.contains(QString::fromStdString(lyric.track.coverImageUrl))) {
        ui->coverImageLabel->setPixmap(coverImages[QString::fromStdString(lyric.track.coverImageUrl)]);
    } else {
        QNetworkRequest request(QUrl(QString::fromStdString(lyric.track.coverImageUrl)));
        request.setRawHeader("User-Agent", "CrystalLyrics/0.0.1");
        networkReply = nam->get(request);
        if (timer->isActive())
            timer->stop();
        timer->start(10000);
    }
}

void SearchWindow::timeout() {
    if (networkReply && networkReply->isRunning()) {
        networkReply->abort();
        networkReply->deleteLater();
    }
}

void SearchWindow::itemDoubleClicked(QTableWidgetItem* item) {
    CLyric& lyric = lyricList[item->row()];
    emit updateLyricSignal(lyric, true);
}

void SearchWindow::coverDownloadfinished(QNetworkReply* reply) {
    int w = ui->coverImageLabel->width(), h = ui->coverImageLabel->height();
    if (timer->isActive())
        timer->stop();
    QPixmap pm;
    pm.loadFromData(reply->readAll());
    coverImages[reply->url().toString()] = pm.scaled(w, h, Qt::KeepAspectRatio);
    ui->coverImageLabel->setPixmap(coverImages[reply->url().toString()]);
}
