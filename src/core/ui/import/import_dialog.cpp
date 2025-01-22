#include "import_dialog.h"

#include <business_layer/import/screenplay/screenplay_import_options.h>
#include <ui/design_system/design_system.h>
#include <ui/import/file_name_delegate.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/tree/tree.h>
#include <ui/widgets/tree/tree_delegate.h>
#include <utils/helpers/extension_helper.h>

#include <QApplication>
#include <QEvent>
#include <QFileInfo>
#include <QGridLayout>
#include <QSettings>
#include <QStandardItemModel>
#include <QStringListModel>


namespace Ui {

namespace {
const QString kGroupKey = "widgets/import-dialog/";
const QString kDocumentType = "document-type";
const QString kImportCharacters = "import-characters";
const QString kImportLocations = "import-locations";
const QString kImportResearch = "import-research";
const QString kImportText = "import-text";
const QString kKeepSceneNumbers = "keep-scene-numbers";

QString settingsKey(const QString& _filePath, const QString& _parameter)
{
    return QString("%1/%2/%3").arg(kGroupKey, _filePath, _parameter);
}

const QVector<QPair<const Domain::DocumentObjectType, const QString>> kDocumentTypes{
    {
        Domain::DocumentObjectType::Audioplay,
        QApplication::translate("ImportDialog", "Audioplay"),
    },
    {
        Domain::DocumentObjectType::ComicBook,
        QApplication::translate("ImportDialog", "Comic Book"),
    },
    {
        Domain::DocumentObjectType::Novel,
        QApplication::translate("ImportDialog", "Novel"),
    },
    {
        Domain::DocumentObjectType::Screenplay,
        QApplication::translate("ImportDialog", "Screenplay"),
    },
    {
        Domain::DocumentObjectType::Stageplay,
        QApplication::translate("ImportDialog", "Stageplay"),
    },
};

QString typeToString(const Domain::DocumentObjectType& _type)
{
    for (const auto& type : kDocumentTypes) {
        if (type.first == _type) {
            return type.second;
        }
    }
    return "";
}

Domain::DocumentObjectType stringToType(const QString& _type)
{
    for (const auto& type : kDocumentTypes) {
        if (type.second == _type) {
            return type.first;
        }
    }
    return Domain::DocumentObjectType::Undefined;
}

QVector<Domain::DocumentObjectType> importTypesForFile(const QString& _path)
{
    auto fileIs = [filePath = _path.toLower()](const QString& _extension) {
        return filePath.endsWith(_extension);
    };

    //
    // Бинарные форматы
    //
    if (fileIs(ExtensionHelper::msOfficeOpenXml()) || fileIs(ExtensionHelper::openDocumentXml())) {
        return { Domain::DocumentObjectType::Screenplay };
    } else if (fileIs(ExtensionHelper::pdf())) {
        return { Domain::DocumentObjectType::Screenplay };
    }
    //
    // Текстовые форматы
    //
    else if (fileIs(ExtensionHelper::fountain())) {
        return {
            Domain::DocumentObjectType::Audioplay,
            Domain::DocumentObjectType::ComicBook,
            Domain::DocumentObjectType::Screenplay,
            Domain::DocumentObjectType::Stageplay,
        };
    } else if (fileIs(ExtensionHelper::markdown())) {
        return { Domain::DocumentObjectType::Novel };
    } else if (fileIs(ExtensionHelper::plainText())) {
        return {
            Domain::DocumentObjectType::Audioplay,  Domain::DocumentObjectType::ComicBook,
            Domain::DocumentObjectType::Screenplay, Domain::DocumentObjectType::Stageplay,
            Domain::DocumentObjectType::Novel,
        };
    }
    //
    // Специализированные форматы
    //
    else if (fileIs(ExtensionHelper::celtx()) || fileIs(ExtensionHelper::finalDraft())
             || fileIs(ExtensionHelper::finalDraftTemplate())
             || fileIs(ExtensionHelper::kitScenarist()) || fileIs(ExtensionHelper::trelby())) {
        return { Domain::DocumentObjectType::Screenplay };
    }
    return { Domain::DocumentObjectType::Undefined };
}

} // namespace

class ImportDialog::Implementation
{
public:
    Implementation(const QStringList& _importFilePaths, ImportDialog* _parent);

