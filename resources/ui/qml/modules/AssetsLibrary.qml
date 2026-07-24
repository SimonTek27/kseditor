import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../widgets"

Rectangle {
    id: root
    width: 1280
    height: 720
    color: "#121212"

    readonly property color cBg: "#121212"
    readonly property color cSurface: "#1e1e1e"
    readonly property color cBorder: "#333333"
    readonly property color cAccent: "#E10600"
    readonly property color cText: "#ffffff"
    readonly property color cMuted: "#888888"
    readonly property color cGreen: "#00cc66"
    readonly property color cOrange: "#ff6600"

    property string selectedCategory: "All"
    property string selectedAsset: ""
    property string searchQuery: ""
    property string viewMode: "grid"
    property string currentFile: "assets"

    property var assetsModel: []
    property var categoriesModel: ["All"]
    property var selectedAssetData: ({})
    property bool loading: false
    property string sortField: "name"
    property bool sortAscending: true
    property var storageStats: ({})

    function refresh() {
        loading = true
        assetsModel = AssetsLibrary.getAssets(selectedCategory, searchQuery, "")
        categoriesModel = AssetsLibrary.getCategories()
        storageStats = AssetsLibrary.getStorageStats()
        if (selectedAsset !== "") {
            selectedAssetData = AssetsLibrary.getAsset(selectedAsset)
        }
        loading = false
    }

    function sortedAssets() {
        if (!assetsModel || assetsModel.length === 0) return []
        var sorted = assetsModel.slice()
        sorted.sort(function(a, b) {
            var va, vb
            if (sortField === "name") {
                va = (a.displayName || a.name || "").toLowerCase()
                vb = (b.displayName || b.name || "").toLowerCase()
            } else if (sortField === "size") {
                va = Number(a.size) || 0
                vb = Number(b.size) || 0
            } else if (sortField === "type") {
                va = (a.type || "").toLowerCase()
                vb = (b.type || "").toLowerCase()
            } else if (sortField === "modified") {
                va = a.modified || ""
                vb = b.modified || ""
            } else {
                va = (a.name || "").toLowerCase()
                vb = (b.name || "").toLowerCase()
            }
            if (va < vb) return sortAscending ? -1 : 1
            if (va > vb) return sortAscending ? 1 : -1
            return 0
        })
        return sorted
    }

    Connections {
        target: AssetsLibrary
        function onAssetsChanged() { refresh() }
        function onScanComplete() { refresh() }
    }

    Component.onCompleted: { refresh() }

    onSelectedCategoryChanged: { refresh() }
    onSearchQueryChanged: { refresh() }
    onSelectedAssetChanged: {
        if (selectedAsset !== "") {
            selectedAssetData = AssetsLibrary.getAsset(selectedAsset)
        } else {
            selectedAssetData = ({})
        }
    }

    function formatSize(bytes) {
        if (!bytes) return "0 B"
        if (bytes < 1024) return bytes + " B"
        if (bytes < 1048576) return (bytes / 1024).toFixed(1) + " KB"
        if (bytes < 1073741824) return (bytes / 1048576).toFixed(1) + " MB"
        return (bytes / 1073741824).toFixed(2) + " GB"
    }

    readonly property var typeColors: ({
        "Model": "#E10600",
        "Texture": "#a855f7",
        "Audio": "#E10600",
        "Config": "#ff6600"
    })

    FileDialog {
        id: importDialog
        title: "Import Asset"
        nameFilters: ["All supported (*.kn5 *.fbx *.glb *.gltf *.obj *.dds *.png *.tga *.wav *.mp3 *.bnk *.ini *.json)", "All files (*)"]
        onAccepted: {
            var fileName = selectedFile.toString().replace("file:///", "")
            AssetsLibrary.importAsset(fileName, "")
        }
    }

    FileDialog {
        id: exportDialog
        title: "Export Asset"
        fileMode: FileDialog.SaveFile
        onAccepted: {
            var path = selectedFile.toString().replace("file:///", "")
            if (AssetsLibrary) {
                var ok = AssetsLibrary.exportAsset(root.selectedAsset, path)
                if (ok) AssetsLibrary.statusMessage("Exported: " + path)
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // --- Toolbar ---
        Rectangle {
            height: 40
            color: "#1e1e1e"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10

                Text {
                    text: "ASSETS LIBRARY"
                    color: "white"
                    font.pixelSize: 14
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                AppButton {
                    text: "Import"
                    flat: true
                    height: 32
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: importDialog.open()
                }
                AppButton {
                    text: "Export"
                    flat: true
                    height: 32
                    bgcolor: "transparent"
                    color: "#ffffff"
                    onClicked: {
                        if (root.selectedAsset !== "") {
                            exportDialog.open()
                        }
                    }
                }
            }
        }

        // --- Main Content ---
        RowLayout {
            anchors.fill: parent
            spacing: 0

            // --- Left sidebar: categories ---
            Rectangle {
                width: 180
                Layout.fillHeight: true
                color: "#1e1e1e"
                border.color: "#333333"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // Header
                    Rectangle {
                        height: 52
                        color: "#252526"
                        border.color: "#333333"
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: root.assetsModel.length + " ASSETS"
                            color: "#E10600"
                            font.pixelSize: 12
                            font.bold: true
                        }
                    }

                    // Import AppButton
                    AppButton {
                        height: 36
                        text: "+ Import Asset"
                        bgcolor: "#E10600"
                        color: "#121212"
                        anchors.margins: 10
                        onClicked: importDialog.open()
                    }

                    // Category list
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        contentWidth: availableWidth
                        clip: true

                        ListView {
                            model: root.categoriesModel
                            anchors.margins: 10
                            spacing: 4
                            delegate: AppButton {
                                width: ListView.view.width
                                height: 36
                                text: modelData
                                bgcolor: selectedCategory === modelData ? "#E10600" : "transparent"
                                color: selectedCategory === modelData ? "#121212" : "#ffffff"
                                onClicked: root.selectedCategory = modelData
                            }
                        }
                    }

                    // Storage stats
                    Rectangle {
                        height: 90
                        color: "#252526"
                        border.color: "#333333"
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 3

                            Text { text: "Storage"; color: "#888888"; font.pixelSize: 10; font.bold: true }

                            Rectangle {
                                width: parent.width
                                height: 6
                                radius: 3
                                color: "#333333"

                                Rectangle {
                                    width: parent.width * (storageStats.usedPercent || 0) / 100.0
                                    height: 6
                                    radius: 3
                                    color: "#E10600"
                                }
                            }

                            Text {
                                text: {
                                    if (storageStats.totalFormatted)
                                        return (storageStats.formattedSize || "0 B") + " / " + storageStats.totalFormatted
                                    return (storageStats.formattedSize || "0 B")
                                }
                                color: "#ffffff"; font.pixelSize: 10
                            }

                            Text {
                                text: storageStats.assetCount !== undefined
                                    ? storageStats.assetCount + " assets"
                                    : ""
                                color: "#888888"; font.pixelSize: 9
                            }
                        }
                    }
                }
            }

            // --- Main area ---
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // Search bar
                Rectangle {
                    height: 48
                    color: "#1e1e1e"
                    border.color: "#333333"
                    border.width: 1

                    RowLayout {
                        anchors { fill: parent; margins: 10 }
                        spacing: 10

                        Rectangle {
                            width: 260
                            height: 30
                            radius: 4
                            color: "#252526"
                            border.color: "#333333"
                            border.width: 1

                            TextInput {
                                anchors { fill: parent; leftMargin: 10; rightMargin: 10 }
                                text: root.searchQuery
                                color: "#ffffff"
                                font.pixelSize: 12
                                onTextChanged: root.searchQuery = text
                            }
                        }

                        ComboBox {
                            width: 130; height: 30
                            model: ["Name \u2191", "Name \u2193", "Size \u2193", "Size \u2191", "Type", "Modified"]
                            currentIndex: 0
                            onActivated: function(index) {
                                var map = [
                                    {f:"name", a:true}, {f:"name", a:false},
                                    {f:"size", a:false}, {f:"size", a:true},
                                    {f:"type", a:true}, {f:"modified", a:false}
                                ]
                                root.sortField = map[index].f
                                root.sortAscending = map[index].a
                            }
                        }

                        Item { Layout.fillWidth: true }

                        AppButton {
                            height: 30; width: 30; text: "⊞"
                            bgcolor: viewMode === "grid" ? "#E1060033" : "transparent"
                            color: viewMode === "grid" ? "#E10600" : "#ffffff"
                            onClicked: root.viewMode = "grid"
                        }
                        AppButton {
                            height: 30; width: 30; text: "☰"
                            bgcolor: viewMode === "list" ? "#E1060033" : "transparent"
                            color: viewMode === "list" ? "#E10600" : "#666666"
                            onClicked: root.viewMode = "list"
                        }
                    }
                }

                // Asset grid / list
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    Flow {
                        id: assetFlow
                        visible: root.viewMode === "grid"
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        Repeater {
                            model: root.sortedAssets()
                            delegate: Rectangle {
                                width: 150
                                height: 176
                                radius: 8
                                color: "#252526"
                                border.color: selectedAsset === modelData.id ? "#E10600" : "#333333"
                                border.width: selectedAsset === modelData.id ? 2 : 1

                                MouseArea {
                                    id: cardMouseArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: root.selectedAsset = modelData.id
                                    onDoubleClicked: {
                                        root.selectedAsset = modelData.id
                                        AssetsLibrary.openInModeler(modelData.id)
                                    }
                                }

                                // Open-in-modeler overlay on hover (for 3D assets)
                                Rectangle {
                                    anchors.fill: parent
                                    visible: cardMouseArea.containsMouse && (modelData.type === "Model" || modelData.type === "Mesh")
                                    color: "#00000088"
                                    radius: 8

                                    ColumnLayout {
                                        anchors.centerIn: parent
                                        spacing: 4
                                        Text { text: "⬆"; color: "#E10600"; font.pixelSize: 24; Layout.alignment: Qt.AlignHCenter }
                                        Text { text: "Open in 3D"; color: "white"; font.pixelSize: 10; font.bold: true; Layout.alignment: Qt.AlignHCenter }
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            root.selectedAsset = modelData.id
                                            AssetsLibrary.openInModeler(modelData.id)
                                        }
                                    }
                                }

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 6

                                    // Type badge
                                    Rectangle {
                                        height: 16
                                        radius: 8
                                        color: (typeColors[modelData.type] || "#666666") + "33"
                                        border.color: typeColors[modelData.type] || "#666666"
                                        border.width: 1
                                        Text {
                                            anchors.centerIn: parent
                                            text: modelData.type
                                            color: typeColors[modelData.type] || "#666666"
                                            font.pixelSize: 8
                                            font.bold: true
                                            leftPadding: 6; rightPadding: 6
                                        }
                                    }

                                    // Preview image
                                    Rectangle {
                                        width: parent.width
                                        height: 70
                                        radius: 6
                                        color: "#1e1e1e"
                                        border.color: "#333333"
                                        border.width: 1

                                        Image {
                                            anchors.fill: parent
                                            fillMode: Image.PreserveAspectFit
                                            source: modelData.thumbnail && modelData.thumbnail !== ""
                                                ? "file:///" + modelData.thumbnail : ""
                                            visible: status === Image.Ready
                                        }
                                        Text {
                                            anchors.centerIn: parent
                                            text: modelData.thumbnail ? "" : "📦"
                                            font.pixelSize: 28
                                            visible: !(modelData.thumbnail && modelData.thumbnail !== "")
                                        }
                                    }

                                    // Name
                                    Text {
                                        width: parent.width
                                        text: modelData.displayName || modelData.name
                                        color: "#ffffff"
                                        font.pixelSize: 10
                                        font.bold: true
                                        elide: Text.ElideRight
                                    }

                                    // Size + brand
                                    RowLayout {
                                        width: parent.width
                                        spacing: 4
                                        Text {
                                            text: formatSize(modelData.size)
                                            color: "#888888"
                                            font.pixelSize: 9
                                        }
                                        Item { Layout.fillWidth: true }
                                        Text {
                                            text: modelData.brand || ""
                                            color: "#E10600"
                                            font.pixelSize: 8
                                            visible: modelData.brand !== ""
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // List view
                    ListView {
                        visible: root.viewMode === "list"
                        anchors.fill: parent
                        anchors.margins: 10
                        model: root.sortedAssets()
                        spacing: 2

                        delegate: Rectangle {
                            width: parent.width
                            height: 36
                            radius: 4
                            color: selectedAsset === modelData.id ? "#E1060033" : "transparent"

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 10

                                Text { text: modelData.displayName || modelData.name; color: "#ffffff"; font.pixelSize: 11; Layout.fillWidth: true; elide: Text.ElideRight }
                                Text { text: modelData.type; color: typeColors[modelData.type] || "#666666"; font.pixelSize: 10; font.bold: true }
                                Text { text: modelData.brand || ""; color: "#E10600"; font.pixelSize: 10; visible: modelData.brand !== "" }
                                Text { text: formatSize(modelData.size); color: "#888888"; font.pixelSize: 10 }
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: root.selectedAsset = modelData.id
                                onDoubleClicked: { root.selectedAsset = modelData.id; console.log("Opening:", modelData.name) }
                            }
                        }
                    }
                }
            }

            // Right: asset details
            Rectangle {
                width: 220
                Layout.fillHeight: true
                color: "#1e1e1e"
                border.color: "#333333"
                border.width: 1

                ColumnLayout {
                    anchors { fill: parent; margins: 15 }
                    spacing: 15

                    Text {
                        text: "ASSET DETAILS"
                        color: "white"
                        font.pixelSize: 14
                        font.bold: true
                    }

                    // Big preview
                    Rectangle {
                        width: parent.width
                        height: 100
                        radius: 8
                        color: "#252526"
                        border.color: "#333333"
                        border.width: 1

                        Image {
                            anchors.fill: parent
                            fillMode: Image.PreserveAspectFit
                            source: selectedAssetData.thumbnail && selectedAssetData.thumbnail !== ""
                                ? "file:///" + selectedAssetData.thumbnail : ""
                            visible: status === Image.Ready
                        }
                        Text {
                            anchors.centerIn: parent
                            text: selectedAssetData.thumbnail ? "" : (root.selectedAsset !== "" ? "📦" : "")
                            font.pixelSize: 48
                            visible: !(selectedAssetData.thumbnail && selectedAssetData.thumbnail !== "")
                        }
                    }

                    Text {
                        text: selectedAssetData.displayName || selectedAssetData.name || "No asset selected"
                        color: "#ffffff"
                        font.pixelSize: 12
                        font.bold: true
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Text { text: root.selectedAssetData.name ? "Type: " + (root.selectedAssetData.type || "Unknown") : ""; color: "#888888"; font.pixelSize: 11 }
                    Text { text: root.selectedAssetData.name ? "Size: " + formatSize(root.selectedAssetData.size) : ""; color: "#888888"; font.pixelSize: 11 }
                    Text { text: root.selectedAssetData.name ? "Category: " + (root.selectedAssetData.category || "-") : ""; color: "#888888"; font.pixelSize: 11 }
                    Text { text: root.selectedAssetData.brand ? "Brand: " + root.selectedAssetData.brand : ""; color: "#E10600"; font.pixelSize: 11; visible: root.selectedAssetData.brand !== undefined && root.selectedAssetData.brand !== "" }
                    Text { text: root.selectedAssetData.author ? "Author: " + root.selectedAssetData.author : ""; color: "#888888"; font.pixelSize: 11; visible: root.selectedAssetData.author !== undefined && root.selectedAssetData.author !== "" }
                    Text { text: root.selectedAssetData.year ? "Year: " + root.selectedAssetData.year : ""; color: "#888888"; font.pixelSize: 11; visible: root.selectedAssetData.year !== undefined && root.selectedAssetData.year !== "" }
                    Text { text: root.selectedAssetData.name ? "Modified: " + (root.selectedAssetData.modified || "-") : ""; color: "#888888"; font.pixelSize: 11 }
                    Text { text: root.selectedAssetData.name ? "Tags: " + (root.selectedAssetData.tags || "-") : ""; color: "#888888"; font.pixelSize: 11; wrapMode: Text.WordWrap; Layout.fillWidth: true }

                    Item { Layout.fillHeight: true }

                    // Actions
                    AppButton {
                        height: 32
                        text: "Open in 3D Editor"
                        bgcolor: "#E10600"
                        color: "#121212"
                        Layout.fillWidth: true
                        enabled: selectedAsset !== ""
                        font.bold: true
                        onClicked: {
                            if (root.selectedAssetData.name) {
                                AssetsLibrary.openInModeler(selectedAsset)
                            }
                        }
                    }
                    AppButton {
                        height: 32
                        text: "Import to Project"
                        bgcolor: "transparent"
                        color: "#ffffff"
                        Layout.fillWidth: true
                        enabled: selectedAsset !== ""
                        onClicked: {
                            if (root.selectedAssetData.name) {
                                AssetsLibrary.importAsset(root.selectedAssetData.path || "", "")
                            }
                        }
                    }
                    AppButton {
                        height: 32
                        text: "Copy Path"
                        bgcolor: "transparent"
                        color: "#ffffff"
                        Layout.fillWidth: true
                        enabled: selectedAsset !== ""
                        onClicked: {
                            if (root.selectedAssetData.name) {
                                console.log("Path:", root.selectedAssetData.path)
                            }
                        }
                    }
                    AppButton {
                        height: 32
                        text: "Delete"
                        bgcolor: "transparent"
                        color: "#e74c3c"
                        Layout.fillWidth: true
                        enabled: selectedAsset !== ""
                        onClicked: {
                            if (root.selectedAssetData.name) {
                                AssetsLibrary.removeAsset(selectedAsset)
                                root.selectedAsset = ""
                            }
                        }
                    }
                }
            }
        }

        // --- Status Bar ---
        Rectangle {
            height: 24
            color: "#252526"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 4

                Text { text: "Ready"; color: "#E10600"; font.pixelSize: 10 }
                Item { Layout.fillWidth: true }
                Text { text: "ksEditor v1.0 - Assets"; color: "#666"; font.pixelSize: 10 }
            }
        }
    }
}

