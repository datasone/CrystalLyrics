//
// Created by datasone.
// This file is part of CrystalLyrics.
//

#include "utils.h"

#include <QLayout>
#include <QPropertyAnimation>

void centerWidgetInScrollArea(QScrollArea* scrollArea, QWidget* displayArea, QWidget* childWidget) {
    if (scrollArea == nullptr || !scrollArea->isAncestorOf(displayArea) || !displayArea->isAncestorOf(childWidget))
        return;

    QScrollBar* horizontalScrollBar = scrollArea->horizontalScrollBar();
    QScrollBar* verticalScrollBar = scrollArea->verticalScrollBar();
    QMargins margins = displayArea->layout()->contentsMargins();
    QPoint pos = childWidget->pos();

    int horizontalScrollBarLength = displayArea->width() - horizontalScrollBar->maximum();
    int verticalScrollBarLength = displayArea->height() - verticalScrollBar->maximum();

    int horizontalTarget = pos.x() + childWidget->width() / 2 - margins.left() - horizontalScrollBarLength / 2;
    int verticalTarget = pos.y() + childWidget->height() / 2 - margins.top() - verticalScrollBarLength / 2;

    horizontalTarget = std::min(std::max(horizontalScrollBar->minimum(), horizontalTarget),
                                horizontalScrollBar->maximum());
    verticalTarget = std::min(std::max(verticalScrollBar->minimum(), verticalTarget), verticalScrollBar->maximum());

    auto* horizontalAnimation = new QPropertyAnimation(horizontalScrollBar, "value");
    horizontalAnimation->setDuration(500);
    horizontalAnimation->setStartValue(horizontalScrollBar->value());
    horizontalAnimation->setEndValue(horizontalTarget);

    auto* verticalAnimation = new QPropertyAnimation(verticalScrollBar, "value");
    verticalAnimation->setDuration(500);
    verticalAnimation->setStartValue(verticalScrollBar->value());
    verticalAnimation->setEndValue(verticalTarget);

    horizontalAnimation->start(QAbstractAnimation::DeleteWhenStopped);
    verticalAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

QPointF getTextStartingPos(int x, int y, float width, float height, Qt::Alignment alignment) {
    float px = 0, py = 0;
    if ((alignment & Qt::AlignLeft) == Qt::AlignLeft) {
        px = 0;
    } else if ((alignment & Qt::AlignHCenter) == Qt::AlignHCenter) {
        px = x / 2.0 - width / 2;
    } else if ((alignment & Qt::AlignRight) == Qt::AlignRight) {
        px = x * 1.0 - width;
    }

    if ((alignment & Qt::AlignTop) == Qt::AlignTop) {
        py = 0;
    } else if ((alignment & Qt::AlignVCenter) == Qt::AlignVCenter) {
        py = y / 2.0 - height / 2;
    } else if ((alignment & Qt::AlignBottom) == Qt::AlignBottom) {
        py = y * 1.0 - height;
    }

    return QPointF(px, py);
}

bool stringContainsKana(const std::string& s) {
    for (int i = 0; i < s.size(); ++i) {
        unsigned char c = s[i], c2 = 0;
        if (c == 0xE3u) {
            c2 = s[i + 1]; // Second byte
            if (c2 >= 0x81u && c2 <= 0x83u)
                return true;
        }
    }
    return false;
}

bool stringContainsCJKCharacter(const std::string& s) {
    for (unsigned char c : s) {
        if (c >= 0xE4u && c <= 0xEAu)
            return true;
    }
    return false;
}