    /**
     * @brief Текущие опции импорта
     */
    BusinessLayer::ImportOptions* currentOptions() const;

    /**
     * @brief Записать текущие настройки импорта в список опций
     */
    void writeCurrentOptions();

    /**
     * @brief Текущий тип документа, в который будет импортирован файл
     */
    Domain::DocumentObjectType currentType() const;

    /**
     * @brief Задать типы документов, в которые можем импортировать
     */
    void setImportDocumentTypes(const QVector<Domain::DocumentObjectType>& _types);

    /**
     * @brief Обновить лейбл галочки импортирования текста в зависимости от текущего типа документа
     */
    void updateImportTextLabel();

    /**
     * @brief Обновить видимость параметров
     */
    void updateParametersVisibility();

    /**
     * @brief Отобразить настройки импорта для файла
     */
    void showOptionsForFile(const QString& _path);

    /**
     * @brief Доступен ли импорт при текущих настройках
     */
    bool isImportAvailable() const;

    /**
     * @brief Может ли файл быть импортирован
     */
    bool fileCanBeImported(const BusinessLayer::ImportOptions* _options) const;

    /**
     * @brief Сохранить настройки импорта для файлов
     */
    void saveSetting();


    ImportDialog* q = nullptr;

    const QStringList importFilePaths;
    OverlineLabel* filesCountTitle = nullptr;
    Tree* tree = nullptr;

    Widget* optionsWidget = nullptr;
    ComboBox* documentType = nullptr;
    OverlineLabel* documentsTitle = nullptr;
    CheckBox* importCharacters = nullptr;
    CheckBox* importLocations = nullptr;
    CheckBox* importResearch = nullptr;
    OverlineLabel* textTitle = nullptr;
    CheckBox* importText = nullptr;
    CheckBox* keepSceneNumbers = nullptr;

    QHBoxLayout* bottomLayout = nullptr;
    CheckBox* sameOptions = nullptr;
    Button* cancelButton = nullptr;
    Button* importButton = nullptr;

    QString currentFile;
    QVector<Domain::DocumentObjectType> documentTypes;

