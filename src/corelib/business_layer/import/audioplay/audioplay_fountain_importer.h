#pragma once

#include "abstract_audioplay_importer.h"

#include <business_layer/import/abstract_fountain_importer.h>

#include <QScopedPointer>


namespace BusinessLayer {

/**
 * @brief Импортер аудиопьес из файлов fountain
 */
class CORE_LIBRARY_EXPORT AudioplayFountainImporter : public AbstractAudioplayImporter,
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
    AudioplayFountainImporter();
    ~AudioplayFountainImporter() override;

    /**
     * @brief Импортировать аудиопьесу
     */
    Audioplay importAudioplay(const ImportOptions* _options) const override;

    /**
     * @brief Получить основной текст аудиопьесы в формате xml из заданного текста
     */
    Audioplay audioplayText(const QString& _audioplayText) const;

protected:
    /**
     * @brief Получить имя персонажа
     */
    QString characterName(const QString& _text) const override;

    /**
     * @brief Получить название локации
     */
    QString locationName(const QString& _text) const override;

    /**
     * @brief Определить тип блока
     */
    TextParagraphType blockType(QString& _paragraphText) const override;

    /**
     * @brief Помещать диалоги в таблицу
     */
    bool placeDialoguesInTable() const override;
};

} // namespace BusinessLayer
