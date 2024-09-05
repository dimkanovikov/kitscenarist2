#pragma once

#include <corelib_global.h>

class QTextDocument;
class QTextCursor;
class QString;
class QXmlStreamWriter;

namespace BusinessLayer {

struct ImportOptions;
enum class TextParagraphType;

/**
 * @brief Класс для импорта из QTextDocument
 */
class CORE_LIBRARY_EXPORT AbstractQTextDocumentImporter
{
public:
    AbstractQTextDocumentImporter();
    virtual ~AbstractQTextDocumentImporter();

    /**
     * @brief Получить из QTextDocument xml-строку
     */
    QString parseDocument(const ImportOptions& _options, QTextDocument& _document) const;

protected:
    /**
     * @brief Очистка блоков от мусора и их корректировки
     */
    QString clearBlockText(TextParagraphType _blockType, const QString& _blockText) const;

    /**
     * @brief Обработать блок заголовка сцены
     */
    virtual QString processSceneHeading(const ImportOptions& _options, QTextCursor& _cursor) const;

    /**
     * @brief Определить тип блока в текущей позиции курсора
     *		  с указанием предыдущего типа и количества предшествующих пустых строк
     */
    TextParagraphType typeForTextCursor(const QTextCursor& _cursor,
                                        TextParagraphType _lastBlockType, int _prevEmptyLines,
                                        int _minLeftMargin) const;

    /**
     * @brief Записать редакторские заметки
     */
    virtual void writeReviewMarks(QXmlStreamWriter& _writer, QTextCursor& _cursor) const;
};


} // namespace BusinessLayer
