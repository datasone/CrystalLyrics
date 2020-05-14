//
// Created by datasone.
// This file is part of CrystalLyrics.
//

#ifndef CRYSTALLYRICS_EDITLYRICSWINDOW_H
#define CRYSTALLYRICS_EDITLYRICSWINDOW_H

#include <QMainWindow>
#include "CLyric.h"

QT_BEGIN_NAMESPACE
namespace Ui { class EditLyricsWindow; }
QT_END_NAMESPACE

class TrayIcon;

class EditLyricsWindow : public QMainWindow {
Q_OBJECT
private slots:

    void autoExtractTranslate();

    void adjustTime();

    void saveLyrics();

public:
    explicit EditLyricsWindow(QWidget* parent = nullptr, TrayIcon* trayIcon = nullptr);

    ~EditLyricsWindow() override;

private:
    Ui::EditLyricsWindow* ui;
    TrayIcon* trayIcon;
};


#endif //CRYSTALLYRICS_EDITLYRICSWINDOW_H
