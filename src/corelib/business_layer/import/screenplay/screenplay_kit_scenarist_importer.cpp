#include "screenplay_kit_scenarist_importer.h"

#include "screenplay_import_options.h"

#include <business_layer/import/text/simple_text_markdown_importer.h>
#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/model/text/text_model_xml.h>
#include <business_layer/templates/screenplay_template.h>
#include <domain/document_object.h>
#include <utils/helpers/text_helper.h>

#include <QApplication>
#include <QDomDocument>
#include <QFileInfo>
#include <QQueue>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTextDocument>
#include <QVariantMap>
#include <QXmlStreamWriter>


namespace BusinessLayer {

namespace {

const QString kSqlDriver = "QSQLITE";
const QString kConnectionName = "import_database";

/**
 * @brief Типы документов из КИТа
 */
enum Type {
    Script,
    TitlePage,
    Synopsis,
    ResearchRoot,
    Folder,
    Text,
    Url,
    ImagesGallery,
    Image,
    MindMap,
    Logline,
    Versions,
    CharactersRoot = 100,
    Character,
    LocationsRoot = 200,
    Location
};

/**
 * @brief Получить простой текст из Html
 */
QString htmlToPlain(const QString& _html)
{
    QTextDocument document;
    document.setHtml(_html);
    return document.toPlainText().replace("\n", "\n\n");
}

/**
 * @brief Считать данные о персонаже
 */
QString readCharacter(const QString& _characterName, const QString& _kitCharacterXml)
{
    QDomDocument kitDocument;
    kitDocument.setContent(_kitCharacterXml);
    const auto kitCharacter = kitDocument.firstChildElement();

    QString characterXml;
    QXmlStreamWriter writer(&characterXml);
    writer.writeStartDocument();
    writer.writeStartElement(xml::kDocumentTag);
    writer.writeAttribute(xml::kMimeTypeAttribute,
                          Domain::mimeTypeFor(Domain::DocumentObjectType::Character));
    writer.writeAttribute(xml::kVersionAttribute, "1.0");

    auto writeTag = [&writer](const QString& _tag, const QString& _content) {
        writer.writeStartElement(_tag);
        writer.writeCDATA(_content);
        writer.writeEndElement();
    };
    writeTag("name", _characterName);
    const auto realName = kitCharacter.firstChildElement("real_name").text();
    if (!realName.isEmpty()) {
        writeTag("real_name", realName);
    }
    const auto description = htmlToPlain(kitCharacter.firstChildElement("description").text());
    if (!description.isEmpty()) {
        writeTag("long_description", description);
    }

    writer.writeEndDocument();

    return characterXml;
}

/**
 * @brief Считать данные о локации
 */
QString readLocation(const QString& _locationName, const QString& _kitLocationXml)
{
    QString locationXml;
    QXmlStreamWriter writer(&locationXml);
    writer.writeStartDocument();
    writer.writeStartElement(xml::kDocumentTag);
    writer.writeAttribute(xml::kMimeTypeAttribute,
                          Domain::mimeTypeFor(Domain::DocumentObjectType::Location));
    writer.writeAttribute(xml::kVersionAttribute, "1.0");

    auto writeTag = [&writer](const QString& _tag, const QString& _content) {
        writer.writeStartElement(_tag);
        writer.writeCDATA(_content);
        writer.writeEndElement();
    };
    writeTag("name", _locationName);
    const auto description = htmlToPlain(_kitLocationXml);
    if (!description.isEmpty()) {
        writeTag("long_description", description);
    }

    writer.writeEndDocument();

    return locationXml;
}

/**
 * @brief Сформировать простой текстовый документ из текстового документа КИТа
 */
QString readPlainTextDocument(const QString& _sourceDocument)
{
    const auto sourceDocumentPlainText = htmlToPlain(_sourceDocument);
    const auto document = SimpleTextMarkdownImporter().importDocument(sourceDocumentPlainText);
    return document.text;
}

} // namespace


class ScreenplayKitScenaristImporter::Implementation
{
public:
    /**
     * @brief Сформировать документ сценария из xml сценария КИТа
     */
    AbstractScreenplayImporter::Screenplay readScreenplay(const QString& _kitScreenplayXml);

