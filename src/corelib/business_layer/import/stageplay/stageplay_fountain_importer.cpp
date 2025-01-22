#include "stageplay_fountain_importer.h"

#include <business_layer/import/import_options.h>
#include <business_layer/model/stageplay/text/stageplay_text_block_parser.h>
#include <business_layer/model/text/text_model_xml.h>
#include <business_layer/templates/stageplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <utils/helpers/text_helper.h>

#include <QFileInfo>
#include <QSet>


namespace BusinessLayer {

namespace {

/**
 * @brief Типы блоков, которые могут быть в документе
 */
static const QSet<TextParagraphType> kPossibleBlockTypes = {
    TextParagraphType::SceneHeading, TextParagraphType::Character,
    TextParagraphType::Dialogue,     TextParagraphType::Action,
    TextParagraphType::InlineNote,   TextParagraphType::UnformattedText,
};

/**
 * @brief Тип блока по умолчанию
 */
static const TextParagraphType kDefaultBlockType = TextParagraphType::Action;

} // namespace


StageplayFountainImporter::StageplayFountainImporter()
    : AbstractStageplayImporter()
    , AbstractFountainImporter(kPossibleBlockTypes, kDefaultBlockType)
{
}

StageplayFountainImporter::~StageplayFountainImporter() = default;

AbstractStageplayImporter::Stageplay StageplayFountainImporter::importStageplay(
    const ImportOptions* _options) const
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
    auto stageplay = stageplayText(fountainFile.readAll());
    if (stageplay.name.isEmpty()) {
        stageplay.name = QFileInfo(_options->filePath).completeBaseName();
    }

    return stageplay;
}

AbstractStageplayImporter::Stageplay StageplayFountainImporter::stageplayText(
    const QString& _stageplayText) const
{
    Stageplay result;
    result.text = documentText(_stageplayText);
    return result;
}

QString StageplayFountainImporter::characterName(const QString& _text) const
{
    return StageplayCharacterParser::name(_text);
}

QString StageplayFountainImporter::locationName(const QString& _text) const
{
    Q_UNUSED(_text)

    return "";
}

bool StageplayFountainImporter::placeDialoguesInTable() const
{
    return TemplatesFacade::stageplayTemplate().placeDialoguesInTable();
}

} // namespace BusinessLayer
