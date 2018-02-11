#ifndef SLIDER_HPP
#define SLIDER_HPP

#include <QSlider>
#include "export.h"

class LIBEXPORT Slider : public QSlider
{
	Q_OBJECT
public:
	Slider();

	inline bool ignoreValueChanged() const
	{
		return _ignoreValueChanged;
	}
public slots:
	void setValue(int val);
    inline void setMaximum(double max)
	{
        QAbstractSlider::setMaximum((int)max);
	}
	inline void setWheelStep(int ws)
	{
		wheelStep = ws;
	}
	void drawRange(int first, int second);
protected:
	void paintEvent(QPaintEvent *);
	void mousePressEvent(QMouseEvent *);
	void mouseReleaseEvent(QMouseEvent *);
	void mouseMoveEvent(QMouseEvent *);
	void wheelEvent(QWheelEvent *);
	void enterEvent(QEvent *);
private:
	int getMousePos(int X);

	bool canSetValue, _ignoreValueChanged;
	int lastMousePos, wheelStep, firstLine, secondLine;
signals:
	void mousePosition(int xPos);
};

#endif
