#include "create_document_dialog.h"

#include "create_document_dialog_option.h"

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <ui/design_system/design_system.h>
#include <ui/layouts/flow_layout/flow_layout.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/dialog_helper.h>
#include <utils/helpers/names_generator.h>
#include <utils/helpers/ui_helper.h>

#include <QFileDialog>
#include <QGridLayout>
#include <QScrollArea>
#include <QSettings>


namespace Ui {

namespace {
const QLatin1String kLastSelectedType("widgets/create-document-dialog/last-selected-type");
}

class CreateDocumentDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    void updateDocumentInfo(Domain::DocumentObjectType _type);


    QScrollArea* content = nullptr;
    Widget* optionsContainer = nullptr;
    QVBoxLayout* optionsLayout = nullptr;
    AbstractLabel* storyTitle;
    FlowLayout* storyOptionsLayout = nullptr;
    AbstractLabel* storyWorldTitle;
    AbstractLabel* otherTitle;
    QVector<CreateDocumentDialogOption*> options;

    H6Label* title = nullptr;
    Body1Label* documentInfo = nullptr;
    TextField* documentName = nullptr;
    bool insertIntoParentEnabled = false;
    CheckBox* makeEpisodic = nullptr;
    TextField* episodesAmount = nullptr;
    TextField* importFilePath = nullptr;
    CheckBox* insertIntoParent = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* cancelButton = nullptr;
    Button* createButton = nullptr;
};

CreateDocumentDialog::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent))
    , optionsContainer(new Widget(_parent))
    , optionsLayout(new QVBoxLayout(optionsContainer))
    , storyTitle(new Subtitle1Label(_parent))
    , storyOptionsLayout(new FlowLayout)
    , storyWorldTitle(new Subtitle1Label(_parent))
    , otherTitle(new Subtitle1Label(_parent))
    , title(new H6Label(_parent))
    , documentInfo(new Body1Label(_parent))
    , documentName(new TextField(_parent))
    , makeEpisodic(new CheckBox(_parent))
    , episodesAmount(new TextField(_parent))
    , importFilePath(new TextField(_parent))
    , insertIntoParent(new CheckBox(_parent))
    , buttonsLayout(new QHBoxLayout)
    , cancelButton(new Button(_parent))
    , createButton(new Button(_parent))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setFrameShape(QFrame::NoFrame);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    content->setVerticalScrollBar(new ScrollBar);

    auto makeOption = [this, &_parent](Domain::DocumentObjectType _type) {
        auto option = new CreateDocumentDialogOption(_type, _parent);
        options.append(option);
        return option;
    };

    optionsLayout->setContentsMargins({});
    optionsLayout->setSpacing(0);
    optionsLayout->addWidget(storyTitle);
    {
        storyOptionsLayout->setContentsMargins({});
        storyOptionsLayout->setSpacing(0);
        if (settingsValue(DataStorageLayer::kComponentsScreenplayAvailableKey).toBool()) {
            storyOptionsLayout->addWidget(makeOption(Domain::DocumentObjectType::Screenplay));
        }
        if (settingsValue(DataStorageLayer::kComponentsComicBookAvailableKey).toBool()) {
            storyOptionsLayout->addWidget(makeOption(Domain::DocumentObjectType::ComicBook));
        }
        if (settingsValue(DataStorageLayer::kComponentsAudioplayAvailableKey).toBool()) {
            storyOptionsLayout->addWidget(makeOption(Domain::DocumentObjectType::Audioplay));
        }
        if (settingsValue(DataStorageLayer::kComponentsStageplayAvailableKey).toBool()) {
            storyOptionsLayout->addWidget(makeOption(Domain::DocumentObjectType::Stageplay));
        }
        if (settingsValue(DataStorageLayer::kComponentsNovelAvailableKey).toBool()) {
            storyOptionsLayout->addWidget(makeOption(Domain::DocumentObjectType::Novel));
        }
        optionsLayout->addLayout(storyOptionsLayout, 1);
    }
    optionsLayout->addWidget(storyWorldTitle);
    {
        auto layout = new FlowLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(makeOption(Domain::DocumentObjectType::Character));
        layout->addWidget(makeOption(Domain::DocumentObjectType::Location));
        layout->addWidget(makeOption(Domain::DocumentObjectType::World));
        optionsLayout->addLayout(layout);
    }
    optionsLayout->addWidget(otherTitle);
    {
        auto layout = new FlowLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        if (settingsValue(DataStorageLayer::kComponentsSimpleTextAvailableKey).toBool()) {
            layout->addWidget(makeOption(Domain::DocumentObjectType::Folder));
            layout->addWidget(makeOption(Domain::DocumentObjectType::SimpleText));
        }
        layout->addWidget(makeOption(Domain::DocumentObjectType::MindMap));
        layout->addWidget(makeOption(Domain::DocumentObjectType::ImagesGallery));
        layout->addWidget(makeOption(Domain::DocumentObjectType::Presentation));
        optionsLayout->addLayout(layout);
    }

    options.constFirst()->setChecked(true);

    content->setWidget(optionsContainer);
    content->setWidgetResizable(true);

    UiHelper::setFocusPolicyRecursively(content, Qt::NoFocus);

    documentName->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    episodesAmount->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    episodesAmount->setText("8");
    importFilePath->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    importFilePath->setTrailingIcon(u8"\U000f0256");

    insertIntoParent->hide();
    makeEpisodic->hide();
    episodesAmount->hide();

    buttonsLayout->setContentsMargins({});
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(createButton);
}

