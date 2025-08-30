import QtQuick
import org.kde.kwin
import com.github.luisbocanegra.trayicon 1.0

Item {
    id: root
    property bool debugEnabled: KWin.readConfig("debugEnabled", false)
    property var hideByDefaultClass: commaSeparate(KWin.readConfig("hideByDefaultClass", ""))
    property bool countUseDot: KWin.readConfig("countUseDot", false)
    property bool hideOnMinimize: KWin.readConfig("hideOnMinimize", false)
    property bool restoreToCurrentDesktop: KWin.readConfig("restoreToCurrentDesktop", true)
    property bool startMinimized: KWin.readConfig("startMinimized", true)
    property var trayIcons: new Object()
    readonly property Component trayIconComponent: TrayIcon {}

    function commaSeparate(string) {
        if (!string || typeof string !== "string") {
            return []
        }
        return string
            .split(",")
            .map((part) => part.trim())
            .filter((part) => part != "")
    }

    function isValidWindow(window) {
        return window.minimizable && window.normalWindow &&
            !window.dialog && !window.modal && !window.specialWindow &&
            !window.appletPopup && !window.splash && !window.utility && !window.transient
    }

    function dumpProps(obj) {
        for (var k of Object.keys(obj)) {
            const val = obj[k];
            if (k === 'metaData')
                continue;
            print(k + "=" + val + "\n");
        }
    }

    function setup(window) {
        if (!isValidWindow(window)) return
        window.captionChanged.connect(() => {
            updateToolTip(window)
        })
        window.desktopsChanged.connect(() => {
            updateToolTip(window)
        })
        window.outputChanged.connect(() => {
            updateToolTip(window)
        })
        window.demandsAttentionChanged.connect(() => {
            updateDemandsAttention(window)
        })
        window.minimizedChanged.connect(() => {
            onMinimizeChanged(window)
        })
        window.iconChanged.connect(() => {
            updateWindowIcon(window)
        })
    }

    function setupTrayicon(window) {
        if (window.skipTaskbar) {
            addTrayIcon(window)
        } else {
            removeTrayIcon(window)
        }
    }

    function formattedWindowInfo(w) {
        let desktops
        if (w.onAllDesktops) {
            desktops = "All Desktops"
        } else {
            desktops = w.desktops.map(desktop => desktop.name).join(", ")
        }
        return `${w.caption||w.resourceName||w.resourceClass}\n${desktops} @ ${w.output.name}`
    }

    function addTrayIcon(window) {
        const windowId = window.internalId
        if (windowId in trayIcons) {
            return;
        }
        let launcherUrl = window.desktopFileName
        launcherUrl = "application://" + launcherUrl
        launcherUrl = launcherUrl + ".desktop"

        const trayItem = trayIconComponent.createObject(root, {
            "icon": window.icon,
            "windowId": windowId,
            "toolTipText": formattedWindowInfo(window),
            "launcherUrl": launcherUrl,
            "xdgName": window.desktopFileName,
            "countUseDot": countUseDot
        });
        trayItem.requestShowHide.connect(toggleShowHide)
        trayItem.requestClose.connect(closeWindow)
        trayItem.requestUnpin.connect(unpinIcon)
        trayIcons[windowId] = trayItem;
    }

    function removeTrayIcon(window) {
        const windowId = window.internalId
        if (!(windowId in trayIcons)) {
            return
        }
        trayIcons[windowId].destroy();
        delete trayIcons[windowId];
    }

    function setMinimize(minimize, window) {
        window.minimized = minimize;
    }

    function setSkip(skip, window) {
        window.skipPager = skip;
        window.skipTaskbar = skip;
        window.skipSwitcher = skip;
    }

    function toggleMinimize(windowId) {
        const window = getWindow(windowId)
        if (!window) return
        window.minimized = !window.minimized
    }

    function toggleShowHide(windowId) {
        const window = getWindow(windowId)
        if (!window) return

        window.minimized = !window.minimized
        if (window.minimized) {
            setSkip(true, window)
        } else {
            setSkip(false, window)
        }
        if (!window.minimized)
        {
            setActiveWindow(window)
        }
    }

    function setActiveWindow(window) {
        if(restoreToCurrentDesktop && window.desktops.length == 1 && !(Workspace.currentDesktop in window.desktops))
        {
            window.desktops[0] = Workspace.currentDesktop
        }
        Workspace.activeWindow = window
    }

    function onMinimizeChanged(window) {
        if (!(window.internalId in trayIcons)) {
            return;
        }
        if (window.minimized && hideOnMinimize) {
            setSkip(true, window);
        }
    }

    function getWindow(windowId) {
        for (let window of Workspace.windows) {
            if (window.internalId === windowId) {
                return window
            }
        }
        return null
    }

    function closeWindow(windowId) {
        const window = getWindow(windowId)
        window.closeWindow()
    }

    function unpinIcon(windowId) {
        const window = getWindow(windowId)
        if (!window) return
        setMinimize(false, window)
        setSkip(false, window)
        removeTrayIcon(window)
        setActiveWindow(window)
    }

    function updateToolTip(window) {
        if (window.internalId in trayIcons) {
            trayIcons[window.internalId].toolTipText = formattedWindowInfo(window)
        }
    }

    function updateWindowIcon(window) {
        if (window.internalId in trayIcons) {
            trayIcons[window.internalId].icon = window.icon
        }
    }

    function updateDemandsAttention(window) {
        if (window.internalId in trayIcons) {
            trayIcons[window.internalId].demandsAttention = window.demandsAttention
        }
    }

    ShortcutHandler {
        name: "Minimize to tray"
        text: "Minimize window to tray (sets skip Taskbar, Switcher & Pager)"
        sequence: "Meta+Alt+PgDown"
        onActivated: {
            let window = Workspace.activeWindow
            while (window.transient) {
                window = window.transientFor
            }
            if (!isValidWindow(window)) return

            root.addTrayIcon(window)
            toggleShowHide(window.internalId)
        }
    }

    function setupAutoHide(window) {
        if (!isValidWindow(window)) return
        if (hideByDefaultClass.includes(window.resourceName) || hideByDefaultClass.includes(window.resourceClass)) {
            root.addTrayIcon(window)
            if(startMinimized)
            {
                toggleShowHide(window.internalId)
            }
        }
    }

    Component.onCompleted: {
        Workspace.windowAdded.connect(window => setup(window));
        Workspace.windowAdded.connect(window => {
            setupAutoHide(window)
        });
        Workspace.windowRemoved.connect(window => removeTrayIcon(window));
        Workspace.windows.forEach(window => setup(window));
    }
}
