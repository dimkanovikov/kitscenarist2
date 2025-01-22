#include "screenplay_fountain_importer.h"

#include "screenplay_import_options.h"

#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/templates/text_template.h>

#include <QFile>
#include <QFileInfo>
#include <QSet>

namespace BusinessLayer {

namespace {

/**
 * @brief Типы блоков, которые могут быть в документе
 */
static const QSet<TextParagraphType> kPossibleBlockTypes = {
    TextParagraphType::SceneHeading,    TextParagraphType::SceneCharacters,
    TextParagraphType::Action,          TextParagraphType::Character,
    TextParagraphType::Parenthetical,   TextParagraphType::Dialogue,
    TextParagraphType::Lyrics,          TextParagraphType::Shot,
    TextParagraphType::Transition,      TextParagraphType::InlineNote,
    TextParagraphType::UnformattedText, TextParagraphType::SequenceHeading,
    TextParagraphType::ActHeading,      TextParagraphType::BeatHeading,
};

/**
 * @brief Тип блока по умолчанию
 */
static const TextParagraphType kDefaultBlockType = TextParagraphType::Action;

} // namespace


ScreenplayFountainImporter::ScreenplayFountainImporter()
    : AbstractScreenplayImporter()
    , AbstractFountainImporter(kPossibleBlockTypes, kDefaultBlockType)
{
}

ScreenplayFountainImporter::~ScreenplayFountainImporter() = default;

QVector<AbstractScreenplayImporter::Screenplay> ScreenplayFountainImporter::importScreenplays(
    const ScreenplayImportOptions* _options) const
{
    if (_options->importText == false) {
        return {};
    }

    //
    // Открываем файл
    //
    QFile fountainFile(_options->filePath);
    if (!fountainFile.open(QIODevice::ReadOnly)) {
        return {};
    }

    //
    // Импортируем
    //
    auto screenplay = screenplayText(fountainFile.readAll(), _options->keepSceneNumbers);
    if (screenplay.name.isEmpty()) {
        screenplay.name = QFileInfo(_options->filePath).completeBaseName();
    }

    return { screenplay };
}

AbstractScreenplayImporter::Screenplay ScreenplayFountainImporter::screenplayText(
    const QString& _screenplayText, bool _keepSceneNumbers) const
{
    Screenplay result;
    result.text = documentText(_screenplayText, _keepSceneNumbers);
    return result;
}

QString ScreenplayFountainImporter::characterName(const QString& _text) const
{
    return ScreenplayCharacterParser::name(_text);
}

QString ScreenplayFountainImporter::locationName(const QString& _text) const
{
    return ScreenplaySceneHeadingParser::location(_text);
}

bool ScreenplayFountainImporter::placeDialoguesInTable() const
{
    return false;
}

} // namespace BusinessLayer
