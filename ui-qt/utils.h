//
// Created by datasone.
// This file is part of CrystalLyrics.
//

#ifndef CRYSTALLYRICS_UTILS_H
#define CRYSTALLYRICS_UTILS_H

#include <string>

#include <QtWidgets/QScrollArea>
#include <QtWidgets/QScrollBar>

void centerWidgetInScrollArea(QScrollArea* scrollArea, QWidget* displayArea, QWidget* childWidget);

QPoint getTextStartingPos(int x, int y, int width, int height, Qt::Alignment alignment);

bool stringContainsKana(const std::string& s);

bool stringContainsCJKCharacter(const std::string& s);

#endif //CRYSTALLYRICS_UTILS_H
