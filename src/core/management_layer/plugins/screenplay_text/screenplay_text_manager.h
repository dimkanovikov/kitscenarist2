#pragma once

#include "../manager_plugin_global.h"

#include <interfaces/management_layer/i_document_manager.h>

#include <QObject>


namespace ManagementLayer {

/**
 * @brief Менеджер текста сценария
 */
class MANAGER_PLUGIN_EXPORT ScreenplayTextManager : public QObject, public IDocumentManager
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "app.starc.ManagementLayer.IDocumentManager" FILE
                          "screenplaytextmanager.json")
    Q_INTERFACES(ManagementLayer::IDocumentManager)

public:
    explicit ScreenplayTextManager(QObject* _parent = nullptr);
    ~ScreenplayTextManager() override;

    /**
     * @brief Реализуем интерфейс менеджера документа
     */
    /** @{ */
    QObject* asQObject() override;
    Ui::IDocumentView* view() override;
    Ui::IDocumentView* view(BusinessLayer::AbstractModel* _model) override;
    Ui::IDocumentView* secondaryView() override;
    Ui::IDocumentView* secondaryView(BusinessLayer::AbstractModel* _model) override;
    Ui::IDocumentView* createView(BusinessLayer::AbstractModel* _model) override;
    void resetModels() override;
    void reconfigure(const QStringList& _changedSettingsKeys) override;
    void bind(IDocumentManager* _manager) override;
    void saveSettings() override;
    void setEditingMode(DocumentEditingMode _mode) override;
    void setAvailableCredits(int _credits) override;
    /** @} */

signals:
    /**
     * @brief Изменился индекс текущего элемента модели в текстовом документе (перестился курсор)
     *        только для primary представления (для связи с навигатором)
     */
    void viewCurrentModelIndexChanged(const QModelIndex& _index);

    /**
     * @brief Изменился индекс текущего элемента в любом из представлений
     */
    void currentModelIndexChanged(const QModelIndex& _index);

    /**
     * @brief Запрос на генерацию текста с заданными настройками
     */
    void rephraseTextRequested(const QString& _sourceText, const QString& _style);
    void expandTextRequested(const QString& _text);
    void shortenTextRequested(const QString& _text);
    void insertTextRequested(const QString& _after, const QString& _before);
    void summarizeTextRequested(const QString& _text);
    void translateTextRequested(const QString& _text, const QString& _languageCode);
    void generateTextRequested(const QString& _prompt, const QString& _promptSuffix);

    /**
     * @brief Пользователь хочет докупить кредитов
     */
    void buyCreditsRequested();

protected:
    /**
     * @brief Переопределяем, чтобы отслеживать событие смены языка
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

private:
    /**
     * @brief Установить в редакторе primary редактора курсор на позицию соответствующую элементу с
     *        заданным индексом в модели
     */
    Q_SLOT void setViewCurrentModelIndex(const QModelIndex& _index);

    /**
     * @brief Установить курсор во всех редакторах в заданный индекс
     */
    Q_SLOT void setCurrentModelIndex(const QModelIndex& _index);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer
