#pragma once

#include <ui/widgets/dialog/abstract_dialog.h>

namespace BusinessLayer {
struct ScreenplayImportOptions;
}


namespace Ui {

/**
 * @brief Диалог настройки параметров импорта
 */
class ImportDialog : public AbstractDialog
{
    Q_OBJECT

public:
    explicit ImportDialog(const QString& _importFilePath, QWidget* _parent = nullptr);
    ~ImportDialog() override;

    /**
     * @brief Получить заданные опции импортирования
     */
    BusinessLayer::ScreenplayImportOptions importOptions() const;

signals:
    /**
     * @brief Пользователь хочет импортировать сценарий с заданными параметрами
     */
    void importRequested();

    /**
     * @brief Пользователь передумал импортировать данные
     */
    void canceled();

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
