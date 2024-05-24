#pragma once

#include "abstract_screenplay_importer.h"

#include <corelib/business_layer/import/abstract_fountain_importer.h>

#include <QScopedPointer>


namespace BusinessLayer {

/**
 * @brief Импортер сценария из файлов fountain
 */
class CORE_LIBRARY_EXPORT ScreenplayFountainImporter : public AbstractScreenplayImporter,
                                                       public AbstractFountainImporter
{
    /*
                  . .
                 ` ' `
             .'''. ' .'''.
               .. ' ' ..
              '  '.'.'  '
              .'''.'.'''
             ' .''.'.''. '
               . . : . .
          {} _'___':'___'_ {}
          ||(_____________)||
          """"""(     )""""""
                _)   (_             .^-^.  ~""~
               (_______)~~"""~~     '._.'
           ~""~                     .' '.
                                    '.,.'
                                       `'`'
     */

public:
    ScreenplayFountainImporter();
    ~ScreenplayFountainImporter() override;

    /**
     * @brief Импорт докуметов (всех, кроме сценариев)
     */
    Documents importDocuments(const ScreenplayImportOptions& _options) const override;

    /**
     * @brief Импортировать сценарии
     */
    QVector<Screenplay> importScreenplays(const ScreenplayImportOptions& _options) const override;

    /**
     * @brief Импортировать сценарий из заданного текста
     */
    Screenplay importScreenplay(const QString& _screenplayText) const;

    /**
     * @brief Обработка блоков текста
     * @return Стек незакрытых дирректорий
     */
    virtual QStack<QString> processBlocks(QVector<QString>& paragraphs,
                                          QXmlStreamWriter& _writer) const override;

    /**
     * @brief Добавить блок
     */
    virtual void appendBlock(const QString& _paragraphText, TextParagraphType _type,
                             QXmlStreamWriter& _writer, bool _shouldClosePrevBlock) const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer
