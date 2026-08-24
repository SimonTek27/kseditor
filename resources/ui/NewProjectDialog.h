#pragma once
#include <QDialog>

class QVBoxLayout;
class QLabel;
class QWidget;
class QPushButton;
class QLineEdit;

class NewProjectDialog : public QDialog {
    Q_OBJECT
public:
    enum ProjectType { None, NewModel, NewPhysics, NewSound, NewSkin, NewFont, NewObject3D, NewShowroom };
    enum SubType { NoneSub, Car, Track, Character, Object3D, Showroom };

    ProjectType selectedType = None;
    SubType selectedSubType = NoneSub;
    QString projectName;
    QString projectPath;

    explicit NewProjectDialog(QWidget* parent = nullptr);

private slots:
    void onProjectTypeSelected();
    void onNameTextChanged(const QString& text);
    void onPreviousClicked();
    void onCreateClicked();

private:
    void setupUI();
    void createProjectStructure();
    void createIniFile(const QString& path, const QString& content);
    void showStep1();
    void applyTheme();
    QString detectAcRoot();
    QPushButton* createProjectTypeButton(const QString& text, const QString& desc, const QString& iconName, ProjectType type);

    QVBoxLayout* m_mainLayout;
    QLabel* m_header;
    QWidget* m_contentWidget;
    QWidget* m_titleBar;
    QPushButton* m_prevBtn;
    QPushButton* m_nextBtn;
    QLineEdit* m_nameEdit;
    QPoint m_dragPos;
    int m_currentStep = 1;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
};
