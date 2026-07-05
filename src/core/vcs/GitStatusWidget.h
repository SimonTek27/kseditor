#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QSplitter>

#include "GitManager.h"

class GitStatusWidget : public QWidget {
    Q_OBJECT
public:
    explicit GitStatusWidget(QWidget* parent = nullptr);

    void setRepoPath(const QString& path);
    QString repoPath() const { return m_repoPath; }
    void refresh();

signals:
    void openFileRequested(const QString& filePath, int line = 0);

private slots:
    void onStatusReady(const QList<GitFileStatus>& files);
    void onDiffReady(const QString& filePath, const QString& diff);
    void onCommitDone(bool success, const QString& output);
    void onItemClicked(QTreeWidgetItem* item, int column);
    void onStageClicked();
    void onUnstageClicked();
    void onCommitClicked();
    void onRefreshClicked();
    void onBranchChanged(int index);

private:
    void setupUI();
    void updateBranchCombo();
    void updateFileTree(const QList<GitFileStatus>& files);

    GitManager* m_git;
    QString m_repoPath;

    QComboBox* m_branchCombo;
    QPushButton* m_refreshBtn;
    QTreeWidget* m_fileTree;
    QTextEdit* m_diffView;
    QPushButton* m_stageBtn;
    QPushButton* m_unstageBtn;
    QLineEdit* m_commitMsg;
    QPushButton* m_commitBtn;
    QLabel* m_statusLabel;

    QList<GitFileStatus> m_currentFiles;
};
