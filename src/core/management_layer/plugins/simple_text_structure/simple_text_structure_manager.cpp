#include "simple_text_structure_manager.h"

#include "business_layer/simple_text_structure_model.h"
#include "ui/simple_text_structure_view.h"

#include <business_layer/model/text/text_model.h>


namespace ManagementLayer {

class SimpleTextStructureManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::SimpleTextStructureView* createView();


    /**
     * @brief Текущая модель документа
     */
    BusinessLayer::TextModel* model = nullptr;

    /**
     * @brief Индекс модели, который необходимо выделить
     * @note Используется в случаях, когда в навигаторе установлена не та модель, что отображается
     *       в редакторе, когда будет установлена нужная модель, в навигаторе будет выделен элемент
     *       с данным индексом
     */
    QModelIndex modelIndexToSelect;

    /**
     * @brief Модель отображения структуры документа
     */
    BusinessLayer::SimpleTextStructureModel* structureModel = nullptr;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::SimpleTextStructureView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::SimpleTextStructureView*> allViews;
};

SimpleTextStructureManager::Implementation::Implementation()
{
    view = createView();
}

Ui::SimpleTextStructureView* SimpleTextStructureManager::Implementation::createView()
{
    allViews.append(new Ui::SimpleTextStructureView);
    return allViews.last();
}


// ****


SimpleTextStructureManager::SimpleTextStructureManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
    connect(d->view, &Ui::SimpleTextStructureView::currentModelIndexChanged, this,
            [this](const QModelIndex& _index) {
                emit currentModelIndexChanged(d->structureModel->mapToSource(_index));
            });
}

SimpleTextStructureManager::~SimpleTextStructureManager() = default;

QObject* SimpleTextStructureManager::asQObject()
{
    return this;
}

void SimpleTextStructureManager::setModel(BusinessLayer::AbstractModel* _model)
{
    //
    // Разрываем соединения со старой моделью
    //
    if (d->model != nullptr) {
        d->view->disconnect(d->model);
    }

    //
    // Определяем новую модель
    //
    d->model = qobject_cast<BusinessLayer::TextModel*>(_model);

    //
    // Создаём прокси модель, если ещё не была создана и настриваем её
    //
    if (d->structureModel == nullptr) {
        d->structureModel = new BusinessLayer::SimpleTextStructureModel(d->view);
        d->view->setModel(d->structureModel);
    }

    //
    // Помещаем модель с данными в прокси
    //
    d->structureModel->setSourceModel(d->model);

    //
    // Настраиваем соединения с новой моделью
    //
    if (d->model != nullptr) {
        d->view->setTitle(d->model->name());
        connect(d->model, &BusinessLayer::TextModel::nameChanged, d->view,
                &Ui::SimpleTextStructureView::setTitle);
    }

    //
    // Если элемент к выделению уже задан, выберем его в структуре
    //
    if (d->modelIndexToSelect.isValid()) {
        setCurrentModelIndex(d->modelIndexToSelect);
    }
}

QWidget* SimpleTextStructureManager::view()
{
    return d->view;
}

QWidget* SimpleTextStructureManager::createView()
{
    return d->createView();
}

void SimpleTextStructureManager::reconfigure(const QStringList& _changedSettingsKeys)
{
    Q_UNUSED(_changedSettingsKeys);
    d->view->reconfigure();
}

void SimpleTextStructureManager::bind(IDocumentManager* _manager)
{
    Q_ASSERT(_manager);

    connect(_manager->asQObject(), SIGNAL(currentModelIndexChanged(QModelIndex)), this,
            SLOT(setCurrentModelIndex(QModelIndex)), Qt::UniqueConnection);
}

void SimpleTextStructureManager::setCurrentModelIndex(const QModelIndex& _index)
{
    if (!_index.isValid()) {
        return;
    }

    if (d->model != _index.model()) {
        d->modelIndexToSelect = _index;
        return;
    }

    QSignalBlocker signalBlocker(this);

    //
    // Из редактора текста мы получаем индексы текстовых элементов, они хранятся внутри
    // глав, которые как раз и отображаются в навигаторе
    //
    d->view->setCurrentModelIndex(d->structureModel->mapFromSource(_index.parent()));
    d->modelIndexToSelect = {};
}

} // namespace ManagementLayer
