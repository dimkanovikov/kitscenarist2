#pragma once

#include <business_layer/export/screenplay/screenplay_export_options.h>
#include <ui/export/script_export_dialog.h>

namespace Ui {

/**
 * @brief Диалог настройки параметров экспорта сценария
 */
class ScreenplayExportDialog : public ScriptExportDialog
{
    Q_OBJECT

public:
    explicit ScreenplayExportDialog(const QString& _uuidKey, QWidget* _parent = nullptr);
    ~ScreenplayExportDialog() override;

    /**
     * @brief Получить опции экспорта
     */
    BusinessLayer::ScreenplayExportOptions& exportOptions() const override;

protected:
    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем UI при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

    /**
     * @brief Обновить видимость параметров
     */
    void updateParametersVisibility() const override;

    /**
     * @brief Следует ли показывать праметры справа
     */
    bool isRightLayoutVisible() const override;

    /**
     * @brief Обработать изменение параметра "Экспортировать текст сценария"
     */
    void processIncludeTextChanged(bool _checked) const override;

    /**
     * @brief Должа ли быть активна кнопка "Экспортировать"
     */
    bool isExportEnabled() const override;

    /**
     * @brief Стоит ли экспортировать текст документа
     */
    bool shouldIncludeText() const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