void CreateDocumentDialog::Implementation::updateDocumentInfo(Domain::DocumentObjectType _type)
{
    const QHash<Domain::DocumentObjectType, QString> documenTypeToTitle = {
        { Domain::DocumentObjectType::Folder, tr("Add folder") },
        { Domain::DocumentObjectType::SimpleText, tr("Add text") },
        { Domain::DocumentObjectType::Character, tr("Add character") },
        { Domain::DocumentObjectType::Location, tr("Add location") },
        { Domain::DocumentObjectType::World, tr("Add world") },
        { Domain::DocumentObjectType::Screenplay, tr("Add screenplay") },
        { Domain::DocumentObjectType::ComicBook, tr("Add comic book") },
        { Domain::DocumentObjectType::Audioplay, tr("Add audioplay") },
        { Domain::DocumentObjectType::Stageplay, tr("Add stageplay") },
        { Domain::DocumentObjectType::Novel, tr("Add novel") },
        { Domain::DocumentObjectType::MindMap, tr("Add mind map") },
        { Domain::DocumentObjectType::ImagesGallery, tr("Add image gallery") },
        { Domain::DocumentObjectType::Presentation, tr("Add presentation") },
    };
    const QHash<Domain::DocumentObjectType, QString> documenTypeToInfo = {
        { Domain::DocumentObjectType::Folder,
          tr("Create a folder to group documents inside the story.") },
        { Domain::DocumentObjectType::SimpleText,
          tr("Create a plain text document to write out ideas and notes.") },
        { Domain::DocumentObjectType::Character,
          tr("Create a document with full character's description to track his relations and "
             "follow his journey within the story.") },
        { Domain::DocumentObjectType::Location,
          tr("Create a document to note down the location's description and keep the details.") },
        { Domain::DocumentObjectType::World,
          tr("Create a document with detailed world description to keep all the notes in one "
             "place") },
        { Domain::DocumentObjectType::Screenplay,
          tr("Create a document set to streamline your work on the feature film, series, or "
             "animation.") },
        { Domain::DocumentObjectType::ComicBook,
          tr("Create a document set to streamline your work on the comic book, graphic novel, or "
             "manga.") },
        { Domain::DocumentObjectType::Audioplay,
          tr("Create a document set to streamline your work on the audio drama, or podcast.") },
        { Domain::DocumentObjectType::Stageplay,
          tr("Create a document set to streamline your work on the stage play, or musical.") },
        { Domain::DocumentObjectType::Novel,
          tr("Create a document set to streamline your work on the fiction book.") },
        { Domain::DocumentObjectType::MindMap,
          tr("Create a mind map to brainstorm ideas and plan your story development.") },
        { Domain::DocumentObjectType::ImagesGallery,
          tr("Create a moodboard with atmospheric images or photos.") },
        { Domain::DocumentObjectType::Presentation,
          tr("Create a presentation document to store valuable outside content with your "
             "project.") },
    };

    title->setText(documenTypeToTitle.value(_type));
    documentInfo->setText(documenTypeToInfo.value(_type));
    if (_type == Domain::DocumentObjectType::Character
        || _type == Domain::DocumentObjectType::Location
        || _type == Domain::DocumentObjectType::World) {
        insertIntoParent->hide();
    } else {
        insertIntoParent->setVisible(insertIntoParentEnabled);
    }

    if (_type == Domain::DocumentObjectType::Presentation) {
        importFilePath->show();
    } else {
        importFilePath->hide();
    }

    if (_type == Domain::DocumentObjectType::Character) {
        NamesGenerator::bind(documentName);
    } else {
        NamesGenerator::unbind(documentName);
    }

    if (_type == Domain::DocumentObjectType::Screenplay) {
        makeEpisodic->show();
        episodesAmount->setVisible(makeEpisodic->isChecked());
    } else {
        makeEpisodic->hide();
        episodesAmount->hide();
    }
}


