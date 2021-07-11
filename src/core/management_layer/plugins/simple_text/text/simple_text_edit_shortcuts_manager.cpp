#include "simple_text_edit_shortcuts_manager.h"

#include "simple_text_edit.h"

#include <business_layer/templates/text_template.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <QShortcut>
#include <QSignalMapper>

using BusinessLayer::TextParagraphType;


namespace Ui {

class SimpleTextEditShortcutsManager::Implementation
{
public:
    explicit Implementation(SimpleTextEdit* _editor);

    /**
     * @brief Создать или обновить комбинацию для заданного типа
     */
    void createOrUpdateShortcut(TextParagraphType _forBlockType);

    //
    // Данные
    //

    /**
     * @brief Редактор текста
     */
    SimpleTextEdit* textEditor = nullptr;

    /**
     * @brief Виджет в контексте которого будут активироваться горячие клавиши
     */
    QWidget* shortcutsContext = nullptr;

    /**
     * @brief Тип блока - горячие клавиши
     */
    QHash<TextParagraphType, QShortcut*> paragraphTypeToShortcut;
};

SimpleTextEditShortcutsManager::Implementation::Implementation(SimpleTextEdit* _editor)
    : textEditor(_editor)
{
}

void SimpleTextEditShortcutsManager::Implementation::createOrUpdateShortcut(
    TextParagraphType _forBlockType)
{
    if (shortcutsContext == nullptr) {
        return;
    }

    const auto blockType = static_cast<TextParagraphType>(_forBlockType);
    const QString typeShortName = BusinessLayer::toString(blockType);
    const QString keySequenceText
        = DataStorageLayer::StorageFacade::settingsStorage()
              ->value(QString("simple-text/editor/shortcuts/%1").arg(typeShortName),
                      DataStorageLayer::SettingsStorage::SettingsPlace::Application)
              .toString();
    const QKeySequence keySequence(keySequenceText);

    if (paragraphTypeToShortcut.contains(_forBlockType)) {
        paragraphTypeToShortcut.value(_forBlockType)->setKey(keySequence);
    } else {
        paragraphTypeToShortcut[_forBlockType]
            = new QShortcut(keySequence, shortcutsContext, 0, 0, Qt::WidgetWithChildrenShortcut);
    }
}


// ****


SimpleTextEditShortcutsManager::SimpleTextEditShortcutsManager(SimpleTextEdit* _parent)
    : QObject(_parent)
    , d(new Implementation(_parent))
{
    Q_ASSERT(_parent);
}

SimpleTextEditShortcutsManager::~SimpleTextEditShortcutsManager() = default;

void SimpleTextEditShortcutsManager::setShortcutsContext(QWidget* _context)
{
    if (d->shortcutsContext == _context) {
        return;
    }

    d->shortcutsContext = _context;
    qDeleteAll(d->paragraphTypeToShortcut);

    //
    // Создаём шорткаты
    //
    d->createOrUpdateShortcut(TextParagraphType::Heading1);
    d->createOrUpdateShortcut(TextParagraphType::Heading2);
    d->createOrUpdateShortcut(TextParagraphType::Heading3);
    d->createOrUpdateShortcut(TextParagraphType::Heading4);
    d->createOrUpdateShortcut(TextParagraphType::Heading5);
    d->createOrUpdateShortcut(TextParagraphType::Heading6);
    d->createOrUpdateShortcut(TextParagraphType::Text);
    d->createOrUpdateShortcut(TextParagraphType::InlineNote);

    //
    // Настраиваем их
    //
    QSignalMapper* mapper = new QSignalMapper(this);
    for (auto shortcutIter = d->paragraphTypeToShortcut.begin();
         shortcutIter != d->paragraphTypeToShortcut.end(); ++shortcutIter) {
        connect(shortcutIter.value(), &QShortcut::activated, mapper,
                qOverload<>(&QSignalMapper::map));
        mapper->setMapping(shortcutIter.value(), static_cast<int>(shortcutIter.key()));
    }
    connect(mapper, &QSignalMapper::mappedInt, this, [this](int _value) {
        d->textEditor->setCurrentParagraphType(static_cast<TextParagraphType>(_value));
    });
}

void SimpleTextEditShortcutsManager::reconfigure()
{
    //
    // Обновим сочетания клавиш для всех блоков
    //
    for (const auto type : d->paragraphTypeToShortcut.keys()) {
        d->createOrUpdateShortcut(type);
    }
}

QString SimpleTextEditShortcutsManager::shortcut(TextParagraphType _forBlockType) const
{
    if (!d->paragraphTypeToShortcut.contains(_forBlockType)) {
        return {};
    }

    return d->paragraphTypeToShortcut.value(_forBlockType)
        ->key()
        .toString(QKeySequence::NativeText);
}

} // namespace Ui
