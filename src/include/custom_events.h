#pragma once

#include <QEvent>
#include <QString>


/**
 * @brief Типы дополнительных событий
 */
enum class EventType {
    //
    // Событие простоя приложения
    //
    IdleEvent = QEvent::User + 1,
    //
    // Событие изменения дизайн системы
    //
    DesignSystemChangeEvent,
    //
    // Событие смены параметров проверки орфографии
    //
    SpellingChangeEvent
};


/**
 * @brief Событие простоя приложения
 */
class IdleEvent : public QEvent
{
public:
    IdleEvent()
        : QEvent(static_cast<QEvent::Type>(EventType::IdleEvent))
    {
    }
};

/**
 * @brief Уведомление об изменении дизайн системы
 */
class DesignSystemChangeEvent : public QEvent
{
public:
    DesignSystemChangeEvent()
        : QEvent(static_cast<QEvent::Type>(EventType::DesignSystemChangeEvent))
    {
    }
};

/**
 * @brief Уведомление об изменении параметров проверки орфографии
 */
class SpellingChangeEvent : public QEvent
{
public:
    SpellingChangeEvent(bool _enabled, const QString& _languageCode = {})
        : QEvent(static_cast<QEvent::Type>(EventType::SpellingChangeEvent))
        , enabled(_enabled)
        , languageCode(_languageCode)
    {
    }

    /**
     * @brief Включена ли проверка орфографии
     */
    const bool enabled = false;

    /**
     * @brief Язык проверки орфографии
     */
    const QString languageCode;
};
