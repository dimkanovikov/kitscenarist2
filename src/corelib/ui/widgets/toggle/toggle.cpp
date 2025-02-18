#include "toggle.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/animations/click_animation.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/image_helper.h>

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QVariantAnimation>


class Toggle::Implementation
{
public:
    explicit Implementation(Toggle* _q);

    /**
     * @brief Размер контента
     */
    QSizeF contentSize() const;

    /**
     * @brief Анимировать переключение
     */
    void animateToggle();

    /**
     * @brief Прекратить аниацию, если виджет невидим
     */
    void finishAnimationIfInvisible();


    Toggle* q = nullptr;
    bool isChecked = true;
    QVariantAnimation tumblerAnimation;

    /**
     * @brief  Декорации переключателя при клике
     */
    ClickAnimation decorationAnimation;
};

Toggle::Implementation::Implementation(Toggle* _q)
    : q(_q)
{
    tumblerAnimation.setDuration(160);
    tumblerAnimation.setEasingCurve(QEasingCurve::OutQuad);
}

QSizeF Toggle::Implementation::contentSize() const
{
    return QSizeF(q->contentsMargins().left() + Ui::DesignSystem::layout().px12()
                      + Ui::DesignSystem::toggle().tumblerOverflow()
                      + Ui::DesignSystem::toggle().trackSize().width()
                      + Ui::DesignSystem::toggle().tumblerOverflow()
                      + Ui::DesignSystem::layout().px12() + q->contentsMargins().right(),
                  q->contentsMargins().top() + Ui::DesignSystem::layout().px12()
                      + Ui::DesignSystem::toggle().tumblerSize().height()
                      + Ui::DesignSystem::layout().px12() + q->contentsMargins().bottom());
}

void Toggle::Implementation::animateToggle()
{
    //
    // Определим крайние положения переключателя
    //

    const QRectF toggleLeftRect({ q->contentsMargins().left() + Ui::DesignSystem::layout().px12(),
                                  q->contentsMargins().top() + Ui::DesignSystem::layout().px12() },
                                Ui::DesignSystem::toggle().tumblerSize());
    const QRectF toggleRightRect({ q->contentsMargins().left() + Ui::DesignSystem::layout().px12()
                                       + Ui::DesignSystem::toggle().tumblerOverflow() * 2
                                       + Ui::DesignSystem::toggle().trackSize().width()
                                       - Ui::DesignSystem::toggle().tumblerSize().width(),
                                   toggleLeftRect.top() },
                                 toggleLeftRect.size());
    if ((isChecked && q->isLeftToRight()) || (!isChecked && q->isRightToLeft())) {
        tumblerAnimation.setStartValue(toggleLeftRect);
        tumblerAnimation.setEndValue(toggleRightRect);
    } else {
        tumblerAnimation.setStartValue(toggleRightRect);
        tumblerAnimation.setEndValue(toggleLeftRect);
    }
    tumblerAnimation.start();

    //
    // Если переключатель скрыт, то выполнять всю анимацию ни к чему
    //
    if (!q->isVisible()) {
        tumblerAnimation.setCurrentTime(tumblerAnimation.duration());
    }
}


// ****


Toggle::Toggle(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    setAttribute(Qt::WA_Hover);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    d->animateToggle();

    connect(&d->tumblerAnimation, &QVariantAnimation::valueChanged, this,
            qOverload<>(&Toggle::update));
    connect(&d->decorationAnimation, &ClickAnimation::valueChanged, this,
            qOverload<>(&Toggle::update));
}

bool Toggle::isChecked() const
{
    return d->isChecked;
}

Toggle::~Toggle() = default;

void Toggle::setChecked(bool _checked)
{
    if (d->isChecked == _checked) {
        return;
    }

    d->isChecked = _checked;
    d->animateToggle();
    emit checkedChanged(d->isChecked);
}

void Toggle::toggle()
{
    setChecked(!isChecked());
}

QSize Toggle::minimumSizeHint() const
{
    return d->contentSize().toSize();
}

bool Toggle::event(QEvent* _event)
{
    if (_event->type() == QEvent::ContentsRectChange
        || _event->type() == QEvent::LayoutDirectionChange) {
        d->animateToggle();
    }

    return Widget::event(_event);
}

