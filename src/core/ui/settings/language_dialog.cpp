#include "language_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/link_label.h>
#include <ui/widgets/radio_button/percent_radio_button.h>
#include <ui/widgets/radio_button/radio_button_group.h>

#include <QGridLayout>
#include <QUrl>


namespace Ui {

namespace {
const char* kLanguageKey = "language";
} // namespace

class LanguageDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Получить список всех кнопок языков
     */
    std::vector<PercentRadioButton*> languages() const;


    PercentRadioButton* arabic = nullptr;
    PercentRadioButton* azerbaijani = nullptr;
    PercentRadioButton* belarusian = nullptr;
    PercentRadioButton* catalan = nullptr;
    PercentRadioButton* chinese = nullptr;
    PercentRadioButton* croatian = nullptr;
    PercentRadioButton* danish = nullptr;
    PercentRadioButton* english = nullptr;
    PercentRadioButton* esperanto = nullptr;
    PercentRadioButton* french = nullptr;
    PercentRadioButton* galician = nullptr;
    PercentRadioButton* german = nullptr;
    PercentRadioButton* hebrew = nullptr;
    PercentRadioButton* hindi = nullptr;
    PercentRadioButton* hungarian = nullptr;
    PercentRadioButton* indonesian = nullptr;
    PercentRadioButton* italian = nullptr;
    PercentRadioButton* kannada = nullptr;
    PercentRadioButton* korean = nullptr;
    PercentRadioButton* persian = nullptr;
    PercentRadioButton* polish = nullptr;
    PercentRadioButton* portuguese = nullptr;
    PercentRadioButton* portugueseBrazil = nullptr;
    PercentRadioButton* romanian = nullptr;
    PercentRadioButton* russian = nullptr;
    PercentRadioButton* slovenian = nullptr;
    PercentRadioButton* spanish = nullptr;
    PercentRadioButton* tagalog = nullptr;
    PercentRadioButton* tamil = nullptr;
    PercentRadioButton* turkish = nullptr;
    PercentRadioButton* ukrainian = nullptr;

    Body1LinkLabel* languageHowToAddLink = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Body2Label* translationProgressLabel = nullptr;
    Button* improveButton = nullptr;
    Button* closeButton = nullptr;
};

