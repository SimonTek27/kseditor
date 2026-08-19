import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import "../../widgets"
import ksEditor.Modeler 1.0

Rectangle {
    id: clothPanel
    anchors.fill: parent
    color: "transparent"

    signal closePanel()

    property var clothListData: Modeler ? Modeler.clothList() : []
    property var objectList: Modeler ? Modeler.getMeshObjects() : []
    property var colliderList: []
    property bool isRunning: Modeler ? Modeler.clothRunning() : false
    property int selectedId: -1

    Connections {
        target: Modeler
        function onClothChanged() {
            clothListData = Modeler ? Modeler.clothList() : []
            isRunning = Modeler ? Modeler.clothRunning() : false
            refreshObjects()
        }
        function onFabricChanged() {
            clothListData = Modeler ? Modeler.clothList() : []
            isRunning = Modeler ? Modeler.clothRunning() : false
            refreshObjects()
        }
        function onSceneChanged() { refreshObjects() }
    }

    function refreshColliders() {
        colliderList = []
        if (!Modeler) return
        var colIds = Modeler.clothCollisionObjects()
        var all = Modeler.getMeshObjects()
        for (var k = 0; k < colIds.length; ++k)
            for (var m = 0; m < all.length; ++m)
                if (all[m].id === colIds[k]) colliderList.push({id: all[m].id, name: all[m].name})
    }

    function addCollider(id) {
        if (!Modeler) return
        var cur = Modeler.clothCollisionObjects()
        for (var i = 0; i < cur.length; ++i) if (cur[i] === id) return
        cur.push(id)
        Modeler.clothSetCollisionObjects(cur)
        refreshColliders()
    }

    function removeCollider(id) {
        if (!Modeler) return
        var cur = Modeler.clothCollisionObjects()
        var out = []
        for (var i = 0; i < cur.length; ++i) if (cur[i] !== id) out.push(cur[i])
        Modeler.clothSetCollisionObjects(out)
        refreshColliders()
    }

    function refreshObjects() {
        var all = Modeler ? Modeler.getMeshObjects() : []
        var have = {}
        for (var i = 0; i < clothListData.length; ++i) have[clothListData[i].objectId] = true
        var filtered = []
        for (var j = 0; j < all.length; ++j)
            if (!have[all[j].id]) filtered.push(all[j])
        objectList = filtered
        refreshColliders()
    }

    function currentFabricIndex() {
        if (!Modeler || selectedId < 0) return 0
        var cur = Modeler.fabricFor(selectedId)
        var names = Modeler.fabricNames()
        for (var i = 0; i < names.length; ++i)
            if (names[i] === cur) return i
        return 0
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "CLOTH (soft body)"
                color: "#E10600"
                font.pixelSize: 13
                font.bold: true
                Layout.fillWidth: true
            }

            AppButton {
                text: "X"
                height: 24
                width: 26
                bgcolor: "#3e3e42"
                color: "#ffffff"
                font.pixelSize: 10
                font.bold: true
                onClicked: closePanel()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        // Add cloth
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            ComboBox {
                id: objectCombo
                Layout.fillWidth: true
                height: 24
                model: objectList
                textRole: "name"
                font.pixelSize: 10
                background: Rectangle { color: parent.hovered ? "#333" : "#252526"; radius: 3 }
                contentItem: Text {
                    text: parent.currentText || "(choose object)"
                    color: "#aaa"; font.pixelSize: 10
                    verticalAlignment: Text.AlignVCenter; leftPadding: 6
                }
                indicator: Text {
                    text: "\u25BC"; color: "#666"; font.pixelSize: 8
                    anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; anchors.rightMargin: 6
                }
            }

            ComboBox {
                id: pinCombo
                width: 110
                height: 24
                model: ["Pin top", "Pin all", "Pin none"]
                currentIndex: 0
                font.pixelSize: 10
                background: Rectangle { color: parent.hovered ? "#333" : "#252526"; radius: 3 }
                contentItem: Text {
                    text: parent.currentText; color: "#aaa"; font.pixelSize: 10
                    verticalAlignment: Text.AlignVCenter; leftPadding: 6
                }
                indicator: Text {
                    text: "\u25BC"; color: "#666"; font.pixelSize: 8
                    anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; anchors.rightMargin: 6
                }
            }

            AppButton {
                text: "Make Cloth"
                height: 24
                bgcolor: "#E10600"; color: "#fff"
                font.pixelSize: 10
                enabled: objectCombo.currentIndex >= 0
                onClicked: {
                    if (!Modeler) return
                    var id = objectList[objectCombo.currentIndex].id
                    Modeler.clothAdd(id, pinCombo.currentIndex)
                    selectedId = id
                    refreshObjects()
                }
            }
        }

        // Cloth list
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            color: "#151515"
            border.color: "#2a2a2a"
            border.width: 1
            radius: 3

            ListView {
                anchors.fill: parent
                clip: true
                spacing: 2
                model: clothListData

                delegate: Rectangle {
                    width: parent.width
                    height: 24
                    color: modelData.objectId === selectedId ? "#2a2222" : "#1c1c1c"
                    border.color: "#2a2a2a"
                    border.width: 1
                    radius: 2

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 4
                        spacing: 6

                        Text {
                            text: modelData.objectName
                            color: "#ddd"; font.pixelSize: 10; font.bold: true
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }

                        Text {
                            text: ["Pin top", "Pin all", "Pin none"][modelData.pinMode] || "?"
                            color: "#888"; font.pixelSize: 9
                        }

                        Text {
                            text: modelData.springs + " springs"
                            color: "#666"; font.pixelSize: 9
                        }

                        AppButton {
                            text: "\u2716"
                            width: 20; height: 20
                            font.pixelSize: 10
                            bgcolor: "transparent"; color: "#ff6666"
                            onClicked: {
                                if (Modeler) Modeler.clothRemove(modelData.objectId)
                            }
                            ToolTip.visible: hovered; ToolTip.text: "Remove cloth"
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            selectedId = modelData.objectId
                            if (Modeler) {
                                Modeler.clothSetStiffness(selectedId, stiffSlider.value)
                                Modeler.clothSetDamping(selectedId, dampSlider.value)
                                Modeler.clothSetWind(selectedId, windSlider.value)
                            }
                        }
                    }
                }
            }

            Text {
                anchors.centerIn: parent
                text: clothListData.length === 0 ? "No cloth objects. Pick an object and press Make Cloth." : ""
                color: "#444"
                font.pixelSize: 10
            }
        }

        // Params for selected cloth
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 6
            rowSpacing: 4
            enabled: selectedId >= 0

            Text { text: "Fabric"; color: "#777"; font.pixelSize: 9 }
            RowLayout {
                Layout.fillWidth: true
                spacing: 4
                ComboBox {
                    id: fabricCombo
                    Layout.fillWidth: true
                    height: 22
                    model: Modeler ? Modeler.clothPresetNames() : []
                    font.pixelSize: 9
                    background: Rectangle { color: parent.hovered ? "#333" : "#252526"; radius: 3 }
                    contentItem: Text {
                        text: parent.currentText || "Pick preset"
                        color: "#ccc"; font.pixelSize: 9
                        verticalAlignment: Text.AlignVCenter; leftPadding: 6
                    }
                    indicator: Text {
                        text: "\u25BC"; color: "#666"; font.pixelSize: 8
                        anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; anchors.rightMargin: 6
                    }
                    onActivated: {
                        if (!Modeler || selectedId < 0) return
                        var ok = Modeler.clothPreset(selectedId, currentText)
                        if (ok) {
                            // Mirror the preset onto the sliders.
                            if (currentText === "Cotton") { stiffSlider.value = 0.55; dampSlider.value = 0.30; windSlider.value = 0.0 }
                            else if (currentText === "Silk") { stiffSlider.value = 0.35; dampSlider.value = 0.30; windSlider.value = 0.05 }
                            else if (currentText === "Denim") { stiffSlider.value = 0.85; dampSlider.value = 0.25; windSlider.value = 0.0 }
                            else if (currentText === "Leather") { stiffSlider.value = 0.95; dampSlider.value = 0.50; windSlider.value = 0.0 }
                            else if (currentText === "Rubber") { stiffSlider.value = 0.90; dampSlider.value = 0.70; windSlider.value = 0.0 }
                            else if (currentText === "Wool") { stiffSlider.value = 0.45; dampSlider.value = 0.40; windSlider.value = 0.03 }
                            else if (currentText === "Satin") { stiffSlider.value = 0.30; dampSlider.value = 0.20; windSlider.value = 0.08 }
                        }
                    }
                }
            }

            Text { text: "Stiffness"; color: "#777"; font.pixelSize: 9 }
            RowLayout {
                Layout.fillWidth: true
                spacing: 4
                Slider {
                    id: stiffSlider
                    Layout.fillWidth: true
                    from: 0.1; to: 1.0; stepSize: 0.05; value: 0.9
                    background: Rectangle {
                        implicitHeight: 3; color: "#333"; radius: 1
                        Rectangle { width: parent.width * (parent.parent.value - parent.parent.from) / (parent.parent.to - parent.parent.from); height: parent.height; color: "#E10600"; radius: 1 }
                    }
                    handle: Rectangle {
                        implicitWidth: 6; implicitHeight: 10; radius: 1; color: "#ff6666"
                        x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width)
                        y: (parent.height - height) / 2
                    }
                    onMoved: { if (Modeler && selectedId >= 0) Modeler.clothSetStiffness(selectedId, value) }
                }
                Text { text: stiffSlider.value.toFixed(2); color: "#888"; font.pixelSize: 9; width: 28 }
            }

            Text { text: "Damping"; color: "#777"; font.pixelSize: 9 }
            RowLayout {
                Layout.fillWidth: true
                spacing: 4
                Slider {
                    id: dampSlider
                    Layout.fillWidth: true
                    from: 0.0; to: 1.0; stepSize: 0.05; value: 0.2
                    background: Rectangle {
                        implicitHeight: 3; color: "#333"; radius: 1
                        Rectangle { width: parent.width * (parent.parent.value - parent.parent.from) / (parent.parent.to - parent.parent.from); height: parent.height; color: "#E10600"; radius: 1 }
                    }
                    handle: Rectangle {
                        implicitWidth: 6; implicitHeight: 10; radius: 1; color: "#ff6666"
                        x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width)
                        y: (parent.height - height) / 2
                    }
                    onMoved: { if (Modeler && selectedId >= 0) Modeler.clothSetDamping(selectedId, value) }
                }
                Text { text: dampSlider.value.toFixed(2); color: "#888"; font.pixelSize: 9; width: 28 }
            }

            Text { text: "Wind"; color: "#777"; font.pixelSize: 9 }
            RowLayout {
                Layout.fillWidth: true
                spacing: 4
                Slider {
                    id: windSlider
                    Layout.fillWidth: true
                    from: 0.0; to: 1.0; stepSize: 0.05; value: 0.0
                    background: Rectangle {
                        implicitHeight: 3; color: "#333"; radius: 1
                        Rectangle { width: parent.width * (parent.parent.value - parent.parent.from) / (parent.parent.to - parent.parent.from); height: parent.height; color: "#E10600"; radius: 1 }
                    }
                    handle: Rectangle {
                        implicitWidth: 6; implicitHeight: 10; radius: 1; color: "#ff6666"
                        x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width)
                        y: (parent.height - height) / 2
                    }
                    onMoved: { if (Modeler && selectedId >= 0) Modeler.clothSetWind(selectedId, value) }
                }
                Text { text: windSlider.value.toFixed(2); color: "#888"; font.pixelSize: 9; width: 28 }
            }

            Text { text: "Collide"; color: "#777"; font.pixelSize: 9 }
            CheckBox {
                checked: selectedId >= 0 && Modeler ? Modeler.clothCollision(selectedId) : true
                onCheckedChanged: { if (Modeler && selectedId >= 0) Modeler.clothSetCollision(selectedId, checked) }
                Text {
                    text: "solid objects"
                    color: "#999"; font.pixelSize: 9
                    anchors.left: parent.right; anchors.leftMargin: 2; anchors.verticalCenter: parent.verticalCenter
                }
            }

            Text { text: "Self collision"; color: "#777"; font.pixelSize: 9 }
            CheckBox {
                checked: selectedId >= 0 && Modeler ? Modeler.clothSelfCollision(selectedId) : true
                onCheckedChanged: { if (Modeler && selectedId >= 0) Modeler.clothSetSelfCollision(selectedId, checked) }
                Text {
                    text: "cloth vs itself"
                    color: "#999"; font.pixelSize: 9
                    anchors.left: parent.right; anchors.leftMargin: 2; anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        // Colliders
        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        Text {
            text: "COLLIDERS (solid objects)"
            color: "#E10600"
            font.pixelSize: 10
            font.bold: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            ComboBox {
                id: colliderCombo
                Layout.fillWidth: true
                height: 24
                model: objectList
                textRole: "name"
                font.pixelSize: 10
                background: Rectangle { color: parent.hovered ? "#333" : "#252526"; radius: 3 }
                contentItem: Text {
                    text: parent.currentText || "(choose solid)"
                    color: "#aaa"; font.pixelSize: 10
                    verticalAlignment: Text.AlignVCenter; leftPadding: 6
                }
                indicator: Text {
                    text: "\u25BC"; color: "#666"; font.pixelSize: 8
                    anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; anchors.rightMargin: 6
                }
            }

            AppButton {
                text: "Add"
                height: 24
                bgcolor: "#3e3e42"; color: "#fff"
                font.pixelSize: 10
                enabled: colliderCombo.currentIndex >= 0
                onClicked: addCollider(objectList[colliderCombo.currentIndex].id)
            }

            AppButton {
                text: "Clear"
                height: 24
                bgcolor: "transparent"; color: "#aaa"
                font.pixelSize: 10
                enabled: colliderList.length > 0
                onClicked: {
                    if (Modeler) Modeler.clothSetCollisionObjects([])
                    refreshColliders()
                }
            }
        }

        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(64, colliderList.length * 20)
            model: colliderList
            spacing: 2
            clip: true

            delegate: Rectangle {
                width: parent.width
                height: 20
                color: "#1c1c1c"
                border.color: "#2a2a2a"; border.width: 1; radius: 2

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 3
                    spacing: 6

                    Text {
                        text: modelData.name
                        color: "#ccc"; font.pixelSize: 9
                        Layout.fillWidth: true; elide: Text.ElideRight
                    }

                    AppButton {
                        text: "\u2716"
                        width: 18; height: 18
                        font.pixelSize: 9
                        bgcolor: "transparent"; color: "#ff6666"
                        onClicked: removeCollider(modelData.id)
                    }
                }
            }
        }

        // Procedural fabric textures (tessuti procedurali 2.5)
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            enabled: selectedId >= 0

            Text {
                text: "TEXTURE:"
                color: "#888"; font.pixelSize: 9
            }

            ComboBox {
                id: fabricTexCombo
                Layout.fillWidth: true
                height: 22
                model: Modeler ? Modeler.fabricNames() : []
                font.pixelSize: 9
                currentIndex: currentFabricIndex()
                background: Rectangle { color: parent.hovered ? "#333" : "#252526"; radius: 3 }
                contentItem: Text {
                    text: parent.currentText || "Fabric"
                    color: "#ccc"; font.pixelSize: 9
                    verticalAlignment: Text.AlignVCenter; leftPadding: 6
                }
                indicator: Text {
                    text: "\u25BC"; color: "#666"; font.pixelSize: 8
                    anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; anchors.rightMargin: 6
                }
                onActivated: { if (Modeler && selectedId >= 0) Modeler.clothApplyFabric(selectedId, currentText, 1.0) }
            }

            AppButton {
                text: "Apply"
                height: 24
                bgcolor: "#E10600"; color: "#121212"
                font.pixelSize: 10; font.bold: true
                enabled: selectedId >= 0
                onClicked: { if (Modeler && selectedId >= 0) Modeler.clothApplyFabric(selectedId, fabricTexCombo.currentText, 1.0) }
            }

            AppButton {
                text: "Clear"
                height: 24
                bgcolor: "transparent"; color: "#aaa"
                font.pixelSize: 10
                enabled: selectedId >= 0
                onClicked: { if (Modeler && selectedId >= 0) Modeler.clothRemoveFabric(selectedId) }
            }
        }

        Text {
            text: "Generates a procedural weave diffuse + normal map (Cotton/Silk/Denim/Leather/Rubber/Wool/Satin/Twill) and applies it to the mesh. Also folds the matching physical preset."
            color: "#666"; font.pixelSize: 8
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Rectangle {
            Layout.fillWidth: true
            height: 2
            color: "#333"
        }

        // Simulation controls
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            AppButton {
                text: isRunning ? "\u23F8 Pause" : "\u25B6 Play"
                height: 24
                bgcolor: isRunning ? "#E10600" : "#3e3e42"
                color: isRunning ? "#121212" : "#fff"
                font.pixelSize: 10
                enabled: clothListData.length > 0
                onClicked: {
                    if (!Modeler) return
                    if (Modeler.clothRunning()) Modeler.clothPause()
                    else Modeler.clothPlay()
                }
            }

            AppButton {
                text: "Reset"
                height: 24
                bgcolor: "#3e3e42"; color: "#fff"
                font.pixelSize: 10
                enabled: clothListData.length > 0
                onClicked: { if (Modeler) Modeler.clothReset() }
                ToolTip.visible: hovered; ToolTip.text: "Restore rest pose"
            }

            AppButton {
                text: "Remove All"
                height: 24
                bgcolor: "transparent"; color: "#aaa"
                font.pixelSize: 10
                enabled: clothListData.length > 0
                onClicked: { if (Modeler) Modeler.clothRemoveAll() }
            }

            Item { width: 4 }

            Text { text: "Gravity Y:"; color: "#777"; font.pixelSize: 9 }
            Slider {
                id: gravSlider
                width: 100
                height: 22
                from: -20; to: 0; stepSize: 0.1
                value: -9.8
                background: Rectangle {
                    implicitHeight: 3
                    color: "#333"; radius: 1
                    Rectangle { width: parent.width * (parent.parent.value - parent.parent.from) / (parent.parent.to - parent.parent.from); height: parent.height; color: "#7777cc"; radius: 1 }
                }
                handle: Rectangle {
                    implicitWidth: 6; implicitHeight: 10; radius: 1
                    color: "#9999ee"
                    x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width)
                    y: (parent.height - height) / 2
                }
                onMoved: { if (Modeler) Modeler.clothSetGravity(0, value, 0) }
            }
            Text {
                text: gravSlider.value.toFixed(1)
                color: "#888"; font.pixelSize: 9; width: 26
            }
        }

        Text {
            text: "Cloth objects: " + clothListData.length + (isRunning ? "  \u25B6 running" : "")
            color: "#888"; font.pixelSize: 9
        }
    }
}
