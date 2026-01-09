#include "ApiClient.h"

#include <cmath>
#include <limits>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

ApiClient::ApiClient(QObject* parent)
    : QObject(parent),
      mgr_(new QNetworkAccessManager(this)) {}

void ApiClient::setBaseUrl(const QUrl& url) {
    baseUrl_ = url;
}

void ApiClient::fetchCurrent() {
    if (!baseUrl_.isValid()) return;
    QUrl url = baseUrl_;
    url.setPath("/api/current");
    QNetworkRequest req(url);
    auto reply = mgr_->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { handleReply(reply, true); });
}

void ApiClient::fetchStats(const QString& bucket, qint64 startMs, qint64 endMs) {
    if (!baseUrl_.isValid()) return;
    QUrl url = baseUrl_;
    url.setPath("/api/stats");
    QUrlQuery q;
    q.addQueryItem("bucket", bucket);
    q.addQueryItem("start", QString::number(startMs));
    q.addQueryItem("end", QString::number(endMs));
    url.setQuery(q);
    QNetworkRequest req(url);
    auto reply = mgr_->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { handleReply(reply, false); });
}

void ApiClient::handleReply(QNetworkReply* reply, bool isCurrent) {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        emit requestFailed(reply->errorString());
        return;
    }

    const auto bytes = reply->readAll();
    const auto doc = QJsonDocument::fromJson(bytes);
    if (doc.isNull()) {
        emit requestFailed(QStringLiteral("Invalid JSON"));
        return;
    }

    if (isCurrent) {
        const auto obj = doc.object();
        const auto v = obj.value("value").toDouble(std::numeric_limits<double>::quiet_NaN());
        const auto t = obj.value("epoch_ms").toVariant().toLongLong();
        if (std::isnan(v) || t == 0) {
            emit requestFailed(QStringLiteral("Invalid current payload"));
            return;
        }
        emit currentReceived(v, t);
        return;
    }

    QVector<Point> points;
    auto obj = doc.object();
    const auto data = obj.value("data");
    if (!data.isArray()) {
        emit requestFailed(QStringLiteral("Invalid stats payload"));
        return;
    }
    const auto arr = data.toArray();
    points.reserve(arr.size());
    for (const auto& item : arr) {
        if (item.isArray()) {
            auto pair = item.toArray();
            if (pair.size() >= 2) {
                const auto t = pair.at(0).toVariant().toLongLong();
                const auto v = pair.at(1).toDouble();
                points.push_back({t, v});
            }
        }
    }
    emit statsReceived(obj.value("bucket").toString(), points);
}