LanguageDialog::Implementation::Implementation(QWidget* _parent)
    : arabic(new PercentRadioButton(_parent, 21))
    , azerbaijani(new PercentRadioButton(_parent, 100))
    , belarusian(new PercentRadioButton(_parent, 64))
    , catalan(new PercentRadioButton(_parent, 79))
    , chinese(new PercentRadioButton(_parent, 7))
    , croatian(new PercentRadioButton(_parent, 81))
    , danish(new PercentRadioButton(_parent, 100))
    , english(new PercentRadioButton(_parent, 100))
    , esperanto(new PercentRadioButton(_parent, 12))
    , french(new PercentRadioButton(_parent, 82))
    , galician(new PercentRadioButton(_parent, 89))
    , german(new PercentRadioButton(_parent, 86))
    , hebrew(new PercentRadioButton(_parent, 100))
    , hindi(new PercentRadioButton(_parent, 47))
    , hungarian(new PercentRadioButton(_parent, 46))
    , indonesian(new PercentRadioButton(_parent, 13))
    , italian(new PercentRadioButton(_parent, 38))
    , kannada(new PercentRadioButton(_parent, 100))
    , korean(new PercentRadioButton(_parent, 100))
    , persian(new PercentRadioButton(_parent, 82))
    , polish(new PercentRadioButton(_parent, 41))
    , portuguese(new PercentRadioButton(_parent, 17))
    , portugueseBrazil(new PercentRadioButton(_parent, 87))
    , romanian(new PercentRadioButton(_parent, 81))
    , russian(new PercentRadioButton(_parent, 100))
    , slovenian(new PercentRadioButton(_parent, 100))
    , spanish(new PercentRadioButton(_parent, 83))
    , tagalog(new PercentRadioButton(_parent, 24))
    , tamil(new PercentRadioButton(_parent, 58))
    , turkish(new PercentRadioButton(_parent, 100))
    , ukrainian(new PercentRadioButton(_parent, 100))
    , languageHowToAddLink(new Body1LinkLabel(_parent))
    , translationProgressLabel(new Body2Label(_parent))
    , improveButton(new Button(_parent))
    , closeButton(new Button(_parent))
{
    arabic->setText("اَلْعَرَبِيَّةُ");
    arabic->setProperty(kLanguageKey, QLocale::Arabic);
    azerbaijani->setText("Azərbaycan");
    azerbaijani->setProperty(kLanguageKey, QLocale::Azerbaijani);
    belarusian->setText("Беларуский");
    belarusian->setProperty(kLanguageKey, QLocale::Belarusian);
    catalan->setText("Català");
    catalan->setProperty(kLanguageKey, QLocale::Catalan);
    chinese->setText("汉语");
    chinese->setProperty(kLanguageKey, QLocale::Chinese);
    croatian->setText("Hrvatski");
    croatian->setProperty(kLanguageKey, QLocale::Croatian);
    danish->setText("Dansk");
    danish->setProperty(kLanguageKey, QLocale::Danish);
    english->setChecked(true);
    english->setText("English");
    english->setProperty(kLanguageKey, QLocale::English);
    esperanto->setText("Esperanto");
    esperanto->setProperty(kLanguageKey, QLocale::Esperanto);
    french->setText("Français");
    french->setProperty(kLanguageKey, QLocale::French);
    galician->setText("Galego");
    galician->setProperty(kLanguageKey, QLocale::Galician);
    german->setText("Deutsch");
    german->setProperty(kLanguageKey, QLocale::German);
    hebrew->setText("עִבְרִית");
    hebrew->setProperty(kLanguageKey, QLocale::Hebrew);
    hindi->setText("हिन्दी");
    hindi->setProperty(kLanguageKey, QLocale::Hindi);
    hungarian->setText("Magyar");
    hungarian->setProperty(kLanguageKey, QLocale::Hungarian);
    indonesian->setText("Indonesian");
    indonesian->setProperty(kLanguageKey, QLocale::Indonesian);
    italian->setText("Italiano");
    italian->setProperty(kLanguageKey, QLocale::Italian);
    kannada->setText("ಕನ್ನಡ");
    kannada->setProperty(kLanguageKey, QLocale::Kannada);
    korean->setText("한국어");
    korean->setProperty(kLanguageKey, QLocale::Korean);
    persian->setText("فارسی");
    persian->setProperty(kLanguageKey, QLocale::Persian);
    polish->setText("Polski");
    polish->setProperty(kLanguageKey, QLocale::Polish);
    portuguese->setText("Português");
    portuguese->setProperty(kLanguageKey, QLocale::LastLanguage + 1);
    portugueseBrazil->setText("Português Brasileiro");
    portugueseBrazil->setProperty(kLanguageKey, QLocale::Portuguese);
    romanian->setText("Română");
    romanian->setProperty(kLanguageKey, QLocale::Romanian);
    russian->setText("Русский");
    russian->setProperty(kLanguageKey, QLocale::Russian);
    slovenian->setText("Slovenski");
    slovenian->setProperty(kLanguageKey, QLocale::Slovenian);
    spanish->setText("Español");
    spanish->setProperty(kLanguageKey, QLocale::Spanish);
    tagalog->setText("Tagalog");
    tagalog->setProperty(kLanguageKey, QLocale::Filipino);
    tamil->setText("தமிழ்");
    tamil->setProperty(kLanguageKey, QLocale::Tamil);
    turkish->setText("Türkçe");
    turkish->setProperty(kLanguageKey, QLocale::Turkish);
    ukrainian->setText("Українська");
    ukrainian->setProperty(kLanguageKey, QLocale::Ukrainian);

    languageHowToAddLink->setLink(QUrl("https://starc.app/translate"));
    translationProgressLabel->hide();
    improveButton->hide();

    buttonsLayout = new QHBoxLayout;
    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(translationProgressLabel, 0, Qt::AlignVCenter);
    buttonsLayout->addWidget(improveButton);
    buttonsLayout->addWidget(closeButton);

    auto projectLocationGroup = new RadioButtonGroup(_parent);
    for (auto language : languages()) {
        projectLocationGroup->add(language);
    }
}

