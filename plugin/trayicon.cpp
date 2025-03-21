#include "trayicon.h"
#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QMenu>
#include <QObject>
#include <QSystemTrayIcon>

TrayIcon::TrayIcon(QObject *parent) : QObject(parent) {
    trayIcon = new QSystemTrayIcon(this);

    QMenu *menu = new QMenu();

    QAction *showAction = new QAction("Show/Hide", menu);
    connect(showAction, &QAction::triggered, this, [this]() { emit requestShowHide(m_windowId); });
    menu->addAction(showAction);

    QAction *unpinAction = new QAction("Unpin", menu);
    connect(unpinAction, &QAction::triggered, this, [this]() { emit requestUnpin(m_windowId); });
    menu->addAction(unpinAction);

    QAction *quitAction = new QAction("Quit", menu);
    connect(quitAction, &QAction::triggered, this, [this]() { emit requestClose(m_windowId); });
    menu->addAction(quitAction);

    connect(trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            emit requestShowHide(m_windowId);
        }
    });
    connect(this, &TrayIcon::toolTipTextChanged, this, [this]() { trayIcon->setToolTip(m_toolTipText); });
    connect(this, &TrayIcon::iconChanged, this, [this]() { trayIcon->setIcon(m_icon); });
    trayIcon->setContextMenu(menu);
    trayIcon->show();
}

void TrayIcon::setIcon(const QIcon newIcon) {
    if (m_icon.cacheKey() != newIcon.cacheKey()) {
        m_icon = newIcon;
        emit iconChanged();
    }
}

void TrayIcon::setWindowId(const QUuid window) { m_windowId = window; }
void TrayIcon::setToolTipText(const QString toolTipText) {
    if (m_toolTipText != toolTipText) {
        m_toolTipText = toolTipText;
        emit toolTipTextChanged();
    }
}
