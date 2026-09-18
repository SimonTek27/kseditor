#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

class Kn5Previewer : public QObject {
    Q_OBJECT
public:
    explicit Kn5Previewer(QObject* parent = nullptr);
    Q_INVOKABLE void loadKn5(const QString& path);
    Q_PROPERTY(QString currentKn5Path READ currentKn5Path NOTIFY kn5PathChanged)
    Q_PROPERTY(QVariantList meshData READ meshData NOTIFY meshDataChanged)
    QString currentKn5Path() const { return m_path; }
    QVariantList meshData() const { return m_meshData; }
signals:
    void kn5PathChanged(const QString& path);
    void meshDataChanged(const QVariantList& data);
private:
    QString m_path;
    QVariantList m_meshData;
};
