import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    color: "#ffffff"

    property string currentFile: ""
    property bool modified: false
    property int currentPage: 1
    property int totalPages: 1
    property int wordCount: 0
    property string zoomLevel: "100%"
    property string editingMode: "Editing"
    property bool tableSelected: false
    property int tableRows: 0
    property int tableCols: 0
    property int tableSelectedRow: 0
    property int tableSelectedCol: 0
    property bool tableHasSelection: false
    property var tableCellSelection: ({})
    property bool imageSelected: false
    property string imageSource: ""
    property real imageWidth: 0
    property real imageHeight: 0
    property int activeTabIndex: 1

    signal fileOpened(string path)
    signal fileClosed()
    signal fileSaved(string path)
    signal tableInsertRequested(int rows, int cols)
    signal tableDeleteRowRequested()
    signal tableDeleteColRequested()
    signal tableInsertRowAboveRequested()
    signal tableInsertRowBelowRequested()
    signal tableInsertColLeftRequested()
    signal tableInsertColRightRequested()
    signal tableMergeCellsRequested()
    signal tableSplitCellRequested()
    signal tableDistributeRowsRequested()
    signal tableDistributeColsRequested()
    signal tableAutoFitRequested()
    signal tablePropertiesRequested()
    signal tableBordersNoneRequested()
    signal tableBordersAllRequested()
    signal tableBordersOutsideRequested()
    signal tableBordersThickRequested()
    signal tableShadingRequested()
    signal tableAlignmentRequested(string align)
    signal tableCellMarginRequested()
    signal tableSortRequested()
    signal tableFormulaRequested()
    signal tableRepeatHeaderRequested()
    signal imageAdjustBrightnessRequested(real value)
    signal imageAdjustContrastRequested(real value)
    signal imageAdjustSharpnessRequested(real value)
    signal imageColorSaturationRequested(real value)
    signal imageColorTemperatureRequested(real value)
    signal imageRecolorRequested(string preset)
    signal imageArtisticEffectRequested(string effect)
    signal imageTransparencyRequested(real value)
    signal imageCompressRequested()
    signal imageChangeRequested()
    signal imageResetRequested()
    signal imageCropRequested()
    signal imageCropToShapeRequested(string shape)
    signal imageAspectRatioRequested(string ratio)
    signal imageAlignRequested(string align)
    signal imageWrapTextRequested(string wrap)
    signal imagePositionRequested(string pos)
    signal imageRotateFlipRequested(string action)
    signal imageBorderStyleRequested(string style)
    signal imageBorderColorRequested(string color)
    signal imageBorderWidthRequested(int width)
    signal imagePictureEffectsRequested(string effect)
    signal imagePictureLayoutRequested(string layout)
    signal printRequested()
    signal printPreviewRequested()
    signal pageSetupRequested()
    signal exportPdfRequested(string filePath)
    signal scanRequested()
    signal scanToFileRequested(string filePath)
    signal scanMultipleRequested()

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

                ToolButton {
                    implicitWidth: 28
                    implicitHeight: 28
                    onClicked: saveDocument()
                    Image { anchors.centerIn: parent; source: "qrc:/icons/document-save.svg"; width: 16; height: 16 }
                }
                ToolButton {
                    implicitWidth: 28
                    implicitHeight: 28
                    onClicked: root.printRequested()
                    Image { anchors.centerIn: parent; source: "qrc:/icons/document-print.svg"; width: 16; height: 16 }
                }
                ToolButton {
                    implicitWidth: 28
                    implicitHeight: 28
                    onClicked: root.printPreviewRequested()
                    Image { anchors.centerIn: parent; source: "qrc:/icons/document-print-preview.svg"; width: 16; height: 16 }
                }
                ToolButton {
                    implicitWidth: 28
                    implicitHeight: 28
                    onClicked: root.pageSetupRequested()
                    Image { anchors.centerIn: parent; source: "qrc:/icons/document-page-setup.svg"; width: 16; height: 16 }
                }
                ToolButton {
                    implicitWidth: 28
                    implicitHeight: 28
                    onClicked: root.scanRequested()
                    Image { anchors.centerIn: parent; source: "qrc:/icons/document-scan.svg"; width: 16; height: 16 }
                }
                ToolButton {
                    implicitWidth: 28
                    implicitHeight: 28
                    onClicked: root.exportPdfRequested(QFileDialog.getSaveFileName(nullptr, tr("Export to PDF"),
                        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/document.pdf",
                        tr("PDF Files (*.pdf)")))
                    Image { anchors.centerIn: parent; source: "qrc:/icons/document-pdf.svg"; width: 16; height: 16 }
                }
                ToolButton {
                    implicitWidth: 28
                    implicitHeight: 28
                    enabled: false
                    Image { anchors.centerIn: parent; source: "qrc:/icons/edit-undo.svg"; width: 16; height: 16; opacity: parent.enabled ? 1.0 : 0.3 }
                }

