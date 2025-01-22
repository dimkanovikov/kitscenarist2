#include "screenplay_celtx_importer.h"

#include "screenplay_import_options.h"

#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/model/text/text_model_xml.h>
#include <business_layer/templates/screenplay_template.h>
#include <domain/document_object.h>
#include <utils/helpers/text_helper.h>

#include <QFileInfo>
#include <QXmlStreamWriter>
#include <qgumbodocument.h>
#include <qgumbonode.h>
#include <qtzip/QtZipReader>

#include <set>


namespace BusinessLayer {

namespace {

/**
 * @brief Считать текст файла-сценария из архива celtx-файла
 */
QString readScript(const QString& _filePath)
{
    //
    // Открыть архив и извлечь содержимое файла, название которого начинается со "script"
    //

    QFile celtxFile(_filePath);
    if (!celtxFile.open(QIODevice::ReadOnly)) {
        return {};
    }

    QtZipReader zip(&celtxFile);
    if (!zip.isReadable()) {
        return {};
    }

    QString scriptHtml;
    for (const QString& fileName : zip.fileList()) {
        if (fileName.startsWith("script")) {
            scriptHtml = zip.fileData(fileName);
            break;
        }
    }

    return scriptHtml;
}

} // namespace

AbstractScreenplayImporter::Documents ScreenplayCeltxImporter::importDocuments(
    const ImportOptions* _options) const
{
    //
    // Открываем файл
    //
    const auto scriptHtml = readScript(_options->filePath);
    if (scriptHtml.isEmpty()) {
        return {};
    }

    //
    // Читаем html text
    //
    std::set<QString> characterNames;
    std::set<QString> locationNames;
    const auto scriptDocument = QGumboDocument::parse(scriptHtml.toUtf8());
    const auto rootNode = scriptDocument.rootNode();
    const auto pNodes = rootNode.getElementsByTagName(HtmlTag::P);
    for (const auto& pNode : pNodes) {
        const QString pNodeClass = pNode.getAttribute("class");
        auto blockType = TextParagraphType::Undefined;
        if (pNodeClass == "sceneheading") {
            blockType = TextParagraphType::SceneHeading;
        } else if (pNodeClass == "character") {
            blockType = TextParagraphType::Character;
        }

        //
        // Сформируем текст
        //
        QString paragraphText = pNode.innerText().simplified();

        switch (blockType) {
        case TextParagraphType::SceneHeading: {
            if (!_options->importLocations) {
                break;
            }

            const auto locationName = ScreenplaySceneHeadingParser::location(paragraphText);
            if (locationName.isEmpty()) {
                break;
            }

            locationNames.emplace(locationName);
            break;
        }

        case TextParagraphType::Character: {
            if (!_options->importCharacters) {
                break;
            }

            const auto characterName = ScreenplayCharacterParser::name(paragraphText);
            if (characterName.isEmpty()) {
                break;
            }

            characterNames.emplace(characterName);
            break;
        }

        default:
            break;
        }
    }

    Documents documents;
    for (const auto& characterName : characterNames) {
        documents.characters.append(
            { Domain::DocumentObjectType::Character, characterName, {}, {} });
    }
    for (const auto& locationName : locationNames) {
        documents.locations.append({ Domain::DocumentObjectType::Location, locationName, {}, {} });
    }
    return documents;
}

QVector<AbstractScreenplayImporter::Screenplay> ScreenplayCeltxImporter::importScreenplays(
    const ScreenplayImportOptions* _options) const
{
    if (_options->importText == false) {
        return {};
    }

    Screenplay result;
    result.name = QFileInfo(_options->filePath).completeBaseName();

    //
    // Открываем файл
    //
    const auto scriptHtml = readScript(_options->filePath);
    if (scriptHtml.isEmpty()) {
        return { result };
    }

    //
    // Читаем html text
    //
    // ... и пишем в сценарий
    //
    QXmlStreamWriter writer(&result.text);
    writer.writeStartDocument();
    writer.writeStartElement(xml::kDocumentTag);
    writer.writeAttribute(xml::kMimeTypeAttribute,
                          Domain::mimeTypeFor(Domain::DocumentObjectType::ScreenplayText));
    writer.writeAttribute(xml::kVersionAttribute, "1.0");

    //
    // Получим список строк текста
    //
    const auto scriptDocument = QGumboDocument::parse(scriptHtml.toUtf8());
    const auto rootNode = scriptDocument.rootNode();
    const auto pNodes = rootNode.getElementsByTagName(HtmlTag::P);
    bool alreadyInScene = false;
    for (const auto& pNode : pNodes) {
        const QString pNodeClass = pNode.getAttribute("class");
        auto blockType = TextParagraphType::UnformattedText;
        if (pNodeClass == "sceneheading") {
            blockType = TextParagraphType::SceneHeading;
        } else if (pNodeClass == "action") {
            blockType = TextParagraphType::Action;
        } else if (pNodeClass == "character") {
            blockType = TextParagraphType::Character;
        } else if (pNodeClass == "parenthetical") {
            blockType = TextParagraphType::Parenthetical;
        } else if (pNodeClass == "dialog") {
            blockType = TextParagraphType::Dialogue;
        } else if (pNodeClass == "transition") {
            blockType = TextParagraphType::Transition;
        } else if (pNodeClass == "shot") {
            blockType = TextParagraphType::Shot;
        }

        //
        // Сформируем текст
        //
        QString paragraphText = pNode.innerText().simplified();

        //
        // Корректируем при необходимости
        //
        if (blockType == TextParagraphType::Parenthetical) {
            if (!paragraphText.isEmpty() && paragraphText.front() == '(') {
                paragraphText.remove(0, 1);
            }
            if (!paragraphText.isEmpty() && paragraphText.back() == ')') {
                paragraphText.chop(1);
            }
        }

        QVector<ScreenplayTextModelTextItem::ReviewMark> reviewMarks;
        QVector<ScreenplayTextModelTextItem::TextFormat> formats;
        for (const auto& child : pNode.children()) {
            if (child.tag() != HtmlTag::SPAN) {
                continue;
            }

            //
            // Обработать заметку
            //
            if (child.hasAttribute("text")) {
                QString colorText = child.getAttribute("style");
                colorText = colorText.remove(0, colorText.indexOf("(") + 1);
                colorText = colorText.left(colorText.indexOf(")"));
                colorText = colorText.remove(" ");
                const QStringList colorComponents = colorText.split(",");
                if (colorComponents.size() == 3) {
                    ScreenplayTextModelTextItem::ReviewMark note;
                    note.from = 0;
                    note.length = paragraphText.length();
                    note.backgroundColor
                        = QColor(colorComponents[0].toInt(), colorComponents[1].toInt(),
                                 colorComponents[2].toInt());
                    note.comments.append({ "celtx user",
                                           {},
                                           QDateTime::currentDateTime().toString(Qt::ISODate),
                                           child.getAttribute("text") });
                    reviewMarks.append(note);
                }
            }
            //
            // Обработать форматированный текст
            //
            else {
                const QString childText = child.innerText();
                const int childStartPosition = pNode.childStartPosition(child);
                paragraphText.insert(childStartPosition, childText);

                const QString style = child.getAttribute("style");
                ScreenplayTextModelTextItem::TextFormat format;
                format.from = childStartPosition;
                format.length = childText.length();
                format.isBold = style.contains("font-weight: bold;");
                format.isItalic = style.contains("font-style: italic;");
                format.isUnderline = style.contains("text-decoration: underline;");
                if (format.isValid()) {
                    formats.append(format);
                }
            }
        }

        //
        // Формируем блок сценария
        //
        if (blockType == TextParagraphType::SceneHeading) {
            if (alreadyInScene) {
                writer.writeEndElement(); // контент предыдущей сцены
                writer.writeEndElement(); // предыдущая сцена
            }
            alreadyInScene = true;

            writer.writeStartElement(toString(TextGroupType::Scene));
            writer.writeAttribute(xml::kUuidAttribute, QUuid::createUuid().toString());
            writer.writeStartElement(xml::kContentTag);
        }
        writer.writeStartElement(toString(blockType));
        writer.writeStartElement(xml::kValueTag);
        writer.writeCDATA(TextHelper::toHtmlEscaped(paragraphText));
        writer.writeEndElement(); // value
        //
        // Пишем редакторские заметки
        //
        if (!reviewMarks.isEmpty()) {
            writer.writeStartElement(xml::kReviewMarksTag);
            for (const auto& reviewMark : std::as_const(reviewMarks)) {
                writer.writeStartElement(xml::kReviewMarkTag);
                writer.writeAttribute(xml::kFromAttribute, QString::number(reviewMark.from));
                writer.writeAttribute(xml::kLengthAttribute, QString::number(reviewMark.length));
                writer.writeAttribute(xml::kBackgroundColorAttribute,
                                      reviewMark.backgroundColor.name());
                for (const auto& comment : std::as_const(reviewMark.comments)) {
                    writer.writeStartElement(xml::kCommentTag);
                    writer.writeAttribute(xml::kAuthorAttribute, comment.author);
                    writer.writeAttribute(xml::kDateAttribute, comment.date);
                    writer.writeCDATA(TextHelper::toHtmlEscaped(comment.text));
                    writer.writeEndElement(); // comment
                }
                writer.writeEndElement();
            }
            writer.writeEndElement(); // review marks
        }
        //
        // Пишем форматирование
        //
        if (!formats.isEmpty()) {
            writer.writeStartElement(xml::kFormatsTag);
            for (const auto& format : std::as_const(formats)) {
                writer.writeEmptyElement(xml::kFormatTag);
                writer.writeAttribute(xml::kFromAttribute, QString::number(format.from));
                writer.writeAttribute(xml::kLengthAttribute, QString::number(format.length));
                if (format.isBold) {
                    writer.writeAttribute(xml::kBoldAttribute, "true");
                }
                if (format.isItalic) {
                    writer.writeAttribute(xml::kItalicAttribute, "true");
                }
                if (format.isUnderline) {
                    writer.writeAttribute(xml::kUnderlineAttribute, "true");
                }
            }
            writer.writeEndElement(); // formats
        }
        //
        writer.writeEndElement(); // block type
    }
    writer.writeEndDocument();

    return { result };
}

} // namespace BusinessLayer