    /**
     * @brief Список опций импорта всех файлов
     */
    std::unordered_map<QString, std::unique_ptr<BusinessLayer::ImportOptions>> filesOptions;
};

ImportDialog::Implementation::Implementation(const QStringList& _importFilePaths,
                                             ImportDialog* _parent)
    : q(_parent)
    , importFilePaths(_importFilePaths)
    , filesCountTitle(new OverlineLabel(_parent))
    , tree(new Tree(_parent))
    , optionsWidget(new Widget(_parent))
    , documentType(new ComboBox(optionsWidget))
    , documentsTitle(new OverlineLabel(optionsWidget))
    , importCharacters(new CheckBox(optionsWidget))
    , importLocations(new CheckBox(optionsWidget))
    , importResearch(new CheckBox(optionsWidget))
    , textTitle(new OverlineLabel(optionsWidget))
    , importText(new CheckBox(optionsWidget))
    , keepSceneNumbers(new CheckBox(optionsWidget))
    , bottomLayout(new QHBoxLayout)
    , sameOptions(new CheckBox(_parent))
    , cancelButton(new Button(_parent))
    , importButton(new Button(_parent))
    , currentFile(_importFilePaths.first())
{
    documentType->setSpellCheckPolicy(SpellCheckPolicy::Manual);

    sameOptions->setChecked(false);

    bottomLayout->setContentsMargins({});
    bottomLayout->setSpacing(0);
    if (_importFilePaths.size() > 1) {
        bottomLayout->addWidget(sameOptions);
    }
    bottomLayout->addStretch();
    bottomLayout->addWidget(cancelButton);
    bottomLayout->addWidget(importButton);

    //
    // Заполняем опции импорта файлов
    //
    QSettings settings;
    for (const auto& path : _importFilePaths) {
        std::unique_ptr<BusinessLayer::ImportOptions> options;

        const auto defaultType = typeToString(Domain::DocumentObjectType::Screenplay);
        const auto type = stringToType(
            settings.value(settingsKey(path, kDocumentType), defaultType).toString());
        if (type == Domain::DocumentObjectType::Screenplay) {
            options = std::make_unique<BusinessLayer::ScreenplayImportOptions>();
            auto screenplayOptions
                = static_cast<BusinessLayer::ScreenplayImportOptions*>(options.get());
            screenplayOptions->importResearch
                = settings.value(settingsKey(path, kImportResearch), true).toBool();
            screenplayOptions->keepSceneNumbers
                = settings.value(settingsKey(path, kKeepSceneNumbers), false).toBool();
        } else {
            options = std::make_unique<BusinessLayer::ImportOptions>();
        }
        options->documentType = type;
        options->filePath = path;
        options->importCharacters
            = settings.value(settingsKey(path, kImportCharacters), true).toBool();
        options->importLocations
            = settings.value(settingsKey(path, kImportLocations), true).toBool();
        options->importText = settings.value(settingsKey(path, kImportText), true).toBool();
        filesOptions[path] = std::move(options);
    }
}

BusinessLayer::ImportOptions* ImportDialog::Implementation::currentOptions() const
{
    BusinessLayer::ImportOptions* options = nullptr;
    const auto type = currentType();
    if (type == Domain::DocumentObjectType::Screenplay) {
        options = new BusinessLayer::ScreenplayImportOptions;
        auto screenplayOptions = static_cast<BusinessLayer::ScreenplayImportOptions*>(options);
        screenplayOptions->importResearch
            = importResearch->isVisibleTo(optionsWidget) && importResearch->isChecked();
        screenplayOptions->keepSceneNumbers
            = keepSceneNumbers->isVisibleTo(optionsWidget) && keepSceneNumbers->isChecked();
    } else {
        options = new BusinessLayer::ImportOptions;
    }
    options->filePath = currentFile;
    options->documentType = type;
    options->importText = importText->isVisibleTo(optionsWidget) && importText->isChecked();

    options->importCharacters
        = importCharacters->isVisibleTo(optionsWidget) && importCharacters->isChecked();
    options->importLocations
        = importLocations->isVisibleTo(optionsWidget) && importLocations->isChecked();
    return options;
}

void ImportDialog::Implementation::writeCurrentOptions()
{
    filesOptions[currentFile].reset(currentOptions());
}

Domain::DocumentObjectType ImportDialog::Implementation::currentType() const
{
    return documentTypes.value(documentType->currentIndex().row(),
                               Domain::DocumentObjectType::Undefined);
}

void ImportDialog::Implementation::setImportDocumentTypes(
    const QVector<Domain::DocumentObjectType>& _types)
{
    documentTypes.clear();
    QStringList list;
    for (const auto& type : _types) {
        list.append(typeToString(type));
        documentTypes.append(type);
    }
    QSignalBlocker signalBlocker(documentType);
    documentType->setModel(new QStringListModel(list));
}

void ImportDialog::Implementation::updateImportTextLabel()
{
    switch (currentType()) {
    case Domain::DocumentObjectType::Audioplay:
    case Domain::DocumentObjectType::ComicBook:
    case Domain::DocumentObjectType::Screenplay:
    case Domain::DocumentObjectType::Stageplay: {
        importText->setText(tr("Import script text"));
        break;
    }
    case Domain::DocumentObjectType::Novel: {
        importText->setText(tr("Import novel text"));
        break;
    }
    default: {
        importText->setText(tr("Import text"));
        break;
    }
    }
}

void ImportDialog::Implementation::updateParametersVisibility()
{
    auto isImportCharactersVisible = true;
    auto isImportLocationsVisible = true;
    auto isImportResearchVisible = true;
    auto isKeepSceneNumbersVisible = true;
    switch (currentType()) {
    case Domain::DocumentObjectType::Audioplay:
    case Domain::DocumentObjectType::ComicBook:
    case Domain::DocumentObjectType::Stageplay: {
        isImportLocationsVisible = false;
        isImportResearchVisible = false;
        isKeepSceneNumbersVisible = false;
        break;
    }
    case Domain::DocumentObjectType::Novel: {
        isImportCharactersVisible = false;
        isImportLocationsVisible = false;
        isImportResearchVisible = false;
        isKeepSceneNumbersVisible = false;
        break;
    }
    case Domain::DocumentObjectType::Screenplay: {
        isKeepSceneNumbersVisible = importText->isChecked();
        break;
    }
    default: {
        isImportLocationsVisible = false;
        isImportResearchVisible = false;
        isKeepSceneNumbersVisible = false;
        break;
    }
    }
    importCharacters->setVisible(isImportCharactersVisible);
    importLocations->setVisible(isImportLocationsVisible);
    importResearch->setVisible(isImportResearchVisible);
    keepSceneNumbers->setVisible(isKeepSceneNumbersVisible);
    documentsTitle->setVisible(isImportCharactersVisible || isImportLocationsVisible);

    updateImportTextLabel();
}

void ImportDialog::Implementation::showOptionsForFile(const QString& _path)
{
    const auto& options = filesOptions[_path];
    const auto type = options->documentType;
    int index = documentTypes.indexOf(type) >= 0 ? documentTypes.indexOf(type) : 0;
    documentType->setCurrentIndex(documentType->model()->index(index, 0));
    updateParametersVisibility();

    importText->setChecked(options->importText);
    importCharacters->setChecked(options->importCharacters);
    importLocations->setChecked(options->importLocations);

    if (type == Domain::DocumentObjectType::Screenplay) {
        auto screenplayOptions
            = static_cast<BusinessLayer::ScreenplayImportOptions*>(options.get());
        importResearch->setChecked(screenplayOptions->importResearch);
        keepSceneNumbers->setChecked(screenplayOptions->keepSceneNumbers);
    }
}

bool ImportDialog::Implementation::isImportAvailable() const
{
    return (importCharacters->isVisibleTo(optionsWidget) && importCharacters->isChecked())
        || (importLocations->isVisibleTo(optionsWidget) && importLocations->isChecked())
        || (importResearch->isVisibleTo(optionsWidget) && importResearch->isChecked())
        || (importText->isVisibleTo(optionsWidget) && importText->isChecked());
}

bool ImportDialog::Implementation::fileCanBeImported(
    const BusinessLayer::ImportOptions* _options) const
{
    if (_options->documentType == Domain::DocumentObjectType::Screenplay) {
        auto screenplayOptions
            = static_cast<const BusinessLayer::ScreenplayImportOptions*>(_options);
        if (screenplayOptions->importResearch) {
            return true;
        }
    }
    if (_options->importCharacters || _options->importLocations || _options->importText) {
        return true;
    }
    return false;
}

void ImportDialog::Implementation::saveSetting()
{
    QSettings settings;
    for (const auto& fileOptions : filesOptions) {
        const auto& options = fileOptions.second;
        const auto type = options->documentType;
        if (type == Domain::DocumentObjectType::Screenplay) {
            auto screenplayOptions
                = static_cast<BusinessLayer::ScreenplayImportOptions*>(options.get());
            settings.setValue(settingsKey(options->filePath, kImportResearch),
                              screenplayOptions->importResearch);
            settings.setValue(settingsKey(options->filePath, kKeepSceneNumbers),
                              screenplayOptions->keepSceneNumbers);
        }
        settings.setValue(settingsKey(options->filePath, kDocumentType), typeToString(type));
        settings.setValue(settingsKey(options->filePath, kImportCharacters),
                          options->importCharacters);
        settings.setValue(settingsKey(options->filePath, kImportLocations),
                          options->importLocations);
        settings.setValue(settingsKey(options->filePath, kImportText), options->importText);
    }
}


// ****


ImportDialog::ImportDialog(const QStringList& _importFilePaths, QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(_importFilePaths, this))
{
    setAcceptButton(d->importButton);
    setRejectButton(d->cancelButton);

    int column = 0;
    if (_importFilePaths.size() > 1) {
        QStandardItemModel* model = new QStandardItemModel(d->tree);
        for (const auto& path : _importFilePaths) {
            const auto& options = d->filesOptions[path];
            const auto type = options->documentType;
            auto item = new QStandardItem(path);
            item->setData(typeToString(type), ImportTypeRole);
            item->setData(d->fileCanBeImported(options.get()), ImportEnabledRole);
            model->appendRow(item);
        }
        d->tree->setModel(model);
        d->tree->setItemDelegate(new FileNameDelegate(d->tree));
        d->tree->setCurrentIndex(d->tree->model()->index(0, 0));

        //
        // Левая сторона
        //
        QVBoxLayout* leftLayout = new QVBoxLayout;
        leftLayout->addWidget(d->filesCountTitle);
        leftLayout->addWidget(d->tree);
        contentsLayout()->addLayout(leftLayout, 0, column++);
    }

    //
    // Правая сторона
    //
    QVBoxLayout* rightLayout = new QVBoxLayout;
    rightLayout->addWidget(d->documentType);
    rightLayout->addWidget(d->documentsTitle);
    rightLayout->addWidget(d->importCharacters);
    rightLayout->addWidget(d->importLocations);
    rightLayout->addWidget(d->importResearch);
    rightLayout->addWidget(d->textTitle);
    rightLayout->addWidget(d->importText);
    rightLayout->addWidget(d->keepSceneNumbers);
    rightLayout->addStretch();
    d->optionsWidget->setLayout(rightLayout);
    contentsLayout()->addWidget(d->optionsWidget, 0, column);

    //
    // Низ
    //
    int columnSpan = column + 1;
    contentsLayout()->addLayout(d->bottomLayout, 1, 0, 1, columnSpan);

    //
    // Определить активность кнопки импорта
    //
    auto configureImportAvailability = [this] {
        bool importEnabled = true;
        const auto currentFileImportAvailable = d->isImportAvailable();
        if (d->tree->model()) {
            d->tree->model()->setData(d->tree->currentIndex(), currentFileImportAvailable,
                                      ImportEnabledRole);
        }
        if (d->sameOptions->isVisibleTo(this) && d->sameOptions->isChecked()) {
            importEnabled = currentFileImportAvailable;
        } else if (!currentFileImportAvailable) {
            importEnabled = false;
        } else {
            for (const auto& fileOptions : d->filesOptions) {
                const auto& options = fileOptions.second;
                if (!d->fileCanBeImported(options.get()) && options->filePath != d->currentFile) {
                    importEnabled = false;
                    break;
                }
            }
        }
        d->importButton->setEnabled(importEnabled);
    };
    configureImportAvailability();

    //
    // Соединения чекбоксов настроек импорта
    //
    connect(d->importCharacters, &CheckBox::checkedChanged, this, configureImportAvailability);
    connect(d->importLocations, &CheckBox::checkedChanged, this, configureImportAvailability);
    connect(d->importResearch, &CheckBox::checkedChanged, this, configureImportAvailability);
    connect(d->importText, &CheckBox::checkedChanged, this, configureImportAvailability);
    connect(d->importText, &CheckBox::checkedChanged, this, [this](bool _checked) {
        if (d->currentType() == Domain::DocumentObjectType::Screenplay) {
            d->keepSceneNumbers->setVisible(_checked);
        }
    });

    //
    // Передать опции импорта всем файлам
    //
    auto setSameOptionsForAll = [this] {
        if (d->sameOptions->isVisibleTo(this) && d->sameOptions->isChecked()) {
            QScopedPointer currentOptions(d->currentOptions());

            for (auto& fileOptions : d->filesOptions) {
                auto& options = fileOptions.second;
                if (options->documentType != currentOptions->documentType) {
                    const auto path = options->filePath;
                    options.reset(d->currentOptions());
                    options->filePath = path;
                }
                if (options->documentType == Domain::DocumentObjectType::Screenplay) {
                    auto currentScreenplayOptions
                        = static_cast<BusinessLayer::ScreenplayImportOptions*>(
                            currentOptions.get());
                    auto screenplayOptions
                        = static_cast<BusinessLayer::ScreenplayImportOptions*>(options.get());
                    screenplayOptions->importResearch = currentScreenplayOptions->importResearch;
                    screenplayOptions->keepSceneNumbers
                        = currentScreenplayOptions->keepSceneNumbers;
                }
                options->importCharacters = currentOptions->importCharacters;
                options->importLocations = currentOptions->importLocations;
                options->importText = currentOptions->importText;

                auto model = static_cast<QStandardItemModel*>(d->tree->model());
                auto items = model->findItems(options->filePath);
                if (!items.isEmpty()) {
                    items.first()->setData(typeToString(options->documentType), ImportTypeRole);
                    items.first()->setData(d->fileCanBeImported(options.get()), ImportEnabledRole);
                }
            }
            d->tree->update();
        }
    };

    //
    // Соединения для работы чекбокса "Same options for all"
    //
    connect(d->sameOptions, &CheckBox::checkedChanged, this,
            [this, setSameOptionsForAll](bool _checked) {
                if (_checked) {
                    d->importButton->setEnabled(d->isImportAvailable());
                    setSameOptionsForAll();
                }
            });
    connect(d->importCharacters, &CheckBox::checkedChanged, this, setSameOptionsForAll);
    connect(d->importLocations, &CheckBox::checkedChanged, this, setSameOptionsForAll);
    connect(d->importText, &CheckBox::checkedChanged, this, setSameOptionsForAll);
    connect(d->importResearch, &CheckBox::checkedChanged, this, setSameOptionsForAll);
    connect(d->keepSceneNumbers, &CheckBox::checkedChanged, this, setSameOptionsForAll);

    //
    // Определить активность чекбокса "Same options for all"
    //
    auto setSameOptionsAvailable = [this] {
        bool state = d->sameOptions->isChecked();
        bool isEnabled = true;
        for (const auto& fileOptions : d->filesOptions) {
            if (!importTypesForFile(fileOptions.second->filePath).contains(d->currentType())) {
                state = false;
                isEnabled = false;
                break;
            }
        }
        d->sameOptions->setChecked(state);
        d->sameOptions->setEnabled(isEnabled);
        return isEnabled;
    };

    //
    // Соединение комбобокса типов для импорта
    //
    connect(d->documentType, &ComboBox::currentIndexChanged, this,
            [this, setSameOptionsAvailable, configureImportAvailability, setSameOptionsForAll] {
                d->updateParametersVisibility();
                if (d->tree->model()) {
                    const auto a = d->documentType->currentText();
                    d->tree->model()->setData(d->tree->currentIndex(),
                                              d->documentType->currentText(), ImportTypeRole);
                    const bool sameOptionsAvailable = setSameOptionsAvailable();
                    configureImportAvailability();
                    if (sameOptionsAvailable) {
                        setSameOptionsForAll();
                    }
                }
            });

    //
    // Соединение списка файлов
    //
    connect(d->tree, &Tree::currentIndexChanged, this,
            [this, setSameOptionsAvailable](const QModelIndex& _index) {
                d->writeCurrentOptions();
                const bool sameOptionsState = d->sameOptions->isChecked();
                d->currentFile = _index.data().toString();
                d->setImportDocumentTypes(importTypesForFile(d->currentFile));
                d->showOptionsForFile(d->currentFile);
                const bool sameOptionsAvailable = setSameOptionsAvailable();
                if (sameOptionsAvailable) {
                    d->sameOptions->setChecked(sameOptionsState);
                }
            });

    //
    // Соединения кнопок импорта/отмены
    //
    connect(d->importButton, &Button::clicked, this, &ImportDialog::importRequested);
    connect(d->cancelButton, &Button::clicked, this, &ImportDialog::canceled);

    //
    // Если импортируем один файл, прячем лишние виджеты
    //
    if (_importFilePaths.size() == 1) {
        d->tree->setVisible(false);
        d->sameOptions->setVisible(false);
        d->filesCountTitle->setVisible(false);
    }

    //
    // Отображаем настройки текущего файла
    //
    d->setImportDocumentTypes(importTypesForFile(d->currentFile));
    d->showOptionsForFile(d->currentFile);
    d->sameOptions->setChecked(false);
    setSameOptionsAvailable();
}

ImportDialog::~ImportDialog()
{
    d->writeCurrentOptions();
    d->saveSetting();
}

std::vector<std::unique_ptr<BusinessLayer::ImportOptions>> ImportDialog::importOptions() const
{
    d->writeCurrentOptions();
    d->saveSetting();

    std::vector<std::unique_ptr<BusinessLayer::ImportOptions>> options;
    for (auto& fileOptions : d->filesOptions) {
        options.push_back(std::move(fileOptions.second));
    }
    d->filesOptions.clear();
    return options;
}

QWidget* ImportDialog::focusedWidgetAfterShow() const
{
    return d->importCharacters;
}

QWidget* ImportDialog::lastFocusableWidget() const
{
    return d->importButton;
}

void ImportDialog::updateTranslations()
{
    if (d->filesOptions.size() > 1) {
        setTitle(tr("Import data from files"));
    } else {
        QFileInfo fileInfo(d->currentFile);
        setTitle(QString("%1 \"%2\"").arg(tr("Import data from the file"), fileInfo.fileName()));
    }

    d->filesCountTitle->setText(QString(tr("Importing files (%n)", 0, d->importFilePaths.count())));
    d->documentsTitle->setText(tr("Documents"));
    d->importCharacters->setText(tr("Import characters"));
    d->importLocations->setText(tr("Import locations"));
    d->importResearch->setText(tr("Import research"));
    d->textTitle->setText(tr("Text"));
    d->updateImportTextLabel();
    d->keepSceneNumbers->setText(tr("Keep scene numbers"));

    d->sameOptions->setText(tr("Same options for all"));
    d->importButton->setText(tr("Import"));
    d->cancelButton->setText(tr("Cancel"));

    //
    // Обновляем выпадающий список
    //
    d->setImportDocumentTypes(importTypesForFile(d->currentFile));
    d->showOptionsForFile(d->currentFile);

    d->documentType->setLabel(tr("Import to"));

    //
    // После установки текста фиксируем размер виджета опций
    //
    if (d->filesOptions.size() > 1) {
        QList<QPair<CheckBox*, bool>> optionsVisability;
        for (auto* option : {
                 d->importCharacters,
                 d->importLocations,
                 d->importResearch,
                 d->importText,
                 d->keepSceneNumbers,
             }) {
            optionsVisability.append(QPair(option, option->isVisibleTo(d->optionsWidget)));
            option->setVisible(true);
        }
        d->optionsWidget->adjustSize();
        d->optionsWidget->setFixedSize(d->optionsWidget->size());
        for (auto option : optionsVisability) {
            option.first->setVisible(option.second);
        }
    }
}

void ImportDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    auto titleMargins = Ui::DesignSystem::label().margins().toMargins();
    titleMargins.setTop(Ui::DesignSystem::layout().px8());
    titleMargins.setBottom(0);

