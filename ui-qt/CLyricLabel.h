//
// Created by datasone.
// This file is part of CrystalLyrics.
//

#ifndef CRYSTALLYRICS_CLYRICLABEL_H
#define CRYSTALLYRICS_CLYRICLABEL_H

#include <QLabel>
#include <QtCore/QPair>
#include <QtCore/QTimer>
#include "CLyric.h"

using cLyric::CLyricItem;

class CLyricLabel : public QLabel {
Q_OBJECT
public:
    explicit CLyricLabel(const QFont &font, QColor color, QColor playedColor,
                         CLyricItem *item = nullptr, bool firstLine = true, QWidget *parent = nullptr);

    QString text;
private:
    QFont font;
    QFontMetricsF metrics = QFontMetricsF(font);
    QColor color, playedColor;
    CLyricItem *item;
    bool firstLine, hasTimeCode = false;
    QList<QPair<int, int>> pixelMap; // QList<QPair<ms, pixels>>
    int segmentIndex = 0, currentTimeInSegment = 0;
    double maskWidth = 0, maskWidthInterval = -1;
    QTimer *fillTimer, *segmentTimer;

public slots:

    void updateLyric(CLyricItem *newItem = nullptr, bool isFirstLine = true, bool convertTCSC = false);

    void updateTime(int timeInMs);

private slots:

    void startFill(int segmentIndex, int currentTimeInSegment = 0);

    void fill();

protected:
    void paintEvent(QPaintEvent *event) override;
};


#endif //CRYSTALLYRICS_CLYRICLABEL_H
