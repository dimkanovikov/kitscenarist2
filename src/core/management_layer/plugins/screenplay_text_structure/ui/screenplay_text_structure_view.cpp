#include "screenplay_text_structure_view.h"

#include "screenplay_text_structure_delegate.h"

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/shadow/shadow.h>
#include <ui/widgets/tree/tree.h>

#include <QStringListModel>
#include <QVBoxLayout>


namespace Ui {

class ScreenplayTextStructureView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    IconsMidLabel* backIcon = nullptr;
    Subtitle2Label* backText = nullptr;
    Tree* content = nullptr;
    ScreenplayTextStructureDelegate* contentDelegate = nullptr;
};

ScreenplayTextStructureView::Implementation::Implementation(QWidget* _parent)
    : backIcon(new IconsMidLabel(_parent))
    , backText(new Subtitle2Label(_parent))
    , content(new Tree(_parent))
    , contentDelegate(new ScreenplayTextStructureDelegate(content))
{
    backIcon->setText(u8"\U000f0141");

    content->setDragDropEnabled(true);
    content->setSelectionMode(QAbstractItemView::ContiguousSelection);
    content->setItemDelegate(contentDelegate);

    new Shadow(Qt::TopEdge, content);
}


// ****


ScreenplayTextStructureView::ScreenplayTextStructureView(QWidget* _parent)
    : AbstractNavigator(_parent)
    , d(new Implementation(this))
{
    QHBoxLayout* topLayout = new QHBoxLayout;
    topLayout->setContentsMargins({});
    topLayout->setSpacing(0);
    topLayout->addWidget(d->backIcon);
    topLayout->addWidget(d->backText, 1);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addLayout(topLayout);
    layout->addWidget(d->content);
    setLayout(layout);

    connect(d->backIcon, &AbstractLabel::clicked, this, &ScreenplayTextStructureView::backPressed);
    connect(d->backText, &AbstractLabel::clicked, this, &ScreenplayTextStructureView::backPressed);
    connect(d->content, &Tree::currentIndexChanged, this,
            &ScreenplayTextStructureView::currentModelIndexChanged);

    updateTranslations();
    designSystemChangeEvent(nullptr);

    reconfigure();
}

ScreenplayTextStructureView::~ScreenplayTextStructureView() = default;

void ScreenplayTextStructureView::reconfigure()
{
    auto settingsValue = [](const QString& _key) {
        return DataStorageLayer::StorageFacade::settingsStorage()->value(
            _key, DataStorageLayer::SettingsStorage::SettingsPlace::Application);
    };

    const bool showSceneNumber
        = settingsValue(DataStorageLayer::kComponentsScreenplayNavigatorShowSceneNumberKey)
              .toBool();
    d->contentDelegate->showSceneNumber(showSceneNumber);

    const bool showSceneText
        = settingsValue(DataStorageLayer::kComponentsScreenplayNavigatorShowSceneTextKey).toBool();
    if (showSceneText == false) {
        d->contentDelegate->setTextLinesSize(0);
    } else {
        const int sceneTextLines
            = settingsValue(DataStorageLayer::kComponentsScreenplayNavigatorSceneTextLinesKey)
                  .toInt();
        d->contentDelegate->setTextLinesSize(sceneTextLines);
    }

    d->content->setItemDelegate(nullptr);
    d->content->setItemDelegate(d->contentDelegate);
}

void ScreenplayTextStructureView::setTitle(const QString& _title)
{
    d->backText->setText(_title);
}

void ScreenplayTextStructureView::setModel(QAbstractItemModel* _model)
{
    d->content->setModel(_model);
}

void ScreenplayTextStructureView::setCurrentModelIndex(const QModelIndex& _index)
{
    d->content->setCurrentIndex(_index);
}

void ScreenplayTextStructureView::updateTranslations()
{
    d->backText->setText("Back to navigator");
}

void ScreenplayTextStructureView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().primary());
    auto backTextColor = DesignSystem::color().onPrimary();
    backTextColor.setAlphaF(Ui::DesignSystem::inactiveTextOpacity());
    for (auto widget : QVector<Widget*>{ d->backIcon, d->backText }) {
        widget->setBackgroundColor(DesignSystem::color().primary());
        widget->setTextColor(backTextColor);
    }
    d->content->setBackgroundColor(DesignSystem::color().primary());
    d->content->setTextColor(DesignSystem::color().onPrimary());

    d->backIcon->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px8(),
                  Ui::DesignSystem::layout().px4(), Ui::DesignSystem::layout().px8())
            .toMargins());
    d->backText->setContentsMargins(QMarginsF(0, Ui::DesignSystem::layout().px12(),
                                              Ui::DesignSystem::layout().px16(),
                                              Ui::DesignSystem::layout().px12())
                                        .toMargins());
}

} // namespace Ui
