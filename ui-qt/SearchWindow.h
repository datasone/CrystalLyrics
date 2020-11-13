//
// Created by datasone.
// This file is part of CrystalLyrics.
//

#ifndef CRYSTALLYRICS_SEARCHWINDOW_H
#define CRYSTALLYRICS_SEARCHWINDOW_H

#include <QMainWindow>
#include <QTableWidgetItem>
#include <QNetworkAccessManager>

class MainApplication;

QT_BEGIN_NAMESPACE
namespace Ui { class SearchWindow; }
QT_END_NAMESPACE

class SearchWindow : public QMainWindow {
Q_OBJECT

private slots:

    void searchLyrics();

    void itemClicked(QTableWidgetItem *item);

    void itemDoubleClicked(QTableWidgetItem *item);

    void appendLyrics(std::vector<CLyric> lyrics);

    void coverDownloadfinished(QNetworkReply *reply);

    void timeout();

public:
    SearchWindow(const std::string &title, const std::string &artist, int duration, MainApplication *mainApp,
                 QWidget *parent = nullptr);

    ~SearchWindow() override;

    void setTrack(const cLyric::Track &track);

signals:

    void searchResultSignal(std::vector<CLyric> lyrics);

    void updateLyricSignal(const CLyric &lyric, bool manualSearch);

private:
    Ui::SearchWindow *ui;
    int duration, selectedRow = -1;

    QNetworkAccessManager *nam;
    QNetworkReply *networkReply = nullptr;
    QTimer *timer;
    QMap<QString, QPixmap> coverImages;

    std::vector<CLyric> lyricList;
};


#endif //CRYSTALLYRICS_SEARCHWINDOW_H