std::vector<PercentRadioButton*> LanguageDialog::Implementation::languages() const
{
    return {
        arabic,     azerbaijani,      belarusian, catalan,  chinese,   croatian, danish,
        english,    esperanto,        french,     galician, german,    hebrew,   hindi,
        hungarian,  indonesian,       italian,    kannada,  korean,    persian,  polish,
        portuguese, portugueseBrazil, romanian,   russian,  slovenian, spanish,  tagalog,
        tamil,      turkish,          ukrainian,
    };
}


// ****


LanguageDialog::LanguageDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    setRejectButton(d->closeButton);

    auto buildFocusChain = [](const QVector<PercentRadioButton*>& _buttons) {
        PercentRadioButton* previousButton = nullptr;
        for (auto button : _buttons) {
            if (previousButton != nullptr) {
                setTabOrder(previousButton, button);
            }
            previousButton = button;
        }
    };
    buildFocusChain({
        d->azerbaijani,
        d->belarusian,
        d->catalan,
        d->danish,
        d->german,
        d->english,
        d->spanish,
        d->esperanto,
        d->french,
        d->galician,
        d->croatian,
        d->indonesian,
        d->italian,
        d->hungarian,
        d->polish,
        d->portuguese,
        d->portugueseBrazil,
        d->romanian,
        d->russian,
        d->slovenian,
        d->tagalog,
        d->turkish,
        d->ukrainian,
        d->arabic,
        d->chinese,
        d->hebrew,
        d->hindi,
        d->tamil,
        d->kannada,
        d->persian,
        d->korean,
    });

    int row = 0;
    contentsLayout()->addWidget(d->azerbaijani, row++, 0);
    contentsLayout()->addWidget(d->belarusian, row++, 0);
    contentsLayout()->addWidget(d->catalan, row++, 0);
    contentsLayout()->addWidget(d->danish, row++, 0);
    contentsLayout()->addWidget(d->german, row++, 0);
    contentsLayout()->addWidget(d->english, row++, 0);
    contentsLayout()->addWidget(d->spanish, row++, 0);
    contentsLayout()->addWidget(d->esperanto, row++, 0);
    //
    int rowForSecondColumn = 0;
    contentsLayout()->addWidget(d->french, rowForSecondColumn++, 1);
    contentsLayout()->addWidget(d->galician, rowForSecondColumn++, 1);
    contentsLayout()->addWidget(d->croatian, rowForSecondColumn++, 1);
    contentsLayout()->addWidget(d->indonesian, rowForSecondColumn++, 1);
    contentsLayout()->addWidget(d->italian, rowForSecondColumn++, 1);
    contentsLayout()->addWidget(d->hungarian, rowForSecondColumn++, 1);
    contentsLayout()->addWidget(d->polish, rowForSecondColumn++, 1);
    contentsLayout()->addWidget(d->portuguese, rowForSecondColumn++, 1);
    int rowForThirdColumn = 0;
    contentsLayout()->addWidget(d->portugueseBrazil, rowForThirdColumn++, 2);
    contentsLayout()->addWidget(d->romanian, rowForThirdColumn++, 2);
    contentsLayout()->addWidget(d->russian, rowForThirdColumn++, 2);
    contentsLayout()->addWidget(d->slovenian, rowForThirdColumn++, 2);
    contentsLayout()->addWidget(d->tagalog, rowForThirdColumn++, 2);
    contentsLayout()->addWidget(d->turkish, rowForThirdColumn++, 2);
    contentsLayout()->addWidget(d->ukrainian, rowForThirdColumn++, 2);
    //
    int rowForFifthColumn = 0;
    contentsLayout()->addWidget(d->arabic, rowForFifthColumn++, 3);
    contentsLayout()->addWidget(d->chinese, rowForFifthColumn++, 3);
    contentsLayout()->addWidget(d->hebrew, rowForFifthColumn++, 3);
    contentsLayout()->addWidget(d->hindi, rowForFifthColumn++, 3);
    contentsLayout()->addWidget(d->tamil, rowForFifthColumn++, 3);
    contentsLayout()->addWidget(d->kannada, rowForFifthColumn++, 3);
    contentsLayout()->addWidget(d->persian, rowForFifthColumn++, 3);
    contentsLayout()->addWidget(d->korean, rowForFifthColumn++, 3);
    //
    contentsLayout()->setRowStretch(row++, 1);
    contentsLayout()->setColumnStretch(4, 1);
    contentsLayout()->addWidget(d->languageHowToAddLink, row++, 0, 1, 5);
    contentsLayout()->addLayout(d->buttonsLayout, row++, 0, 1, 5);

    for (auto radioButton : d->languages()) {
        connect(radioButton, &PercentRadioButton::checkedChanged, this,
                [this, radioButton](bool _checked) {
                    if (_checked) {
                        auto language = radioButton->property(kLanguageKey).toInt();
                        emit languageChanged(static_cast<QLocale::Language>(language));
                        if (radioButton->percents == 100) {
                            d->translationProgressLabel->hide();
                            d->improveButton->hide();
                        } else {
                            d->translationProgressLabel->setText(
                                tr("Translation is ready for %1%").arg(radioButton->percents));
                            d->translationProgressLabel->show();
                            d->improveButton->show();
                        }
                    }
                });
    }
    connect(d->improveButton, &Button::clicked, d->languageHowToAddLink, &Body1LinkLabel::clicked);
    connect(d->closeButton, &Button::clicked, this, &LanguageDialog::hideDialog);
}

