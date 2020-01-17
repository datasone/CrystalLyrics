//
// Created by datasone on 3/1/2020.
//

#include "window.h"
#include <QApplication>

Window::Window(QWidget *parent) : QWidget(parent) {
    setFixedSize(100, 50);

    m_button = new QPushButton("Hello World!", this);
    m_button->setGeometry(10, 10, 80, 30);
    m_button->setCheckable(true);

    m_counter = 0;

    connect(m_button, &QPushButton::clicked, this, &Window::specialButtonClicked);
    connect(this, &Window::counterReached, QApplication::instance(), &QApplication::quit);
}

void Window::specialButtonClicked(bool checked) {
    if (checked) {
        m_button->setText("Checked");
    } else {
        m_button->setText("Hello World");
    }

    ++m_counter;

    if (m_counter == 10) {
        emit counterReached();
    }
}