    /**
     * @brief Записать блок
     */
    void writeBlock(const QDomNode& _paragraph, TextParagraphType _blockType,
                    QXmlStreamWriter& _writer);

    /**
     * @brief Записать биты
     */
    void writeBeats(QXmlStreamWriter& _writer);


    /**
     * @brief Зашли ли мы уже в сцену
     */
    bool alreadyInScene = false;

    /**
     * @brief Имя закладки
     */
    QString bookmarkName;

    /**
     * @brief Цвет закладки
     */
    QString bookmarkColor;

    /**
     * @brief Буфер битов
     */
    QQueue<QDomNode> beatsBuffer;
};

AbstractScreenplayImporter::Screenplay ScreenplayKitScenaristImporter::Implementation::
    readScreenplay(const QString& _kitScreenplayXml)
{
    AbstractScreenplayImporter::Screenplay screenplay;

    //
    // Читаем XML
    //
    QDomDocument kitDocument;
    kitDocument.setContent(_kitScreenplayXml);
    //
    // ... и пишем в сценарий
    //
    QXmlStreamWriter writer(&screenplay.text);
    writer.writeStartDocument();
    writer.writeStartElement(xml::kDocumentTag);
    writer.writeAttribute(xml::kMimeTypeAttribute,
                          Domain::mimeTypeFor(Domain::DocumentObjectType::ScreenplayText));
    writer.writeAttribute(xml::kVersionAttribute, "1.0");

    QDomElement rootElement = kitDocument.documentElement();
    for (QDomNode paragraph = rootElement.firstChild(); !paragraph.isNull();
         paragraph = paragraph.nextSibling()) {
        //
        // Определим тип блока
        //
        const QString paragraphType = paragraph.nodeName();
        auto blockType = TextParagraphType::Action;
        if (paragraphType == "scene_heading") {
            blockType = TextParagraphType::SceneHeading;
        } else if (paragraphType == "scene_characters") {
            blockType = TextParagraphType::SceneCharacters;
        } else if (paragraphType == "action") {
            blockType = TextParagraphType::Action;
        } else if (paragraphType == "character") {
            blockType = TextParagraphType::Character;
        } else if (paragraphType == "parenthetical") {
            blockType = TextParagraphType::Parenthetical;
        } else if (paragraphType == "dialog") {
            blockType = TextParagraphType::Dialogue;
        } else if (paragraphType == "transition") {
            blockType = TextParagraphType::Transition;
        } else if (paragraphType == "note") {
            blockType = TextParagraphType::Shot;
        } else if (paragraphType == "noprintable_text") {
            blockType = TextParagraphType::InlineNote;
        } else if (paragraphType == "folder_header") {
            blockType = TextParagraphType::SequenceHeading;
        } else if (paragraphType == "folder_footer") {
            blockType = TextParagraphType::SequenceFooter;
        } else if (paragraphType == "scene_description") {
            blockType = TextParagraphType::BeatHeading;
        } else if (paragraphType == "lyrics") {
            blockType = TextParagraphType::Lyrics;
        }

        //
        // Биты пишем не сразу, а сохраним в буфер и запишем в конце сцены
        //
        if (blockType == TextParagraphType::BeatHeading) {
            beatsBuffer.enqueue(paragraph);
        } else {
            writeBlock(paragraph, blockType, writer);
        }
    }

    //
    // Запишем оставшиеся биты
    //
    writeBeats(writer);

    writer.writeEndDocument();

    return screenplay;
}

void ScreenplayKitScenaristImporter::Implementation::writeBlock(const QDomNode& _paragraph,
                                                                TextParagraphType _blockType,
                                                                QXmlStreamWriter& _writer)
{
    //
    // Получим текст блока
    //
    QString paragraphText;
    QVector<ScreenplayTextModelTextItem::ReviewMark> reviewMarks;
    QVector<ScreenplayTextModelTextItem::TextFormat> formats;
    {
        QDomElement textNode = _paragraph.firstChildElement("v");
        if (!textNode.isNull()) {
            //
            // ... читаем текст
            //
            paragraphText = textNode.text();
            //
            // ... читаем закладки
            //
            if (const auto paragraphNode = _paragraph.toElement();
                !paragraphNode.isNull() && paragraphNode.hasAttribute("bookmark")) {
                bookmarkName = paragraphNode.attribute("bookmark");
                bookmarkColor = paragraphNode.attribute("bookmark_color");
            } else {
                bookmarkName.clear();
                bookmarkColor.clear();
            }
            //
            // ... читаем редакторские заметки
            //
            auto reviewsNode = _paragraph.firstChildElement("reviews");
            if (!reviewsNode.isNull()) {
                auto reviewNode = reviewsNode.firstChildElement("review");
                while (!reviewNode.isNull()) {
                    ScreenplayTextModelTextItem::ReviewMark reviewMark;
                    reviewMark.from = reviewNode.attribute("from").toInt();
                    reviewMark.length = reviewNode.attribute("length").toInt();
                    if (reviewNode.attribute("color") != "#000000") {
                        reviewMark.textColor = reviewNode.attribute("color");
                    }
                    if (reviewNode.attribute("bgcolor") != "#000000") {
                        reviewMark.backgroundColor = reviewNode.attribute("bgcolor");
                    }
                    reviewMark.isDone = reviewNode.attribute("done") == "true";
                    auto reviewCommentNode = reviewNode.firstChildElement("review_comment");
                    while (!reviewCommentNode.isNull()) {
                        reviewMark.comments.append({ reviewCommentNode.attribute("author"),
                                                     {},
                                                     reviewCommentNode.attribute("date"),
                                                     TextHelper::fromHtmlEscaped(
                                                         reviewCommentNode.attribute("comment")) });

                        reviewCommentNode = reviewCommentNode.nextSiblingElement("review_comment");
                    }
                    if (reviewMark.comments.isEmpty()) {
                        reviewMark.comments.append(ScreenplayTextModelTextItem::ReviewComment());
                    }
                    reviewMarks.append(reviewMark);

                    reviewNode = reviewNode.nextSiblingElement("review");
                }
            }
            //
            // ... читаем форматирование
            //
            auto formatsNode = _paragraph.firstChildElement("formatting");
            if (!formatsNode.isNull()) {
                auto formatNode = formatsNode.firstChildElement("format");
                while (!formatNode.isNull()) {
                    ScreenplayTextModelTextItem::TextFormat format;
                    format.from = formatNode.attribute("from").toInt();
                    format.length = formatNode.attribute("length").toInt();
                    format.isBold = formatNode.attribute("bold") == "true";
                    format.isItalic = formatNode.attribute("italic") == "true";
                    format.isUnderline = formatNode.attribute("underline") == "true";
                    formats.append(format);

                    formatNode = formatNode.nextSiblingElement("format");
                }
            }
        }

        //
        // Корректируем при необходимости
        //
        if (_blockType == TextParagraphType::Parenthetical) {
            if (!paragraphText.isEmpty() && paragraphText.front() == '(') {
                paragraphText.remove(0, 1);
            }
            if (!paragraphText.isEmpty() && paragraphText.back() == ')') {
                paragraphText.chop(1);
            }
        }
    }

    //
    // Формируем блок сценария
    //
    switch (_blockType) {
    case TextParagraphType::SequenceHeading: {
        if (alreadyInScene) {
            //
            // В конце сцены пишем биты
            //
            writeBeats(_writer);
            _writer.writeEndElement(); // контент предыдущей сцены
            _writer.writeEndElement(); // предыдущая сцена
        }
        alreadyInScene = false; // вышли из сцены

        _writer.writeStartElement(toString(TextFolderType::Sequence));
        _writer.writeAttribute(xml::kUuidAttribute, QUuid::createUuid().toString());
        _writer.writeStartElement(xml::kContentTag);
        break;
    }

    case TextParagraphType::SequenceFooter: {
        if (alreadyInScene) {
            //
            // В конце сцены пишем биты
            //
            writeBeats(_writer);
            _writer.writeEndElement(); // контент предыдущей сцены
            _writer.writeEndElement(); // предыдущая сцена
        }
        alreadyInScene = false; // вышли из сцены

        break;
    }

    case TextParagraphType::SceneHeading: {
        if (alreadyInScene) {
            //
            // В конце сцены пишем биты
            //
            writeBeats(_writer);
            _writer.writeEndElement(); // контент предыдущей сцены
            _writer.writeEndElement(); // предыдущая сцена
        }
        alreadyInScene = true; // вошли в новую сцену

        _writer.writeStartElement(toString(TextGroupType::Scene));
        _writer.writeAttribute(xml::kUuidAttribute, QUuid::createUuid().toString());
        if (_paragraph.attributes().contains("color")) {
            _writer.writeStartElement(xml::kColorAttribute);
            _writer.writeCDATA(_paragraph.attributes().namedItem("color").nodeValue());
            _writer.writeEndElement();
        }
        if (_paragraph.attributes().contains("title")) {
            _writer.writeStartElement(xml::kTitleTag);
            _writer.writeCDATA(_paragraph.attributes().namedItem("title").nodeValue());
            _writer.writeEndElement();
        }
        if (_paragraph.attributes().contains("stamp")) {
            _writer.writeStartElement(xml::kStampTag);
            _writer.writeCDATA(_paragraph.attributes().namedItem("stamp").nodeValue());
            _writer.writeEndElement();
        }
        _writer.writeStartElement(xml::kContentTag);
        break;
    }

    case TextParagraphType::BeatHeading: {
        _writer.writeStartElement(toString(TextGroupType::Beat));
        _writer.writeAttribute(xml::kUuidAttribute, QUuid::createUuid().toString());
        _writer.writeStartElement(xml::kContentTag);
        break;
    }

    default: {
        break;
    }
    }

    //
    // Начинаем писать блок
    //
    _writer.writeStartElement(toString(_blockType));
    //
    // Пишем закладку
    //
    if (!bookmarkColor.isEmpty() || !bookmarkName.isEmpty()) {
        _writer.writeStartElement(xml::kBookmarkTag);
        _writer.writeAttribute("color", bookmarkColor);
        _writer.writeCDATA(bookmarkName);
        _writer.writeEndElement(); // bookmark
    }
    //
    // Пишем текст блока
    //
    _writer.writeStartElement(xml::kValueTag);
    _writer.writeCDATA(TextHelper::toHtmlEscaped(paragraphText));
    _writer.writeEndElement(); // value
    //
    // Пишем редакторские заметки
    //
    if (!reviewMarks.isEmpty()) {
        _writer.writeStartElement(xml::kReviewMarksTag);
        for (const auto& reviewMark : std::as_const(reviewMarks)) {
            _writer.writeStartElement(xml::kReviewMarkTag);
            _writer.writeAttribute(xml::kFromAttribute, QString::number(reviewMark.from));
            _writer.writeAttribute(xml::kLengthAttribute, QString::number(reviewMark.length));
            _writer.writeAttribute(xml::kBackgroundColorAttribute,
                                   reviewMark.backgroundColor.name());
            for (const auto& comment : std::as_const(reviewMark.comments)) {
                _writer.writeStartElement(xml::kCommentTag);
                _writer.writeAttribute(xml::kAuthorAttribute, comment.author);
                _writer.writeAttribute(xml::kDateAttribute, comment.date);
                _writer.writeCDATA(TextHelper::toHtmlEscaped(comment.text));
                _writer.writeEndElement(); // comment
            }
            _writer.writeEndElement();
        }
        _writer.writeEndElement(); // review marks
    }
    //
    // Пишем форматирование
    //
    if (!formats.isEmpty()) {
        _writer.writeStartElement(xml::kFormatsTag);
        for (const auto& format : std::as_const(formats)) {
            _writer.writeStartElement(xml::kFormatTag);
            //
            // Данные пользовательского форматирования
            //
            _writer.writeAttribute(xml::kFromAttribute, QString::number(format.from));
            _writer.writeAttribute(xml::kLengthAttribute, QString::number(format.length));
            if (format.isBold) {
                _writer.writeAttribute(xml::kBoldAttribute, "true");
            }
            if (format.isItalic) {
                _writer.writeAttribute(xml::kItalicAttribute, "true");
            }
            if (format.isUnderline) {
                _writer.writeAttribute(xml::kUnderlineAttribute, "true");
            }
            //
            _writer.writeEndElement(); // format
        }
        _writer.writeEndElement(); // formats
    }
    _writer.writeEndElement(); // block type

    //
    // Если пишем футер папки, то нужно закрыть и саму папку
    //
    if (_blockType == TextParagraphType::SequenceFooter) {
        _writer.writeEndElement(); // контент текущей папки
        _writer.writeEndElement(); // текущая папка
    }
}

void ScreenplayKitScenaristImporter::Implementation::writeBeats(QXmlStreamWriter& _writer)
{
    while (!beatsBuffer.isEmpty()) {
        const auto beatParagraph = beatsBuffer.dequeue();
        writeBlock(beatParagraph, TextParagraphType::BeatHeading, _writer);
        _writer.writeEndElement(); // контент предыдущего бита
        _writer.writeEndElement(); // предыдущий бит
    }
}


// ****


ScreenplayKitScenaristImporter::ScreenplayKitScenaristImporter()
    : AbstractScreenplayImporter()
    , d(new Implementation)
{
}

ScreenplayKitScenaristImporter::~ScreenplayKitScenaristImporter() = default;

AbstractScreenplayImporter::Documents ScreenplayKitScenaristImporter::importDocuments(
    const ImportOptions* _options) const
{
    const auto& options = static_cast<const ScreenplayImportOptions*>(_options);
    Documents result;

    {
        QSqlDatabase database = QSqlDatabase::addDatabase(kSqlDriver, kConnectionName);
        database.setDatabaseName(options->filePath);
        if (database.open()) {
            auto typeFor = [](const QSqlQuery& _record) {
                switch (_record.value("type").toUInt()) {
                case Character: {
                    return Domain::DocumentObjectType::Character;
                }
                case Location: {
                    return Domain::DocumentObjectType::Location;
                }
                case Text: {
                    return Domain::DocumentObjectType::SimpleText;
                }
                case Folder: {
                    return Domain::DocumentObjectType::Folder;
                }
                case MindMap: {
                    return Domain::DocumentObjectType::MindMap;
                }
                case ImagesGallery: {
                    return Domain::DocumentObjectType::ImagesGallery;
                }
                case Image: {
                    return Domain::DocumentObjectType::Image;
                }
                default: {
                    return Domain::DocumentObjectType::Undefined;
                }
                }
            };

            //
            // Загрузим данные о персонажах
            //
            if (options->importCharacters) {
                QSqlQuery charactersQuery(database);
                charactersQuery.prepare(
                    "SELECT name, description FROM research WHERE type = ? ORDER by sort_order");
                charactersQuery.addBindValue(Character);
                charactersQuery.exec();
                while (charactersQuery.next()) {
                    const auto name = charactersQuery.value("name").toString();
                    const auto content
                        = readCharacter(name, charactersQuery.value("description").toString());
                    result.characters.append({ typeFor(charactersQuery), name, content, {} });
                }
            }

            //
            // Загрузим данные о локациях
            //
            if (options->importLocations) {
                QSqlQuery locationsQuery(database);
                locationsQuery.prepare("SELECT * FROM research WHERE type = ? ORDER by sort_order");
                locationsQuery.addBindValue(Location);
                locationsQuery.exec();
                while (locationsQuery.next()) {
                    const auto name = locationsQuery.value("name").toString();
                    const auto content
                        = readLocation(name, locationsQuery.value("description").toString());
                    result.locations.append({ typeFor(locationsQuery), name, content, {} });
                }
            }

            //
            // Загрузим данные разработки
            //
            if (options->importResearch) {
                QSqlQuery documentsQuery(database);
                documentsQuery.prepare("SELECT * FROM research WHERE type IN (?, ?) ORDER BY "
                                       "parent_id, sort_order");
                documentsQuery.addBindValue(Folder);
                documentsQuery.addBindValue(Text);
                documentsQuery.exec();
                while (documentsQuery.next()) {
                    const auto id = documentsQuery.value("id").toInt();
                    const auto parentId = documentsQuery.value("parent_id").toInt();
                    const auto name = documentsQuery.value("name").toString();
                    const auto content
                        = readPlainTextDocument(documentsQuery.value("description").toString());
                    const Document newDocument = { typeFor(documentsQuery), name, content, {}, id };
                    //
                    // Если нет родителя, то добавляем в корень
                    //
                    if (parentId == 0) {
                        result.research.append(newDocument);
                    }
                    //
                    // а если родитель есть, то рекурсивно ищем его внутри
                    //
                    else {
                        std::function<bool(Document&)> placeDocument;
                        placeDocument
                            = [&placeDocument, newDocument, parentId](Document& _document) {
                                  if (_document.id == parentId) {
                                      _document.children.append(newDocument);
                                      return true;
                                  } else {
                                      for (auto& child : _document.children) {
                                          const auto isFound = placeDocument(child);
                                          if (isFound) {
                                              return true;
                                          }
                                      }
                                  }
                                  return false;
                              };

                        for (auto& document : result.research) {
                            placeDocument(document);
                        }
                    }
                }
            }
        }
    }
    QSqlDatabase::removeDatabase(kConnectionName);

    return result;
}

QVector<AbstractScreenplayImporter::Screenplay> ScreenplayKitScenaristImporter::importScreenplays(
    const ScreenplayImportOptions* _options) const
{
    if (_options->importText == false) {
        return {};
    }

    QVector<Screenplay> result;

    {
        QSqlDatabase database = QSqlDatabase::addDatabase(kSqlDriver, kConnectionName);
        database.setDatabaseName(_options->filePath);
        if (database.open()) {
            QSqlQuery query(database);

            //
            // Читаем сценарий (scenario - текст сценария)
            //
            QString screenplayName = QFileInfo(_options->filePath).completeBaseName();
            {
                query.exec("SELECT text FROM scenario WHERE is_draft = 0");
                query.next();
                const auto kitScreenplayXml = query.record().value("text").toString();
                auto screenplay = d->readScreenplay(kitScreenplayXml);

                //
                // Читаем данные
                //
                query.exec("SELECT data_name, data_value FROM scenario_data");
                while (query.next()) {
                    const auto dataName = query.record().value("data_name").toString();
                    const auto dataValue = query.record().value("data_value").toString();
                    if (dataValue.isNull() || dataValue.isEmpty()) {
                        continue;
                    }

                    if (dataName == "name") {
                        screenplayName = dataValue;
                        screenplay.name = dataValue;
                    } else if (dataName == "header") {
                        screenplay.header = dataValue;
                    } else if (dataName == "footer") {
                        screenplay.footer = dataValue;
                    } else if (dataName == "logline") {
                        screenplay.logline = htmlToPlain(dataValue);
                    } else if (dataName == "synopsis") {
                        screenplay.synopsis = readPlainTextDocument(dataValue);
                    }
                }

                result.append(screenplay);
            }

            //
            // Читаем черновик
            //
            {
                query.exec("SELECT text FROM scenario WHERE is_draft = 1");
                query.next();
                const auto kitScreenplayXml = query.record().value("text").toString();
                const auto defaultKitScreenplay
                    = "<?xml version=\"1.0\"?>\n"
                      "<scenario version=\"1.0\">\n"
                      "<scene_heading uuid=\"{000000-0000000-000000}\">\n"
                      "<v><![CDATA[]]></v>\n"
                      "</scene_heading>\n"
                      "</scenario>\n";
                if (kitScreenplayXml != defaultKitScreenplay) {
                    auto screenplay = d->readScreenplay(kitScreenplayXml);
                    screenplay.name = QString("%1 (%2)").arg(
                        screenplayName,
                        //: Draft screenplay imported from KIT Scenarist file
                        QApplication::translate("BusinessLayer::KitScenaristImporter", "draft"));
                    result.append(screenplay);
                }
            }
        }
    }
    QSqlDatabase::removeDatabase(kConnectionName);

    return result;
}

} // namespace BusinessLayer
