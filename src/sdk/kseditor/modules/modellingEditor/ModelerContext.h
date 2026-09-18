#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>

namespace ks {

class ModelerContext : public QObject
{
    Q_OBJECT
public:
    static ModelerContext* instance();

    enum EditorType { TypeCar, TypeTrack, TypeCharacter };
    enum EditMode { ModeSelect, ModeEdit, ModePaint, ModeAnimate };

    void setEditorType(EditorType type) { m_editorType = type; emit editorTypeChanged(type); }
    EditorType editorType() const { return m_editorType; }

    void setEditMode(EditMode mode) { m_editMode = mode; emit editModeChanged(mode); }
    EditMode editMode() const { return m_editMode; }

    void setCurrentTool(const QString& tool) { m_currentTool = tool; emit toolChanged(tool); }
    QString currentTool() const { return m_currentTool; }

    void setActiveObject(const QString& id) { m_activeObject = id; emit activeObjectChanged(id); }
    QString activeObject() const { return m_activeObject; }

    QStringList getToolsForType(EditorType type) const;
    QStringList getToolsForMode(EditMode mode) const;
    bool isToolValid(const QString& tool) const;

signals:
    void editorTypeChanged(EditorType type);
    void editModeChanged(EditMode mode);
    void toolChanged(const QString& tool);
    void activeObjectChanged(const QString& id);

private:
    explicit ModelerContext(QObject* parent = nullptr);
    static ModelerContext* s_instance;

    EditorType m_editorType = TypeCar;
    EditMode m_editMode = ModeSelect;
    QString m_currentTool;
    QString m_activeObject;

    QMap<EditorType, QStringList> m_toolsByType;
    QMap<EditMode, QStringList> m_toolsByMode;
};

} // namespace ks
