#pragma once

#include <ui/widgets/dialog/abstract_dialog.h>


namespace Domain {
enum class DocumentObjectType;
}

namespace Ui {

/**
 * @brief Диалог создания нового документа в проекте
 */
class CreateDocumentDialog : public AbstractDialog
{
    Q_OBJECT

public:
    explicit CreateDocumentDialog(QWidget* _parent = nullptr);
    ~CreateDocumentDialog() override;

    /**
     * @brief Последний выбранный тип документов
     */
    Domain::DocumentObjectType lastSelectedType() const;

    /**
     * @brief Сохранить выбранный тип элемента
     */
    void saveSelectedType();

    /**
     * @brief Выбрать документ заданного типа
     */
    void setDocumentType(Domain::DocumentObjectType _type);

    /**
     * @brief Задать ошибку имени
     */
    void setNameError(const QString& _error);

    /**
     * @brief Задать ошибку файл презентации
     */
    void setImportFileError(const QString& _error);

    /**
     * @brief Установить возможность вставки элемента в родителя
     */
    void setInsertionParent(const QString& _parentName);

    /**
     * @brief Количество эпизодов
     */
    int episodesAmount() const;

    /**
     * @brief Желает ли пользователь вставлять элемент в заданного родителя
     */
    bool needInsertIntoParent() const;

    /**
     * @brief Путь к файлу для импорта
     */
    QString importFilePath() const;

signals:
    /**
     * @brief Пользователь хочет создать документ заданного типа с заданным именем (опционально: из
     * заданного файла)
     */
    void createPressed(Domain::DocumentObjectType _type, const QString& _name,
                       const QString& _filePath);

    /**
     * @brief Нужно импортировать файл
     */
    void importFileRequested(Domain::DocumentObjectType _type);

protected:
    /**
     * @brief Определим виджет, который необходимо сфокусировать после отображения диалога
     */
    QWidget* focusedWidgetAfterShow() const override;

    /**
     * @brief Опеределим последний фокусируемый виджет в диалоге
     */
    QWidget* lastFocusableWidget() const override;

    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем UI при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