void Toggle::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);
    painter.setPen(Qt::NoPen);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(_event->rect(), backgroundColor());

    const qreal opacity = isEnabled() ? 1.0 : Ui::DesignSystem::disabledTextOpacity();
    painter.setOpacity(opacity);

    const auto tumblerColor = d->isChecked
        ? Ui::DesignSystem::color().accent()
        : (ColorHelper::isColorLight(backgroundColor()) ? backgroundColor() : textColor());
    const auto trackColor
        = d->isChecked ? tumblerColor.lighter(140) : ColorHelper::nearby(tumblerColor, 160);
    //
    // Трэк
    //
    painter.setBrush(trackColor);
    const QRectF trackRect({ contentsMargins().left() + Ui::DesignSystem::layout().px12()
                                 + Ui::DesignSystem::toggle().tumblerOverflow(),
                             contentsMargins().top() + Ui::DesignSystem::layout().px12()
                                 + Ui::DesignSystem::toggle().tumblerOverflow() },
                           Ui::DesignSystem::toggle().trackSize());
    const QRectF tumblerRect = d->tumblerAnimation.currentValue().toRectF();
    //
    // Переключатель
    //
    painter.setBrush(tumblerColor);
    const QRectF toggleRect = d->tumblerAnimation.currentValue().toRectF();
    const qreal borderRadius = toggleRect.height() / 2.0;
    //
    // Определение фигур для отрисовки
    //
    QPainterPath tumblerPath;
    tumblerPath.addRoundedRect(tumblerRect, tumblerRect.height() / 2.0, tumblerRect.height() / 2.0);
    QPainterPath trackPath;
    trackPath.addRoundedRect(trackRect, trackRect.height() / 2.0, trackRect.height() / 2.0);
    trackPath = trackPath.subtracted(tumblerPath);
    painter.fillPath(trackPath, trackColor);
    //
    // ... подготовим тень
    //
    static QPixmap backgroundImage;
    if (backgroundImage.size() != toggleRect.size().toSize()) {
        backgroundImage = QPixmap(toggleRect.size().toSize());
        backgroundImage.fill(Qt::transparent);
        QPainter backgroundImagePainter(&backgroundImage);
        backgroundImagePainter.setPen(Qt::NoPen);
        backgroundImagePainter.setBrush(Ui::DesignSystem::color().textEditor());
        backgroundImagePainter.drawRoundedRect(QRect({ 0, 0 }, backgroundImage.size()),
                                               borderRadius, borderRadius);
    }

    if (!isEnabled()) {
        QRegion region(rect());
        QRegion tumblerRegion(tumblerPath.toFillPolygon().toPolygon());
        region = region.subtracted(tumblerRegion);
        painter.setClipRegion(region);
    }
    // ... рисуем тень
    const qreal shadowHeight = Ui::DesignSystem::card().minimumShadowBlurRadius();
    const bool useCache = true;
    QPixmap shadow = ImageHelper::dropShadow(backgroundImage, Ui::DesignSystem::card().shadowMargins(),
                                             shadowHeight, Ui::DesignSystem::color().shadow(), useCache);

    painter.drawPixmap(toggleRect.topLeft()
                           - QPointF{ Ui::DesignSystem::card().shadowMargins().left(),
                                      Ui::DesignSystem::card().shadowMargins().top() },
                       shadow);
    if (!isEnabled()) {
        painter.setClipRegion(QRegion());
    }

    //
    // ... рисуем декорацию
    //
    if (isEnabled()) {
        if (underMouse() || hasFocus()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(isChecked() ? Ui::DesignSystem::color().accent() : textColor());
            painter.setOpacity(hasFocus() ? Ui::DesignSystem::focusBackgroundOpacity()
                                          : Ui::DesignSystem::hoverBackgroundOpacity());
            const auto radius = d->decorationAnimation.maximumRadius();
            painter.drawEllipse(toggleRect.center(), radius, radius);
            painter.setOpacity(1.0);
        }
        if (d->decorationAnimation.state() == ClickAnimation::Running) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(isChecked() ? Ui::DesignSystem::color().accent() : textColor());
            painter.setOpacity(d->decorationAnimation.opacity());
            const auto radius = d->decorationAnimation.radius();
            painter.drawEllipse(toggleRect.center(), radius, radius);
            painter.setOpacity(1.0);
        }
    }
    painter.setBrush(tumblerColor);
    //
    // ... рисуем сам переключатель
    //
    painter.drawRoundedRect(toggleRect, toggleRect.width() / 2.0, toggleRect.width() / 2.0);
}

void Toggle::mousePressEvent(QMouseEvent* _event)
{
    Q_UNUSED(_event);
    d->decorationAnimation.start();
}

void Toggle::mouseReleaseEvent(QMouseEvent* _event)
{
    if (!rect().contains(_event->pos())) {
        return;
    }

    setChecked(!d->isChecked);
}

void Toggle::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event)

    d->decorationAnimation.setRadiusInterval(Ui::DesignSystem::toggle().tumblerSize().height()
                                                 / 2.0,
                                             Ui::DesignSystem::toggle().tumblerSize().height());

    updateGeometry();
    update();
}