    d->tree->setBackgroundColor(DesignSystem::color().primary());
    d->tree->setTextColor(DesignSystem::color().onPrimary());

    d->filesCountTitle->setContentsMargins(titleMargins);
    d->filesCountTitle->setBackgroundColor(Ui::DesignSystem::color().background());
    d->filesCountTitle->setTextColor(Ui::DesignSystem::color().onBackground());

    d->optionsWidget->setBackgroundColor(DesignSystem::color().background());

    for (auto label : { d->documentsTitle, d->textTitle }) {
        label->setContentsMargins(titleMargins);
        label->setBackgroundColor(Ui::DesignSystem::color().background());
        label->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    for (auto checkBox : {
             d->importCharacters,
             d->importLocations,
             d->importResearch,
             d->importText,
             d->keepSceneNumbers,
             d->sameOptions,
         }) {
        checkBox->setBackgroundColor(Ui::DesignSystem::color().background());
        checkBox->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    for (auto button : {
             d->importButton,
             d->cancelButton,
         }) {
        button->setBackgroundColor(Ui::DesignSystem::color().accent());
        button->setTextColor(Ui::DesignSystem::color().accent());
    }

    for (auto textField : std::vector<TextField*>{
             d->documentType,
         }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    for (auto combobox : {
             d->documentType,
         }) {
        combobox->setPopupBackgroundColor(Ui::DesignSystem::color().background());
    }

    contentsLayout()->setSpacing(static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->bottomLayout->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12(),
                  Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px16())
            .toMargins());
}

} // namespace Ui
