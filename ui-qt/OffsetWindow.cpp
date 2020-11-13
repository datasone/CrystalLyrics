//
// Created by datasone on 14/8/2020.
//

#include "OffsetWindow.h"

#include "MainApplication.h"
#include <QCloseEvent>
#include <QMessageBox>

OffsetWindow::OffsetWindow(MainApplication *mainApp, CLyric *lyric, QWidget *parent)
        : QWidget(parent), cLyric(lyric), mainApp(mainApp) {
    this->setWindowFlags(
        #ifdef Q_OS_MACOS
            Qt::Window |
        #else
            Qt::Tool |
        #endif
            Qt::WindowStaysOnTopHint);

    layout = new QHBoxLayout(this);

    spinBox = new QDoubleSpinBox(this);
    spinBox->setDecimals(0);
    spinBox->setMaximum(10000);
    spinBox->setMinimum(-10000);
    spinBox->setSingleStep(100);
    spinBox->setValue(lyric ? lyric->offset : 0);
    connect(spinBox, qOverload<double>(&QDoubleSpinBox::valueChanged),
            [mainApp](double offset) { mainApp->updateLyricOffset(int(offset)); });

    saveButton = new QPushButton("Save Offset", this);
    connect(saveButton, &QPushButton::clicked, [mainApp, this]() { mainApp->saveLyricOffset(int(spinBox->value())); });

    layout->addWidget(new QLabel("Adjust lyric offset "));
    layout->addWidget(spinBox);
    layout->addWidget(new QLabel(" ms"));
    layout->addWidget(saveButton);
}

void OffsetWindow::closeEvent(QCloseEvent *event) {
    if (event->spontaneous() && cLyric && spinBox->value() != cLyric->offset) {
        const QMessageBox::StandardButton resBtn = QMessageBox::question(this, "Offset",
                                                                         "Do you want to save the offset?\n",
                                                                         QMessageBox::Cancel | QMessageBox::No |
                                                                         QMessageBox::Yes, QMessageBox::Yes);
        if (resBtn == QMessageBox::Cancel) {
            event->ignore();
        } else if (resBtn == QMessageBox::No) {
            mainApp->clearLyricOffset();
            event->accept();
        } else if (resBtn == QMessageBox::Yes) {
            mainApp->saveLyricOffset(int(spinBox->value()));
            event->accept();
        }
    } else {
        QWidget::closeEvent(event);
    }
}
