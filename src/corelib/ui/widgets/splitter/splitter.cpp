#include "splitter.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>

#include <QAction>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QVariantAnimation>


class Splitter::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Получить список размеров виджетов в зависимости от ориентации разделителя
     */
    QVector<int> widgetsSizes() const;

    /**
     * @brief Изменить размер, сохраняя заданные пропорции
     */
    void resize(Splitter* _splitter, const QVector<qreal>& _sizes);

    /**
     * @brief Анимировать кнопку отображения тулбара
     */
    void animateShowHiddenPanelToolbar(const QPointF& _finalPosition);

    Qt::Orientation orientation = Qt::Horizontal;
    Widget* handle = nullptr;
    QVector<qreal> sizes;
    QVector<QWidget*> widgets;

    FloatingToolBar* showHiddenPanelToolbar = nullptr;
    QAction* showLeftPanelAction = nullptr;
    QAction* showRightPanelAction = nullptr;

    QVariantAnimation showHiddenPanelToolbarPositionAnimation;
};

Splitter::Implementation::Implementation(QWidget* _parent)
    : handle(new Widget(_parent))
    , showHiddenPanelToolbar(new FloatingToolBar(_parent))
    , showLeftPanelAction(new QAction(showHiddenPanelToolbar))
    , showRightPanelAction(new QAction(showHiddenPanelToolbar))
{
    handle->setBackgroundColor(Qt::transparent);
    handle->setCursor(Qt::SplitHCursor);

    showHiddenPanelToolbar->addAction(showRightPanelAction);
    showHiddenPanelToolbar->addAction(showLeftPanelAction);
    showHiddenPanelToolbar->hide();
    showLeftPanelAction->setIconText(u8"\U000F0142");
    showRightPanelAction->setIconText(u8"\U000F0141");

    showHiddenPanelToolbarPositionAnimation.setEasingCurve(QEasingCurve::OutQuad);
    showHiddenPanelToolbarPositionAnimation.setDuration(240);
}

QVector<int> Splitter::Implementation::widgetsSizes() const
{
    QVector<int> sizes;
    for (auto widget : widgets) {
        sizes.append(orientation == Qt::Horizontal ? widget->width() : widget->height());
    }
    return sizes;
}

void Splitter::Implementation::resize(Splitter* _splitter, const QVector<qreal>& _sizes)
{
    //
    // ормируем список новых размеров
    //
    QVector<int> newSizes;
    for (auto& size : _sizes) {
        newSizes.append(size * 1000);
    }
    //
    // Применяем их
    //
    _splitter->setSizes(newSizes);
    //
    // Восстанавливаем значение пропорций
    //
    sizes = _sizes;
}

void Splitter::Implementation::animateShowHiddenPanelToolbar(const QPointF& _finalPosition)
{
    //
    // Показать
    //
    if (!_finalPosition.isNull()) {
        if (showHiddenPanelToolbarPositionAnimation.direction() == QVariantAnimation::Forward
            && showHiddenPanelToolbarPositionAnimation.endValue() == _finalPosition) {
            return;
        }

        showHiddenPanelToolbarPositionAnimation.setDirection(QVariantAnimation::Forward);
        const QPointF startPosition(_finalPosition.x() < 0 ? -1 * showHiddenPanelToolbar->width()
                                                           : _finalPosition.x()
                                            + (showHiddenPanelToolbar->width() / 2),
                                    _finalPosition.y());
        showHiddenPanelToolbarPositionAnimation.setStartValue(startPosition);
        showHiddenPanelToolbarPositionAnimation.setEndValue(_finalPosition);

        showHiddenPanelToolbar->move(
            showHiddenPanelToolbarPositionAnimation.startValue().toPointF().toPoint());
        showHiddenPanelToolbar->show();
    }
    //
    // Скрыть
    //
    else {
        if ((showHiddenPanelToolbarPositionAnimation.direction() == QVariantAnimation::Backward
             && showHiddenPanelToolbarPositionAnimation.state() == QVariantAnimation::Running)
            || !showHiddenPanelToolbar->isVisible()) {
            return;
        }

        showHiddenPanelToolbarPositionAnimation.setDirection(QVariantAnimation::Backward);
        const QPointF startPosition(showHiddenPanelToolbar->pos().x() < 0
                                        ? -1 * showHiddenPanelToolbar->width()
                                        : showHiddenPanelToolbar->pos().x()
                                            + (showHiddenPanelToolbar->width() / 2),
                                    showHiddenPanelToolbar->pos().y());
        showHiddenPanelToolbarPositionAnimation.setStartValue(startPosition);
        showHiddenPanelToolbarPositionAnimation.setEndValue(QPointF(showHiddenPanelToolbar->pos()));
    }

    showHiddenPanelToolbarPositionAnimation.start();
}


// ****


