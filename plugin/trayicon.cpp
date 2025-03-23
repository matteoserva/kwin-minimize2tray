#include "trayicon.h"
#include <QAction>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDebug>
#include <QMenu>
#include <QVariantMap>

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

    QDBusConnection::sessionBus().connect(QString(),                           // service
                                          QString(),                           // path
                                          "com.canonical.Unity.LauncherEntry", // interface
                                          "Update",                            // member
                                          this, SLOT(launcherAPIUpdate(QString, QMap<QString, QVariant>)));
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

void TrayIcon::launcherAPIUpdate(const QString &uri, const QMap<QString, QVariant> &properties) {
    // ignore signals from other applications
    if (m_launcherUrl != uri) {
        return;
    }

    auto propertiesEnd = properties.constEnd();
    auto foundCount = properties.constFind(QStringLiteral("count"));
    if (foundCount != propertiesEnd) {
        qint64 newCount = foundCount->toLongLong();
        // 2 billion unread emails ought to be enough for anybody
        if (newCount < std::numeric_limits<int>::max()) {
            int saneCount = static_cast<int>(newCount);
            if (saneCount != m_count) {
                m_count = saneCount;
                emit countChanged(saneCount);
            }
        }
    }

    auto foundCountVisible = properties.constFind(QStringLiteral("count-visible"));
    if (foundCountVisible != propertiesEnd) {
        bool newCountVisible = foundCountVisible->toBool();
        if (newCountVisible != m_countVisible) {
            m_countVisible = newCountVisible;
        }
    }

    // the API gives us progress as 0..1 double but we'll use percent to avoid unnecessary
    // changes when it just changed a fraction of a percent, hence not using our fancy updateLauncherProperty method
    auto foundProgress = properties.constFind(QStringLiteral("progress"));
    if (foundProgress != propertiesEnd) {
        int newProgress = qRound(foundProgress->toDouble() * 100);
        if (newProgress != m_progress) {
            m_progress = newProgress;
            emit progressChanged(newProgress);
        }
    }

    auto foundProgressVisible = properties.constFind(QStringLiteral("progress-visible"));
    if (foundProgressVisible != propertiesEnd) {
        bool newProgressVisible = foundProgressVisible->toBool();
        if (newProgressVisible != m_progressVisible) {
            m_progressVisible = newProgressVisible;
            emit progressVisibleChanged(newProgressVisible);
        }
    }

    auto foundUrgent = properties.constFind(QStringLiteral("urgent"));
    if (foundUrgent != propertiesEnd) {
        bool newUrgent = foundUrgent->toBool();
        if (newUrgent != m_urgent) {
            m_urgent = newUrgent;
            emit urgentChanged(newUrgent);
        }
    }

    qDebug() << uri << properties;
    qDebug() << "count: " << m_countVisible << " " << m_count;
    qDebug() << "progress: " << m_progressVisible << " " << m_progress;
    qDebug() << "urgent: " << m_urgent;
}

QString TrayIcon::launcherUrl() const { return m_launcherUrl; }

void TrayIcon::setLauncherUrl(const QString &launcherUrl) {
    if (launcherUrl != m_launcherUrl) {
        m_launcherUrl = launcherUrl;
        Q_EMIT launcherUrlChanged(launcherUrl);
    }
}

int TrayIcon::count() const { return m_count; }

void TrayIcon::setCount(int count) {
    if (m_count != count) {
        m_count = count;
        Q_EMIT countChanged(count);
    }
}

bool TrayIcon::countVisible() const { return m_countVisible; }

void TrayIcon::setCountVisible(bool countVisible) {
    if (m_countVisible != countVisible) {
        m_countVisible = countVisible;
        Q_EMIT countVisibleChanged(countVisible);
    }
}

int TrayIcon::progress() const { return m_progress; }

void TrayIcon::setProgress(int progress) {
    int boundedProgress = std::clamp(progress, 0, 100);

    if (progress != boundedProgress) {
        qDebug() << qUtf8Printable(m_launcherUrl) << ": Progress value " << progress << " is out of bounds!";
    }

    if (m_progress != boundedProgress) {
        m_progress = boundedProgress;
        Q_EMIT progressChanged(boundedProgress);
    }
}

bool TrayIcon::progressVisible() const { return m_progressVisible; }

void TrayIcon::setProgressVisible(bool progressVisible) {
    if (m_progressVisible != progressVisible) {
        m_progressVisible = progressVisible;
        Q_EMIT progressVisibleChanged(progressVisible);
    }
}

bool TrayIcon::urgent() const { return m_urgent; }

void TrayIcon::setUrgent(bool urgent) {
    if (m_urgent != urgent) {
        m_urgent = urgent;
        Q_EMIT urgentChanged(urgent);
    }
}
