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
    if (trayIcon->pcLyric == nullptr)
        return;

    ui->setupUi(this);

    ui->lyricsText->setText(QString::fromStdString(trayIcon->pcLyric->readableString()));

    connect(ui->extractTranslationButton, &QPushButton::clicked, this, &EditLyricsWindow::autoExtractTranslate);
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
