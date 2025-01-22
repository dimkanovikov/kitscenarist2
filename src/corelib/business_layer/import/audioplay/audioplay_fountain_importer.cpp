#include "audioplay_fountain_importer.h"

#include <business_layer/import/import_options.h>
#include <business_layer/model/audioplay/text/audioplay_text_block_parser.h>
#include <business_layer/model/text/text_model_xml.h>
#include <business_layer/templates/audioplay_template.h>
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
    TextParagraphType::Dialogue,     TextParagraphType::Sound,
    TextParagraphType::Music,        TextParagraphType::Cue,
    TextParagraphType::InlineNote,   TextParagraphType::UnformattedText,
};

/**
 * @brief Тип блока по умолчанию
 */
static const TextParagraphType kDefaultBlockType = TextParagraphType::Cue;

} // namespace


AudioplayFountainImporter::AudioplayFountainImporter()
    : AbstractAudioplayImporter()
    , AbstractFountainImporter(kPossibleBlockTypes, kDefaultBlockType)
{
}

AudioplayFountainImporter::~AudioplayFountainImporter() = default;

AbstractAudioplayImporter::Audioplay AudioplayFountainImporter::importAudioplay(
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
    auto audioplay = audioplayText(fountainFile.readAll());
    if (audioplay.name.isEmpty()) {
        audioplay.name = QFileInfo(_options->filePath).completeBaseName();
    }

    return audioplay;
}

AbstractAudioplayImporter::Audioplay AudioplayFountainImporter::audioplayText(
    const QString& _audioplayText) const
{
    Audioplay result;
    result.text = documentText(_audioplayText);
    return result;
}

QString AudioplayFountainImporter::characterName(const QString& _text) const
{
    return AudioplayCharacterParser::name(_text);
}

QString AudioplayFountainImporter::locationName(const QString& _text) const
{
    Q_UNUSED(_text)

    return "";
}

TextParagraphType AudioplayFountainImporter::blockType(QString& _paragraphText) const
{
    TextParagraphType blockType = TextParagraphType::Undefined;
    if (_paragraphText.startsWith("Music: ")) {
        _paragraphText.remove("Music: ");
        movePreviousTypes(0, QString("Music: ").size());
        blockType = TextParagraphType::Music;
    } else if (_paragraphText.startsWith("Sound: ")) {
        _paragraphText.remove("Sound: ");
        movePreviousTypes(0, QString("Sound: ").size());
        blockType = TextParagraphType::Sound;
    } else {
        blockType = AbstractFountainImporter::blockType(_paragraphText);
    }
    return blockType;
}

bool AudioplayFountainImporter::placeDialoguesInTable() const
{
    return TemplatesFacade::audioplayTemplate().placeDialoguesInTable();
}

} // namespace BusinessLayer
