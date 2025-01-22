#include "abstract_document_importer.h"

#include "import_options.h"

#include <business_layer/model/text/text_model_xml.h>
#include <business_layer/templates/text_template.h>
#include <utils/helpers/text_helper.h>

#include <QRegularExpression>
#include <QTextBlock>
#include <QTextDocument>
#include <QXmlStreamWriter>

#include <set>


namespace BusinessLayer {

namespace {

/**
 * @brief Регулярное выражение для определения блока "Время и место" по наличию слов места
 */
const QRegularExpression kPlaceContainsChecker(
    "^(INT|EXT|INT/EXT|ИНТ|НАТ|ИНТ/НАТ|ПАВ|ЭКСТ|ИНТ/ЭКСТ)([.]|[ - ])");

/**
 * @brief Регулярное выражение для определения строки, начинающейся с номера
 */
const QRegularExpression kStartFromNumberChecker(
    "^([\\d]{1,}[\\d\\S]{0,})([.]|[-])(([\\d\\S]{1,})([.]|)|) ");

/**
 * @brief Регулярное выражение для определения блока "Титр" по наличию ключевых слов
 */
const QRegularExpression kTitleChecker("(^|[^\\S])(TITLE|ТИТР)([:] )");

/**
 * @brief Регулярное выражение для определения текста в скобках
 */
const QRegularExpression kTextInParenthesisChecker(".{1,}\\([^\\)]{1,}\\)");

/**
 * @brief Допущение для блоков, которые по идее вообще не должны иметь отступа в пикселях (16 мм)
 */
const int kLeftMarginDelta = 60;

/**
 * @brief Некоторые программы выравнивают текст при помощи пробелов
 */
const QString kOldSchoolCenteringPrefix = "                    ";

/**
 * @brief Шум, который может встречаться в тексте
 */
const QString NOISE("([.]|[,]|[:]|[ ]|[-]){1,}");

/**
 * @brief Регулярное выражение для удаления мусора в начале текста
 */
const QRegularExpression NOISE_AT_START("^" + NOISE);

/**
 * @brief Регулярное выражение для удаления мусора в конце текста
 */
const QRegularExpression NOISE_AT_END(NOISE + "$");

/**
 * @brief Очистить блок от лишних пробельных символов
 * @note Аналог QString::simplified(), только при этом форматы символов остаются на своих местах
 */
void simplifyTextBlock(QTextCursor& _cursor)
{
    auto findEndPosition = [&_cursor]() {
        const int currentPosition = _cursor.position();
        _cursor.movePosition(QTextCursor::EndOfBlock);
        const int end = _cursor.position();
        _cursor.setPosition(currentPosition);
        return end;
    };

    int endPosition = findEndPosition();
    _cursor.movePosition(QTextCursor::StartOfBlock);
    while (true) {
        Q_ASSERT(_cursor.position() < endPosition);
        _cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);

        //
        // Удаляем пробельные символы
        //
        while (_cursor.selectedText().simplified().isEmpty()) {
            _cursor.removeSelectedText();
            endPosition = findEndPosition();
            if (_cursor.position() == endPosition) {
                break;
            }
            _cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        }

        //
        // Если конец, выходим из цикла
        //
        if (_cursor.position() == endPosition) {
            break;
        }

        //
        // Перемещаем anchor на позицию курсора
        //
        _cursor.movePosition(QTextCursor::NextCharacter);

        //
        // Пропускаем непробельные символы
        //
        _cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        while (_cursor.position() != endPosition
               && !_cursor.selectedText().simplified().isEmpty()) {
            //
            // Перемещаем anchor на позицию курсора
            //
            _cursor.movePosition(QTextCursor::NextCharacter);

            _cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        }

        //
        // Если конец, выходим из цикла
        //
        if (_cursor.position() == endPosition) {
            break;
        } else {
            //
            // ... если не конец, вставляем пробел после слова
            //
            const auto format = _cursor.charFormat();
            _cursor.removeSelectedText();
            _cursor.insertText(QString(QChar::Space), format);
        }
    }