Splitter::Splitter(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    d->handle->installEventFilter(this);

    connect(d->showLeftPanelAction, &QAction::triggered, this, [this] { setSizes({ 2, 7 }); });
    connect(d->showRightPanelAction, &QAction::triggered, this, [this] { setSizes({ 7, 2 }); });
    connect(&d->showHiddenPanelToolbarPositionAnimation, &QVariantAnimation::valueChanged, this,
            [this] {
                d->showHiddenPanelToolbar->move(
                    d->showHiddenPanelToolbarPositionAnimation.currentValue().toPoint());
            });
    connect(&d->showHiddenPanelToolbarPositionAnimation, &QVariantAnimation::finished, this,
            [this] {
                if (d->showHiddenPanelToolbarPositionAnimation.direction()
                    == QVariantAnimation::Backward) {
                    d->showHiddenPanelToolbar->hide();
                }
            });

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

Splitter::~Splitter() = default;

void Splitter::setWidgets(QWidget* _first, QWidget* _second)
{
    if (d->widgets.contains(_first) && d->widgets.contains(_second)) {
        return;
    }

    if (!d->widgets.isEmpty()) {
        qDeleteAll(d->widgets);
        d->widgets.clear();
    }

    //
    // Сохраняем виджеты
    //
    auto addWidget = [this](QWidget* _widget) {
        d->widgets.append(_widget);
        if (_widget->parent() != this) {
            _widget->setParent(this);
        }
        _widget->installEventFilter(this);
    };
    addWidget(_first);
    addWidget(_second);

    //
    // Настроим дефолтный размер
    //
    setSizes({ _first->isVisible() ? 1 : 0, _second->isVisible() ? 1 : 0 });

    //
    // Поднимем хендл, чтобы не терять управление
    //
    d->handle->raise();
    d->showHiddenPanelToolbar->raise();
}

void Splitter::setSizes(const QVector<int>& _sizes)
{
    if (topLevelWidget()->isMinimized()) {
        return;
    }

    Q_ASSERT(d->widgets.size() == _sizes.size());

    //
    // Автоматом распределяем пространство между вложенными виджетами
    //
    d->sizes.clear();
    const auto sizesMax = std::accumulate(_sizes.begin(), _sizes.end(), 0);
    for (auto size : _sizes) {
        d->sizes.append(static_cast<qreal>(size) / sizesMax);
    }
    const auto sizeDelta = static_cast<qreal>(width()) / sizesMax;
    auto widgets = d->widgets;
    auto widgetsSizes = _sizes;
    for (auto& widgetSize : widgetsSizes) {
        widgetSize *= sizeDelta;
    }
    if (isRightToLeft()) {
        std::reverse(widgets.begin(), widgets.end());
        std::reverse(widgetsSizes.begin(), widgetsSizes.end());
    }
    QRect widgetGeometry = geometry();
    for (int index = 0; index < d->widgets.size(); ++index) {
        auto widget = widgets[index];
        const auto widgetSize = widgetsSizes[index];

        //
        // Последний виджет занимает всю оставшуюся область,
        // которая может возникнуть из-за остатка от деления
        //
        if (widget == widgets.constLast()) {
            widgetGeometry.setRight(width());
            widgetGeometry.setBottom(height());
        } else {
            if (d->orientation == Qt::Horizontal) {
                widgetGeometry.setWidth(widgetSize);
            } else {
                widgetGeometry.setHeight(widgetSize);
            }
        }
        //
        // Позиционируем виджет
        //
        widget->setGeometry(widgetGeometry);
        //
        // И корректируем геометрию следующего
        // +1, чтобы виджеты не накладывались друг на друга
        //
        if (d->orientation == Qt::Horizontal) {
            widgetGeometry.moveLeft(widgetGeometry.right() + 1);
        } else {
            widgetGeometry.moveTop(widgetGeometry.bottom() + 1);
        }
    }

    //
    // Позиционируем хэндл
    //
    const QRect handleGeometry(widgets.constFirst()->geometry().right() - 2, 0, 5, height());
    d->handle->setGeometry(handleGeometry);
    //
    // ... и кнопку отображения скрытой панели
    //
    if ((widgets.constFirst()->geometry().isEmpty() && widgets.constFirst()->isVisible())
        || (widgets.constLast()->geometry().isEmpty() && widgets.constLast()->isVisible())) {
        if (_sizes.constFirst() == 0) {
            d->showHiddenPanelToolbar->setActionCustomWidth(d->showRightPanelAction,
                                                            Ui::DesignSystem::layout().px8());
            d->showHiddenPanelToolbar->clearActionCustomWidth(d->showLeftPanelAction);
        } else {
            d->showHiddenPanelToolbar->setActionCustomWidth(d->showLeftPanelAction,
                                                            Ui::DesignSystem::layout().px8());
            d->showHiddenPanelToolbar->clearActionCustomWidth(d->showRightPanelAction);
        }
        d->showHiddenPanelToolbar->resize(d->showHiddenPanelToolbar->sizeHint());

        d->animateShowHiddenPanelToolbar(
            QPointF(handleGeometry.center().x() - (d->showHiddenPanelToolbar->width() / 2),
                    (height() - d->showHiddenPanelToolbar->height()) / 2));
    } else {
        d->animateShowHiddenPanelToolbar({});
    }
}

QByteArray Splitter::saveState() const
{
    QByteArray state;
    QDataStream stream(&state, QIODevice::WriteOnly);
    for (auto size : std::as_const(d->sizes)) {
        stream << size;
    }
    return state;
}

void Splitter::restoreState(const QByteArray& _state)
{
    auto state = _state;
    QDataStream stream(&state, QIODevice::ReadOnly);
    qreal size = 0;
    QVector<qreal> sizes;
    while (!stream.atEnd()) {
        stream >> size;
        sizes.append(size);
    }

    //
    // Избегаем кривых данных
    //
    if (sizes.size() != 2) {
        return;
    }

    //
    // Восстановим состояние
    //
    d->resize(this, sizes);
}

bool Splitter::event(QEvent* _event)
{
    //
    // Если произошла смена направления компоновки, переустановим текущие размеры,
    // чтобы перекомпоновать содержимое
    //
    switch (_event->type()) {
    case QEvent::LayoutDirectionChange: {
        setSizes(d->widgetsSizes());
        break;
    }

    case QEvent::WindowStateChange: {
        const auto event = static_cast<QWindowStateChangeEvent*>(_event);
        //            event->
        break;
    }

    default:
        break;
    }

    return Widget::event(_event);
}

void Splitter::resizeEvent(QResizeEvent* _event)
{
    Q_UNUSED(_event)

    d->resize(this, d->sizes);
}

bool Splitter::eventFilter(QObject* _watched, QEvent* _event)
{
    //
    // Если сам виджет скрыт, то не делаем никакую обработку
    //
    if (!isVisible()) {
        return false;
    }

    const char* lastSizeKey = "last-size";

    switch (_event->type()) {
    //
    // При смещении разделителя, корректируем размеры вложенных виджетов
    //
    case QEvent::MouseMove: {
        if (_watched == d->handle) {
            auto mouseEvent = static_cast<QMouseEvent*>(_event);
            const auto maxSize = d->orientation == Qt::Horizontal ? width() : height();
            const auto firstWidgetSize = std::min(
                std::max(0, mapFromGlobal(mouseEvent->globalPos()).x()),
                maxSize + 1); // Без единицы от второго виджета торчит однопиксельный край
            if (isLeftToRight()) {
                setSizes({ firstWidgetSize, maxSize - firstWidgetSize });
            } else {
                setSizes({ maxSize - firstWidgetSize, firstWidgetSize });
            }
        }
        break;
    }

    //
    // При отображении одного из вложенных виджетов, пробуем восстановить его размер
    //
    case QEvent::Show: {
        auto widget = qobject_cast<QWidget*>(_watched);
        const auto widgetIndex = d->widgets.indexOf(widget);
        if (widgetIndex == -1) {
            break;
        }

        if (d->sizes.at(widgetIndex) > 0) {
            break;
        }

        int widgetSize = widget->property(lastSizeKey).toInt();
        if (widgetSize <= 0) {
            const auto widgetSizeHint = widget->sizeHint();
            widgetSize = d->orientation == Qt::Horizontal ? widgetSizeHint.width()
                                                          : widgetSizeHint.height();
        }
        const auto maxSize = d->orientation == Qt::Horizontal ? width() : height();
        if (widget == d->widgets.constFirst()) {
            setSizes({ widgetSize, maxSize - widgetSize });
        } else {
            setSizes({ maxSize - widgetSize, widgetSize });
        }
        break;
    }

    //
    // При скрытии одного из вложенных виджетов, используем занимаемую им область под видимый
    //
    case QEvent::Hide: {
        auto widget = qobject_cast<QWidget*>(_watched);
        const auto widgetIndex = d->widgets.indexOf(widget);
        if (widgetIndex == -1) {
            break;
        }

        if (d->sizes.at(widgetIndex) == 0) {
            break;
        }

        widget->setProperty(lastSizeKey,
                            d->orientation == Qt::Horizontal ? widget->width() : widget->height());
        if (widget == d->widgets.constFirst()) {
            setSizes({ 0, 1 });
        } else {
            setSizes({ 1, 0 });
        }
        break;
    }

    default:
        break;
    }

    return Widget::eventFilter(_watched, _event);
}

void Splitter::updateTranslations()
{
    d->showLeftPanelAction->setToolTip(tr("Show hidden panel"));
    d->showRightPanelAction->setToolTip(tr("Show hidden panel"));
}

void Splitter::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    d->showHiddenPanelToolbar->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->showHiddenPanelToolbar->setTextColor(Ui::DesignSystem::color().onPrimary());
}
