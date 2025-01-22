#pragma once

#include "abstract_screenplay_importer.h"

#include <business_layer/import/abstract_fountain_importer.h>

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
     * @brief Импортировать сценарии
     */
    QVector<Screenplay> importScreenplays(const ScreenplayImportOptions* _options) const override;

    /**
     * @brief Получить основной текст сценария в формате xml из заданного текста
     */
    Screenplay screenplayText(const QString& _screenplayText, bool _keepSceneNumbers = true) const;

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
