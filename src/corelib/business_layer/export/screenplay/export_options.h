#pragma once

#include <QColor>
#include <QString>

#include <corelib_global.h>


namespace BusinessLayer {

/**
 * @brief Опции экспорта
 */
struct CORE_LIBRARY_EXPORT ExportOptions {
    /**
     * @brief Путь к файлу
     */
    QString filePath;

    /**
     * @brief Формат файла
     */
    int fileFormat = 0;

    /**
     * @brief Идентификатор шаблона экспорта
     */
    QString templateId;

    /**
     * @brief Печатать титульную страницу
     */
    bool printTiltePage = true;

    /**
     * @brief Печатать ли блоки папок
     */
    bool printFolders = true;

    /**
     * @brief Печатать ли комментарии по тексту
     */
    bool printInlineNotes = false;

    /**
     * @brief Печатать номера сцен
     */
    bool printScenesNumbers = true;
    bool printScenesNumbersOnLeft = true;
    bool printScenesNumbersOnRight = true;

    /**
     * @brief Печатать номера реплик
     */
    bool printDialoguesNumbers = false;

    /**
     * @brief Печатать редакторские пометки
     */
    bool printReviewMarks = true;

    /**
     * @brief Водяной знак
     */
    QString watermark;
    QColor watermarkColor;

    //
    // Параметры самого документа
    //
    QString header;
    QString footer;
};

} // namespace BusinessLayer
