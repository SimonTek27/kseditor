import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    color: "#ffffff"

    property string currentFile: ""
    property bool modified: false
    property int currentRow: 0
    property int currentCol: 0
    property string currentCellRef: "A1"
    property string cellValue: ""
    property var sheetNames: ["Sheet1", "Sheet2", "Sheet3"]
    property int currentSheetIndex: 0
    property string zoomLevel: "100%"
    property string sumValue: ""
    property string countValue: ""
    property string averageValue: ""

    signal fileOpened(string path)
    signal fileClosed()
    signal fileSaved(string path)
    signal cellSelected(int row, int col)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ==================== EDITOR HEADER ====================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            color: "#ffffff"
            border.color: "#e0e0e0"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 4

                ToolButton { implicitWidth: 28; implicitHeight: 28; onClicked: saveSpreadsheet(); Image { anchors.centerIn: parent; source: "qrc:/icons/document-save.svg"; width: 16; height: 16 } }
                ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/document-print.svg"; width: 16; height: 16 } }
                ToolButton { implicitWidth: 28; implicitHeight: 28; enabled: false; Image { anchors.centerIn: parent; source: "qrc:/icons/edit-undo.svg"; width: 16; height: 16; opacity: parent.enabled ? 1.0 : 0.3 } }
                ToolButton { implicitWidth: 28; implicitHeight: 28; enabled: false; Image { anchors.centerIn: parent; source: "qrc:/icons/edit-redo.svg"; width: 16; height: 16; opacity: parent.enabled ? 1.0 : 0.3 } }

                ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/document-properties.svg"; width: 16; height: 16 } }

                Label { text: root.currentFile ? root.currentFile.split('/').pop() : "Untitled"; font.pointSize: 11; font.bold: true; color: "#333"; elide: Text.ElideRight; Layout.maximumWidth: 200 }

                Item { Layout.fillWidth: true }

                ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/edit.svg"; width: 16; height: 16 } }
                ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/home.svg"; width: 16; height: 16 } }
            }
        }

        // ==================== TAB BAR ====================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            color: "#f5f5f5"
            border.color: "#e0e0e0"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 4
                spacing: 0

                Repeater {
                    model: ["File", "Home", "Insert", "Draw", "Layout", "Formula", "Data", "Collaboration", "Protection", "View", "Plugins"]

                    Rectangle {
                        Layout.preferredWidth: tabLabel.implicitWidth + 16
                        Layout.fillHeight: true
                        color: tabMouseArea.containsMouse ? "#e0e0e0" : (index === 1 ? "#ffffff" : "transparent")
                        border.color: index === 1 ? "#4472c4" : "transparent"
                        border.width: index === 1 ? 2 : 0

                        Label { id: tabLabel; anchors.centerIn: parent; text: modelData; font.pointSize: 10; color: "#333" }

                        MouseArea { id: tabMouseArea; anchors.fill: parent; hoverEnabled: true; onClicked: { } }
                    }
                }

                Item { Layout.fillWidth: true }
            }
        }

        // ==================== TOP TOOLBAR ====================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            color: "#ffffff"
            border.color: "#e0e0e0"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 4
                spacing: 8

                // Clipboard group
                ColumnLayout {
                    spacing: 2
                    RowLayout {
                        spacing: 4
                        ToolButton { implicitWidth: 32; implicitHeight: 32; Image { anchors.centerIn: parent; source: "qrc:/icons/edit-paste.svg"; width: 20; height: 20 } }
                    }
                    Label { text: "Paste"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                }

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#e0e0e0" }

                // Font group
                RowLayout {
                    spacing: 4
                    ComboBox { Layout.preferredWidth: 120; Layout.preferredHeight: 24; model: ["Arial", "Times New Roman", "Courier New", "Calibri"]; currentIndex: 0; font.pointSize: 9 }
                    ComboBox { Layout.preferredWidth: 40; Layout.preferredHeight: 24; model: ["8", "9", "10", "11", "12", "14", "16", "18", "20", "24", "28", "36"]; currentIndex: 2; font.pointSize: 9 }
                    ToolButton { text: "B"; font.bold: true; font.pointSize: 10; implicitWidth: 28; implicitHeight: 28; checkable: true }
                    ToolButton { text: "I"; font.italic: true; font.pointSize: 10; implicitWidth: 28; implicitHeight: 28; checkable: true }
                    ToolButton { text: "U"; font.underline: true; font.pointSize: 10; implicitWidth: 28; implicitHeight: 28; checkable: true }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/format-font-color.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/format-highlight.svg"; width: 16; height: 16 } }
                }

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#e0e0e0" }

                // Format group
                RowLayout {
                    spacing: 4
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/format-clear.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/format-paint.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/format-border.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/format-fill-color.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/format-shading.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/format-line-spacing.svg"; width: 16; height: 16 } }
                }

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#e0e0e0" }

                // Number format group
                RowLayout {
                    spacing: 4
                    ComboBox { Layout.preferredWidth: 80; Layout.preferredHeight: 24; model: ["General", "Number", "Currency", "Percentage"]; currentIndex: 0; font.pointSize: 9 }
                }

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#e0e0e0" }

                // Alignment group
                RowLayout {
                    spacing: 4
                    ToolButton { implicitWidth: 28; implicitHeight: 28; checkable: true; checked: true; Image { anchors.centerIn: parent; source: "qrc:/icons/format-align-left.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; checkable: true; Image { anchors.centerIn: parent; source: "qrc:/icons/format-align-center.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; checkable: true; Image { anchors.centerIn: parent; source: "qrc:/icons/format-align-right.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/format-list-ordered.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/format-list-unordered.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/format-indent-more.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/format-indent-less.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/format-superscript.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/format-subscript.svg"; width: 16; height: 16 } }
                }

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#e0e0e0" }

                // Insert group
                RowLayout {
                    spacing: 4
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/insert-table.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/insert-image.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/insert-icons.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/insert-smartart.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/insert-chart.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/insert-link.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/insert-textbox.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/insert-header-footer.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/insert-equation.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/insert-symbol.svg"; width: 16; height: 16 } }
                }

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#e0e0e0" }

                // Layout group
                RowLayout {
                    spacing: 4
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/layout-margins.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/layout-orientation-portrait.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/layout-orientation-landscape.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/layout-size.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/layout-columns.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/layout-breaks.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/layout-line-numbers.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/layout-hyphenation.svg"; width: 16; height: 16 } }
                }

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#e0e0e0" }

                // References group
                RowLayout {
                    spacing: 4
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/references-toc.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/references-footnotes.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/references-citations.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/references-captions.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/references-index.svg"; width: 16; height: 16 } }
                }

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#e0e0e0" }

                // Review group
                RowLayout {
                    spacing: 4
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/review-spelling.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/review-thesaurus.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/review-word-count.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/review-track-changes.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/review-accept.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/review-reject.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/review-delete-comment.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/review-previous.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/review-next.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/review-compare.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/review-protect.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/review-find.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/review-replace.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/review-select-all.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/review-multilevel-list.svg"; width: 16; height: 16 } }
                }

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#e0e0e0" }

                // View group
                RowLayout {
                    spacing: 4
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/view-web-layout.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/view-draft-layout.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/view-outline.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/view-zoom.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/view-gridlines.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/view-sheet-views.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/view-freeze-panes.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/view-switch-windows.svg"; width: 16; height: 16 } }
                }

                Item { Layout.fillWidth: true }
            }
        }

        // ==================== FORMULA BAR ====================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            color: "#ffffff"
            border.color: "#e0e0e0"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 4
                anchors.rightMargin: 4
                spacing: 4

                Rectangle {
                    Layout.preferredWidth: 70
                    Layout.fillHeight: true
                    color: "#ffffff"
                    border.color: "#c0c0c0"
                    border.width: 1

                    TextInput {
                        id: cellRefInput
                        anchors.fill: parent
                        anchors.margins: 2
                        font.pointSize: 10
                        clip: true
                        text: root.currentCellRef
                        verticalAlignment: TextInput.AlignVCenter
                        onAccepted: { }
                    }
                }

                ToolButton {
                    implicitWidth: 30
                    implicitHeight: 22
                    Image { anchors.centerIn: parent; source: "qrc:/icons/formula.svg"; width: 16; height: 16 }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#ffffff"
                    border.color: "#c0c0c0"
                    border.width: 1

                    TextInput {
                        id: formulaInput
                        anchors.fill: parent
                        anchors.margins: 2
                        font.pointSize: 10
                        clip: true
                        verticalAlignment: TextInput.AlignVCenter
                        onAccepted: { root.cellValue = text; root.modified = true }
                    }
                }
            }
        }

        // ==================== WORKING AREA ====================
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Left sidebar
            Rectangle {
                Layout.preferredWidth: 36
                Layout.fillHeight: true
                color: "#f5f5f5"
                border.color: "#e0e0e0"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 4
                    spacing: 4

                    ToolButton { implicitWidth: 28; implicitHeight: 28; ToolTip.text: "Search"; ToolTip.visible: hovered; Image { anchors.centerIn: parent; source: "qrc:/icons/edit.svg"; width: 18; height: 18 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; ToolTip.text: "Comments"; ToolTip.visible: hovered; Image { anchors.centerIn: parent; source: "qrc:/icons/collab.svg"; width: 18; height: 18 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; ToolTip.text: "Spellcheck"; ToolTip.visible: hovered; Image { anchors.centerIn: parent; source: "qrc:/icons/checkpoints.svg"; width: 18; height: 18 } }

                    Item { Layout.fillHeight: true }
                }
            }

            // Grid area
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#ffffff"
                border.color: "#e0e0e0"
                border.width: 1

                // Column headers
                Rectangle {
                    id: colHeaders
                    anchors.top: parent.top
                    anchors.left: rowHeaders.right
                    anchors.right: parent.right
                    anchors.leftMargin: 1
                    height: 22
                    color: "#f5f5f5"
                    border.color: "#e0e0e0"
                    border.width: 1

                    Row {
                        anchors.fill: parent
                        Repeater {
                            model: 26
                            Rectangle {
                                width: 80
                                height: colHeaders.height
                                color: "#f5f5f5"
                                border.color: "#e0e0e0"
                                border.width: 1
                                Label { anchors.centerIn: parent; text: String.fromCharCode(65 + index); font.pointSize: 9; color: "#666" }
                            }
                        }
                    }
                }

                // Row headers
                Rectangle {
                    id: rowHeaders
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.topMargin: 22
                    width: 40
                    height: parent.height - 22
                    color: "#f5f5f5"
                    border.color: "#e0e0e0"
                    border.width: 1

                    ListView {
                        anchors.fill: parent
                        anchors.topMargin: 1
                        clip: true
                        model: 100
                        delegate: Rectangle {
                            width: rowHeaders.width
                            height: 22
                            color: "#f5f5f5"
                            border.color: "#e0e0e0"
                            border.width: 1
                            Label { anchors.centerIn: parent; text: index + 1; font.pointSize: 9; color: "#666" }
                        }
                    }
                }

                // Grid content
                Rectangle {
                    anchors.top: colHeaders.bottom
                    anchors.left: rowHeaders.right
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.topMargin: 1
                    anchors.leftMargin: 1
                    clip: true

                    GridView {
                        id: grid
                        anchors.fill: parent
                        cellWidth: 80
                        cellHeight: 22
                        model: 2600

                        delegate: Rectangle {
                            width: grid.cellWidth
                            height: grid.cellHeight
                            color: "#ffffff"
                            border.color: "#e0e0e0"
                            border.width: 1

                            TextInput {
                                anchors.fill: parent
                                anchors.margins: 2
                                font.pointSize: 10
                                clip: true
                                verticalAlignment: TextInput.AlignVCenter
                                onActiveFocusChanged: {
                                    if (activeFocus) {
                                        root.currentRow = Math.floor(index / 26)
                                        root.currentCol = index % 26
                                        root.currentCellRef = getCellRef(root.currentRow, root.currentCol)
                                        root.cellSelected(root.currentRow, root.currentCol)
                                    }
                                }
                                onTextChanged: { root.modified = true }
                            }

                            MouseArea { anchors.fill: parent; acceptedButtons: Qt.RightButton; onClicked: { } }
                        }
                    }
                }
            }
        }

        // ==================== STATUS BAR ====================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            color: "#f5f5f5"
            border.color: "#e0e0e0"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 16

                // Sheet navigation
                RowLayout {
                    spacing: 4
                    ToolButton { implicitWidth: 18; implicitHeight: 18; Image { anchors.centerIn: parent; source: "qrc:/icons/edit-undo.svg"; width: 12; height: 12 } }
                    ToolButton { implicitWidth: 18; implicitHeight: 18; Image { anchors.centerIn: parent; source: "qrc:/icons/edit-undo.svg"; width: 12; height: 12 } }
                    Label { text: "Sheet " + (root.currentSheetIndex + 1) + " of " + root.sheetNames.length; font.pointSize: 9; color: "#666" }
                    ToolButton { implicitWidth: 18; implicitHeight: 18; Image { anchors.centerIn: parent; source: "qrc:/icons/edit-redo.svg"; width: 12; height: 12 } }
                    ToolButton { implicitWidth: 18; implicitHeight: 18; Image { anchors.centerIn: parent; source: "qrc:/icons/edit-redo.svg"; width: 12; height: 12 } }
                    ToolButton { implicitWidth: 18; implicitHeight: 18; ToolTip.text: "Add Sheet"; ToolTip.visible: hovered; Image { anchors.centerIn: parent; source: "qrc:/icons/add.svg"; width: 12; height: 12 } }
                }

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#d0d0d0" }

                // Sheet tabs
                RowLayout {
                    spacing: 0
                    Repeater {
                        model: root.sheetNames
                        Rectangle {
                            Layout.preferredWidth: 80
                            Layout.fillHeight: true
                            color: index === root.currentSheetIndex ? "#ffffff" : "#e8e8e8"
                            border.color: "#d0d0d0"
                            border.width: 1
                            Label { anchors.centerIn: parent; text: modelData; font.pointSize: 9; color: index === root.currentSheetIndex ? "#333" : "#666" }
                            MouseArea { anchors.fill: parent; onClicked: { root.currentSheetIndex = index } }
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                // Calculations
                RowLayout {
                    spacing: 8
                    Label { text: "Sum: " + (root.sumValue || "0"); font.pointSize: 9; color: "#666"; visible: root.sumValue !== "" }
                    Label { text: "Count: " + (root.countValue || "0"); font.pointSize: 9; color: "#666"; visible: root.countValue !== "" }
                    Label { text: "Average: " + (root.averageValue || "0"); font.pointSize: 9; color: "#666"; visible: root.averageValue !== "" }
                }

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#d0d0d0" }

                // Zoom controls
                ToolButton { implicitWidth: 20; implicitHeight: 20; Image { anchors.centerIn: parent; source: "qrc:/icons/zoom-out.svg"; width: 14; height: 14 } }
                Label { text: root.zoomLevel; font.pointSize: 9; color: "#666" }
                ToolButton { implicitWidth: 20; implicitHeight: 20; Image { anchors.centerIn: parent; source: "qrc:/icons/zoom-in.svg"; width: 14; height: 14 } }
            }
        }
    }

    function getCellRef(row, col) {
        var colChar = String.fromCharCode(65 + col % 26)
        var colPrefix = col >= 26 ? String.fromCharCode(64 + Math.floor(col / 26)) : ""
        return colPrefix + colChar + (row + 1)
    }

    function newSpreadsheet() {
        if (root.modified) { }
        root.currentFile = ""
        root.modified = false
        root.currentRow = 0
        root.currentCol = 0
        root.currentCellRef = "A1"
        root.cellValue = ""
    }

    function openSpreadsheet() { }
    function saveSpreadsheet() {
        if (root.currentFile === "") { saveSpreadsheetAs(); return }
        root.modified = false
        root.fileSaved(root.currentFile)
    }
    function saveSpreadsheetAs() { }
}
