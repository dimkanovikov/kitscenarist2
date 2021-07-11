#include "CircleTransparentAnimator.h"

#include "CircleTransparentDecorator.h"

#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

#include <cmath>

using WAF::CircleTransparentAnimator;
using WAF::CircleTransparentDecorator;


CircleTransparentAnimator::CircleTransparentAnimator(QWidget* _widgetForFill)
    : AbstractAnimator(_widgetForFill)
    , m_decorator(new CircleTransparentDecorator(_widgetForFill))
    , m_animation(new QPropertyAnimation(m_decorator, "radius"))
{
    Q_ASSERT(_widgetForFill);

    m_animation->setEasingCurve(QEasingCurve::OutQuart);
    m_animation->setDuration(420);

    m_decorator->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_decorator->hide();

    connect(m_animation, &QPropertyAnimation::finished, this, &CircleTransparentAnimator::finalize);
}

void CircleTransparentAnimator::setStartPoint(const QPoint& _globalPoint)
{
    m_decorator->setStartPoint(_globalPoint);
}

void CircleTransparentAnimator::setFillImage(const QPixmap& _image)
{
    m_decorator->setFillImage(_image);
}

void CircleTransparentAnimator::setHideAfterFinish(bool _hide)
{
    m_hideAfterFinish = _hide;
}

int CircleTransparentAnimator::animationDuration() const
{
    return m_animation->duration();
}

void CircleTransparentAnimator::animateForward()
{
    fillIn();
}

void CircleTransparentAnimator::fillIn()
{
    //
    // Прерываем выполнение, если клиент хочет повторить его
    //
    if (isAnimated() && isAnimatedForward())
        return;
    setAnimatedForward();

    //
    // Определим стартовые и финальные позиции для декораций
    //
    const int startRadius = 0;
    //
    const QPoint startPoint = widgetForFill()->mapFromGlobal(m_decorator->startPoint());
    const int w = qMax(startPoint.x(), widgetForFill()->width() - startPoint.x());
    const int h = qMax(startPoint.y(), widgetForFill()->height() - startPoint.y());
    const int finalRadius = sqrt(w * w + h * h);

    //
    // Позиционируем декораторы
    //
    m_decorator->move(0, 0);
    m_decorator->show();
    m_decorator->raise();
    //
    // ... и принудительно его рисуем, чтобы отрисовка декоратора произошла как можно раньше
    //
    m_decorator->repaint();

    //
    // Анимируем виджет
    //
    if (m_animation->state() == QPropertyAnimation::Running) {
        //
        // ... если ещё не закончилась предыдущая анимация реверсируем её
        //
        m_animation->pause();
        m_animation->setDirection(QPropertyAnimation::Backward);
        m_animation->resume();
    } else {
        //
        // ... если предыдущая анимация закончилась, запускаем новую анимацию
        //
        m_animation->setEasingCurve(QEasingCurve::OutQuart);
        m_animation->setDirection(QPropertyAnimation::Forward);
        m_animation->setStartValue(startRadius);
        m_animation->setEndValue(finalRadius);
        m_animation->start();
    }
}

void CircleTransparentAnimator::animateBackward()
{
    fillOut();
}

void CircleTransparentAnimator::fillOut()
{
    //
    // Прерываем выполнение, если клиент хочет повторить его
    //
    if (isAnimated() && isAnimatedBackward())
        return;
    setAnimatedBackward();

    //
    // Определим стартовые и финальные позиции для декораций
    //
    int startRadius = sqrt(widgetForFill()->height() * widgetForFill()->height()
                           + widgetForFill()->width() * widgetForFill()->width());
    int finalRadius = 0;

    //
    // Позиционируем декораторы
    //
    m_decorator->move(0, 0);
    m_decorator->show();
    m_decorator->raise();

    //
    // Анимируем виджет
    //
    if (m_animation->state() == QPropertyAnimation::Running) {
        //
        // ... если ещё не закончилась предыдущая анимация реверсируем её
        //
        m_animation->pause();
        m_animation->setDirection(QPropertyAnimation::Backward);
        m_animation->resume();
    } else {
        //
        // ... если предыдущая анимация закончилась, запускаем новую анимацию
        //
        m_animation->setEasingCurve(QEasingCurve::OutQuart);
        m_animation->setDirection(QPropertyAnimation::Forward);
        m_animation->setStartValue(startRadius);
        m_animation->setEndValue(finalRadius);
        m_animation->start();
    }
}

void CircleTransparentAnimator::finalize()
{
    setAnimatedStopped();
    if (m_hideAfterFinish) {
        m_decorator->hide();
    }
}

QWidget* CircleTransparentAnimator::widgetForFill() const
{
    return qobject_cast<QWidget*>(parent());
}
