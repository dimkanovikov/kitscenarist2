#pragma once

#include "abstract_stageplay_importer.h"

#include <business_layer/import/abstract_fountain_importer.h>

#include <QScopedPointer>


namespace BusinessLayer {

/**
 * @brief Импортер пьес из файлов fountain
 */
class CORE_LIBRARY_EXPORT StageplayFountainImporter : public AbstractStageplayImporter,
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
    StageplayFountainImporter();
    ~StageplayFountainImporter() override;

    /**
     * @brief Импортировать пьесу
     */
    Stageplay importStageplay(const ImportOptions* _options) const override;

    /**
     * @brief Получить основной текст пьесы в формате xml из заданного текста
     */
    Stageplay stageplayText(const QString& _stageplayText) const;

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
     * @brief Помещать диалоги в таблицу
     */
    bool placeDialoguesInTable() const override;
};

} // namespace BusinessLayer
