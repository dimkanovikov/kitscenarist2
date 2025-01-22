#include "comic_book_fountain_importer.h"

#include <business_layer/import/import_options.h>
#include <business_layer/model/comic_book/text/comic_book_text_block_parser.h>
#include <business_layer/model/text/text_model_xml.h>
#include <business_layer/templates/comic_book_template.h>
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
    TextParagraphType::PageHeading,     TextParagraphType::PanelHeading,
    TextParagraphType::Description,     TextParagraphType::Character,
    TextParagraphType::Dialogue,        TextParagraphType::InlineNote,
    TextParagraphType::UnformattedText,
};

/**
 * @brief Тип блока по умолчанию
 */
static const TextParagraphType kDefaultBlockType = TextParagraphType::Description;

} // namespace

class ComicBookFountainImporter::Implementation
{
public:
    /**
     * @brief Зашли ли мы уже в панель
     */
    bool alreadyInPanel = false;

    /**
     * @brief Зашли ли мы уже на страницу
     */
    bool alreadyInPage = false;
};


// ****


ComicBookFountainImporter::ComicBookFountainImporter()
    : AbstractComicBookImporter()
    , AbstractFountainImporter(kPossibleBlockTypes, kDefaultBlockType)
    , d(new Implementation)
{
}

ComicBookFountainImporter::~ComicBookFountainImporter() = default;

AbstractComicBookImporter::ComicBook ComicBookFountainImporter::importComicBook(
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
    auto comicbook = comicbookText(fountainFile.readAll());
    if (comicbook.name.isEmpty()) {
        comicbook.name = QFileInfo(_options->filePath).completeBaseName();
    }

    return comicbook;
}

AbstractComicBookImporter::ComicBook ComicBookFountainImporter::comicbookText(
    const QString& _comicbookText) const
{
    ComicBook result;
    result.text = documentText(_comicbookText);
    return result;
}

QString ComicBookFountainImporter::characterName(const QString& _text) const
{
    return ComicBookCharacterParser::name(_text);
}

QString ComicBookFountainImporter::locationName(const QString& _text) const
{
    Q_UNUSED(_text)

    return "";
}

TextParagraphType ComicBookFountainImporter::blockType(QString& _paragraphText) const
{
    TextParagraphType blockType = TextParagraphType::Undefined;
    if (_paragraphText.startsWith("# ")) {
        _paragraphText.remove(0, 2);
        movePreviousTypes(0, 2);
        blockType = TextParagraphType::PageHeading;
    } else if (_paragraphText.startsWith(".")) {
        _paragraphText.remove(0, 1);
        movePreviousTypes(0, 1);
        blockType = TextParagraphType::PanelHeading;
    } else if (_paragraphText.startsWith("!")) {
        _paragraphText.remove(0, 1);
        movePreviousTypes(0, 1);
        blockType = TextParagraphType::Description;
    } else {
        blockType = AbstractFountainImporter::blockType(_paragraphText);
    }
    return blockType;
}

void ComicBookFountainImporter::writeBlock(const QString& _paragraphText, TextParagraphType _type,
                                           QXmlStreamWriter& _writer) const
{

    auto writeValue = [&_writer](const QString& _text) {
        _writer.writeStartElement(xml::kValueTag);
        _writer.writeCDATA(TextHelper::toHtmlEscaped(_text));
        _writer.writeEndElement(); // value
    };

    switch (_type) {
    case TextParagraphType::PageHeading: {

        if (d->alreadyInPanel) {
            _writer.writeEndElement(); // контент предыдущей панели
            _writer.writeEndElement(); // предыдущая панель
            d->alreadyInPanel = false; // вышли из панели
        }

        if (d->alreadyInPage) {
            _writer.writeEndElement(); // контент предыдущей страницы
            _writer.writeEndElement(); // предыдущая страница
        }

        d->alreadyInPage = true; // вошли на новую страницу

        _writer.writeStartElement(toString(TextGroupType::Page));
        _writer.writeAttribute(xml::kUuidAttribute, QUuid::createUuid().toString());
        _writer.writeStartElement(xml::kContentTag);
        _writer.writeStartElement(toString(_type));
        writeValue(_paragraphText);
        break;
    }

    case TextParagraphType::PanelHeading: {

        if (d->alreadyInPanel) {
            _writer.writeEndElement(); // контент предыдущей панели
            _writer.writeEndElement(); // предыдущая панель
        }

        d->alreadyInPanel = true; // вошли в новую панель

        _writer.writeStartElement(toString(TextGroupType::Panel));
        _writer.writeAttribute(xml::kUuidAttribute, QUuid::createUuid().toString());
        _writer.writeStartElement(xml::kContentTag);
        _writer.writeStartElement(toString(_type));
        writeValue(_paragraphText);
        break;
    }

    default: {
        AbstractFountainImporter::writeBlock(_paragraphText, _type, _writer);
        break;
    }
    }
}

bool ComicBookFountainImporter::placeDialoguesInTable() const
{
    return TemplatesFacade::comicBookTemplate().placeDialoguesInTable();
}

} // namespace BusinessLayer
