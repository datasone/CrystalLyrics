//
// Created by datasone on 14/8/2020.
//

#ifndef CRYSTALLYRICS_OFFSETWINDOW_H
#define CRYSTALLYRICS_OFFSETWINDOW_H

#include "CLyric.h"
#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QPushButton>

class TrayIcon;

using cLyric::CLyric;

class OffsetWindow: public QWidget {
Q_OBJECT
public:
    explicit OffsetWindow(QWidget* parent = nullptr, CLyric* lyric = nullptr, TrayIcon* trayIcon = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    CLyric* cLyric;
    TrayIcon* trayIcon;
    QHBoxLayout* layout;
    QDoubleSpinBox* spinBox;
    QPushButton* saveButton;
};


#endif //CRYSTALLYRICS_OFFSETWINDOW_H
