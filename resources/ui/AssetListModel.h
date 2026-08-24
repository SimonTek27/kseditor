#pragma once
#include <QAbstractListModel>
#include <QVector>
#include <QDir>
#include <QString>

struct AssetEntry {
    QString name;
    QString type;
    QString icon;
    QString path;
    QString size;
};

class AssetListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum AssetRoles {
        NameRole = Qt::UserRole + 1,
        TypeRole,
        IconRole,
        PathRole,
        SizeRole,
    };

    explicit AssetListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE QString assetPath(int row) const;

private:
    void scanAssets();
    void scanDir(const QDir& dir, const QString& relPath);
    static QString formatSize(qint64 bytes);

    QVector<AssetEntry> m_assets;
};