void LanguageDialog::setCurrentLanguage(QLocale::Language _language)
{
    for (auto radioButton : d->languages()) {
        if (radioButton->property(kLanguageKey).toInt() == _language) {
            radioButton->setChecked(true);
            break;
        }
    }
}

LanguageDialog::~LanguageDialog() = default;

QWidget* LanguageDialog::focusedWidgetAfterShow() const
{
    return d->azerbaijani;
}

QWidget* LanguageDialog::lastFocusableWidget() const
{
    return d->closeButton;
}

void LanguageDialog::updateTranslations()
{
    setTitle(tr("Change application language"));

    d->languageHowToAddLink->setText(
        tr("Did not find your preffered language? Read how you can add it yourself."));

    d->improveButton->setText(tr("Improve"));
    d->closeButton->setText(tr("Close"));
}

void LanguageDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    setContentMaximumWidth(Ui::DesignSystem::layout().px(800));

    const auto languages = d->languages();
    for (auto languge : languages) {
        languge->setBackgroundColor(Ui::DesignSystem::color().background());
        languge->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    d->languageHowToAddLink->setContentsMargins(Ui::DesignSystem::label().margins().toMargins());
    d->languageHowToAddLink->setBackgroundColor(DesignSystem::color().background());
    d->languageHowToAddLink->setTextColor(DesignSystem::color().secondary());
    d->translationProgressLabel->setContentsMargins(0, 0, Ui::DesignSystem::layout().px24(), 0);
    d->translationProgressLabel->setBackgroundColor(DesignSystem::color().background());
    d->translationProgressLabel->setTextColor(DesignSystem::color().onBackground());

    for (auto button : {
             d->improveButton,
             d->closeButton,
         }) {
        button->setBackgroundColor(Ui::DesignSystem::color().secondary());
        button->setTextColor(Ui::DesignSystem::color().secondary());
    }

    contentsLayout()->setSpacing(static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->buttonsLayout->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12(),
                  Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px16())
            .toMargins());
}

} // namespace Ui
