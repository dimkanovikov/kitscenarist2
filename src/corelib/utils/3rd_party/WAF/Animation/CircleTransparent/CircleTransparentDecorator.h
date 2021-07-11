/*
 * Copyright (C) 2020  Dimka Novikov, to@dimkanovikov.pro
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * Full license: https://github.com/dimkanovikov/WidgetAnimationFramework/blob/master/LICENSE
 */


#pragma once

#include <QWidget>


/**
 * Widgets Animation Framework
 */
namespace WAF {
/**
 * @brief Класс рисующий круг, заполняющий пространство
 */
class CircleTransparentDecorator : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(int radius READ radius WRITE setRadius)

public:
    explicit CircleTransparentDecorator(QWidget* _parent);

    /**
     * @brief Установить точку центра круга
     */
    void setStartPoint(const QPoint& _globalPoint);

    /**
     * @brief Получить точку центра круга в глобальных координатах
     */
    QPoint startPoint() const;

    /**
     * @brief Радиус рисуемой окружности
     */
    /** @{ */
    int radius() const;
    void setRadius(int _radius);
    /** @} */

    /**
     * @brief Установить изображение заливки
     */
    void setFillImage(const QPixmap& _fillImage);

protected:
    /**
     * @brief Переопределяется для прорисовки декорации
     */
    void paintEvent(QPaintEvent* _event);

private:
    /**
     * @brief Точка из которой начинается заполенние
     */
    QPoint m_startPoint;

    /**
     * @brief Прозрачность изображения виджета для прорисовки
     */
    int m_radius;

    /**
     * @brief Изображение растворения
     */
    QPixmap m_fillImage;
};
} // namespace WAF
