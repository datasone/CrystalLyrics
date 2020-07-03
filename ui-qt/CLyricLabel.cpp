//
// Created by datasone.
// This file is part of CrystalLyrics.
//

#include <QPainter>
#include <QMouseEvent>
#include <QPushButton>
#include <utility>
#include "CLyricLabel.h"
#include "ui-qt/utils.h"
#include "TrayIcon.h"

// TODO: Font Shadow

CLyricLabel::CLyricLabel(QWidget* parent, const QFont& font, QColor color, QColor playedColor, CLyricItem* item,
                         bool firstLine)
        : QLabel(parent), font(font), color(std::move(color)), playedColor(std::move(playedColor)), item(item),
          firstLine(firstLine) {
    if (item->translation.empty()) text = QString::fromStdString(item->content);
    text = QString::fromStdString(firstLine ? item->content : item->translation);
    hasTimeCode = !(item->timecodes.empty());

    metrics = QFontMetricsF(font);
    if (hasTimeCode) {
        for (const auto& timecode : item->timecodes) {
            pixelMap.append(QPair(timecode.first, metrics.horizontalAdvance(text,
                                                                            timecode == item->timecodes.back() ? -1
                                                                                                               : timecode.second)));
        }
    }

    fillTimer = new QTimer(this);
    segmentTimer = new QTimer(this);
    segmentTimer->setSingleShot(true);

    connect(fillTimer, &QTimer::timeout, this, &CLyricLabel::fill);
    connect(segmentTimer, &QTimer::timeout, this, [=]() { startFill(segmentIndex, 0); });
}

void CLyricLabel::paintEvent([[maybe_unused]] QPaintEvent* event) {
    if (item == nullptr) {
        return;
    }

    QPainter painter(this);
    painter.setFont(font);
    painter.setPen(color);
    painter.drawText(0, 0, width(), height(), alignment(), text);

    if (hasTimeCode) {
        QPointF textStartingPos = getTextStartingPos(width(), height(), metrics.horizontalAdvance(text), metrics.height(),
                                             alignment());

        painter.setPen(playedColor);
        painter.drawText(QRectF(textStartingPos, QSizeF(maskWidth, height())), Qt::AlignTop | Qt::AlignLeft, text);
    }
}

void CLyricLabel::startFill(int currentSegmentIndex, int currentTimeInSegment) {
    segmentIndex = currentSegmentIndex;

    fillTimer->stop();
    if (!firstLine || !hasTimeCode) {
        update();
        return;
    }

    if (segmentIndex >= pixelMap.size())
        return;

    if (maskWidthInterval < 0 && segmentIndex > 0) {
        const int timeInterval = pixelMap[segmentIndex].first - pixelMap[segmentIndex - 1].first;
        const int pixelInterval = pixelMap[segmentIndex].second - pixelMap[segmentIndex - 1].second;

        maskWidthInterval = pixelInterval * 16.0 / timeInterval;
    }

    maskWidth = pixelMap[segmentIndex].second + currentTimeInSegment / 16.0 * maskWidthInterval;
    update();

    if (++segmentIndex == pixelMap.size()) return;

    const int timeInterval = pixelMap[segmentIndex].first - pixelMap[segmentIndex - 1].first;
    const int pixelInterval = pixelMap[segmentIndex].second - pixelMap[segmentIndex - 1].second;

    maskWidthInterval = pixelInterval * 16.0 / timeInterval;

    fillTimer->start(16);

    segmentTimer->setInterval(timeInterval - currentTimeInSegment);
    segmentTimer->start();
}

void CLyricLabel::fill() {
    maskWidth += maskWidthInterval;
    update();
}

void CLyricLabel::updateLyric(CLyricItem* newItem, bool isFirstLine, bool convertTCSC) {

    if (newItem == nullptr) { // Refresh and restart
        updateTime(0);
    } else {
        item = newItem;
        maskWidth = 0;
        maskWidthInterval = -1;
        pixelMap.clear();

        text = QString::fromStdString(isFirstLine ? item->content : item->translation);

        if (convertTCSC) {
            text = QString::fromStdString(TrayIcon::openCCSimpleConverter.Convert(text.toStdString()));
        }

        hasTimeCode = !(item->timecodes.empty());

        if (hasTimeCode) {
            for (const auto& timecode: item->timecodes) {
                pixelMap.append(QPair(timecode.first,
                                      metrics.horizontalAdvance(text,
                                                                timecode == item->timecodes.back() ? -1
                                                                                                   : timecode.second)));
            }
            updateTime(0);
        }
    }
    update();
}

void CLyricLabel::updateTime(int timeInMs) {
    int timeInSegment = -1;
    if (hasTimeCode) {
        for (auto i = 0; i < item->timecodes.size() - 2;) {
            if (item->timecodes[++i].first > timeInMs) {
                segmentIndex = i - 1;
                timeInSegment = timeInMs - item->timecodes[segmentIndex].first;
                break;
            }
        }
        if (timeInSegment >= 0)
            startFill(segmentIndex, timeInSegment);
    }
}