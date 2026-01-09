#pragma once

#include <QObject>
#include <QUrl>
#include <QVector>

struct QNetworkReply;

struct Point {
    qint64 t;
    double v;
};

class ApiClient : public QObject {
    Q_OBJECT
public:
    explicit ApiClient(QObject* parent = nullptr);

    void setBaseUrl(const QUrl& url);
    void fetchCurrent();
    void fetchStats(const QString& bucket, qint64 startMs, qint64 endMs);

signals:
    void currentReceived(double value, qint64 epochMs);
    void statsReceived(const QString& bucket, const QVector<Point>& points);
    void requestFailed(const QString& message);

private:
    void handleReply(QNetworkReply* reply, bool isCurrent);

    QUrl baseUrl_;
    class QNetworkAccessManager* mgr_;
};
