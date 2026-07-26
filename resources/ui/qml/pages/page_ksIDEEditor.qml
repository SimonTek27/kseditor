import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Rectangle {
    id: root
    color: "#1e1e1e"

    readonly property color cBg:      "#1e1e1e"
    readonly property color cPanel:   "#252526"
    readonly property color cBar:     "#2d2d2d"
    readonly property color cBorder:  "#3a3a3a"
    readonly property color cAccent:  "#007acc"
    readonly property color cText:    "#d4d4d4"
    readonly property color cMuted:   "#666666"
    readonly property color cLineNum: "#858585"
    readonly property color cKeyword: "#569cd6"
    readonly property color cString:  "#ce9178"
    readonly property color cComment: "#6a9955"
    readonly property color cNumber:  "#b5cea8"
    readonly property color cType:    "#4ec9b0"

    property string currentFilePath: ""
    property var    openFiles: []
    property int    activeTabIndex: -1

    FileDialog {
        id: openFileDialog
        title: "Open File"
        nameFilters: ["All Files (*)", "QML Files (*.qml)", "C++ Files (*.cpp *.h)", "Lua Scripts (*.lua)", "Config Files (*.ini *.cfg)"]
        onAccepted: {
            var path = selectedFile.toString()
            if (Qt.platform.os === "windows") path = path.replace("file:///", "")
            else path = path.replace("file://", "")
            loadFile(path)
        }
    }

    FileDialog {
        id: saveFileDialog
        title: "Save File As"
        fileMode: FileDialog.SaveFile
        nameFilters: ["All Files (*)"]
        onAccepted: {
            var path = selectedFile.toString()
            if (Qt.platform.os === "windows") path = path.replace("file:///", "")
            else path = path.replace("file://", "")
            saveFile(path)
        }
    }

    function loadFile(path) {
        for (var i = 0; i < openFiles.length; i++) {
            if (openFiles[i].path === path) {
                activeTabIndex = i
                return
            }
        }
        var xhr = new XMLHttpRequest()
        xhr.open("GET", "file:///" + path, false)
        xhr.send()
        if (xhr.status === 200) {
            var name = path.split("/").pop().split("\\").pop()
            openFiles.push({ path: path, name: name, content: xhr.responseText, modified: false })
            activeTabIndex = openFiles.length - 1
            updateEditor()
        }
    }

    function saveFile(path) {
        if (activeTabIndex < 0 || activeTabIndex >= openFiles.length) return
        var file = openFiles[activeTabIndex]
        var xhr = new XMLHttpRequest()
        xhr.open("PUT", "file:///" + path, false)
        xhr.send(file.content)
        if (xhr.status === 200 || xhr.status === 0) {
            file.path = path
            file.name = path.split("/").pop().split("\\").pop()
            file.modified = false
            updateEditor()
        }
    }

    function saveCurrentFile() {
        if (activeTabIndex < 0) return
        var file = openFiles[activeTabIndex]
        if (file.path === "") {
            saveFileDialog.open()
        } else {
            saveFile(file.path)
        }
    }

    function updateEditor() {
        if (activeTabIndex >= 0 && activeTabIndex < openFiles.length) {
            codeEditor.text = openFiles[activeTabIndex].content
            currentFilePath = openFiles[activeTabIndex].path
            updateLineNumbers()
        } else {
            codeEditor.text = ""
            currentFilePath = ""
            lineNumbers.text = "1"
        }
        tabRepeater.model = 0
        tabRepeater.model = openFiles.length
    }

    function closeTab(index) {
        if (index < 0 || index >= openFiles.length) return
        openFiles.splice(index, 1)
        if (activeTabIndex >= openFiles.length) activeTabIndex = openFiles.length - 1
        if (activeTabIndex < 0) activeTabIndex = -1
        updateEditor()
    }

    function updateLineNumbers() {
        var lines = codeEditor.text.split("\n").length
        var nums = ""
        for (var i = 1; i <= lines; i++) {
            nums += i + "\n"
        }
        lineNumbers.text = nums
    }

    function getFileName() {
        if (activeTabIndex >= 0 && activeTabIndex < openFiles.length)
            return openFiles[activeTabIndex].name
        return "Untitled"
    }

    function getLineCol() {
        var pos = codeEditor.cursorPosition
        var textBefore = codeEditor.text.substring(0, pos)
        var lines = textBefore.split("\n")
        return "Ln " + lines.length + ", Col " + (lines[lines.length - 1].length + 1)
    }

    // ── Toolbar ──────────────────────────────────────────────────────────
    Rectangle {
        id: toolbar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 36
        color: cBar

        Row {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 8
            spacing: 2

            Repeater {
                model: ["New", "Open", "Save", "SaveAs"]
                delegate: Rectangle {
                    width: btnLabel.implicitWidth + 16
                    height: 28
                    radius: 4
                    color: btnMa.containsMouse ? "#3a3a3a" : "transparent"

                    Text {
                        id: btnLabel
                        anchors.centerIn: parent
                        text: ["New", "Open", "Save", "Save As"][index]
                        color: cText
                        font.pixelSize: 12
                    }

                    MouseArea {
                        id: btnMa
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            if (index === 0) {
                                openFiles.push({ path: "", name: "Untitled", content: "", modified: false })
                                activeTabIndex = openFiles.length - 1
                                updateEditor()
                            } else if (index === 1) {
                                openFileDialog.open()
                            } else if (index === 2) {
                                saveCurrentFile()
                            } else if (index === 3) {
                                saveFileDialog.open()
                            }
                        }
                    }
                }
            }

            Rectangle { width: 1; height: 20; color: cBorder; anchors.verticalCenter: parent.verticalCenter }

            Repeater {
                model: ["Cut", "Copy", "Paste", "Undo", "Redo"]
                delegate: Rectangle {
                    width: editLabel.implicitWidth + 16
                    height: 28
                    radius: 4
                    color: editMa.containsMouse ? "#3a3a3a" : "transparent"

                    Text {
                        id: editLabel
                        anchors.centerIn: parent
                        text: ["Cut", "Copy", "Paste", "Undo", "Redo"][index]
                        color: cText
                        font.pixelSize: 12
                    }

                    MouseArea {
                        id: editMa
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            if (index === 0) codeEditor.cut()
                            else if (index === 1) codeEditor.copy()
                            else if (index === 2) codeEditor.paste()
                            else if (index === 3) codeEditor.undo()
                            else if (index === 4) codeEditor.redo()
                        }
                    }
                }
            }
        }

        Text {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 12
            text: currentFilePath
            color: cMuted
            font.pixelSize: 11
            elide: Text.ElideLeft
            width: 300
            horizontalAlignment: Text.AlignRight
        }
    }

    // ── Tab Bar ──────────────────────────────────────────────────────────
    Rectangle {
        id: tabBar
        anchors.top: toolbar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 32
        color: cPanel

        Row {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 0

            Repeater {
                id: tabRepeater
                model: openFiles.length

                delegate: Rectangle {
                    width: tabContent.implicitWidth + 32
                    height: 32
                    color: index === activeTabIndex ? cBg : "transparent"

                    Rectangle {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: 1
                        color: cBorder
                    }

                    Row {
                        id: tabContent
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        spacing: 6

                        Rectangle {
                            width: 8; height: 8; radius: 4
                            color: openFiles[index].modified ? cAccent : "transparent"
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Text {
                            text: openFiles[index].name
                            color: index === activeTabIndex ? cText : cMuted
                            font.pixelSize: 12
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    Rectangle {
                        width: 18; height: 18; radius: 3
                        color: closeMa.containsMouse ? "#c42b1c" : "transparent"
                        anchors.right: parent.right
                        anchors.rightMargin: 4
                        anchors.verticalCenter: parent.verticalCenter
                        visible: openFiles[index].modified || index === activeTabIndex

                        Text {
                            anchors.centerIn: parent
                            text: "x"
                            color: cText
                            font.pixelSize: 10
                        }

                        MouseArea {
                            id: closeMa
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: closeTab(index)
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            activeTabIndex = index
                            updateEditor()
                        }
                    }
                }
            }
        }
    }

    // ── Editor Area ──────────────────────────────────────────────────────
    Rectangle {
        id: editorArea
        anchors.top: tabBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: statusBar.top
        color: cBg

        // Line numbers
        Rectangle {
            id: lineNumberPanel
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 50
            color: cPanel

            Rectangle {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 1
                color: cBorder
            }

            ScrollView {
                anchors.fill: parent
                anchors.topMargin: 4
                clip: true
                contentWidth: -1

                Text {
                    id: lineNumbers
                    color: cLineNum
                    font.family: "Consolas, Courier New, monospace"
                    font.pixelSize: 14
                    lineHeight: 1.5
                    text: "1"
                    rightPadding: 8
                }
            }
        }

        // Code editor
        ScrollView {
            id: editorScroll
            anchors.left: lineNumberPanel.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            clip: true

            TextArea {
                id: codeEditor
                color: cText
                font.family: "Consolas, Courier New, monospace"
                font.pixelSize: 14
                lineHeight: 1.5
                selectByMouse: true
                tabStopDistance: 32
                background: null
                wrapMode: TextArea.NoWrap
                leftPadding: 8

                onTextChanged: {
                    if (activeTabIndex >= 0 && activeTabIndex < openFiles.length) {
                        var old = openFiles[activeTabIndex].content
                        openFiles[activeTabIndex].content = text
                        openFiles[activeTabIndex].modified = (old !== text)
                    }
                    updateLineNumbers()
                }

                onCursorPositionChanged: {
                    lineColIndicator.text = getLineCol()
                }

                Keys.onPressed: function(event) {
                    if (event.key === Qt.Key_S && (event.modifiers & Qt.ControlModifier)) {
                        saveCurrentFile()
                        event.accepted = true
                    } else if (event.key === Qt.Key_O && (event.modifiers & Qt.ControlModifier)) {
                        openFileDialog.open()
                        event.accepted = true
                    } else if (event.key === Qt.Key_W && (event.modifiers & Qt.ControlModifier)) {
                        closeTab(activeTabIndex)
                        event.accepted = true
                    } else if (event.key === Qt.Key_Tab) {
                        insert(cursorPosition, "    ")
                        event.accepted = true
                    }
                }
            }
        }
    }

    // ── Status Bar ───────────────────────────────────────────────────────
    Rectangle {
        id: statusBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 24
        color: cAccent

        Row {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 10
            spacing: 16

            Text {
                id: lineColIndicator
                text: "Ln 1, Col 1"
                color: "#ffffff"
                font.pixelSize: 11
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: "UTF-8"
                color: "#ffffff"
                font.pixelSize: 11
                opacity: 0.8
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: "Spaces: 4"
                color: "#ffffff"
                font.pixelSize: 11
                opacity: 0.8
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        Text {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 10
            text: "ksIDEEditor"
            color: "#ffffff"
            font.pixelSize: 11
            font.bold: true
        }
    }
}
