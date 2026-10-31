#pragma once
#include <QWidget>
#include <QPainter>
#include <QMouseEvent>

// implementation of TrimSlider in VideoPlayer.cpp

// TODO: middle click to drag and pan around
class TrimSlider : public QWidget {
  Q_OBJECT
public:
  TrimSlider(QWidget* parent = nullptr) : QWidget(parent) {
    setMinimumHeight(48);
  }

  void setDuration(qint64 ms) {
    m_duration = qMax(0, ms);
    m_start = qBound(0, m_start, m_duration);
    m_end = qBound(m_start, m_end, m_duration);
    m_zoom = 1.0;
    m_viewStart = 0;

    zoomUpdate();
    update();
  }
  
  void setPosition(qint64 ms) {
    m_position = qBound(0, ms, m_duration);
    update();
  }

  void setRange(qint64 start, qint64 end) {
    m_start = qBound(0, start, m_duration);
    m_end = qBound(m_start, end, m_duration);
    emit rangeChanged(m_start, m_end);
    update();
  }

  qint64 start() const { return m_start; }
  qint64 end() const { return m_end; }

  double zoom() const { return m_zoom; }
  qint64 viewStart() const { return m_viewStart; }
  qint64 viewEnd() const { return m_viewEnd; }

signals:
  void sliderMoved(qint64 pos);
  void rangeChanged(qint64 start, qint64 end);


private:
  qint64 m_duration = 0;
  qint64 m_position = 0;
  qint64 m_start = 0;
  qint64 m_end = 0;
  enum class Drag { None, Start, End };
  Drag m_drag = Drag::None;

  double m_zoom = 1.0;
  qint64 m_viewStart = 0;
  qint64 m_viewEnd = 0;

  double time2x(qint64 ms) const {
    const double left = 12;
    const double right = width()-12;
    const double trackW = right-left;
    if (m_viewEnd <= m_viewStart) return left;
    return left + trackW * (ms-m_viewStart) / double(m_viewEnd-m_viewStart);
  }

  qint64 x2time(double x) const {
    if (m_viewEnd <= m_viewStart) return m_viewStart;
    const double left = 12;
    const double right= width()-12;
    const double frac = qBound<double>(0, (x-left)/(right-left), 1);
    return m_viewStart + frac * double(m_viewEnd-m_viewStart);
  }

  void zoomUpdate() {
    if (m_duration <= 0) {
      m_viewStart = m_viewEnd = 0;
      return;
    }
    m_zoom = qBound(1.0, m_zoom, qMax(1.0, double(m_duration)/50.0));
    
    qint64 viewLen = qMax(1.0, m_duration/m_zoom);
    viewLen = qMin(viewLen, m_duration);
    m_viewStart = qBound(0, m_viewStart, m_duration-viewLen);
    m_viewEnd   = m_viewStart + viewLen;
  }

  void zoomAt(double fact, double x) {
    const double left = 12;
    const double right= width()-12;
    const double frac = qBound<double>(0, (x-left)/(right-left), 1);

    const double anchor = x2time(x);
    m_zoom = qBound(1.0, m_zoom*fact, qMax(1.0, double(m_duration)/50.0));
    qint64 viewLen = qMax(1.0, m_duration/m_zoom);
    viewLen = qMin(viewLen, m_duration);
    m_viewStart = anchor - frac * viewLen;
    zoomUpdate();
    update();
  }


  
  void paintEvent(QPaintEvent*) override {
    // https://doc.qt.io/qt-6/qtwidgets-painting-basicdrawing-example.html
    QPainter p(this);
    const QPalette& pal = palette();

    const double left = 12;
    const double right = width()-12;
    const double trackY = height()/2.0;
    const double trackW = right-left;
    const double trackH = 16;

    // background track
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(pal.color(QPalette::Midlight)));
    p.drawRect(QRectF(left, trackY - trackH / 2, trackW, trackH));
    
    if (m_duration <= 0) return;
    const double xStart = time2x(m_start);
    const double xEnd = time2x(m_end);
    const double xPlay = time2x(m_position);

    // selected trim track
    p.setBrush(QColor(pal.color(QPalette::Highlight)));
    p.drawRect(QRectF(xStart, trackY - trackH/2, qMax<double>(0, xEnd-xStart), trackH));

    
    auto drawHandle = [&](double x, const QColor& color) {
      p.setBrush(color);
      p.drawRect(QRectF(x-4, trackY-15, 8, 30));
    };

    drawHandle(xStart, QColor(pal.color(QPalette::Text)));
    drawHandle(xEnd, QColor(pal.color(QPalette::Text)));

    // playhead
    p.setPen(QPen(pal.color(QPalette::BrightText), 2));
    p.drawLine(QPointF(xPlay, trackY-19), QPointF(xPlay, trackY+19));
  }

  //
  void wheelEvent(QWheelEvent* evt) override {
    if (evt->angleDelta().y() > 0)
      zoomAt(1.25, evt->position().x());
    else if (evt->angleDelta().y() < 0)
      zoomAt(1.0/1.25, evt->position().x());
    
    evt->accept();
  }

  void mousePressEvent(QMouseEvent* evt) override {
    if (m_duration <= 0) return;
    const double x = evt->position().x();
    const double xStart = time2x(m_start);
    const double xEnd =   time2x(m_end);

    if (qAbs(x-xStart) <= 10) {
      m_drag = Drag::Start;
    } else if (qAbs(x-xEnd) <= 10) {
      m_drag = Drag::End;
    } else {
      m_drag = Drag::None;
      emit sliderMoved(x2time(x));
    }
  }

  void mouseMoveEvent(QMouseEvent* evt) override {
    if (m_drag != Drag::None) {
      const qint64 ms = x2time(evt->position().x());
      if (m_drag == Drag::Start) m_start = qBound(0, ms, m_end);
      else if (m_drag == Drag::End) m_end = qBound(m_start, ms, m_duration);
      else return;
      
      update();
      emit rangeChanged(m_start, m_end);
    }
  }

  void mouseReleaseEvent(QMouseEvent* evt) override {
    m_drag = Drag::None;
    evt->accept();
  }
};