// ****

CreateDocumentDialog::CreateDocumentDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    setAcceptButton(d->createButton);
    setRejectButton(d->cancelButton);

    contentsLayout()->setContentsMargins({});
    contentsLayout()->setSpacing(0);
    int row = 0;
    contentsLayout()->addWidget(d->title, row++, 1, 1, 1);
    contentsLayout()->addWidget(d->documentInfo, row++, 1, 1, 1);
    contentsLayout()->addWidget(d->documentName, row++, 1, 1, 1);
    contentsLayout()->addWidget(d->makeEpisodic, row++, 1, 1, 1);
    contentsLayout()->addWidget(d->episodesAmount, row++, 1, 1, 1);
    contentsLayout()->addWidget(d->importFilePath, row++, 1, 1, 1);
    contentsLayout()->addWidget(d->insertIntoParent, row++, 1, 1, 1);
    contentsLayout()->setRowStretch(row++, 1);
    contentsLayout()->addLayout(d->buttonsLayout, row++, 1, 1, 1);
    contentsLayout()->addWidget(d->content, 0, 0, row, 1);
    contentsLayout()->setColumnStretch(0, 2);
    contentsLayout()->setColumnStretch(1, 1);


    for (auto option : std::as_const(d->options)) {
        connect(option, &CreateDocumentDialogOption::checkedChanged, this, [this](bool _isChecked) {
            if (!_isChecked) {
                return;
            }

            for (auto option : std::as_const(d->options)) {
                if (option != sender()) {
                    option->setChecked(false);
                    continue;
                }

                d->updateDocumentInfo(option->documentType());
            }
        });
    }
    connect(d->documentName, &TextField::textChanged, this,
            [this] { d->documentName->clearError(); });
    connect(d->makeEpisodic, &CheckBox::checkedChanged, this, [this](bool _checked) {
        d->episodesAmount->setVisible(d->makeEpisodic->isVisible() && _checked);
    });
    connect(d->episodesAmount, &TextField::textChanged, this, [this] {
        d->episodesAmount->clearError();

        if (d->episodesAmount->text().isEmpty()) {
            return;
        }

        bool ok = false;
        const auto amount = d->episodesAmount->text().toInt(&ok);
        if (!ok || amount < 0) {
            d->episodesAmount->setError(tr("Should be a number"));
        }
    });
    connect(d->importFilePath, &TextField::textChanged, d->importFilePath, &TextField::clearError);
    connect(d->importFilePath, &TextField::trailingIconPressed, this, [this] {
        const auto projectImportFolder
            = settingsValue(DataStorageLayer::kProjectImportFolderKey).toString();
        const auto filePath = QFileDialog::getOpenFileName(
            this, tr("Choose the file to import"), projectImportFolder, DialogHelper::pdfFilter());
        d->importFilePath->setText(filePath);
        if (!filePath.isEmpty()) {
            setSettingsValue(DataStorageLayer::kProjectImportFolderKey, filePath);
        }
    });
    connect(d->createButton, &Button::clicked, this, [this] {
        Domain::DocumentObjectType documentType = Domain::DocumentObjectType::Undefined;
        for (auto option : std::as_const(d->options)) {
            if (option->isChecked()) {
                documentType = option->documentType();
                break;
            }
        }

        //
        // Персонажи и локации нельзя создавать без названия
        //
        if (d->documentName->text().isEmpty()) {
            QString errorMessage;
            if (documentType == Domain::DocumentObjectType::Character) {
                errorMessage = tr("The character should have a name");
            } else if (documentType == Domain::DocumentObjectType::Location) {
                errorMessage = tr("The location should have a name");
            } else if (documentType == Domain::DocumentObjectType::World) {
                errorMessage = tr("The world should have a name");
            }
            if (!errorMessage.isEmpty()) {
                d->documentName->setFocus();
                d->documentName->setError(errorMessage);
                return;
            }
        }

        //
        // Если нужно, преобразуем тип в эпизодический
        //
        if (d->makeEpisodic->isVisibleTo(this) && d->makeEpisodic->isChecked()) {
            switch (documentType) {
            default: {
                break;
            }

            case Domain::DocumentObjectType::Screenplay: {
                documentType = Domain::DocumentObjectType::ScreenplaySeries;
                break;
            }
            }
        }

        //
        // Количество эпизодов должно быть задано корректно
        //
        if (d->episodesAmount->isVisibleTo(this) && !d->episodesAmount->error().isEmpty()) {
            d->episodesAmount->setFocus();
            return;
        }

        //
        // Презентацию нельзя создать без исходного файла
        //
        if (documentType == Domain::DocumentObjectType::Presentation) {
            if (d->importFilePath->text().isEmpty()) {
                d->importFilePath->setError(tr("Please, select file for importing"));
                return;
            }
        }

        //
        // Проверить доступность файла
        //
        auto fileIsAvailable = [this]() {
            QFile file(importFilePath());
            if (!file.exists()) {
                return false;
            }
            if (file.open(QIODevice::ReadOnly)) {
                file.close();
            } else {
                return false;
            }
            return true;
        };

        //
        // Если задан файл, проверим что он доступен
        //
        QString filePath;
        if (d->importFilePath->isVisibleTo(this) && !importFilePath().isEmpty()) {
            if (fileIsAvailable()) {
                filePath = d->importFilePath->text();
            } else {
                QFile file(importFilePath());
                if (!file.exists()) {
                    setImportFileError(tr("The file doesn't exist"));
                    return;
                }
                if (file.open(QIODevice::ReadOnly)) {
                    file.close();
                } else {
                    setImportFileError(tr("Can't read data from the file"));
                    return;
                }
            }
        }

        emit createPressed(documentType, d->documentName->text(), filePath);
    });
    connect(d->cancelButton, &Button::clicked, this, &CreateDocumentDialog::hideDialog);
}

