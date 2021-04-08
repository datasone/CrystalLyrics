//
// Created by datasone.
// This file is part of CrystalLyrics.
//

#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QScrollBar>

#include "utils.h"
#include "LyricsWindow.h"
#include "MainApplication.h"

inline QString labelStylesheet(const QString &color) { return QString("QLabel { color : %1 }").arg(color); }

LyricsWindow::LyricsWindow(MainApplication *mainApp, CLyric *lyric, QWidget *parent)
        : QWidget(parent), cLyric(lyric), mainApp(mainApp) {

    this->setWindowFlags(Qt::FramelessWindowHint);
    this->resize(400, 400);

    windowBackgroundColor = settings.value("windowBackgroundColor", "#2B2B2B").toString();
    windowLyricsTextColor = settings.value("windowLyricsTextColor", "#C0C0C0").toString();
    windowLyricsTextPlayingColor = settings.value("windowLyricsTextPlayingColor", "#E2FDCA").toString();

    this->setStyleSheet(QString("QWidget { background-color : %1; }").arg(windowBackgroundColor));

    QPainterPath path;
    path.addRoundedRect(this->rect(), 16, 16);
    this->setMask(path.toFillPolygon().toPolygon());

    this->setContentsMargins(0, 40, 0, 40);

    if (settings.contains("lyricsWindowPosX"))
        this->move(settings.value("lyricsWindowPosX").toInt(), settings.value("lyricsWindowPosY").toInt());

    rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    scrollArea = new QScrollArea(this);
    scrollArea->setFrameStyle(QFrame::NoFrame);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    scrollArea->setWidgetResizable(true);
    rootLayout->addWidget(scrollArea);

    displayArea = new QWidget(scrollArea);
    textLayout = new QVBoxLayout(displayArea);
    textLayout->setSpacing(40);
    textLayout->setContentsMargins(0, 0, 0, 0);

    auto windowFontFamily = settings.value("windowFont/family");
    if (windowFontFamily.isNull()) {
        windowFont = windowFont.defaultFamily();
    } else {
        auto fontSize = settings.value("windowFont/size", 16);
        auto fontWeight = settings.value("windowFont/weight", QFont::Normal);
        auto fontItalic = settings.value("windowFont/italic", false);
        windowFont = QFont(windowFontFamily.toString(), fontSize.toInt(), fontWeight.toInt(), fontItalic.toBool());
    }

    scrollArea->setWidget(displayArea);

    closeButton = new QPushButton(this);
    closeButton->move(10, 10);
    closeButton->setFixedSize(16, 16);
    closeButton->setStyleSheet(
            "QPushButton { border-image: url(:/image/closeButtonWhite.png); } QPushButton:pressed { border-image: url(:/image/closeButtonBlack.png) }");

    connect(closeButton, &QPushButton::clicked, this, &QWidget::close);

    updateLyric(lyric);

    if (mainApp->currentLine > 0)
        activateLine(mainApp->currentLine);
}

void LyricsWindow::mousePressEvent(QMouseEvent *event) {
    oldPos = event->globalPosition();
}

void LyricsWindow::mouseMoveEvent(QMouseEvent *event) {
    const QPointF delta = event->globalPosition() - oldPos;
    move(x() + delta.x(), y() + delta.y());
    oldPos = event->globalPosition();
}

void LyricsWindow::activateLine(int lineNum) {
    if (labels.isEmpty())
        return;
    if (activatedLabel != nullptr) {
        activatedLabel->setStyleSheet(labelStylesheet(windowLyricsTextColor));
    }
    activatedLabel = labels[lineNum];
    activatedLabel->setStyleSheet(labelStylesheet(windowLyricsTextPlayingColor));
    centerWidgetInScrollArea(scrollArea, displayArea, activatedLabel);
}

void LyricsWindow::updateLyric(CLyric *lyric) {
    if (lyric != nullptr)
        cLyric = lyric;

    clearLyrics();

    if (cLyric != nullptr) {
        for (const CLyricItem &item : cLyric->lyrics) {
            auto *label = new QLabel(displayArea);
            label->setFont(windowFont);
            label->setStyleSheet(labelStylesheet(windowLyricsTextColor));
            label->setWordWrap(true);
            label->setAlignment(Qt::AlignHCenter);
            std::string contentText = item.content;
            if (mainApp && mainApp->currentTrack.contentLanguage == Track::Language::zh)
                contentText = MainApplication::openCCSimpleConverter.Convert(item.content);
            if (item.isDoubleLine()) {
                contentText.append("\n");
                if (mainApp && mainApp->currentTrack.translateLanguage == Track::Language::zh) {
                    contentText.append(MainApplication::openCCSimpleConverter.Convert(item.translation));
                } else contentText.append(item.translation);
            }
            label->setText(QString::fromStdString(contentText));
            labels.append(label);
            textLayout->addWidget(label);
        }
    }
}

void LyricsWindow::closeEvent([[maybe_unused]] QCloseEvent *event) {
    settings.setValue("lyricsWindowPosX", x());
    settings.setValue("lyricsWindowPosY", y());
}

void LyricsWindow::clearLyrics() {
    activatedLabel = nullptr;
    qDeleteAll(labels);
    labels.clear();
    for (auto *label : displayArea->findChildren<QWidget *>())
        delete label;
}