    //
    // В конце мог остаться пробельный символ
    //
    _cursor.movePosition(QTextCursor::EndOfBlock);
    _cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
    if (_cursor.selectedText().simplified().isEmpty()) {
        _cursor.removeSelectedText();
    }

    _cursor.movePosition(QTextCursor::EndOfBlock);
}

} // namespace


AbstractDocumentImporter::AbstractDocumentImporter()
{
}

AbstractDocumentImporter::~AbstractDocumentImporter() = default;


AbstractImporter::Documents AbstractDocumentImporter::importDocuments(
    const ImportOptions* _options) const
{
    //
    // Преобразовать заданный документ в QTextDocument
    //
    QTextDocument document;
    const bool documentDone = documentForImport(_options->filePath, document);
    if (!documentDone) {
        return {};
    }

    //
    // Найти минимальный отступ слева для всех блоков
    // ЗАЧЕМ: во многих программах (Final Draft, Screeviner) сделано так, что поля
    //		  задаются за счёт оступов. Получается что и заглавие сцены и описание действия
    //		  имеют отступы. Так вот это и будет минимальным отступом, который не будем считать
    //
    int minLeftMargin = 1000;
    {
        QTextCursor cursor(&document);
        while (!cursor.atEnd()) {
            if (minLeftMargin > cursor.blockFormat().leftMargin()) {
                minLeftMargin = std::max(0.0, cursor.blockFormat().leftMargin());
            }

            cursor.movePosition(QTextCursor::NextBlock);
            cursor.movePosition(QTextCursor::EndOfBlock);
        }
    }

    QTextCursor cursor(&document);

    //
    // Для каждого блока текста определяем тип
    //
    // ... последний стиль блока
    auto lastBlockType = TextParagraphType::Undefined;
    // ... количество пустых строк
    int emptyLines = 0;
    std::set<QString> characterNames;
    std::set<QString> locationNames;
    do {
        cursor.movePosition(QTextCursor::EndOfBlock);

        //
        // Если в блоке есть текст
        //
        if (!cursor.block().text().simplified().isEmpty()) {
            //
            // ... определяем тип
            //
            const auto blockType
                = typeForTextCursor(cursor, lastBlockType, emptyLines, minLeftMargin);

            //
            // ... удаляем лишние пробельные символы
            //
            simplifyTextBlock(cursor);

            QString paragraphText = cursor.block().text();

            //
            // Если текущий тип "Время и место", то удалим номер сцены
            //
            if (blockType == TextParagraphType::SceneHeading) {
                paragraphText = TextHelper::smartToUpper(paragraphText);
                const auto match = kStartFromNumberChecker.match(paragraphText);
                if (match.hasMatch()) {
                    paragraphText = paragraphText.mid(match.capturedEnd());
                }
            }

            //
            // Выполняем корректировки
            //
            paragraphText = clearBlockText(blockType, paragraphText);

            switch (blockType) {
            case TextParagraphType::SceneHeading: {
                if (!_options->importLocations) {
                    break;
                }

                const auto currentLocationName = locationName(paragraphText);
                if (currentLocationName.isEmpty()) {
                    break;
                }

                locationNames.emplace(currentLocationName);
                break;
            }

            case TextParagraphType::Character: {
                if (!_options->importCharacters) {
                    break;
                }

                const auto currentCharacterName = characterName(paragraphText);
                if (currentCharacterName.isEmpty()) {
                    break;
                }

                characterNames.emplace(currentCharacterName);
                break;
            }

            default:
                break;
            }

            //
            // Запомним последний стиль блока и обнулим счётчик пустых строк
            //
            lastBlockType = blockType;
            emptyLines = 0;
        }
        //
        // Если в блоке нет текста, то увеличиваем счётчик пустых строк
        //
        else {
            ++emptyLines;
        }

        cursor.movePosition(QTextCursor::NextCharacter);
    } while (!cursor.atEnd());

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

QString AbstractDocumentImporter::parseDocument(const ImportOptions* _options,
                                                QTextDocument& _document) const
{
    //
    // Найти минимальный отступ слева для всех блоков
    // ЗАЧЕМ: во многих программах (Final Draft, Screeviner) сделано так, что поля
    //		  задаются за счёт оступов. Получается что и заглавие сцены и описание действия
    //		  имеют отступы. Так вот это и будет минимальным отступом, который не будем считать
    //
    int minLeftMargin = 1000;
    {
        QTextCursor cursor(&_document);
        while (!cursor.atEnd()) {
            if (minLeftMargin > cursor.blockFormat().leftMargin()) {
                minLeftMargin = cursor.blockFormat().leftMargin();
            }
            cursor.movePosition(QTextCursor::NextBlock);
            cursor.movePosition(QTextCursor::EndOfBlock);
        }
    }

    QString result;
    QXmlStreamWriter writer(&result);
    writer.writeStartDocument();
    writer.writeStartElement(xml::kDocumentTag);
    writer.writeAttribute(xml::kMimeTypeAttribute,
                          Domain::mimeTypeFor(Domain::DocumentObjectType::ScreenplayText));
    writer.writeAttribute(xml::kVersionAttribute, "1.0");

    //
    // Для каждого блока текста определяем тип
    //
    // ... последний стиль блока
    auto lastBlockType = TextParagraphType::Undefined;
    // ... количество пустых строк
    int emptyLines = 0;
    bool alreadyInScene = false;
    QTextCursor cursor(&_document);
    do {
        cursor.movePosition(QTextCursor::EndOfBlock);

        //
        // Если в блоке есть текст
        //
        if (!cursor.block().text().simplified().isEmpty()) {
            //
            // ... определяем тип
            //
            const auto blockType
                = typeForTextCursor(cursor, lastBlockType, emptyLines, minLeftMargin);

            //
            // Извлечем номер сцены
            //
            QString sceneNumber;
            if (blockType == TextParagraphType::SceneHeading) {
                const auto match
                    = kStartFromNumberChecker.match(cursor.block().text().simplified());
                if (match.hasMatch()) {
                    cursor.movePosition(QTextCursor::StartOfBlock);
                    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                                        match.capturedEnd());
                    if (cursor.hasSelection()) {
                        if (shouldKeepSceneNumbers(_options)) {
                            sceneNumber = cursor.selectedText().trimmed();
                            if (sceneNumber.endsWith('.')) {
                                sceneNumber.chop(1);
                            }
                        }
                        cursor.deleteChar();
                    }
                    cursor.movePosition(QTextCursor::EndOfBlock);
                }
            }

            //
            // Выполняем корректировки
            //
            simplifyTextBlock(cursor);
            const auto paragraphText = clearBlockText(blockType, cursor.block().text());

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

                if (!sceneNumber.isEmpty()) {
                    writer.writeStartElement(xml::kNumberTag);
                    writer.writeAttribute(xml::kNumberValueAttribute, sceneNumber);
                    writer.writeAttribute(xml::kNumberIsCustomAttribute, "true");
                    writer.writeAttribute(xml::kNumberIsEatNumberAttribute, "true");
                    writer.writeEndElement();
                }

                writer.writeStartElement(xml::kContentTag);
            }
            writer.writeStartElement(toString(blockType));
            writer.writeStartElement(xml::kValueTag);
            writer.writeCDATA(TextHelper::toHtmlEscaped(paragraphText));
            writer.writeEndElement(); // value
            //
            // Пишем редакторские комментарии
            //
            writeReviewMarks(writer, cursor);

            //
            // Пишем форматирование
            //
            {
                const QTextBlock currentBlock = cursor.block();
                if (!currentBlock.textFormats().isEmpty()) {
                    writer.writeStartElement(xml::kFormatsTag);
                    for (const auto& range : currentBlock.textFormats()) {
                        if (range.format.fontWeight() != QFont::Normal || range.format.fontItalic()
                            || range.format.fontUnderline() || range.format.fontStrikeOut()) {
                            writer.writeEmptyElement(xml::kFormatTag);
                            writer.writeAttribute(xml::kFromAttribute,
                                                  QString::number(range.start));
                            writer.writeAttribute(xml::kLengthAttribute,
                                                  QString::number(range.length));
                            if (range.format.fontWeight() != QFont::Normal) {
                                writer.writeAttribute(xml::kBoldAttribute, "true");
                            }
                            if (range.format.fontItalic()) {
                                writer.writeAttribute(xml::kItalicAttribute, "true");
                            }
                            if (range.format.fontUnderline()) {
                                writer.writeAttribute(xml::kUnderlineAttribute, "true");
                            }
                            if (range.format.fontStrikeOut()) {
                                writer.writeAttribute(xml::kStrikethroughAttribute, "true");
                            }
                        }
                    }
                    writer.writeEndElement();
                }
            }
            writer.writeEndElement(); // block type

            //
            // Запомним последний стиль блока и обнулим счётчик пустых строк
            //
            lastBlockType = blockType;
            emptyLines = 0;
        }
        //
        // Если в блоке нет текста, то увеличиваем счётчик пустых строк
        //
        else {
            ++emptyLines;
        }

        cursor.movePosition(QTextCursor::NextCharacter);
    } while (!cursor.atEnd());

    writer.writeEndDocument();

    return { result };
}

