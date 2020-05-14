//
// Created by datasone.
// This file is part of CrystalLyrics.
//

#include "EditLyricsWindow.h"
#include "ui_EditLyricsWindow.h"
#include "TrayIcon.h"

#include <QMessageBox>

EditLyricsWindow::EditLyricsWindow(QWidget* parent, TrayIcon* trayIcon) : QMainWindow(parent),
                                                                          ui(new Ui::EditLyricsWindow),
                                                                          trayIcon(trayIcon) {

    ui->setupUi(this);

    if (trayIcon->pcLyric != nullptr) {
        ui->lyricsText->setText(QString::fromStdString(trayIcon->pcLyric->readableString()));
    }

    connect(ui->extractTranslationButton, &QPushButton::clicked, this, &EditLyricsWindow::autoExtractTranslate);
    connect(ui->adjustTimeButton, &QPushButton::clicked, this, &EditLyricsWindow::adjustTime);
    connect(ui->saveButton, &QPushButton::clicked, this, &EditLyricsWindow::saveLyrics);
}

EditLyricsWindow::~EditLyricsWindow() {
    delete ui;
}

void EditLyricsWindow::autoExtractTranslate() {
    ui->lyricsText->setPlainText(
            ui->lyricsText->toPlainText().replace(QRegularExpression(R"(^(\[.*\])(.*) *\/ *(.*)$)", QRegularExpression::MultilineOption),
                                                  R"(\1\2
\1[tr]\3)")
    );
}

void EditLyricsWindow::saveLyrics() {
    CLyric lyric = CLyric(ui->lyricsText->toPlainText().toStdString(), LyricStyle::CLrcStyle);
    if (!lyric.isValid()) {
        QMessageBox(QMessageBox::Critical, "CrystalLyrics", "Invalid Lyric Text", QMessageBox::Ok).show();
        return;
    }
    trayIcon->updateLyric(lyric, true);
}

void EditLyricsWindow::adjustTime() {
    CLyric lyric = *(trayIcon->pcLyric);
    for (auto& item: lyric.lyrics) {
        int startTime = item.startTime, minuteTime = 0, secondTime = 0, millisecondTime = 0;

        if (startTime % 1000 != 0) {
            millisecondTime = startTime % 1000 / 10;
            startTime = startTime / 1000 * 1000;
            startTime -= millisecondTime * 1000;
            minuteTime = startTime / (60 * 60 * 1000);
            secondTime = startTime % (60 * 60 * 1000) / (60 * 1000);
        } else {
            minuteTime = startTime / (60 * 60 * 1000);
            secondTime = startTime % (60 * 60 * 1000) / (60 * 1000);
            millisecondTime = startTime % (60 * 1000) / 1000;
        }

        item.startTime = minuteTime * (60 * 1000) + secondTime * 1000 + millisecondTime;
    }
    ui->lyricsText->setPlainText(QString::fromStdString(lyric.readableString()));
}
