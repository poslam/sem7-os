#pragma once

#include <QMainWindow>
#include <QVector>

class QLabel;
class QComboBox;
class QPushButton;
class QTableWidget;
class QTimer;
class QwtPlot;
class QwtPlotCurve;
class ApiClient;
struct Point;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onPoll();
    void onCurrent(double value, qint64 epochMs);
    void onStats(const QString& bucket, const QVector<Point>& pts);
    void onError(const QString& msg);
    void onLive();
    void onRefresh();

private:
    void setupUi();
    void requestData();
    qint64 nowMs() const;

    ApiClient* api_;
    QLabel* currentLabel_;
    QComboBox* rangeCombo_;
    QComboBox* bucketCombo_;
    QPushButton* liveBtn_;
    QPushButton* refreshBtn_;
    QTableWidget* table_;
    QwtPlot* plot_;
    QwtPlotCurve* curve_;
    QTimer* timer_;
    bool liveMode_ = true;
};
