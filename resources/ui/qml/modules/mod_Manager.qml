import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Window

Rectangle {
    id: root
    color: "#1a1a1a"

    property int modCount: ModManager ? ModManager.modCount : 0
    property int updateCount: ModManager ? ModManager.updatesAvailable : 0
    property string currentProfile: ModManager ? ModManager.currentProfile : "Default"
    property var profileNames: ModManager ? ModManager.profileNames : ["Default"]
    property string filterText: ""
    property string filterCategory: "All"
    property var modDetails: ({})

    function formatSize(bytes) {
        if (bytes < 1024) return bytes + " B"
        if (bytes < 1048576) return (bytes / 1024).toFixed(0) + " KB"
        return (bytes / 1048576).toFixed(0) + " MB"
    }

    function statusIcon(modIndex) {
        var d = ModManager ? ModManager.getModDetails(modIndex) : null
        if (!d) return "●"
        if (d.hasActiveConflicts) return "✕"
        if (!d.dependenciesSatisfied) return "◉"
        if (d.hasUpdate) return "↑"
        return "●"
    }

    function statusColor(modIndex) {
        var d = ModManager ? ModManager.getModDetails(modIndex) : null
        if (!d) return "#666"
        if (d.hasActiveConflicts) return "#E10600"
        if (!d.dependenciesSatisfied) return "#ff8800"
        if (d.hasUpdate) return "#4ec9b0"
        return "#4ec9b0"
    }

    function statusTooltip(modIndex) {
        var d = ModManager ? ModManager.getModDetails(modIndex) : null
        if (!d) return ""
        var tips = []
        if (d.hasActiveConflicts) tips.push("Has active conflicts")
        if (!d.dependenciesSatisfied) tips.push(d.satisfiedDepCount + "/" + d.dependencyCount + " dependencies met")
        if (d.hasUpdate) tips.push("Update available: " + d.newVersion)
        if (tips.length === 0 && d.isBuiltIn) tips.push("Built-in mod")
        return tips.length > 0 ? tips.join("\n") : "OK"
    }

    FileDialog {
        id: modInstallDialog
        title: "Install Mod"
        nameFilters: ["ZIP files (*.zip)", "All files (*)"]
        onAccepted: {
            if (ModManager) {
                ModManager.installModWithDependencies(selectedFile.toString().replace("file:///", ""))
            }
        }
    }

    FileDialog {
        id: modInstallSimpleDialog
        title: "Install Mod (no deps)"
        nameFilters: ["ZIP files (*.zip)", "All files (*)"]
        onAccepted: {
            if (ModManager) {
                ModManager.installMod(selectedFile.toString().replace("file:///", ""))
            }
        }
    }

    // Profile dialog
    Window {
        id: profileDialog
        width: 400
        height: 300
        flags: Qt.Dialog
        title: "Manage Profiles"
        color: "#1e1e1e"
        modality: Qt.ApplicationModal

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 8

            Text { text: "Profiles"; color: "#E10600"; font.pixelSize: 14; font.bold: true }

            ListView {
                id: profileList
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: profileNames
                clip: true

                delegate: Rectangle {
                    width: parent.width
                    height: 32
                    color: ListView.isCurrentItem ? "#3a3a3a" : "#252525"

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 4
                        spacing: 8

                        Text {
                            text: modelData
                            color: "#fff"
                            font.pixelSize: 12
                            Layout.fillWidth: true
                        }
                        Text {
                            text: modelData === ModManager.currentProfile ? "active" : ""
                            color: "#4ec9b0"
                            font.pixelSize: 10
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            profileList.currentIndex = index
                        }
                    }
                }
            }

            RowLayout {
                spacing: 4
                Button {
                    text: "Switch"
                    onClicked: {
                        if (profileList.currentItem) {
                            ModManager.switchProfile(profileList.currentItem.text)
                        }
                    }
                }
                Button {
                    text: "New"
                    onClicked: newProfileDialog.open()
                }
                Button {
                    text: "Delete"
                    onClicked: {
                        if (profileList.currentItem && profileList.currentItem.text !== "Default") {
                            ModManager.deleteProfile(profileList.currentItem.text)
                        }
                    }
                }
            }
        }
    }

    Window {
        id: newProfileDialog
        width: 300
        height: 150
        flags: Qt.Dialog
        title: "New Profile"
        color: "#1e1e1e"
        modality: Qt.ApplicationModal

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 8

            Text { text: "Profile Name"; color: "#aaa"; font.pixelSize: 11 }
            TextField {
                id: newProfileName
                Layout.fillWidth: true
                color: "#fff"
                background: Rectangle { color: "#333" }
                placeholderText: "Enter name..."
                onAccepted: {
                    if (text.length > 0) {
                        ModManager.createProfile(text)
                        newProfileDialog.close()
                    }
                }
            }
            Button {
                text: "Create"
                onClicked: {
                    if (newProfileName.text.length > 0) {
                        ModManager.createProfile(newProfileName.text)
                        newProfileDialog.close()
                    }
                }
            }
        }
    }

    // Dependency tree dialog
    Window {
        id: depTreeDialog
        width: 500
        height: 400
        flags: Qt.Dialog
        title: "Dependency Tree"
        color: "#1e1e1e"
        modality: Qt.ApplicationModal

        property string modName: ""

        ListModel {
            id: depTreeModel
        }

        function refreshDepTree() {
            depTreeModel.clear()
            var data = ModManager ? ModManager.getDependencyTree(depTreeDialog.modName) : []
            for (var i = 0; i < data.length; i++) {
                depTreeModel.append(data[i])
            }
        }

        onVisibleChanged: {
            if (visible) refreshDepTree()
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 8

            Text {
                text: depTreeDialog.modName + " — Dependencies"
                color: "#E10600"
                font.pixelSize: 14
                font.bold: true
            }

            ListView {
                id: depTree
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: depTreeModel

                delegate: Item {
                    implicitHeight: 28
                    implicitWidth: parent.width

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 4
                        spacing: 4
                        Text {
                            text: model.depth !== undefined ? "  ".repeat(model.depth) + "• " : "  "
                            color: "#666"
                        }
                        Text {
                            text: model.name || ""
                            color: model.installed ? "#4ec9b0" : "#E10600"
                            font.pixelSize: 11
                        }
                        Text {
                            text: model.installed ? "✓" : "✗ missing"
                            color: model.installed ? "#4ec9b0" : "#E10600"
                            font.pixelSize: 10
                        }
                    }
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header bar
        Rectangle {
            height: 52
            color: "#252525"
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 4

                RowLayout {
                    spacing: 8
                    Text { text: "Mod Manager"; color: "#E10600"; font.pixelSize: 14; font.bold: true }
                    Text { text: modCount + " mods"; color: "#888"; font.pixelSize: 11 }
                    Rectangle {
                        visible: updateCount > 0
                        color: "#4ec9b0"
                        radius: 8
                        width: updateBadgeText.width + 12
                        height: 18
                        Text {
                            id: updateBadgeText
                            text: updateCount + " updates"
                            color: "#121212"
                            font.pixelSize: 10
                            font.bold: true
                            anchors.centerIn: parent
                        }
                    }
                    Item { Layout.fillWidth: true }

                    Button {
                        text: "Profile: " + root.currentProfile
                        flat: true
                        font.pixelSize: 11
                        color: "#aaa"
                        onClicked: profileDialog.visible = !profileDialog.visible
                    }
                    Button {
                        text: "Check Updates"
                        flat: true
                        font.pixelSize: 11
                        color: updateCount > 0 ? "#4ec9b0" : "#aaa"
                        onClicked: { if (ModManager) ModManager.checkForUpdates() }
                    }
                }

                RowLayout {
                    spacing: 6
                    Button {
                        text: "Install Mod"
                        font.pixelSize: 11
                        color: "#aaa"
                        flat: true
                        onClicked: modInstallDialog.open()
                    }
                    Button {
                        text: "Scan Mods"
                        flat: true
                        font.pixelSize: 11
                        color: "#aaa"
                        onClicked: { if (ModManager) ModManager.refreshMods() }
                    }
                    Item { Layout.fillWidth: true }
                    TextField {
                        id: searchField
                        placeholderText: "Filter mods..."
                        color: "#fff"
                        font.pixelSize: 11
                        background: Rectangle {
                            color: "#333"
                            radius: 3
                            border.color: "#555"
                        }
                        Layout.preferredWidth: 160
                        Layout.preferredHeight: 22
                        onTextChanged: root.filterText = text.toLowerCase()
                    }
                }
            }
        }

        // Mod table
        TableView {
            id: modTable
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            sortIndicatorVisible: true

            model: sortModel

            property var columns: [
                { role: "enabled", title: "", width: 36 },
                { role: "status", title: "", width: 24 },
                { role: "name", title: "Name", width: 180 },
                { role: "version", title: "Version", width: 80 },
                { role: "author", title: "Author", width: 100 },
                { role: "size", title: "Size", width: 70 },
                { role: "category", title: "Category", width: 80 },
                { role: "date", title: "Installed", width: 80 }
            ]

            columnWidthProvider: function(column) {
                return modTable.columns[column] ? modTable.columns[column].width : 80
            }

            rowHeightProvider: function(row) { return 30 }

            delegate: Rectangle {
                color: row % 2 === 0 ? "#1e1e1e" : "#222222"

                property var modItem: model

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 2
                    spacing: 2

                    // Checkbox (column 0)
                    CheckBox {
                        visible: column === 0
                        checked: modItem.enabled === true || modItem.enabled === "true"
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 36
                        onCheckedChanged: {
                            if (ModManager) {
                                if (checked) ModManager.enableMod(modItem.name)
                                else ModManager.disableMod(modItem.name)
                            }
                        }
                    }

                    // Status icon (column 1)
                    Item {
                        visible: column === 1
                        Layout.preferredWidth: 24
                        Layout.fillHeight: true
                        Text {
                            anchors.centerIn: parent
                            text: root.statusIcon(model.index)
                            color: root.statusColor(model.index)
                            font.pixelSize: 10
                            ToolTip {
                                text: root.statusTooltip(model.index)
                                visible: parent.containsMouse
                                delay: 300
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.NoButton
                        }
                    }

                    // Name (column 2)
                    Text {
                        visible: column === 2
                        text: modItem.name || ""
                        color: "#fff"
                        font.pixelSize: 11
                        font.bold: modItem.isBuiltIn === true
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                        Layout.leftMargin: 4
                    }

                    // Version (column 3)
                    Text {
                        visible: column === 3
                        text: modItem.version || ""
                        color: "#aaa"
                        font.pixelSize: 11
                        Layout.preferredWidth: 80
                    }

                    // Author (column 4)
                    Text {
                        visible: column === 4
                        text: modItem.author || ""
                        color: "#888"
                        font.pixelSize: 11
                        elide: Text.ElideRight
                        Layout.preferredWidth: 100
                    }

                    // Size (column 5)
                    Text {
                        visible: column === 5
                        text: modItem.size || ""
                        color: "#666"
                        font.pixelSize: 11
                        Layout.preferredWidth: 70
                    }

                    // Category (column 6)
                    Text {
                        visible: column === 6
                        text: modItem.category || ""
                        color: "#888"
                        font.pixelSize: 11
                        Layout.preferredWidth: 80
                    }

                    // Date (column 7)
                    Text {
                        visible: column === 7
                        text: modItem.date || ""
                        color: "#666"
                        font.pixelSize: 11
                        Layout.preferredWidth: 80
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.RightButton
                    onClicked: {
                        modTable.currentRow = row
                        contextMenu.modIndex = model.index
                        contextMenu.modName = modItem.name
                        contextMenu.popup()
                    }
                }
            }
        }

        // Status bar
        Rectangle {
            height: 22
            color: "#007acc"
            Layout.fillWidth: true
            visible: false
        }
    }

    // Context menu
    Menu {
        id: contextMenu
        property int modIndex: -1
        property string modName: ""

        MenuItem {
            text: "Show Dependencies"
            onTriggered: {
                if (ModManager) {
                    depTreeDialog.modName = contextMenu.modName
                    depTreeDialog.visible = true
                }
            }
        }
        MenuItem {
            text: "Show Details"
            onTriggered: {
                if (ModManager) {
                    var d = ModManager.getModDetails(contextMenu.modIndex)
                    root.modDetails = d
                    detailDialog.visible = true
                }
            }
        }
        MenuSeparator {}
        MenuItem {
            text: "Enable"
            onTriggered: { if (ModManager) ModManager.enableMod(contextMenu.modName) }
        }
        MenuItem {
            text: "Disable"
            onTriggered: { if (ModManager) ModManager.disableMod(contextMenu.modName) }
        }
        MenuSeparator {}
        MenuItem {
            text: "Backup"
            onTriggered: { if (ModManager) ModManager.backupMod(contextMenu.modName) }
        }
        MenuItem {
            text: "Restore"
            onTriggered: { if (ModManager) ModManager.restoreMod(contextMenu.modName) }
        }
        MenuSeparator {}
        MenuItem {
            text: "Uninstall"
            onTriggered: { if (ModManager) ModManager.uninstallMod(contextMenu.modName) }
        }
    }

    // Detail dialog
    Window {
        id: detailDialog
        width: 400
        height: 350
        flags: Qt.Dialog
        title: "Mod Details"
        color: "#1e1e1e"
        modality: Qt.ApplicationModal

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 6

            Text { text: root.modDetails.name || ""; color: "#E10600"; font.pixelSize: 16; font.bold: true }
            Text { text: root.modDetails.description || ""; color: "#aaa"; font.pixelSize: 11; wrapMode: Text.WordWrap }
            Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

            GridLayout {
                columns: 2
                columnSpacing: 12
                rowSpacing: 4

                Text { text: "Version:"; color: "#888"; font.pixelSize: 11 }
                Text { text: root.modDetails.version || ""; color: "#fff"; font.pixelSize: 11 }
                Text { text: "Author:"; color: "#888"; font.pixelSize: 11 }
                Text { text: root.modDetails.author || ""; color: "#fff"; font.pixelSize: 11 }
                Text { text: "Category:"; color: "#888"; font.pixelSize: 11 }
                Text { text: root.modDetails.category || ""; color: "#fff"; font.pixelSize: 11 }
                Text { text: "Size:"; color: "#888"; font.pixelSize: 11 }
                Text { text: root.modDetails.size || ""; color: "#fff"; font.pixelSize: 11 }
                Text { text: "Installed:"; color: "#888"; font.pixelSize: 11 }
                Text { text: root.modDetails.date || ""; color: "#fff"; font.pixelSize: 11 }
                Text { text: "Status:"; color: "#888"; font.pixelSize: 11 }
                Text {
                    text: root.modDetails.hasUpdate ? "Update available: " + (root.modDetails.newVersion || "") :
                          root.modDetails.hasActiveConflicts ? "Has conflicts" :
                          !root.modDetails.dependenciesSatisfied ? (root.modDetails.satisfiedDepCount + "/" + root.modDetails.dependencyCount + " deps") :
                          root.modDetails.isBuiltIn ? "Built-in" : "OK"
                    color: root.modDetails.hasUpdate ? "#4ec9b0" :
                           root.modDetails.hasActiveConflicts ? "#E10600" :
                           !root.modDetails.dependenciesSatisfied ? "#ff8800" : "#4ec9b0"
                    font.pixelSize: 11
                }
            }

            Rectangle { height: 1; color: "#333"; Layout.fillWidth: true }

            Text {
                text: "Dependencies: " + (root.modDetails.dependencyCount || 0)
                color: "#aaa"; font.pixelSize: 11
            }
            Text {
                text: "Conflicts: " + (root.modDetails.conflictCount || 0) +
                      (root.modDetails.hasActiveConflicts ? " (active!)" : "")
                color: root.modDetails.hasActiveConflicts ? "#E10600" : "#aaa"
                font.pixelSize: 11
            }
            Text {
                visible: root.modDetails.reverseDependencies && root.modDetails.reverseDependencies.length > 0
                text: "Depended by: " + (root.modDetails.reverseDependencies ? root.modDetails.reverseDependencies.join(", ") : "")
                color: "#888"; font.pixelSize: 11; wrapMode: Text.WordWrap
            }
        }
    }

    // Sort filter proxy model
    ListModel { id: sortModel }
    onModCountChanged: { refreshModel() }

    function refreshModel() {
        sortModel.clear()
        if (!ModManager) return
        var mods = ModManager.getMods()
        for (var i = 0; i < mods.length; i++) {
            var m = mods[i]
            if (root.filterText !== "") {
                var name = (m.name || "").toLowerCase()
                var author = (m.author || "").toLowerCase()
                if (name.indexOf(root.filterText) < 0 && author.indexOf(root.filterText) < 0)
                    continue
            }
            if (root.filterCategory !== "All" && m.category !== root.filterCategory)
                continue
            sortModel.append({
                index: i,
                enabled: m.enabled,
                name: m.name,
                version: m.version,
                author: m.author,
                size: m.size,
                category: m.category,
                date: m.date,
                description: m.description,
                isBuiltIn: m.isBuiltIn
            })
        }
    }

    Connections {
        target: ModManager
        function onModCountChanged() { root.modCount = ModManager.modCount; root.refreshModel() }
        function onRefreshFinished() { root.refreshModel() }
        function onUpdatesChanged() { root.updateCount = ModManager.updatesAvailable }
        function onProfileChanged() { root.currentProfile = ModManager.currentProfile; root.profileNames = ModManager.profileNames }
    }

    Component.onCompleted: {
        if (ModManager) {
            root.modCount = ModManager.modCount
            root.updateCount = ModManager.updatesAvailable
            root.refreshModel()
            profileNames = ModManager.profileNames
        }
    }
}