CreateDocumentDialog::~CreateDocumentDialog() = default;

Domain::DocumentObjectType CreateDocumentDialog::lastSelectedType() const
{
    return static_cast<Domain::DocumentObjectType>(QSettings().value(kLastSelectedType).toInt());
}

void CreateDocumentDialog::saveSelectedType()
{
    Domain::DocumentObjectType documentType = Domain::DocumentObjectType::Undefined;
    for (auto option : std::as_const(d->options)) {
        if (option->isChecked()) {
            documentType = option->documentType();
            break;
        }
    }
    if (documentType == Domain::DocumentObjectType::Undefined) {
        return;
    }

    QSettings().setValue(kLastSelectedType, static_cast<int>(documentType));
}

void CreateDocumentDialog::setDocumentType(Domain::DocumentObjectType _type)
{
    for (auto option : std::as_const(d->options)) {
        if (option->documentType() == _type) {
            option->setChecked(true);
            //
            // Скорректируем прокрутку для опций идущих за первой группой
            //
            if (option->documentType() >= Domain::DocumentObjectType::Characters) {
                d->content->ensureWidgetVisible(option);
            }
            break;
        }
    }
}

void CreateDocumentDialog::setNameError(const QString& _error)
{
    d->documentName->setError(_error);
}

void CreateDocumentDialog::setImportFileError(const QString& _error)
{
    d->importFilePath->setError(_error);
}

void CreateDocumentDialog::setInsertionParent(const QString& _parentName)
{
    if (_parentName.isEmpty()) {
        d->insertIntoParentEnabled = false;
        d->insertIntoParent->hide();
        return;
    }

    d->insertIntoParentEnabled = true;
    d->insertIntoParent->setText(QString("%1 \"%2\"").arg(tr("Insert into"), _parentName));
    d->insertIntoParent->setChecked(true);
    d->insertIntoParent->show();
}

int CreateDocumentDialog::episodesAmount() const
{
    return d->episodesAmount->text().toInt();
}

bool CreateDocumentDialog::needInsertIntoParent() const
{
    return d->insertIntoParent->isChecked();
}

QString CreateDocumentDialog::importFilePath() const
{
    return d->importFilePath->text();
}

QWidget* CreateDocumentDialog::focusedWidgetAfterShow() const
{
    return d->documentName;
}

