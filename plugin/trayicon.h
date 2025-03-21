#ifndef TRAYICON_H
#define TRAYICON_H

#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QUuid>

class TrayIcon : public QObject {
    Q_OBJECT
    Q_PROPERTY(QIcon icon READ icon WRITE setIcon NOTIFY iconChanged);
    Q_PROPERTY(QUuid windowId READ windowId WRITE setWindowId NOTIFY windowIdChanged);
    Q_PROPERTY(QString toolTipText READ toolTipText WRITE setToolTipText NOTIFY toolTipTextChanged)

  public:
    explicit TrayIcon(QObject *parent = nullptr);

    ~TrayIcon() { delete trayIcon; }

  signals:
    void iconChanged();
    void windowIdChanged();
    void toolTipTextChanged();
    void requestRemoveTrayIcon();
    void requestShowHide(const QUuid windowId);
    void requestClose(const QUuid windowId);
    void requestUnpin(const QUuid windowId);

  private:
    QSystemTrayIcon *trayIcon;
    QIcon m_icon;
    QIcon icon() const { return m_icon; }
    void setIcon(QIcon icon);
    QUuid m_windowId;
    QUuid windowId() const { return m_windowId; }
    void setWindowId(QUuid windowId);
    QString m_toolTipText;
    QString toolTipText() const { return m_toolTipText; }
    void setToolTipText(QString toolTipText);
};

#endif
