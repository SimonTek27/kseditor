import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: ksApp
    width: 1280
    height: 720
    color: "#121212"

    // ─── routing ─────────────────────────────────────────────────────────────
    // Index must match order of pages in contentStack below
    readonly property var modeIndex: ({
        "home":    0,
        "track":   1,
        "car":     2,
        "physics": 3,
        "audio":   4,
        "fonts":   5,
        "license": 6,
        "display": 7,
        "assets":  8,
        "ai":      9,
        "workshop": 10,
        "modmanager": 11,
        "telemetry": 12,
        "setup":   13,
        "ppfilters": 14,
        "formattools":  15,
        "cspconfig": 16,
        "character": 17,
        "modeler": 18,
        "livery": 19
    })

    property string currentMode: "home"

    function switchTo(mode) {
        currentMode = mode
        contentStack.currentIndex = modeIndex[mode]
    }

    // ─── layout ──────────────────────────────────────────────────────────────
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Toolbar ──────────────────────────────────────────────────────────
        Rectangle {
            height: 40
            color: "#1e1e1e"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10

                // Home button (no Repeater — special styling)
                Button {
                    flat: true
                    width: 36
                    height: 32
                    onClicked: switchTo("home")
                    background: Rectangle {
                        color: currentMode === "home" ? "#E10600" : "transparent"
                        radius: 3
                    }
                    contentItem: Image {
                        source: "qrc:/icons/home.svg"
                        sourceSize: Qt.size(20, 20)
                        fillMode: Image.PreserveAspectFit
                        horizontalAlignment: Image.AlignHCenter
                        verticalAlignment: Image.AlignVCenter
                    }
                }

                Rectangle { width: 1; height: 20; color: "#444444" }

                // Module buttons
                Repeater {
                    model: [
                        { icon: "qrc:/icons/track.svg",       mode: "track"   },
                        { icon: "qrc:/icons/car-model.svg",   mode: "car"     },
                        { icon: "qrc:/icons/car-physics.svg", mode: "physics" },
                        { icon: "qrc:/icons/audio.svg",       mode: "audio"   },
                        { icon: "qrc:/icons/font.svg",       mode: "fonts"   },
                        { icon: "qrc:/icons/licenseplate.svg", mode: "license" },
                        { icon: "qrc:/icons/display.svg",    mode: "display" },
                        { icon: "qrc:/icons/assets.svg",     mode: "assets"  },
                        { icon: "qrc:/icons/ai.svg",         mode: "ai"      },
                        { icon: "qrc:/icons/workshop.svg",   mode: "workshop"},
                        { icon: "qrc:/icons/modmanager.svg",  mode: "modmanager" },
                        { icon: "qrc:/icons/telemetry.svg",  mode: "telemetry" },
                        { icon: "qrc:/icons/params.svg",     mode: "setup"   },
                        { icon: "qrc:/icons/ppfilters.svg",  mode: "ppfilters" },
                        { icon: "qrc:/icons/assets.svg",     mode: "formattools" },
                        { icon: "qrc:/icons/settings.svg",   mode: "cspconfig" },
                        { icon: "qrc:/icons/skeleton.svg", mode: "character" }
                    ]
                    Button {
                        required property var modelData
                        flat: true
                        width: 36
                        height: 32
                        onClicked: switchTo(modelData.mode)
                        background: Rectangle {
                            color: currentMode === modelData.mode ? "#E10600" : "transparent"
                            radius: 3
                        }
                        contentItem: Image {
                            source: modelData.icon
                            sourceSize: Qt.size(20, 20)
                            fillMode: Image.PreserveAspectFit
                            horizontalAlignment: Image.AlignHCenter
                            verticalAlignment: Image.AlignVCenter
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                Button {
                    flat: true
                    width: 36
                    height: 32
                    onClicked: switchTo("cspconfig")
                    background: Rectangle { color: "transparent" }
                    contentItem: Image {
                        source: "qrc:/icons/settings.svg"
                        sourceSize: Qt.size(20, 20)
                        fillMode: Image.PreserveAspectFit
                        horizontalAlignment: Image.AlignHCenter
                        verticalAlignment: Image.AlignVCenter
                    }
                }
            }
        }

        // ── Content stack ─────────────────────────────────────────────────────
        StackLayout {
            id: contentStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: 0

            // ── 0: Home ───────────────────────────────────────────────────────
            Rectangle {
                color: "#121212"

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 30

                    Text {
                        text: "ksEditor"
                        color: "#ffffff"
                        font.pixelSize: 36
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Text {
                        text: "ASSETTO CORSA MODDING SUITE"
                        color: "#E10600"
                        font.pixelSize: 14
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Item { height: 20 }

                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 15
                        Button {
                            width: 140; height: 48; text: "New Project"
                            onClicked: switchTo("modeler")
                            background: Rectangle { color: "#E10600"; radius: 3 }
                            contentItem: Text { text: parent.text; color: "#121212"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                        Button {
                            width: 140; height: 48; text: "Open Project"
                            onClicked: switchTo("modeler")
                            background: Rectangle { color: "#252526"; radius: 3; border.color: "#444"; border.width: 1 }
                            contentItem: Text { text: parent.text; color: "#ffffff"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                        Button {
                            width: 140; height: 48; text: "Recent"
                            onClicked: switchTo("modeler")
                            background: Rectangle { color: "#252526"; radius: 3; border.color: "#444"; border.width: 1 }
                            contentItem: Text { text: parent.text; color: "#ffffff"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                    }

                    Item { height: 10 }

                    Text {
                        text: "QUICK START"
                        color: "#666"
                        font.pixelSize: 12
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 10
                        Repeater {
                            model: [
                                { label: "Car\nEditor",       mode: "car"       },
                                { label: "Character\nEditor", mode: "character" },
                                { label: "Track\nEditor",     mode: "track"     },
                                { label: "Physics\nEditor",   mode: "physics" },
                                { label: "Audio\nStation",    mode: "audio"   },
                                { label: "Font\nCreator",     mode: "fonts"   },
                                { label: "License\nPlates",   mode: "license" }
                            ]
                            Button {
                                required property var modelData
                                width: 100; height: 80
                                onClicked: switchTo(modelData.mode)
                                background: Rectangle { color: "#252526"; radius: 4; border.color: "#444"; border.width: 1 }
                                contentItem: Text {
                                    text: modelData.label
                                    color: "#ffffff"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }

                    Item { height: 20 }

                    Text {
                        text: "SUITE"
                        color: "#666"
                        font.pixelSize: 12
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                    }

                    GridLayout {
                        Layout.alignment: Qt.AlignHCenter
                        columns: 5
                        columnSpacing: 12
                        rowSpacing: 12

                        Repeater {
                            model: [
                                { icon: "qrc:/icons/modeler.svg",     label: "3D\nModeler",        mode: "modeler" },
                                { icon: "qrc:/icons/livery.svg",      label: "Livery\nLivery",     mode: "livery" },
                                { icon: "qrc:/icons/font.svg",        label: "Font\nCreator",      mode: "fonts" },
                                { icon: "qrc:/icons/sound.svg",       label: "Audio\nStudio",      mode: "audio" },
                                { icon: "qrc:/icons/waveform.svg",    label: "Audio\nEditor",      mode: "audio" },
                                { icon: "qrc:/icons/physics.svg",     label: "Physics\nEditor",    mode: "physics" }
                            ]
                            Button {
                                required property var modelData
                                width: 90
                                height: 90
                                onClicked: switchTo(modelData.mode)
                                background: Rectangle {
                                    color: "#252526"
                                    radius: 6
                                    border.color: "#444"
                                    border.width: 1
                                }
                                contentItem: ColumnLayout {
                                    anchors.centerIn: parent
                                    spacing: 6
                                    Image {
                                        source: modelData.icon
                                        sourceSize: Qt.size(32, 32)
                                        fillMode: Image.PreserveAspectFit
                                        Layout.alignment: Qt.AlignHCenter
                                    }
                                    Text {
                                        text: modelData.label
                                        color: "#ffffff"
                                        font.pixelSize: 10
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        wrapMode: Text.WordWrap
                                        Layout.alignment: Qt.AlignHCenter
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ── 1: Track Editor ───────────────────────────────────────────────
            Loader {
                active: currentMode === "track"
                source: "pages/page_ksModeler.qml"
                onStatusChanged: if (status === Loader.Error) console.error("page_ksModeler.qml failed to load")
            }

            // ── 2: Car Editor ─────────────────────────────────────────────────
            Loader {
                active: currentMode === "car"
                source: "pages/page_ksModeler.qml"
                onStatusChanged: if (status === Loader.Error) console.error("page_ksModeler.qml failed to load")
            }

            // ── 3: Physics Editor ─────────────────────────────────────────────
            Loader {
                active: currentMode === "physics"
                source: "modules/physicsEditor/phys_Editor.qml"
                onStatusChanged: if (status === Loader.Error) console.error("phys_Editor.qml failed to load")
            }

            // ── 4: Audio Editor ───────────────────────────────────────────────
            Loader {
                active: currentMode === "audio"
                source: "modules/Audio/AudioEditor/audio_Editor.qml"
                onStatusChanged: if (status === Loader.Error) console.error("audio_Editor.qml failed to load")
            }

            // ── 5: Font Creator ───────────────────────────────────────────────
            Loader {
                active: currentMode === "fonts"
                source: "modules/font_KSFontCreator.qml"
                onStatusChanged: if (status === Loader.Error) console.error("font_KSFontCreator.qml failed to load")
            }

            // ── 6: License Plates ─────────────────────────────────────────────
            Loader {
                active: currentMode === "license"
                source: "modules/Car_LicensePlates.qml"
                onStatusChanged: if (status === Loader.Error) console.error("Car_LicensePlates.qml failed to load")
            }

            // ── 7: Display Editor ─────────────────────────────────────────────
            Loader {
                active: currentMode === "display"
                source: "modules/car_DisplayEditor.qml"
                onStatusChanged: if (status === Loader.Error) console.error("car_DisplayEditor.qml failed to load")
            }

            // ── 8: Assets Library ─────────────────────────────────────────────
            Loader {
                active: currentMode === "assets"
                source: "modules/AssetsLibrary.qml"
                onStatusChanged: if (status === Loader.Error) console.error("AssetsLibrary.qml failed to load")
            }

            // ── 9: AI Editor ──────────────────────────────────────────────────
            Loader {
                active: currentMode === "ai"
                source: "modules/ai_Editor.qml"
                onStatusChanged: if (status === Loader.Error) console.error("ai_Editor.qml failed to load")
            }

            // ── 10: Workshop ──────────────────────────────────────────────────
            Loader {
                active: currentMode === "workshop"
                source: "modules/workshop_Browser.qml"
                onStatusChanged: if (status === Loader.Error) console.error("workshop_Browser.qml failed to load")
            }

            // ── 11: Mod Manager ───────────────────────────────────────────────
            Loader {
                active: currentMode === "modmanager"
                source: "modules/mod_Manager.qml"
                onStatusChanged: if (status === Loader.Error) console.error("mod_Manager.qml failed to load")
            }

            // ── 12: Telemetry ─────────────────────────────────────────────────
            Loader {
                active: currentMode === "telemetry"
                source: "modules/car_telemetry_Viewer.qml"
                onStatusChanged: if (status === Loader.Error) console.error("car_telemetry_Viewer.qml failed to load")
            }

            // ── 13: Setup Editor ──────────────────────────────────────────────
            Loader {
                active: currentMode === "setup"
                source: "modules/car_setup_Editor.qml"
                onStatusChanged: if (status === Loader.Error) console.error("car_setup_Editor.qml failed to load")
            }

            // ── 14: PP Filters ────────────────────────────────────────────────
            Loader {
                active: currentMode === "ppfilters"
                source: "modules/PPFiltersEditor.qml"
                onStatusChanged: if (status === Loader.Error) console.error("PPFiltersEditor.qml failed to load")
            }

            // ── 15: Format Tools ─────────────────────────────────────────────
            Loader {
                active: currentMode === "formattools"
                source: "modules/format_Tools.qml"
                onStatusChanged: if (status === Loader.Error) console.error("format_Tools.qml failed to load")
            }

            // ── 16: CSP Config Editor ────────────────────────────────────────
            Loader {
                active: currentMode === "cspconfig"
                source: "modules/csp_Editor.qml"
                onStatusChanged: if (status === Loader.Error) console.error("csp_Editor.qml failed to load")
            }

            // ── 17: Character Builder ────────────────────────────────────────
            Loader {
                active: currentMode === "character"
                source: "modules/3DModeling/CharacterBuilder/character_Editor.qml"
                onStatusChanged: if (status === Loader.Error) console.error("character_Editor.qml failed to load")
            }

            // ── 18: 3D Modeler ───────────────────────────────────────────────
            Loader {
                active: currentMode === "modeler"
                source: "pages/page_ksModeler.qml"
                onStatusChanged: if (status === Loader.Error) console.error("page_ksModeler.qml failed to load")
            }

            // ── 19: Livery Editor ────────────────────────────────────────────
            Loader {
                active: currentMode === "livery"
                source: "pages/page_ksLiveryEditor.qml"
                onStatusChanged: if (status === Loader.Error) console.error("page_ksLiveryEditor.qml failed to load")
            }
        }

        // ── Display Editor request from C++ ribbon ──────────────────────────
        Connections {
            target: DisplayEditor
            function onEditorRequested() {
                switchTo("display")
            }
        }

        // ── Assets Library "open in modeler" ──────────────────────────────
        Connections {
            target: AssetsLibrary
            function onAssetOpenedInModeler(path) {
                switchTo("car")
            }
        }

        // ── Status bar ────────────────────────────────────────────────────────
        Rectangle {
            height: 24
            color: "#252526"
            Layout.fillWidth: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 4
                Text { text: "Ready"; color: "#E10600"; font.pixelSize: 10 }
                Item { Layout.fillWidth: true }
                Text { text: currentMode.toUpperCase(); color: "#555"; font.pixelSize: 10 }
                Text { text: "  |  ksEditor v1.0"; color: "#666"; font.pixelSize: 10 }
            }
        }
    }
}
