#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QString>
#include <QByteArray>

namespace ks {

// ─────────────────────────────────────────────────────────────────────────────
// AcdManagerWidget — ACD extract/repack UI
// ─────────────────────────────────────────────────────────────────────────────
class AcdManagerWidget : public QWidget {
    Q_OBJECT
public:
    explicit AcdManagerWidget(QWidget* parent = nullptr);

    void setCarPath(const QString& path);

signals:
    void acdExtracted(const QString& folder);

private slots:
    void onExtract();
    void onRepack();

private:
    QString createKey(const QString& folderName) const;
    QByteArray decryptAcd(const QByteArray& data, const QString& key) const;
    QByteArray encryptAcd(const QByteArray& data, const QString& key) const;

    QString  m_carPath;
    QLabel*  m_acdStatus;
    QPushButton* m_extractBtn;
    QPushButton* m_repackBtn;
    QTextEdit*   m_logEdit;
};

} // namespace ks