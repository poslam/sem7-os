#include "MainWindow.h"

#include "ApiClient.h"

#include <QComboBox>
#include <QCursor>
#include <QDateTime>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>
#include <QPen>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QScreen>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      api_(new ApiClient(this)),
      timer_(new QTimer(this)) {
    api_->setBaseUrl(QUrl(QStringLiteral("http://localhost:8080")));
    setupUi();
    setWindowFlag(Qt::FramelessWindowHint, true);
    setWindowFlag(Qt::WindowStaysOnTopHint, true);
    setCursor(Qt::BlankCursor);
    if (auto screen = QGuiApplication::primaryScreen()) {
        const auto geom = screen->geometry();
        setGeometry(geom);
        setMinimumSize(geom.size());
    } else {
        setGeometry(0, 0, 1920, 1080);
        setMinimumSize(1920, 1080);
    }

    connect(api_, &ApiClient::currentReceived, this, &MainWindow::onCurrent);
    connect(api_, &ApiClient::statsReceived, this, &MainWindow::onStats);
    connect(api_, &ApiClient::requestFailed, this, &MainWindow::onError);

    timer_->setInterval(1000);
    connect(timer_, &QTimer::timeout, this, &MainWindow::onPoll);
    timer_->start();

    requestData();
}

void MainWindow::setupUi() {
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);

    auto* topRow = new QHBoxLayout();
    currentLabel_ = new QLabel(QStringLiteral("— °C"), this);
    auto* currentTitle = new QLabel(QStringLiteral("Текущее"), this);
    auto* currentBox = new QVBoxLayout();
    currentBox->addWidget(currentTitle);
    currentBox->addWidget(currentLabel_);
    topRow->addLayout(currentBox);

    rangeCombo_ = new QComboBox(this);
    rangeCombo_->addItem("1 час", QVariant::fromValue<qint64>(60LL * 60 * 1000));
    rangeCombo_->addItem("24 часа", QVariant::fromValue<qint64>(24LL * 60 * 60 * 1000));
    rangeCombo_->addItem("30 дней", QVariant::fromValue<qint64>(30LL * 24 * 60 * 60 * 1000));

    bucketCombo_ = new QComboBox(this);
    bucketCombo_->addItem("Сырые", "raw");
    bucketCombo_->addItem("Почасовые", "hourly");
    bucketCombo_->addItem("Дневные", "daily");

    liveBtn_ = new QPushButton("Live", this);
    liveBtn_->setCheckable(true);
    liveBtn_->setChecked(true);
    refreshBtn_ = new QPushButton("Обновить", this);

    topRow->addWidget(new QLabel("Диапазон:", this));
    topRow->addWidget(rangeCombo_);
    topRow->addWidget(new QLabel("Агрегация:", this));
    topRow->addWidget(bucketCombo_);
    topRow->addWidget(liveBtn_);
    topRow->addWidget(refreshBtn_);
    topRow->addStretch(1);
    layout->addLayout(topRow);

    plot_ = new QwtPlot(this);
    plot_->setCanvasBackground(Qt::black);
    curve_ = new QwtPlotCurve();
    curve_->setPen(QPen(Qt::cyan, 2));
    curve_->attach(plot_);
    layout->addWidget(plot_, 3);

    table_ = new QTableWidget(this);
    table_->setColumnCount(2);
    table_->setHorizontalHeaderLabels({"Время", "Значение"});
    table_->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(table_, 2);

    setCentralWidget(central);
    setWindowTitle(QStringLiteral("Lab7 Kiosk"));

    connect(liveBtn_, &QPushButton::clicked, this, &MainWindow::onLive);
    connect(refreshBtn_, &QPushButton::clicked, this, &MainWindow::onRefresh);
    connect(rangeCombo_, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this](int) { onRefresh(); });
    connect(bucketCombo_, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this](int) { onRefresh(); });
}

void MainWindow::onPoll() {
    if (liveMode_) {
        requestData();
    }
}

void MainWindow::onCurrent(double value, qint64 epochMs) {
    const auto dt = QDateTime::fromMSecsSinceEpoch(epochMs);
    currentLabel_->setText(QString("%1 °C @ %2").arg(value, 0, 'f', 2).arg(dt.toString("dd.MM HH:mm:ss")));
}

void MainWindow::onStats(const QString& bucket, const QVector<Point>& pts) {
    Q_UNUSED(bucket);
    QVector<QPointF> poly;
    poly.reserve(pts.size());
    for (const auto& p : pts) {
        poly.push_back(QPointF(p.t, p.v));
    }
    curve_->setSamples(poly);
    plot_->replot();

    table_->setRowCount(static_cast<int>(pts.size()));
    int row = 0;
    for (auto it = pts.crbegin(); it != pts.crend(); ++it, ++row) {
        const auto dt = QDateTime::fromMSecsSinceEpoch(it->t).toString("dd.MM HH:mm:ss");
        table_->setItem(row, 0, new QTableWidgetItem(dt));
        table_->setItem(row, 1, new QTableWidgetItem(QString::number(it->v, 'f', 2)));
    }
}

void MainWindow::onError(const QString& msg) {
    QMessageBox::warning(this, tr("Ошибка запроса"), msg);
}

void MainWindow::onLive() {
    liveMode_ = liveBtn_->isChecked();
    if (liveMode_) {
        requestData();
    }
}

void MainWindow::onRefresh() {
    requestData();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    event->ignore();
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    const auto mods = event->modifiers();
    if ((event->key() == Qt::Key_F4 && mods.testFlag(Qt::AltModifier)) ||
        (event->key() == Qt::Key_Tab && mods.testFlag(Qt::AltModifier)) ||
        (event->key() == Qt::Key_Escape) ||
        (event->key() == Qt::Key_Q && mods.testFlag(Qt::ControlModifier))) {
        event->accept();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::requestData() {
    api_->fetchCurrent();
    const auto now = nowMs();
    const auto span = rangeCombo_->currentData().toLongLong();
    const auto start = now - span;
    const auto bucket = bucketCombo_->currentData().toString();
    api_->fetchStats(bucket, start, now);
}

qint64 MainWindow::nowMs() const {
    return QDateTime::currentMSecsSinceEpoch();
}
