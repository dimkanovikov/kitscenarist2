#pragma once

#include <ui/widgets/text_edit/base/base_text_edit.h>

namespace BusinessLayer {
class TextCursor;
class TextModel;
class TextTemplate;
enum class TextParagraphType;
} // namespace BusinessLayer


namespace Ui {

/**
 * @brief Текстовый редактор аудиопостановки
 */
class CORE_LIBRARY_EXPORT ScriptTextEdit : public BaseTextEdit
{
    Q_OBJECT

public:
    explicit ScriptTextEdit(QWidget* _parent = nullptr);
    ~ScriptTextEdit() override;

    /**
     * @brief Включить отображение номеров блоков
     */
    void setShowBlockNumbers(bool _show, bool _continue);

    /**
     * @brief Настроить необходимость корректировок
     */
    void setCorrectionOptions(bool _needToCorrectPageBreaks);

    /**
     * @brief Задать модель текста сценария
     */
    void initWithModel(BusinessLayer::TextModel* _model);

    /**
     * @brief Перенастроить редактор в соответствии с текущей моделью
     * @note Например в случае смены шаблона оформления сценария
     */
    void reinit();

    /**
     * @brief Текущий используемый шаблон оформления
     */
    const BusinessLayer::AudioplayTemplate& audioplayTemplate() const;

    /**
     * @brief Получить модель персонажей
     */
    QAbstractItemModel* characters() const;

    /**
     * @brief Создать персонажа с заданным именем
     */
    void createCharacter(const QString& _name);

    /**
     * @brief Отменить последнее изменение
     */
    void undo();

    /**
     * @brief Повторить последнее отменённое изменение
     */
    void redo();

    /**
     * @brief Вставить новый блок
     * @param Тип блока
     */
    void addParagraph(BusinessLayer::TextParagraphType _type);

    /**
     * @brief Установить тип текущего блока
     * @param Тип блока
     */
    void setCurrentParagraphType(BusinessLayer::TextParagraphType _type);

    /**
     * @brief Получить тип блока в котором находится курсор
     */
    BusinessLayer::TextParagraphType currentParagraphType() const;

    /**
     * @brief Своя реализация установки курсора
     */
    void setTextCursorReimpl(const QTextCursor& _cursor);

    /**
     * @brief Получить индекс элемента модели в текущей позиции курсора
     */
    QModelIndex currentModelIndex() const;

    /**
     * @brief Поставить курсор в позицию элемента с заданным индексом модели сценария
     */
    void setCurrentModelIndex(const QModelIndex& _index);

    /**
     * @brief Получить позицию заданного элемента модели
     */
    int positionForModelIndex(const QModelIndex& _index);

    /**
     * @brief Добавить редакторскую заметку для текущего выделения
     */
    void addReviewMark(const QColor& _textColor, const QColor& _backgroundColor,
                       const QString& _comment);

signals:
    /**
     * @brief Изменён тип абзаца
     */
    void paragraphTypeChanged();

    /**
     * @brief Пользователь хочет добавить закладку
     */
    void addBookmarkRequested();

    /**
     * @brief Пользователь хочет изменить закладку
     */
    void editBookmarkRequested();

    /**
     * @brief Пользователь хочет удалить закладку
     */
    void removeBookmarkRequested();

    /**
     * @brief Пользователь хочет показать/скрыть список закладок
     */
    void showBookmarksRequested();

protected:
    /**
     * @brief Нажатия многих клавиш обрабатываются вручную
     */
    void keyPressEvent(QKeyEvent* _event) override;

    /**
     * @brief Обрабатываем специфичные ситуации для редактора сценария
     */
    /** @{ */
    bool keyPressEventReimpl(QKeyEvent* _event) override;
    bool updateEnteredText(const QString& _eventText) override;
    /** @} */

    /**
     * @brief Реализуем отрисовку дополнительных элементов
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Переопределяем для добавления в меню собственных пунктов
     */
    ContextMenu* createContextMenu(const QPoint& _position, QWidget* _parent = nullptr) override;

    /**
     * @brief Переопределяем работу с буфером обмена для использования собственного майм типа данных
     */
    /** @{ */
    bool canInsertFromMimeData(const QMimeData* _source) const override;
    QMimeData* createMimeDataFromSelection() const override;
    void insertFromMimeData(const QMimeData* _source) override;
    void dropEvent(QDropEvent* _event) override;
    /** @} */

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
