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

class CLyricLabel : public QLabel {
Q_OBJECT
public:
    explicit CLyricLabel(QWidget* parent = nullptr, const QFont& font = QFont(), QColor color = QColor(Qt::white),
                         QColor playedColor = QColor(Qt::lightGray), CLyricItem* item = nullptr, bool firstLine = true);

private:
    QFont font;
    QFontMetrics metrics = QFontMetrics(font);
    QColor color, playedColor;
    CLyricItem* item;
    bool firstLine, hasTimeCode = false;
    QString text;
    QList<QPair<int, int>> pixelMap; // QList<QPair<ms, pixels>>
    int segmentIndex = 0, currentTimeInSegment = 0;
    double maskWidth = 0, maskWidthInterval = -1;
    QPoint textStartingPos;
    QTimer* fillTimer, * segmentTimer;

public slots:

    void updateLyric(CLyricItem* newItem = nullptr, bool isFirstLine = true, bool convertTCSC = false);

    void updateTime(int timeInMs);

private slots:

    void startFill(int segmentIndex, int currentTimeInSegment = 0);

    void fill();

protected:
    void paintEvent(QPaintEvent* event) override;
};


#endif //CRYSTALLYRICS_CLYRICLABEL_H