TextParagraphType AbstractDocumentImporter::typeForTextCursor(const QTextCursor& _cursor,
                                                              TextParagraphType _lastBlockType,
                                                              int _prevEmptyLines,
                                                              int _minLeftMargin) const
{
    //
    // Определим текст блока
    //
    const QString blockText = _cursor.block().text();
    const QString blockTextUppercase = TextHelper::smartToUpper(blockText);
    const QString blockTextWithoutParentheses
        = _cursor.block().text().remove(kTextInParenthesisChecker);

    //
    // Для всех нераспознаных блоков ставим тип "Описание действия"
    //
    TextParagraphType blockType = TextParagraphType::Action;

    //
    // Определим некоторые характеристики исследуемого текста
    //
    // ... стили блока
    const QTextBlockFormat blockFormat = _cursor.blockFormat();
    const QTextCharFormat charFormat = _cursor.charFormat();
    // ... текст в верхнем регистре (FIXME: такие строки, как "Я.")
    bool textIsUppercase = charFormat.fontCapitalization() == QFont::AllUppercase
        || blockText == TextHelper::smartToUpper(blockText);
    // ... блоки находящиеся в центре
    bool isCentered = !blockFormat.alignment().testFlag(Qt::AlignRight)
        && (((blockFormat.leftMargin() + blockFormat.indent() + blockFormat.textIndent()) > 0
             && (blockFormat.leftMargin() + blockFormat.indent() + blockFormat.textIndent())
                 > kLeftMarginDelta + _minLeftMargin)
            || (blockFormat.alignment().testFlag(Qt::AlignHCenter))
            || blockText.startsWith(kOldSchoolCenteringPrefix));

    //
    // Собственно определение типа
    //
    {
        //
        // Самым первым пробуем определить заголовок сцены
        // 1. содержит ключевые сокращения места действия или начинается с номера сцены
        //
        if (blockTextUppercase.contains(kPlaceContainsChecker)
            || blockTextUppercase.contains(kStartFromNumberChecker)) {
            blockType = TextParagraphType::SceneHeading;
        }

        //
        // Блоки текста посередине
        //
        if (isCentered) {
            //
            // Переход
            // 1. в верхнем регистре
            // 2. заканчивается двоеточием (без учета пробелов)
            //
            if (textIsUppercase && blockText.simplified().endsWith(":")) {
                blockType = TextParagraphType::Transition;
            }
            //
            // Ремарка
            // 1. начинается скобкой
            //
            else if (blockText.startsWith("(")) {
                blockType = TextParagraphType::Parenthetical;
            }
            //
            // Персонаж
            // 1. В верхнем регистре
            //
            else if ((textIsUppercase
                      || blockTextWithoutParentheses
                          == TextHelper::smartToUpper(blockTextWithoutParentheses))
                     && _lastBlockType != TextParagraphType::Character) {
                blockType = TextParagraphType::Character;
            }
            //
            // Реплика
            // 1. не имеет сверху отступа
            //
            else if (blockFormat.topMargin() == 0) {
                blockType = TextParagraphType::Dialogue;
            }
            //
            // Заметка по тексту
            // 1. всё что осталось
            //
            else {
                blockType = TextParagraphType::InlineNote;
            }

        }
        //
        // Не посередине
        //
        else {
            //
            // Блоки текста в верхнем регистре
            //
            if (textIsUppercase) {
                //
                // Участники сцены
                // 1. в верхнем регистре
                // 2. идут сразу же после сцены или участника сцены
                // 3. не имеют сверху отступа
                //
                if ((_lastBlockType == TextParagraphType::SceneHeading
                     || _lastBlockType == TextParagraphType::SceneCharacters)
                    && _prevEmptyLines == 0 && blockFormat.topMargin() == 0) {
                    blockType = TextParagraphType::SceneCharacters;
                }
                //
                // Заголовок сцены
                // 1. в верхнем регистре
                // 2. не имеет отступов
                // 3. выровнен по левому краю
                //
                else if (blockFormat.alignment().testFlag(Qt::AlignLeft) && !isCentered) {
                    blockType = TextParagraphType::SceneHeading;
                }
                //
                // Переход
                // 1. в верхнем регистре
                // 2. выровнен по правому краю
                //
                else if (blockFormat.alignment().testFlag(Qt::AlignRight)) {
                    blockType = TextParagraphType::Transition;
                }
            }
        }

        //
        // Отдельные проверки для блоков, которые могут быть в разных регистрах и в разных местах
        // страницы
        //
        // Титр (формируем, как описание действия)
        // 1. начинается со слова ТИТР:
        //
        if (blockTextUppercase.contains(kTitleChecker)) {
            blockType = TextParagraphType::Action;
        }
    }

    return blockType;
}