ToolButton {
                    implicitWidth: 28
                    implicitHeight: 28
                    onClicked: root.exportPdfRequested(QFileDialog.getSaveFileName(nullptr, tr("Export to PDF"),
                        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/document.pdf",
                        tr("PDF Files (*.pdf)")))
                    Image { anchors.centerIn: parent; source: "qrc:/icons/document-pdf.svg"; width: 16; height: 16 }
                }
                ToolButton {

                Label {
                    text: root.currentFile ? root.currentFile.split('/').pop() : "Untitled"
                    font.pointSize: 11
                    font.bold: true
                    color: "#333"
                    elide: Text.ElideRight
                    Layout.maximumWidth: 200
                }

                Item { Layout.fillWidth: true }

                ComboBox {
                    id: modeCombo
                    Layout.preferredWidth: 90
                    Layout.preferredHeight: 26
                    model: ["Editing", "Reviewing", "Viewing"]
                    currentIndex: 0
                    font.pointSize: 9
                }

                ToolButton {
                    implicitWidth: 28
                    implicitHeight: 28
                    Image { anchors.centerIn: parent; source: "qrc:/icons/edit.svg"; width: 16; height: 16 }
                }
                ToolButton {
                    implicitWidth: 28
                    implicitHeight: 28
                    Image { anchors.centerIn: parent; source: "qrc:/icons/home.svg"; width: 16; height: 16 }
                }
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
                    model: root.tableSelected && root.imageSelected
                        ? ["File", "Home", "Insert", "Draw", "Layout", "Table Design", "Layout", "Picture Format", "References", "Collaboration", "Protection", "View", "Plugins"]
                        : root.tableSelected
                            ? ["File", "Home", "Insert", "Draw", "Layout", "Table Design", "Layout", "References", "Collaboration", "Protection", "View", "Plugins"]
                            : root.imageSelected
                                ? ["File", "Home", "Insert", "Draw", "Layout", "Picture Format", "References", "Collaboration", "Protection", "View", "Plugins"]
                                : ["File", "Home", "Insert", "Draw", "Layout", "References", "Collaboration", "Protection", "View", "Plugins"]

                    Rectangle {
                        Layout.preferredWidth: tabLabel.implicitWidth + 16
                        Layout.fillHeight: true
                        color: tabMouseArea.containsMouse ? "#e0e0e0" : (index === root.activeTabIndex ? "#ffffff" : "transparent")
                        border.color: index === root.activeTabIndex ? "#4472c4" : "transparent"
                        border.width: index === root.activeTabIndex ? 2 : 0

                        Label {
                            id: tabLabel
                            anchors.centerIn: parent
                            text: modelData
                            font.pointSize: 10
                            color: modelData === "Table Design" || modelData === "Layout" ? "#4472c4" : "#333"
                            font.bold: modelData === "Table Design" || modelData === "Layout"
                        }

                        MouseArea {
                            id: tabMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                root.activeTabIndex = index
                            }
                        }
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

                // ==================== HOME TAB TOOLBAR ====================
                // Clipboard group
                ColumnLayout {
                    spacing: 2
                    visible: root.activeTabIndex === 1
                    RowLayout {
                        spacing: 4
                        ToolButton {
                            implicitWidth: 32
                            implicitHeight: 32
                            Image { anchors.centerIn: parent; source: "qrc:/icons/edit-paste.svg"; width: 20; height: 20 }
                        }
                    }
                    Label { text: "Paste"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                }

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#e0e0e0"; visible: root.activeTabIndex === 1 }

                // Font group
                RowLayout {
                    spacing: 4
                    visible: root.activeTabIndex === 1
                    ComboBox { Layout.preferredWidth: 120; Layout.preferredHeight: 24; model: ["Arial", "Times New Roman", "Courier New", "Calibri"]; currentIndex: 0; font.pointSize: 9 }
                    ComboBox { Layout.preferredWidth: 40; Layout.preferredHeight: 24; model: ["8", "9", "10", "11", "12", "14", "16", "18", "20", "24", "28", "36"]; currentIndex: 2; font.pointSize: 9 }
                    ToolButton { text: "B"; font.bold: true; font.pointSize: 10; implicitWidth: 28; implicitHeight: 28; checkable: true }
                    ToolButton { text: "I"; font.italic: true; font.pointSize: 10; implicitWidth: 28; implicitHeight: 28; checkable: true }
                    ToolButton { text: "U"; font.underline: true; font.pointSize: 10; implicitWidth: 28; implicitHeight: 28; checkable: true }
                    ToolButton { text: "S"; font.strikeout: true; font.pointSize: 10; implicitWidth: 28; implicitHeight: 28; checkable: true }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/format-font-color.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/format-highlight.svg"; width: 16; height: 16 } }
                }

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#e0e0e0"; visible: root.activeTabIndex === 1 }

                // Format group
                RowLayout {
                    spacing: 4
                    visible: root.activeTabIndex === 1
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/format-clear.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/format-paint.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/format-shading.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/format-line-spacing.svg"; width: 16; height: 16 } }
                }

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#e0e0e0"; visible: root.activeTabIndex === 1 }

                // Paragraph group
                RowLayout {
                    spacing: 4
                    visible: root.activeTabIndex === 1
                    ToolButton { implicitWidth: 28; implicitHeight: 28; checkable: true; checked: true; Image { anchors.centerIn: parent; source: "qrc:/icons/format-align-left.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; checkable: true; Image { anchors.centerIn: parent; source: "qrc:/icons/format-align-center.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; checkable: true; Image { anchors.centerIn: parent; source: "qrc:/icons/format-align-right.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; checkable: true; Image { anchors.centerIn: parent; source: "qrc:/icons/format-align-justify.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/format-list-unordered.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/format-list-ordered.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/format-indent-more.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/format-indent-less.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/format-superscript.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/format-subscript.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/format-border.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/format-fill-color.svg"; width: 16; height: 16 } }
                }

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#e0e0e0"; visible: root.activeTabIndex === 1 }

                // Insert group
                RowLayout {
                    spacing: 4
                    visible: root.activeTabIndex === 1
                    ToolButton {
                        implicitWidth: 28
                        implicitHeight: 28
                        Image { anchors.centerIn: parent; source: "qrc:/icons/insert-table.svg"; width: 16; height: 16 }
                        onClicked: tableGridPopup.open()
                    }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/insert-image.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/insert-icons.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/insert-smartart.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/insert-chart.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/insert-link.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/insert-textbox.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/insert-wordart.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/insert-header-footer.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/insert-equation.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/insert-symbol.svg"; width: 16; height: 16 } }
                    ToolButton {
                        implicitWidth: 28
                        implicitHeight: 28
                        onClicked: root.scanRequested()
                        Image { anchors.centerIn: parent; source: "qrc:/icons/document-scan.svg"; width: 16; height: 16 }
                    }
                    ToolButton {
                        implicitWidth: 28
                        implicitHeight: 28
                        onClicked: root.scanToFileRequested("")
                        Image { anchors.centerIn: parent; source: "qrc:/icons/document-scan-to-file.svg"; width: 16; height: 16 }
                    }
                    ToolButton {
                        implicitWidth: 28
                        implicitHeight: 28
                        onClicked: root.scanMultipleRequested()
                        Image { anchors.centerIn: parent; source: "qrc:/icons/document-scan-multiple.svg"; width: 16; height: 16 }
                    }
                }

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#e0e0e0"; visible: root.activeTabIndex === 1 }

                // Layout group
                RowLayout {
                    spacing: 4
                    visible: root.activeTabIndex === 1
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/layout-margins.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/layout-orientation-portrait.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/layout-orientation-landscape.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/layout-size.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/layout-columns.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/layout-breaks.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/layout-line-numbers.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/layout-hyphenation.svg"; width: 16; height: 16 } }
                }

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#e0e0e0"; visible: root.activeTabIndex === 1 }

                // References group
                RowLayout {
                    spacing: 4
                    visible: root.activeTabIndex === 1
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/references-toc.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/references-footnotes.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/references-citations.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/references-captions.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/references-index.svg"; width: 16; height: 16 } }
                }

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#e0e0e0"; visible: root.activeTabIndex === 1 }

                // Review group
                RowLayout {
                    spacing: 4
                    visible: root.activeTabIndex === 1
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

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#e0e0e0"; visible: root.activeTabIndex === 1 }

                // View group
                RowLayout {
                    spacing: 4
                    visible: root.activeTabIndex === 1
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/view-web-layout.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/view-draft-layout.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/view-outline.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/view-zoom.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; Image { anchors.centerIn: parent; source: "qrc:/icons/view-switch-windows.svg"; width: 16; height: 16 } }
                }

                // ==================== TABLE DESIGN TAB ====================
                // Table Styles group
                RowLayout {
                    spacing: 4
                    visible: root.activeTabIndex === 5 && root.tableSelected

                    Repeater {
                        model: 6
                        Rectangle {
                            implicitWidth: 48
                            implicitHeight: 28
                            color: "#ffffff"
                            border.color: index === 0 ? "#4472c4" : "#c0c0c0"
                            border.width: index === 0 ? 2 : 1
                            radius: 2

                            Column {
                                anchors.centerIn: parent
                                spacing: 1
                                Rectangle { width: 20; height: 3; color: index === 0 ? "#4472c4" : "#333"; anchors.horizontalCenter: parent.horizontalCenter }
                                Rectangle { width: 20; height: 3; color: index === 0 ? "#4472c4" : "#666"; anchors.horizontalCenter: parent.horizontalCenter }
                                Rectangle { width: 20; height: 3; color: index === 0 ? "#4472c4" : "#999"; anchors.horizontalCenter: parent.horizontalCenter }
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: { }
                            }
                        }
                    }
                }

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#e0e0e0"; visible: root.activeTabIndex === 5 && root.tableSelected }

                // Table Properties group
                RowLayout {
                    spacing: 4
                    visible: root.activeTabIndex === 5 && root.tableSelected

                    ToolButton { implicitWidth: 28; implicitHeight: 28; ToolTip.text: "Table Properties"; ToolTip.visible: hovered; Image { anchors.centerIn: parent; source: "qrc:/icons/document-properties.svg"; width: 16; height: 16 } }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; ToolTip.text: "Delete Table"; ToolTip.visible: hovered; Image { anchors.centerIn: parent; source: "qrc:/icons/edit-delete.svg"; width: 16; height: 16 } }
                }

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#e0e0e0"; visible: root.activeTabIndex === 5 && root.tableSelected }

                // Shading group
                RowLayout {
                    spacing: 4
                    visible: root.activeTabIndex === 5 && root.tableSelected

                    ColumnLayout {
                        spacing: 2
                        ToolButton { implicitWidth: 28; implicitHeight: 28; ToolTip.text: "Shading"; ToolTip.visible: hovered; Image { anchors.centerIn: parent; source: "qrc:/icons/format-fill-color.svg"; width: 16; height: 16 } }
                        Label { text: "Shading"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }
                }

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#e0e0e0"; visible: root.activeTabIndex === 5 && root.tableSelected }

                // Borders group
                RowLayout {
                    spacing: 4
                    visible: root.activeTabIndex === 5 && root.tableSelected

                    ColumnLayout {
                        spacing: 2
                        ToolButton { implicitWidth: 28; implicitHeight: 28; ToolTip.text: "Borders"; ToolTip.visible: hovered; Image { anchors.centerIn: parent; source: "qrc:/icons/format-border.svg"; width: 16; height: 16 } }
                        Label { text: "Borders"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "All Borders"
                            ToolTip.visible: hovered
                            Image { anchors.centerIn: parent; source: "qrc:/icons/insert-table.svg"; width: 16; height: 16 }
                            onClicked: root.tableBordersAllRequested()
                        }
                        Label { text: "All"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "No Border"
                            ToolTip.visible: hovered
                            Image { anchors.centerIn: parent; source: "qrc:/icons/format-border-styles.svg"; width: 16; height: 16 }
                            onClicked: root.tableBordersNoneRequested()
                        }
                        Label { text: "None"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "Outside Borders"
                            ToolTip.visible: hovered
                            Image { anchors.centerIn: parent; source: "qrc:/icons/format-border.svg"; width: 16; height: 16 }
                            onClicked: root.tableBordersOutsideRequested()
                        }
                        Label { text: "Outside"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }
                }

                // ==================== TABLE LAYOUT TAB ====================
                // Table group
                RowLayout {
                    spacing: 4
                    visible: root.activeTabIndex === 6 && root.tableSelected

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "Insert Row Above"
                            ToolTip.visible: hovered
                            Image { anchors.centerIn: parent; source: "qrc:/icons/insert-table-rows.svg"; width: 16; height: 16 }
                            onClicked: root.tableInsertRowAboveRequested()
                        }
                        Label { text: "Above"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "Insert Row Below"
                            ToolTip.visible: hovered
                            Image { anchors.centerIn: parent; source: "qrc:/icons/insert-table-rows.svg"; width: 16; height: 16 }
                            onClicked: root.tableInsertRowBelowRequested()
                        }
                        Label { text: "Below"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "Insert Column Left"
                            ToolTip.visible: hovered
                            Image { anchors.centerIn: parent; source: "qrc:/icons/insert-table-columns.svg"; width: 16; height: 16 }
                            onClicked: root.tableInsertColLeftRequested()
                        }
                        Label { text: "Left"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "Insert Column Right"
                            ToolTip.visible: hovered
                            Image { anchors.centerIn: parent; source: "qrc:/icons/insert-table-columns.svg"; width: 16; height: 16 }
                            onClicked: root.tableInsertColRightRequested()
                        }
                        Label { text: "Right"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "Delete Rows"
                            ToolTip.visible: hovered
                            Image { anchors.centerIn: parent; source: "qrc:/icons/edit-delete.svg"; width: 16; height: 16 }
                            onClicked: root.tableDeleteRowRequested()
                        }
                        Label { text: "Delete"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }
                }

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#e0e0e0"; visible: root.activeTabIndex === 6 && root.tableSelected }

                // Merge group
                RowLayout {
                    spacing: 4
                    visible: root.activeTabIndex === 6 && root.tableSelected

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "Merge Cells"
                            ToolTip.visible: hovered
                            enabled: root.tableHasSelection
                            Image { anchors.centerIn: parent; source: "qrc:/icons/format-merge-cells.svg"; width: 16; height: 16; opacity: parent.enabled ? 1.0 : 0.3 }
                            onClicked: root.tableMergeCellsRequested()
                        }
                        Label { text: "Merge"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "Split Cells"
                            ToolTip.visible: hovered
                            enabled: root.tableHasSelection
                            Image { anchors.centerIn: parent; source: "qrc:/icons/format-split-cells.svg"; width: 16; height: 16; opacity: parent.enabled ? 1.0 : 0.3 }
                            onClicked: root.tableSplitCellRequested()
                        }
                        Label { text: "Split"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }
                }

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#e0e0e0"; visible: root.activeTabIndex === 6 && root.tableSelected }

                // Cell Size group
                RowLayout {
                    spacing: 4
                    visible: root.activeTabIndex === 6 && root.tableSelected

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "AutoFit Contents"
                            ToolTip.visible: hovered
                            Image { anchors.centerIn: parent; source: "qrc:/icons/view-zoom.svg"; width: 16; height: 16 }
                            onClicked: root.tableAutoFitRequested()
                        }
                        Label { text: "AutoFit"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "Distribute Rows"
                            ToolTip.visible: hovered
                            Image { anchors.centerIn: parent; source: "qrc:/icons/format-lines.svg"; width: 16; height: 16 }
                            onClicked: root.tableDistributeRowsRequested()
                        }
                        Label { text: "Rows"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "Distribute Columns"
                            ToolTip.visible: hovered
                            Image { anchors.centerIn: parent; source: "qrc:/icons/format-lines.svg"; width: 16; height: 16 }
                            onClicked: root.tableDistributeColsRequested()
                        }
                        Label { text: "Cols"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }
                }

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#e0e0e0"; visible: root.activeTabIndex === 6 && root.tableSelected }

                // Alignment group
                RowLayout {
                    spacing: 4
                    visible: root.activeTabIndex === 6 && root.tableSelected

                    ToolButton { implicitWidth: 28; implicitHeight: 28; checkable: true; checked: true; ToolTip.text: "Align Left"; ToolTip.visible: hovered; Image { anchors.centerIn: parent; source: "qrc:/icons/format-align-left.svg"; width: 16; height: 16 }; onClicked: root.tableAlignmentRequested("left") }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; checkable: true; ToolTip.text: "Align Center"; ToolTip.visible: hovered; Image { anchors.centerIn: parent; source: "qrc:/icons/format-align-center.svg"; width: 16; height: 16 }; onClicked: root.tableAlignmentRequested("center") }
                    ToolButton { implicitWidth: 28; implicitHeight: 28; checkable: true; ToolTip.text: "Align Right"; ToolTip.visible: hovered; Image { anchors.centerIn: parent; source: "qrc:/icons/format-align-right.svg"; width: 16; height: 16 }; onClicked: root.tableAlignmentRequested("right") }

                    ToolButton { implicitWidth: 28; implicitHeight: 28; ToolTip.text: "Cell Margins"; ToolTip.visible: hovered; Image { anchors.centerIn: parent; source: "qrc:/icons/layout-margins.svg"; width: 16; height: 16 }; onClicked: root.tableCellMarginRequested() }
                }

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#e0e0e0"; visible: root.activeTabIndex === 6 && root.tableSelected }

                // Data group
                RowLayout {
                    spacing: 4
                    visible: root.activeTabIndex === 6 && root.tableSelected

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "Sort"
                            ToolTip.visible: hovered
                            Image { anchors.centerIn: parent; source: "qrc:/icons/review-sort.svg"; width: 16; height: 16 }
                            onClicked: root.tableSortRequested()
                        }
                        Label { text: "Sort"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "Formula"
                            ToolTip.visible: hovered
                            Image { anchors.centerIn: parent; source: "qrc:/icons/formula.svg"; width: 16; height: 16 }
                            onClicked: root.tableFormulaRequested()
                        }
                        Label { text: "Formula"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "Repeat Header Rows"
                            ToolTip.visible: hovered
                            Image { anchors.centerIn: parent; source: "qrc:/icons/review-repeat-header.svg"; width: 16; height: 16 }
                            onClicked: root.tableRepeatHeaderRequested()
                        }
                        Label { text: "Header"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }
                }

                // ==================== PICTURE FORMAT TAB ====================
                // Adjust group
                RowLayout {
                    spacing: 4
                    visible: (root.activeTabIndex === 7 && root.tableSelected && root.imageSelected) || (root.activeTabIndex === 6 && root.imageSelected && !root.tableSelected)

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "Corrections"
                            ToolTip.visible: hovered
                            Image { anchors.centerIn: parent; source: "qrc:/icons/format-effects.svg"; width: 16; height: 16 }
                            onClicked: { }
                        }
                        Label { text: "Corrections"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "Color"
                            ToolTip.visible: hovered
                            Image { anchors.centerIn: parent; source: "qrc:/icons/format-fill-color.svg"; width: 16; height: 16 }
                            onClicked: { }
                        }
                        Label { text: "Color"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "Artistic Effects"
                            ToolTip.visible: hovered
                            Image { anchors.centerIn: parent; source: "qrc:/icons/format-effects.svg"; width: 16; height: 16 }
                            onClicked: { }
                        }
                        Label { text: "Artistic"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "Transparency"
                            ToolTip.visible: hovered
                            Image { anchors.centerIn: parent; source: "qrc:/icons/format-shape-fill.svg"; width: 16; height: 16 }
                            onClicked: { }
                        }
                        Label { text: "Transparent"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }
                }

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#e0e0e0"; visible: (root.activeTabIndex === 7 && root.tableSelected && root.imageSelected) || (root.activeTabIndex === 6 && root.imageSelected && !root.tableSelected) }

                // Picture Styles group
                RowLayout {
                    spacing: 4
                    visible: (root.activeTabIndex === 7 && root.tableSelected && root.imageSelected) || (root.activeTabIndex === 6 && root.imageSelected && !root.tableSelected)

                    Repeater {
                        model: 7
                        Rectangle {
                            implicitWidth: 48
                            implicitHeight: 28
                            color: "#ffffff"
                            border.color: index === 0 ? "#4472c4" : "#c0c0c0"
                            border.width: index === 0 ? 2 : 1
                            radius: 2

                            Rectangle {
                                anchors.centerIn: parent
                                width: 24
                                height: 18
                                color: index === 0 ? "#4472c4" : "#333"
                                radius: 1
                                border.color: "#666"
                                border.width: 1
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: { }
                            }
                        }
                    }
                }

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#e0e0e0"; visible: (root.activeTabIndex === 7 && root.tableSelected && root.imageSelected) || (root.activeTabIndex === 6 && root.imageSelected && !root.tableSelected) }

                // Arrange group
                RowLayout {
                    spacing: 4
                    visible: (root.activeTabIndex === 7 && root.tableSelected && root.imageSelected) || (root.activeTabIndex === 6 && root.imageSelected && !root.tableSelected)

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "Wrap Text"
                            ToolTip.visible: hovered
                            Image { anchors.centerIn: parent; source: "qrc:/icons/format-shape-text-wrap.svg"; width: 16; height: 16 }
                            onClicked: { }
                        }
                        Label { text: "Wrap"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "Position"
                            ToolTip.visible: hovered
                            Image { anchors.centerIn: parent; source: "qrc:/icons/format-position.svg"; width: 16; height: 16 }
                            onClicked: { }
                        }
                        Label { text: "Position"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "Bring Forward"
                            ToolTip.visible: hovered
                            Image { anchors.centerIn: parent; source: "qrc:/icons/format-shape-arrange.svg"; width: 16; height: 16 }
                            onClicked: { }
                        }
                        Label { text: "Forward"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "Send Backward"
                            ToolTip.visible: hovered
                            Image { anchors.centerIn: parent; source: "qrc:/icons/format-shape-arrange.svg"; width: 16; height: 16 }
                            onClicked: { }
                        }
                        Label { text: "Backward"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "Selection Pane"
                            ToolTip.visible: hovered
                            Image { anchors.centerIn: parent; source: "qrc:/icons/view-outline.svg"; width: 16; height: 16 }
                            onClicked: { }
                        }
                        Label { text: "Selection"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "Align"
                            ToolTip.visible: hovered
                            Image { anchors.centerIn: parent; source: "qrc:/icons/format-align-center.svg"; width: 16; height: 16 }
                            onClicked: { }
                        }
                        Label { text: "Align"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "Group"
                            ToolTip.visible: hovered
                            Image { anchors.centerIn: parent; source: "qrc:/icons/format-group.svg"; width: 16; height: 16 }
                            onClicked: { }
                        }
                        Label { text: "Group"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "Rotate"
                            ToolTip.visible: hovered
                            Image { anchors.centerIn: parent; source: "qrc:/icons/format-rotate.svg"; width: 16; height: 16 }
                            onClicked: { }
                        }
                        Label { text: "Rotate"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }
                }

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#e0e0e0"; visible: (root.activeTabIndex === 7 && root.tableSelected && root.imageSelected) || (root.activeTabIndex === 6 && root.imageSelected && !root.tableSelected) }

                // Size group
                RowLayout {
                    spacing: 4
                    visible: (root.activeTabIndex === 7 && root.tableSelected && root.imageSelected) || (root.activeTabIndex === 6 && root.imageSelected && !root.tableSelected)

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "Crop"
                            ToolTip.visible: hovered
                            Image { anchors.centerIn: parent; source: "qrc:/icons/format-crop.svg"; width: 16; height: 16 }
                            onClicked: root.imageCropRequested()
                        }
                        Label { text: "Crop"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "Height"
                            ToolTip.visible: hovered
                            Image { anchors.centerIn: parent; source: "qrc:/icons/format-shape-fit.svg"; width: 16; height: 16 }
                            onClicked: { }
                        }
                        Label { text: "Height"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }

                    ColumnLayout {
                        spacing: 2
                        ToolButton {
                            implicitWidth: 28
                            implicitHeight: 28
                            ToolTip.text: "Width"
                            ToolTip.visible: hovered
                            Image { anchors.centerIn: parent; source: "qrc:/icons/format-shape-fit.svg"; width: 16; height: 16 }
                            onClicked: { }
                        }
                        Label { text: "Width"; font.pointSize: 8; color: "#666"; Layout.alignment: Qt.AlignHCenter }
                    }
                }

                Item { Layout.fillWidth: true }
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
                    ToolButton { implicitWidth: 28; implicitHeight: 28; ToolTip.text: "Navigation"; ToolTip.visible: hovered; Image { anchors.centerIn: parent; source: "qrc:/icons/home.svg"; width: 18; height: 18 } }

                    Item { Layout.fillHeight: true }
                }
            }

            // Document area
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#808080"

                Flickable {
                    id: flickable
                    anchors.fill: parent
                    anchors.margins: 20
                    contentWidth: 595
                    contentHeight: 842
                    clip: true
                    flickableDirection: Flickable.HorizontalAndVerticalFlick

                    Rectangle {
                        width: 595
                        height: Math.max(842, textArea.implicitHeight + 40)
                        color: "#ffffff"
                        border.color: "#d0d0d0"
                        border.width: 1

                        TextArea {
                            id: textArea
                            anchors.fill: parent
                            anchors.margins: 20
                            wrapMode: TextArea.Wrap
                            font.pointSize: 11
                            font.family: "Arial"
                            selectByMouse: true
                            background: null

                            onTextChanged: {
                                root.modified = true
                                root.wordCount = text.split(/\s+/).filter(function(w) { return w.length > 0 }).length
                            }

                            onCursorRectangleChanged: {
                                detectTableAtCursor()
                            }
                        }
                    }
                }
            }
        }

        // ==================== STATUS BAR ====================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 24
            color: "#f5f5f5"
            border.color: "#e0e0e0"
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 16

                Label { text: "Page " + root.currentPage + " of " + root.totalPages; font.pointSize: 9; color: "#666" }
                Label { text: root.wordCount + " words"; font.pointSize: 9; color: "#666" }

                Item { Layout.fillWidth: true }

                Label {
                    text: root.tableSelected ? "Table " + root.tableRows + "x" + root.tableCols :
                          root.imageSelected ? "Picture " + Math.round(root.imageWidth) + "x" + Math.round(root.imageHeight) : "All changes saved"
                    font.pointSize: 9
                    color: root.tableSelected || root.imageSelected ? "#4472c4" : "#666"
                }

                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#d0d0d0" }

                ToolButton { implicitWidth: 20; implicitHeight: 20; Image { anchors.centerIn: parent; source: "qrc:/icons/zoom-out.svg"; width: 14; height: 14 } }
                Label { text: root.zoomLevel; font.pointSize: 9; color: "#666" }
                ToolButton { implicitWidth: 20; implicitHeight: 20; Image { anchors.centerIn: parent; source: "qrc:/icons/zoom-in.svg"; width: 14; height: 14 } }
            }
        }
    }

    function detectTableAtCursor() {
        var cursorPos = textArea.cursorPosition
        var text = textArea.text
        var tableStart = text.indexOf("╔")
        var tableEnd = text.indexOf("╝")

        if (tableStart !== -1 && tableEnd !== -1 && cursorPos >= tableStart && cursorPos <= tableEnd) {
            if (!root.tableSelected) {
                root.tableSelected = true
                root.activeTabIndex = 5
                parseTableInfo(text, tableStart, tableEnd)
            }
        } else {
            if (root.tableSelected) {
                root.tableSelected = false
                if (!root.imageSelected) {
                    root.activeTabIndex = 1
                }
                root.tableRows = 0
                root.tableCols = 0
                root.tableHasSelection = false
            }
        }
        detectImageAtCursor()
    }

    function detectImageAtCursor() {
        var cursorPos = textArea.cursorPosition
        var text = textArea.text
        var imgStart = text.indexOf("![](image:")
        if (imgStart === -1) {
            imgStart = text.indexOf("<img ")
        }
        if (imgStart === -1) {
            imgStart = text.indexOf("[[image:")
        }

        if (imgStart !== -1) {
            var imgEnd = text.indexOf(")", imgStart)
            if (imgEnd === -1) imgEnd = text.indexOf(">", imgStart)
            if (imgEnd === -1) imgEnd = text.indexOf("]]", imgStart)
            if (imgEnd === -1) imgEnd = imgStart + 100

            if (cursorPos >= imgStart && cursorPos <= imgEnd) {
                if (!root.imageSelected) {
                    root.imageSelected = true
                    if (root.tableSelected) {
                        root.activeTabIndex = 7
                    } else {
                        root.activeTabIndex = 6
                    }
                    parseImageInfo(text, imgStart, imgEnd)
                }
            } else {
                if (root.imageSelected) {
                    root.imageSelected = false
                    if (!root.tableSelected) {
                        root.activeTabIndex = 1
                    }
                    root.imageSource = ""
                    root.imageWidth = 0
                    root.imageHeight = 0
                }
            }
        } else {
            if (root.imageSelected) {
                root.imageSelected = false
                if (!root.tableSelected) {
                    root.activeTabIndex = 1
                }
                root.imageSource = ""
                root.imageWidth = 0
                root.imageHeight = 0
            }
        }
    }

    function parseImageInfo(text, start, end) {
        var imgText = text.substring(start, end)
        root.imageSource = imgText
        root.imageWidth = 300
        root.imageHeight = 200
    }

    function parseTableInfo(text, start, end) {
        var tableText = text.substring(start, end + 1)
        var rows = tableText.split("\n").filter(function(r) { return r.indexOf("║") !== -1 })
        root.tableRows = rows.length
        if (rows.length > 0) {
            root.tableCols = (rows[0].match(/║/g) || []).length - 1
        }
    }

    function insertTable(rows, cols) {
        var table = ""
        for (var r = 0; r < rows; r++) {
            var row = "║"
            for (var c = 0; c < cols; c++) {
                row += " Cell ║"
            }
            table += row + "\n"
        }
        textArea.insert(textArea.cursorPosition, table)
        root.tableSelected = true
        root.tableRows = rows
        root.tableCols = cols
        root.activeTabIndex = 5
    }

    function newDocument() {
        if (root.modified) { }
        textArea.text = ""
        root.currentFile = ""
        root.modified = false
        root.currentPage = 1
        root.totalPages = 1
        root.wordCount = 0
        root.tableSelected = false
        root.tableRows = 0
        root.tableCols = 0
        root.imageSelected = false
        root.imageSource = ""
        root.imageWidth = 0
        root.imageHeight = 0
    }

    function insertImage(source) {
        var imgMarkdown = "![](" + source + ")"
        textArea.insert(textArea.cursorPosition, imgMarkdown)
        root.imageSelected = true
        root.imageSource = source
        root.imageWidth = 300
        root.imageHeight = 200
        if (root.tableSelected) {
            root.activeTabIndex = 7
        } else {
            root.activeTabIndex = 6
        }
    }

    function openDocument() { }
    function saveDocument() {
        if (root.currentFile === "") { saveDocumentAs(); return }
        root.modified = false
        root.fileSaved(root.currentFile)
    }
    function saveDocumentAs() { }

    Popup {
        id: tableGridPopup
        x: 0
        y: 0
        width: 220
        height: 260
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 4

            Label {
                text: "Insert Table"
                font.pointSize: 11
                font.bold: true
                color: "#333"
            }

            Label {
                text: "Select grid size:"
                font.pointSize: 9
                color: "#666"
            }

            Grid {
                columns: 10
                spacing: 2

                Repeater {
                    model: 100

                    Rectangle {
                        width: 16
                        height: 16
                        color: {
                            var row = Math.floor(index / 10)
                            var col = index % 10
                            return (row < tableGridHover.row && col < tableGridHover.col) ? "#4472c4" : "#ffffff"
                        }
                        border.color: "#c0c0c0"
                        border.width: 1

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: {
                                tableGridHover.row = Math.floor(index / 10) + 1
                                tableGridHover.col = (index % 10) + 1
                                tableGridLabel.text = tableGridHover.row + "x" + tableGridHover.col
                            }
                            onClicked: {
                                insertTable(tableGridHover.row, tableGridHover.col)
                                tableGridPopup.close()
                            }
                        }
                    }
                }
            }

            QtObject {
                id: tableGridHover
                property int row: 3
                property int col: 3
            }

            Label {
                id: tableGridLabel
                text: "3x3"
                font.pointSize: 9
                color: "#333"
                Layout.alignment: Qt.AlignHCenter
            }

            Button {
                Layout.fillWidth: true
                text: "Insert Table..."
                onClicked: {
                    tableGridPopup.close()
                    root.tableInsertRequested(3, 3)
                }
            }
        }
    }
}