QWidget* CreateDocumentDialog::lastFocusableWidget() const
{
    return d->createButton;
}

void CreateDocumentDialog::updateTranslations()
{
    d->storyTitle->setText(tr("Story"));
    d->storyWorldTitle->setText(tr("World of story"));
    d->otherTitle->setText(tr("Structure & notes"));

    d->title->setText(tr("Add document"));
    for (auto option : std::as_const(d->options)) {
        if (option->isChecked()) {
            d->updateDocumentInfo(option->documentType());
            break;
        }
    }
    d->documentName->setLabel(tr("Name"));
    d->makeEpisodic->setText(tr("Split to episodes"));
    d->episodesAmount->setLabel(tr("Episodes amount"));
    d->importFilePath->setLabel(tr("Choose file for importing"));
    d->importFilePath->setTrailingIconToolTip(tr("Choose file for importing"));
    d->cancelButton->setText(tr("Cancel"));
    d->createButton->setText(tr("Create"));
}

void CreateDocumentDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    contentsLayout()->setColumnMinimumWidth(1, Ui::DesignSystem::layout().px(299));
    setContentFixedWidth(
        Ui::DesignSystem::layout().px(d->storyOptionsLayout->count() > 3 ? 898 : 754));

    d->optionsContainer->setBackgroundColor(DesignSystem::color().surface());
    d->content->setFixedHeight(DesignSystem::layout().px(556));
    d->optionsLayout->setContentsMargins(
        isLeftToRight() ? DesignSystem::layout().px16() : 0.0, DesignSystem::layout().px8(),
        isLeftToRight() ? 0.0 : DesignSystem::layout().px16(), DesignSystem::layout().px16());
    for (auto title : std::vector<Widget*>{
             d->storyTitle,
             d->storyWorldTitle,
             d->otherTitle,
         }) {
        title->setBackgroundColor(DesignSystem::color().surface());
        title->setTextColor(DesignSystem::color().onSurface());
        title->setContentsMargins(DesignSystem::layout().px16(), DesignSystem::layout().px12(),
                                  DesignSystem::layout().px16(), DesignSystem::layout().px4());
    }
    for (auto option : std::as_const(d->options)) {
        option->setBackgroundColor(DesignSystem::color().surface());
        option->setTextColor(DesignSystem::color().onSurface());
        option->setContentsMargins(DesignSystem::layout().px8(), DesignSystem::layout().px8(),
                                   DesignSystem::layout().px8(), DesignSystem::layout().px8());
    }

    d->title->setBackgroundColor(Ui::DesignSystem::color().background());
    d->title->setTextColor(Ui::DesignSystem::color().onBackground());
    d->title->setContentsMargins(DesignSystem::layout().px24(), DesignSystem::layout().px24(),
                                 DesignSystem::layout().px24(), DesignSystem::layout().px16());

    for (auto textField : std::vector<TextField*>{ d->documentName, d->importFilePath }) {
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setCustomMargins(
            { DesignSystem::layout().px24(), DesignSystem::compactLayout().px8(),
              DesignSystem::layout().px24(), DesignSystem::compactLayout().px8() });
    }

    auto documentInfoMargins = Ui::DesignSystem::label().margins().toMargins();
    documentInfoMargins.setTop(0);
    d->documentInfo->setContentsMargins(documentInfoMargins);
    d->documentInfo->setTextColor(Ui::DesignSystem::color().onBackground());
    d->documentInfo->setBackgroundColor(Ui::DesignSystem::color().background());

    d->makeEpisodic->setTextColor(Ui::DesignSystem::color().onBackground());
    d->makeEpisodic->setBackgroundColor(Ui::DesignSystem::color().background());
    d->episodesAmount->setTextColor(Ui::DesignSystem::color().onBackground());
    d->episodesAmount->setBackgroundColor(Ui::DesignSystem::color().onBackground());

    d->insertIntoParent->setTextColor(Ui::DesignSystem::color().onBackground());
    d->insertIntoParent->setBackgroundColor(Ui::DesignSystem::color().background());

    for (auto button : { d->cancelButton, d->createButton }) {
        button->setBackgroundColor(Ui::DesignSystem::color().accent());
        button->setTextColor(Ui::DesignSystem::color().accent());
    }

    d->buttonsLayout->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12(),
                  Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px16())
            .toMargins());
}

} // namespace Ui