QString AbstractDocumentImporter::clearBlockText(TextParagraphType _blockType,
                                                 const QString& _blockText) const
{
    QString result = _blockText;

    //
    // Удаляем длинные тире
    //
    result = result.replace("–", "-");

    //
    // Для блока заголовка сцены:
    // * всевозможные "инт - " меняем на "инт. "
    // * убираем точки в конце названия локации
    //
    if (_blockType == TextParagraphType::SceneHeading) {
        const QString location = /*ScreenplaySceneHeadingParser::location*/ (_blockText);
        QString clearLocation = location.simplified();
        clearLocation.remove(NOISE_AT_START);
        clearLocation.remove(NOISE_AT_END);
        if (location != clearLocation) {
            result = result.replace(location, clearLocation);
        }
    }
    //
    // Для персонажей
    // * убираем точки в конце
    //
    else if (_blockType == TextParagraphType::Character) {
        const QString name = /*ScreenplayCharacterParser::name*/ (_blockText);
        QString clearName = name.simplified();
        clearName.remove(NOISE_AT_END);
        if (name != clearName) {
            result = result.replace(name, clearName);
        }
    }
    //
    // Ремарка
    // * убираем скобки
    //
    else if (_blockType == TextParagraphType::Parenthetical) {
        QString clearParenthetical = _blockText.simplified();
        if (!clearParenthetical.isEmpty() && clearParenthetical.front() == '(') {
            clearParenthetical.remove(0, 1);
        }
        if (!clearParenthetical.isEmpty() && clearParenthetical.back() == ')') {
            clearParenthetical.chop(1);
        }
        result = clearParenthetical;
    }

    return result;
}

QRegularExpression AbstractDocumentImporter::startFromNumberChecker() const
{
    return kStartFromNumberChecker;
}

bool AbstractDocumentImporter::shouldKeepSceneNumbers(const ImportOptions* _options) const
{
    Q_UNUSED(_options)

    return false;
}

void AbstractDocumentImporter::writeReviewMarks(QXmlStreamWriter& _writer,
                                                QTextCursor& _cursor) const
{
    Q_UNUSED(_writer)
    Q_UNUSED(_cursor)
}

} // namespace BusinessLayer
