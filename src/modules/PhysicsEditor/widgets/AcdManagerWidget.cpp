#include "AcdManagerWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>

namespace ks {

AcdManagerWidget::AcdManagerWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    m_acdStatus = new QLabel(tr("No ACD file detected"), this);
    m_acdStatus->setStyleSheet("font-weight: bold; color: gray;");

    m_extractBtn = new QPushButton(tr("Extract ACD"), this);
    m_repackBtn  = new QPushButton(tr("Repack ACD"), this);
    m_logEdit    = new QTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumHeight(120);
    m_logEdit->setFont(QFont("Consolas", 9));

    auto* btnRow = new QHBoxLayout;
    btnRow->addWidget(m_extractBtn);
    btnRow->addWidget(m_repackBtn);

    layout->addWidget(new QLabel(tr("ACD Archive Manager"), this));
    layout->addWidget(m_acdStatus);
    layout->addLayout(btnRow);
    layout->addWidget(new QLabel(tr("Log:"), this));
    layout->addWidget(m_logEdit);

    connect(m_extractBtn, &QPushButton::clicked, this, &AcdManagerWidget::onExtract);
    connect(m_repackBtn, &QPushButton::clicked, this, &AcdManagerWidget::onRepack);
}

void AcdManagerWidget::setCarPath(const QString& path) {
    m_carPath = path;
    QFileInfo acd(path + "/data/acd_0.accd");
    if (acd.exists()) {
        m_acdStatus->setText(tr("ACD found: %1").arg(acd.fileName()));
        m_acdStatus->setStyleSheet("font-weight: bold; color: green;");
        m_extractBtn->setEnabled(true);
        m_repackBtn->setEnabled(true);
    } else {
        m_acdStatus->setText(tr("No ACD file detected"));
        m_acdStatus->setStyleSheet("font-weight: bold; color: gray;");
        m_extractBtn->setEnabled(false);
        m_repackBtn->setEnabled(false);
    }
}

QString AcdManagerWidget::createKey(const QString& folderName) const {
    QString lower = folderName.toLower();
    int sum = 0;
    for (QChar c : lower) sum += c.unicode();

    int octet1 = ((sum % 256) + 256) % 256;

    int n = lower.length();
    int temp = 0;
    for (int i = 0; i < n; ++i) {
        temp += lower[i].unicode() * (i + 1);
    }
    int octet2 = ((temp % 256) + 256) % 256;

    temp = 0;
    for (int i = 0; i < n; ++i) {
        temp += lower[i].unicode() * (n - i);
    }
    int octet3 = ((temp % 256) + 256) % 256;

    temp = 5763;
    for (int i = 0; i < n; ++i) {
        temp += lower[i].unicode() * (i + 1);
    }
    int octet4 = ((temp % 256) + 256) % 256;

    temp = 66;
    for (int i = 0; i < n; ++i) {
        temp += lower[i].unicode() * (n + i + 1);
    }
    int octet5 = ((temp % 256) + 256) % 256;

    temp = 101;
    for (int i = 0; i < n; ++i) {
        temp += lower[i].unicode() * (n - i + 1);
    }
    int octet6 = ((temp % 256) + 256) % 256;

    temp = 171;
    for (int i = 0; i < n; ++i) {
        temp += lower[i].unicode() * (i + 2);
    }
    int octet7 = ((temp % 256) + 256) % 256;

    temp = 171;
    for (int i = 0; i < n; ++i) {
        temp += lower[i].unicode() * (i + 3);
    }
    int octet8 = ((temp % 256) + 256) % 256;

    return QString("%1-%2-%3-%4-%5-%6-%7-%8")
        .arg(octet1).arg(octet2).arg(octet3).arg(octet4)
        .arg(octet5).arg(octet6).arg(octet7).arg(octet8);
}

QByteArray AcdManagerWidget::decryptAcd(const QByteArray& data, const QString& key) const {
    QStringList parts = key.split('-');
    if (parts.size() != 8) return data;

    QVector<int> keyBytes;
    for (const QString& p : parts) keyBytes.append(p.toInt());

    QByteArray result = data;
    int dataLen = data.size();
    int keyLen = keyBytes.size();

    for (int i = 0; i < dataLen; ++i) {
        int rot = keyBytes[i % keyLen];
        int val = (int)(unsigned char)data[i];
        val = ((val - rot) % 256 + 256) % 256;
        result[i] = (char)val;
    }
    return result;
}

QByteArray AcdManagerWidget::encryptAcd(const QByteArray& data, const QString& key) const {
    QStringList parts = key.split('-');
    if (parts.size() != 8) return data;

    QVector<int> keyBytes;
    for (const QString& p : parts) keyBytes.append(p.toInt());

    QByteArray result = data;
    int dataLen = data.size();
    int keyLen = keyBytes.size();

    for (int i = 0; i < dataLen; ++i) {
        int rot = keyBytes[i % keyLen];
        int val = (int)(unsigned char)data[i];
        val = ((val + rot) % 256 + 256) % 256;
        result[i] = (char)val;
    }
    return result;
}

void AcdManagerWidget::onExtract() {
    if (m_carPath.isEmpty()) return;

    QString folderName = QFileInfo(m_carPath).fileName();
    QString key = createKey(folderName);

    // Try standard data.acd first, then acd_0.accd as fallback
    QString acdPath = m_carPath + "/data/data.acd";
    if (!QFile::exists(acdPath)) {
        acdPath = m_carPath + "/data/acd_0.accd";
    }
    QFile file(acdPath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_logEdit->append(tr("[ERROR] Cannot open ACD file"));
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QByteArray decrypted = decryptAcd(data, key);

    QString outDir = m_carPath + "/data_extracted";
    if (!QDir().mkpath(outDir)) {
        m_logEdit->append(tr("[ERROR] Failed to create directory: %1").arg(outDir));
        return;
    }

    QFile out(outDir + "/decrypted.bin");
    if (out.open(QIODevice::WriteOnly)) {
        out.write(decrypted);
        out.close();
    }

    m_logEdit->append(tr("[OK] ACD extracted to: %1").arg(outDir));
    emit acdExtracted(m_carPath);
}

void AcdManagerWidget::onRepack() {
    if (m_carPath.isEmpty()) return;

    QString folderName = QFileInfo(m_carPath).fileName();
    QString key = createKey(folderName);

    QString inFile = m_carPath + "/data_extracted/decrypted.bin";
    QFile file(inFile);
    if (!file.open(QIODevice::ReadOnly)) {
        m_logEdit->append(tr("[ERROR] Cannot open extracted file"));
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QByteArray encrypted = encryptAcd(data, key);

    QString acdPath = m_carPath + "/data/acd_0.accd";
    QFile out(acdPath);
    if (out.open(QIODevice::WriteOnly)) {
        out.write(encrypted);
        out.close();
        m_logEdit->append(tr("[OK] ACD repacked"));
    } else {
        m_logEdit->append(tr("[ERROR] Cannot write ACD file"));
    }
}

} // namespace ks