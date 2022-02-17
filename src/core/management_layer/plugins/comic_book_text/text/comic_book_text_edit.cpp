#include "comic_book_text_edit.h"

#include "handlers/key_press_handler_facade.h"

#include <business_layer/document/comic_book/text/comic_book_text_block_data.h>
#include <business_layer/document/comic_book/text/comic_book_text_corrector.h>
#include <business_layer/document/comic_book/text/comic_book_text_cursor.h>
#include <business_layer/document/comic_book/text/comic_book_text_document.h>
#include <business_layer/import/comic_book/comic_book_plain_text_importer.h>
#include <business_layer/model/comic_book/comic_book_information_model.h>
#include <business_layer/model/comic_book/text/comic_book_text_model.h>
#include <business_layer/model/comic_book/text/comic_book_text_model_page_item.h>
#include <business_layer/model/comic_book/text/comic_book_text_model_text_item.h>
#include <business_layer/templates/comic_book_template.h>
#include <business_layer/templates/templates_facade.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>

#include <QAction>
#include <QCoreApplication>
#include <QLocale>
#include <QMimeData>
#include <QPainter>
#include <QPointer>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTextTable>

using BusinessLayer::ComicBookBlockStyle;
using BusinessLayer::ComicBookParagraphType;
using BusinessLayer::TemplatesFacade;

namespace Ui {

class ComicBookTextEdit::Implementation
{
public:
    explicit Implementation(ComicBookTextEdit* _q);

    void revertAction(bool previous);


    ComicBookTextEdit* q = nullptr;

    QPointer<BusinessLayer::ComicBookTextModel> model;
    BusinessLayer::ComicBookTextDocument document;

    bool showSceneNumber = false;
    bool showSceneNumberOnLeft = false;
    bool showSceneNumberOnRight = false;
    bool showDialogueNumber = false;
};

ComicBookTextEdit::Implementation::Implementation(ComicBookTextEdit* _q)
    : q(_q)
{
}

void ComicBookTextEdit::Implementation::revertAction(bool previous)
{
    if (model == nullptr) {
        return;
    }

    const auto lastCursorPosition = q->textCursor().position();
    //
    if (previous) {
        model->undo();
    } else {
        model->redo();
    }
    //
    if (document.characterCount() > lastCursorPosition) {
        auto cursor = q->textCursor();
        cursor.setPosition(lastCursorPosition);
        q->setTextCursorReimpl(cursor);
        q->ensureCursorVisible();

        //
        // При отмене последнего действия позиция курсора могла и не поменяться,
        // но тип параграфа сменился, поэтому перестраховываемся и говорим будто бы
        // сменилась позиция курсора, чтобы обновить состояние панелей
        //
        emit q->cursorPositionChanged();
    }
}


// ****


ComicBookTextEdit::ComicBookTextEdit(QWidget* _parent)
    : BaseTextEdit(_parent)
    , d(new Implementation(this))
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    setFrameShape(QFrame::NoFrame);
    setShowPageNumbers(true);

    setDocument(&d->document);
    setCapitalizeWords(false);
}

ComicBookTextEdit::~ComicBookTextEdit() = default;

void ComicBookTextEdit::setShowSceneNumber(bool _show, bool _onLeft, bool _onRight)
{
    d->showSceneNumber = _show;
    d->showSceneNumberOnLeft = _onLeft;
    d->showSceneNumberOnRight = _onRight;
    update();
}

void ComicBookTextEdit::setShowDialogueNumber(bool _show)
{
    d->showDialogueNumber = _show;
    update();
}

void ComicBookTextEdit::initWithModel(BusinessLayer::ComicBookTextModel* _model)
{
    if (d->model && d->model->informationModel()) {
        disconnect(d->model->informationModel());
    }
    d->model = _model;

    //
    // Сбрасываем модель, чтобы не вылезали изменения документа при изменении параметров страницы
    //
    d->document.setModel(nullptr);

    //
    // Обновляем параметры страницы из шаблона
    //
    if (usePageMode()) {
        const auto currentTemplate = TemplatesFacade::comicBookTemplate();
        setPageFormat(currentTemplate.pageSizeId());
        setPageMarginsMm(currentTemplate.pageMargins());
        setPageNumbersAlignment(currentTemplate.pageNumbersAlignment());
    }

    //
    // Документ нужно формировать только после того, как редактор настроен, чтобы избежать лишний
    // изменений
    //
    d->document.setModel(d->model);

    //
    // Отслеживаем изменения некоторых параметров
    //
    if (d->model && d->model->informationModel()) {
        setHeader(d->model->informationModel()->header());
        setFooter(d->model->informationModel()->footer());

        connect(d->model->informationModel(),
                &BusinessLayer::ComicBookInformationModel::headerChanged, this,
                &ComicBookTextEdit::setHeader);
        connect(d->model->informationModel(),
                &BusinessLayer::ComicBookInformationModel::footerChanged, this,
                &ComicBookTextEdit::setFooter);
    }
}

void ComicBookTextEdit::reinit()
{
    //
    // Перенастроим всё, что зависит от шаблона
    //
    initWithModel(d->model);
}

BusinessLayer::ComicBookDictionariesModel* ComicBookTextEdit::dictionaries() const
{
    if (d->model == nullptr) {
        return nullptr;
    }

    return d->model->dictionariesModel();
}

BusinessLayer::CharactersModel* ComicBookTextEdit::characters() const
{
    if (d->model == nullptr) {
        return nullptr;
    }

    return d->model->charactersModel();
}

void ComicBookTextEdit::undo()
{
    d->revertAction(true);
}

void ComicBookTextEdit::redo()
{
    d->revertAction(false);
}

void ComicBookTextEdit::addParagraph(BusinessLayer::ComicBookParagraphType _type)
{
    d->document.addParagraph(_type, textCursor());

    //
    // Если вставляется персонаж, то разделяем страницу, для добавления реплики
    //
    if (_type == BusinessLayer::ComicBookParagraphType::Character) {
        const auto cursorPosition = textCursor().position();
        d->document.splitParagraph(textCursor());
        auto cursor = textCursor();
        cursor.setPosition(cursorPosition + 1); // +1 чтобы войти внутрь таблицы
        setTextCursor(cursor);
        cursor.movePosition(BusinessLayer::ComicBookTextCursor::NextBlock);
        d->document.setParagraphType(BusinessLayer::ComicBookParagraphType::Dialogue, cursor);
        //
        // Очищаем диалог, от текста, который туда добавляет корректор, пока там был блок персонажа
        //
        if (cursor.movePosition(BusinessLayer::ComicBookTextCursor::EndOfBlock,
                                BusinessLayer::ComicBookTextCursor::KeepAnchor)) {
            cursor.removeSelectedText();
        }
    }

    emit paragraphTypeChanged();
}

void ComicBookTextEdit::setCurrentParagraphType(BusinessLayer::ComicBookParagraphType _type)
{
    if (currentParagraphType() == _type) {
        return;
    }

    //
    // Если раньше это был персонаж, то объединяем блоки, чтобы убрать лишнюю таблицу
    //
    BusinessLayer::ComicBookTextCursor cursor = textCursor();
    const auto needSplitParagraph
        = _type == BusinessLayer::ComicBookParagraphType::Character && !cursor.inTable();
    const auto needMergeParagraph = currentParagraphType() == ComicBookParagraphType::Character
        && cursor.inTable() && cursor.inFirstColumn();

    d->document.setParagraphType(_type, textCursor());

    //
    // Если вставили папку, то нужно перейти к предыдущему блоку (из футера к хидеру)
    //
    if (_type == ComicBookParagraphType::FolderHeader) {
        moveCursor(QTextCursor::PreviousBlock);
    }
    //
    // Если вставляется персонаж, то разделяем страницу, для добавления реплики
    //
    else {
        //        cursor = textCursor();
        auto cursorPosition = cursor.position();
        if (needSplitParagraph) {
            d->document.splitParagraph(textCursor());
            cursor.setPosition(cursorPosition + 1); // +1 чтобы войти внутрь таблицы
            setTextCursor(cursor);
            cursor.movePosition(BusinessLayer::ComicBookTextCursor::NextBlock);
            d->document.setParagraphType(BusinessLayer::ComicBookParagraphType::Dialogue, cursor);
            //
            // Очищаем диалог, от текста, который туда добавляет корректор, пока там был блок
            // персонажа
            //
            if (cursor.movePosition(BusinessLayer::ComicBookTextCursor::EndOfBlock,
                                    BusinessLayer::ComicBookTextCursor::KeepAnchor)) {
                cursor.removeSelectedText();
            }
        } else if (needMergeParagraph) {
            d->document.mergeParagraph(textCursor());
            cursor.setPosition(cursorPosition - 1); // -1 т.к. таблицы больше нет
            setTextCursor(cursor);
        }
    }

    emit paragraphTypeChanged();
}

BusinessLayer::ComicBookParagraphType ComicBookTextEdit::currentParagraphType() const
{
    return ComicBookBlockStyle::forBlock(textCursor().block());
}

void ComicBookTextEdit::setTextCursorReimpl(const QTextCursor& _cursor)
{
    //
    // TODO: пояснить зачем это необходимо делать?
    //
    const int verticalScrollValue = verticalScrollBar()->value();
    setTextCursor(_cursor);
    verticalScrollBar()->setValue(verticalScrollValue);
}

QModelIndex ComicBookTextEdit::currentModelIndex() const
{
    if (d->model == nullptr) {
        return {};
    }

    auto userData = textCursor().block().userData();
    if (userData == nullptr) {
        return {};
    }

    auto comicBookBlockData = static_cast<BusinessLayer::ComicBookTextBlockData*>(userData);
    return d->model->indexForItem(comicBookBlockData->item());
}

void ComicBookTextEdit::setCurrentModelIndex(const QModelIndex& _index)
{
    BusinessLayer::ComicBookTextCursor textCursor(document());
    textCursor.setPosition(d->document.itemStartPosition(_index));
    ensureCursorVisible(textCursor);
}

int ComicBookTextEdit::positionForModelIndex(const QModelIndex& _index)
{
    return d->document.itemStartPosition(_index);
}

void ComicBookTextEdit::addReviewMark(const QColor& _textColor, const QColor& _backgroundColor,
                                      const QString& _comment)
{
    BusinessLayer::ComicBookTextCursor cursor(textCursor());
    if (!cursor.hasSelection()) {
        return;
    }

    d->document.addReviewMark(_textColor, _backgroundColor, _comment, cursor);
}

void ComicBookTextEdit::keyPressEvent(QKeyEvent* _event)
{
    if (isReadOnly()) {
        BaseTextEdit::keyPressEvent(_event);
        return;
    }

    //
    // Подготовим событие к обработке
    //
    _event->setAccepted(false);

    //
    // Получим обработчик
    //
    auto handler = KeyProcessingLayer::KeyPressHandlerFacade::instance(this);

    //
    // Подготовка к обработке
    //
    handler->prepare(_event);

    //
    // Предварительная обработка
    //
    handler->prepareForHandle(_event);

    //
    // Отправить событие в базовый класс
    //
    if (handler->needSendEventToBaseClass()) {
        if (keyPressEventReimpl(_event)) {
            _event->accept();
        } else {
            BaseTextEdit::keyPressEvent(_event);
            _event->ignore();
        }

        updateEnteredText(_event->text());
    }

    //
    // Обработка
    //
    if (!_event->isAccepted()) {
        handler->handle(_event);
    }

    //
    // Событие дошло по назначению
    //
    _event->accept();

    //
    // Убедимся, что курсор виден
    //
    if (handler->needEnsureCursorVisible()) {
        ensureCursorVisible();
    }

    //
    // Подготовим следующий блок к обработке
    //
    if (handler->needPrehandle()) {
        handler->prehandle();
    }
}

bool ComicBookTextEdit::keyPressEventReimpl(QKeyEvent* _event)
{
    bool isEventHandled = true;

    //
    // Переопределяем
    //
    // ... отмена последнего действия
    //
    if (_event == QKeySequence::Undo) {
        undo();
    }
    //
    // ... отмена последнего действия
    //
    else if (_event == QKeySequence::Redo) {
        redo();
    }
    //
    // ... вырезать текст
    //
    else if (_event == QKeySequence::Cut) {
        copy();
        BusinessLayer::ComicBookTextCursor cursor = textCursor();
        cursor.removeCharacters(this);
        d->model->saveChanges();
    }
    //
    // ... вставить текст
    //
    else if (_event == QKeySequence::Paste) {
        paste();
        d->model->saveChanges();
    }
    //
    // ... перевод курсора к следующему символу
    //
    else if (_event == QKeySequence::MoveToNextChar) {
        if (textCursor().block().textDirection() == Qt::LeftToRight) {
            moveCursor(QTextCursor::NextCharacter);
        } else {
            moveCursor(QTextCursor::PreviousCharacter);
        }

        while (!textCursor().atEnd()
               && (!textCursor().block().isVisible()
                   || ComicBookBlockStyle::forBlock(textCursor().block())
                       == ComicBookParagraphType::PageSplitter
                   || textCursor().blockFormat().boolProperty(
                       ComicBookBlockStyle::PropertyIsCorrection))) {
            moveCursor(QTextCursor::NextBlock);
        }
    }
    //
    // ... перевод курсора к предыдущему символу
    //
    else if (_event == QKeySequence::MoveToPreviousChar) {
        if (textCursor().block().textDirection() == Qt::LeftToRight) {
            moveCursor(QTextCursor::PreviousCharacter);
        } else {
            moveCursor(QTextCursor::NextCharacter);
        }
        while (!textCursor().atStart()
               && (!textCursor().block().isVisible()
                   || ComicBookBlockStyle::forBlock(textCursor().block())
                       == ComicBookParagraphType::PageSplitter
                   || textCursor().blockFormat().boolProperty(
                       ComicBookBlockStyle::PropertyIsCorrection))) {
            moveCursor(QTextCursor::StartOfBlock);
            if (textCursor().block().textDirection() == Qt::LeftToRight) {
                moveCursor(QTextCursor::PreviousCharacter);
            } else {
                moveCursor(QTextCursor::NextCharacter);
            }
        }
    }
    //
    // ... вставим перенос строки внутри абзаца
    //
    else if ((_event->key() == Qt::Key_Enter || _event->key() == Qt::Key_Return)
             && _event->modifiers().testFlag(Qt::ShiftModifier)) {
        textCursor().insertText(QChar(QChar::LineSeparator));
    }
    //
    // Обрабатываем в базовом классе
    //
    else {
        isEventHandled = BaseTextEdit::keyPressEventReimpl(_event);
    }

    return isEventHandled;
}

bool ComicBookTextEdit::updateEnteredText(const QString& _eventText)
{
    if (_eventText.isEmpty()) {
        return false;
    }

    //
    // Получим значения
    //
    // ... курсора
    QTextCursor cursor = textCursor();
    // ... блок текста в котором находится курсор
    QTextBlock currentBlock = cursor.block();
    // ... текст блока
    QString currentBlockText = currentBlock.text();
    // ... текст до курсора
    QString cursorBackwardText = currentBlockText.left(cursor.positionInBlock());
    // ... текст после курсора
    QString cursorForwardText = currentBlockText.mid(cursor.positionInBlock());
    // ... стиль шрифта блока
    QTextCharFormat currentCharFormat = currentBlock.charFormat();

    //
    // Определяем необходимость установки верхнего регистра для первого символа блока
    //
    if (currentCharFormat.boolProperty(ComicBookBlockStyle::PropertyIsFirstUppercase)
        && cursorBackwardText != " " && cursorBackwardText == _eventText
        && _eventText[0] != TextHelper::smartToUpper(_eventText[0])) {
        //
        // Сформируем правильное представление строки
        //
        QString correctedText = _eventText;
        correctedText[0] = TextHelper::smartToUpper(correctedText[0]);

        //
        // Стираем предыдущий введённый текст
        //
        for (int repeats = 0; repeats < _eventText.length(); ++repeats) {
            cursor.deletePreviousChar();
        }

        //
        // Выводим необходимый
        //
        cursor.insertText(correctedText);
        setTextCursor(cursor);

        return true;
    }

    //
    // Если перед нами конец предложения
    // и не сокращение
    // и после курсора нет текста (для ремарки допустима скобка)
    //
    const QString endOfSentancePattern = QString("([.]|[?]|[!]|[…]) %1$").arg(_eventText);
    if (cursorBackwardText.contains(QRegularExpression(endOfSentancePattern))
        && cursorForwardText.isEmpty()
        && _eventText[0] != TextHelper::smartToUpper(_eventText[0])) {
        //
        // Сделаем первую букву заглавной
        //
        QString correctedText = _eventText;
        correctedText[0] = TextHelper::smartToUpper(correctedText[0]);

        //
        // Стираем предыдущий введённый текст
        //
        for (int repeats = 0; repeats < _eventText.length(); ++repeats) {
            cursor.deletePreviousChar();
        }

        //
        // Выводим необходимый
        //
        cursor.insertText(correctedText);
        setTextCursor(cursor);

        return true;
    }

    return BaseTextEdit::updateEnteredText(_eventText);
}

void ComicBookTextEdit::paintEvent(QPaintEvent* _event)
{
    BaseTextEdit::paintEvent(_event);

    //
    // Определить область прорисовки по краям от текста
    //
    const bool isLeftToRight = QLocale().textDirection() == Qt::LeftToRight;
    const qreal pageLeft = 0;
    const qreal pageRight = viewport()->width();
    const qreal spaceBetweenSceneNumberAndText = 10 * Ui::DesignSystem::scaleFactor();
    ;
    const qreal textLeft = pageLeft - (isLeftToRight ? 0 : horizontalScrollBar()->maximum())
        + document()->rootFrame()->frameFormat().leftMargin() - spaceBetweenSceneNumberAndText;
    const qreal textRight = pageRight + (isLeftToRight ? horizontalScrollBar()->maximum() : 0)
        - document()->rootFrame()->frameFormat().rightMargin() + spaceBetweenSceneNumberAndText;
    const qreal leftDelta = (isLeftToRight ? -1 : 1) * horizontalScrollBar()->value();
    //    int colorRectWidth = 0;
    qreal verticalMargin = 0;
    const auto currentTemplate = TemplatesFacade::comicBookTemplate();
    const qreal splitterX = leftDelta + textLeft
        + (textRight - textLeft) * currentTemplate.leftHalfOfPageWidthPercents() / 100;


    //
    // Определим начальный блок на экране
    //
    QTextBlock topBlock = document()->lastBlock();
    {
        QTextCursor topCursor;
        for (int delta = 0; delta < viewport()->height() / 4; delta += 10) {
            topCursor = cursorForPosition(viewport()->mapFromParent(QPoint(0, delta)));
            if (topBlock.blockNumber() > topCursor.block().blockNumber()) {
                topBlock = topCursor.block();
            }
        }
    }
    //
    // ... идём до начала сцены
    //
    while (ComicBookBlockStyle::forBlock(topBlock) != ComicBookParagraphType::Page
           && ComicBookBlockStyle::forBlock(topBlock) != ComicBookParagraphType::Panel
           && ComicBookBlockStyle::forBlock(topBlock) != ComicBookParagraphType::FolderHeader
           && topBlock != document()->firstBlock()) {
        topBlock = topBlock.previous();
    }

    //
    // Определим последний блок на экране
    //
    QTextBlock bottomBlock = document()->firstBlock();
    {
        BusinessLayer::ComicBookTextCursor bottomCursor;
        for (int delta = viewport()->height(); delta > viewport()->height() * 3 / 4; delta -= 10) {
            bottomCursor = cursorForPosition(viewport()->mapFromParent(QPoint(0, delta)));
            if (bottomBlock.blockNumber() < bottomCursor.block().blockNumber()) {
                bottomBlock = bottomCursor.block();
            }
        }
    }
    if (bottomBlock == document()->firstBlock()) {
        bottomBlock = document()->lastBlock();
    }
    bottomBlock = bottomBlock.next();
    //
    // ... в случае, если блок попал в таблицу, нужно дойти до конца таблицы
    //
    {
        BusinessLayer::ComicBookTextCursor bottomCursor(document());
        bottomCursor.setPosition(bottomBlock.position());
        while (bottomCursor.inTable() && bottomCursor.movePosition(QTextCursor::NextBlock)) {
            bottomBlock = bottomCursor.block();
        }
    }

    //
    // Прорисовка дополнительных элементов редактора
    //
    {
        //
        // Декорации текста
        //
        {
            QPainter painter(viewport());
            clipPageDecorationRegions(&painter);

            //
            // Проходим блоки на экране и декорируем их
            //
            QTextBlock block = topBlock;
            const QRectF viewportGeometry = viewport()->geometry();
            int lastSceneBlockBottom = 0;
            QColor lastItemColor;
            bool isLastBlockSceneHeadingWithNumberAtRight = false;
            int lastCharacterBlockBottom = 0;
            QColor lastCharacterColor;

            BusinessLayer::ComicBookTextCursor cursor(document());
            while (block.isValid() && block != bottomBlock) {
                //
                // Стиль текущего блока
                //
                const auto blockType = ComicBookBlockStyle::forBlock(block);

                cursor.setPosition(block.position());
                const QRect cursorR = cursorRect(cursor);
                cursor.movePosition(QTextCursor::EndOfBlock);
                const QRect cursorREnd = cursorRect(cursor);
                //
                verticalMargin = cursorR.height() / 2;

                //
                // Определим цвет сцены
                //
                if (blockType == ComicBookParagraphType::Page
                    || blockType == ComicBookParagraphType::Panel
                    || blockType == ComicBookParagraphType::FolderHeader) {
                    lastSceneBlockBottom = cursorR.top();
                    lastItemColor = d->document.itemColor(block);
                }

                //
                // Нарисуем цвет панели
                //
                if (lastItemColor.isValid()) {
                    const auto isBlockSceneHeadingWithNumberAtRight
                        = blockType == ComicBookParagraphType::Panel && d->showSceneNumber
                        && d->showSceneNumberOnRight;
                    if (!isBlockSceneHeadingWithNumberAtRight) {
                        const QPointF topLeft(
                            isLeftToRight ? textRight + leftDelta + DesignSystem::layout().px8()
                                          : (textLeft - DesignSystem::layout().px4() + leftDelta),
                            isLastBlockSceneHeadingWithNumberAtRight
                                ? cursorR.top() - verticalMargin
                                : lastSceneBlockBottom - verticalMargin);
                        const QPointF bottomRight(isLeftToRight ? textRight
                                                          + DesignSystem::layout().px4() + leftDelta
                                                                : textLeft + leftDelta,
                                                  cursorREnd.bottom() + verticalMargin);
                        const QRectF rect(topLeft, bottomRight);
                        painter.fillRect(rect, lastItemColor);
                    }

                    isLastBlockSceneHeadingWithNumberAtRight = isBlockSceneHeadingWithNumberAtRight;
                }

                //                //
                //                // Определим цвет персонажа
                //                //
                //                if (blockType == ScenarioBlockStyle::Character) {
                //                    lastCharacterBlockBottom = cursorR.top();
                //                    colorRectWidth =
                //                    QFontMetrics(cursor.charFormat().font()).width(".");
                //                    lastCharacterColor = QColor();
                //                    const QString characterName =
                //                    BusinessLogic::CharacterParser::name(block.text()); if (auto
                //                    character =
                //                    DataStorageLayer::StorageFacade::researchStorage()->character(characterName))
                //                    {
                //                        if (character->color().isValid()) {
                //                            lastCharacterColor = character->color();
                //                        }
                //                    }
                //                } else if (blockType != ScenarioBlockStyle::Parenthetical
                //                           && blockType != ScenarioBlockStyle::Dialogue
                //                           && blockType != ScenarioBlockStyle::Lyrics) {
                //                    lastCharacterColor = QColor();
                //                }

                //                //
                //                // Нарисуем цвет персонажа
                //                //
                //                if (lastCharacterColor.isValid()) {
                //                    const QPointF topLeft(isLeftToRight
                //                                    ? textLeft - colorRectWidth + leftDelta
                //                                    : textRight + leftDelta,
                //                                    lastCharacterBlockBottom - verticalMargin);
                //                    const QPointF bottomRight(isLeftToRight
                //                                        ? textLeft + leftDelta
                //                                        : textRight + colorRectWidth + leftDelta,
                //                                        cursorREnd.bottom() + verticalMargin);
                //                    const QRectF rect(topLeft, bottomRight);
                //                    painter.fillRect(rect, lastCharacterColor);
                //                }

                //
                // Курсор на экране
                //
                // ... ниже верхней границы
                if ((cursorR.top() > 0 || cursorR.bottom() > 0)
                    // ... и выше нижней
                    && cursorR.top() < viewportGeometry.bottom()) {

                    //                    //
                    //                    // Прорисовка закладок
                    //                    //
                    //                    if (blockInfo != nullptr
                    //                        && blockInfo->hasBookmark()) {
                    //                        //
                    //                        // Определим область для отрисовки и выведем закладку
                    //                        в редактор
                    //                        //
                    //                        QPointF topLeft(isLeftToRight
                    //                                        ? pageLeft + leftDelta
                    //                                        : textRight + leftDelta,
                    //                                        cursorR.top());
                    //                        QPointF bottomRight(isLeftToRight
                    //                                            ? textLeft + leftDelta
                    //                                            : pageRight + leftDelta,
                    //                                            cursorR.bottom());
                    //                        QRectF rect(topLeft, bottomRight);
                    //                        painter.setBrush(blockInfo->bookmarkColor());
                    //                        painter.setPen(Qt::transparent);
                    //                        painter.drawRect(rect);
                    //                        painter.setPen(Qt::white);
                    //                    } else {
                    //                        painter.setPen(palette().text().color());
                    //                    }

                    //                    //
                    //                    // Новый способ отрисовки
                    //                    //
                    //                    //
                    //                    // Определим область для отрисовки и выведем номер сцены в
                    //                    редактор в зависимости от стороны
                    //                    //
                    //                    QPointF topLeft(isLeftToRight
                    //                                    ? pageLeft + leftDelta
                    //                                    : textRight + leftDelta,
                    //                                    cursorR.top());
                    //                    QPointF bottomRight(isLeftToRight
                    //                                        ? textLeft + leftDelta
                    //                                        : pageRight + leftDelta,
                    //                                        cursorR.bottom());
                    //                    QRectF rect(topLeft, bottomRight);
                    //                    rect.adjust(38, 0, 0, 0);
                    //                    painter.setFont(DesignSystem::font().iconsMid());
                    //                    const int size = painter.fontMetrics().lineSpacing();
                    //                    QRectF circle(rect.left() - size, rect.top() - size, size
                    //                    * 3, size * 3);
                    //                    painter.setBrush(ColorHelper::transparent(palette().text().color(),
                    //                    Ui::DesignSystem::hoverBackgroundOpacity()));
                    //                    painter.setPen(Qt::NoPen);
                    //                    painter.drawRect(circle);
                    //                    painter.setPen(palette().text().color());
                    //                    painter.drawText(rect, Qt::AlignLeft | Qt::AlignTop,
                    //                    u8"\U000F024B");

                    //
                    // Прорисовка декораций пустой строки
                    //
                    if (!block.blockFormat().boolProperty(ComicBookBlockStyle::PropertyIsCorrection)
                        && blockType != ComicBookParagraphType::PageSplitter
                        && block.text().simplified().isEmpty()) {
                        //
                        // Для пустого футера рисуем плейсхолдер
                        //
                        if (blockType == ComicBookParagraphType::FolderFooter) {
                            painter.setFont(block.charFormat().font());

                            //
                            // Ищем открывающий блок папки
                            //
                            auto headerBlock = block.previous();
                            int openedFolders = 0;
                            while (headerBlock.isValid()) {
                                const auto headerBlockType
                                    = ComicBookBlockStyle::forBlock(headerBlock);
                                if (headerBlockType == ComicBookParagraphType::FolderHeader) {
                                    if (openedFolders > 0) {
                                        --openedFolders;
                                    } else {
                                        break;
                                    }
                                } else if (headerBlockType
                                           == ComicBookParagraphType::FolderFooter) {
                                    ++openedFolders;
                                }

                                headerBlock = headerBlock.previous();
                            }

                            //
                            // Определим область для отрисовки плейсхолдера
                            //
                            const auto placeholderText = QString("%1 %2").arg(
                                QCoreApplication::translate(
                                    "KeyProcessingLayer::FolderFooterHandler", "END OF"),
                                headerBlock.text());
                            const QPoint topLeft
                                = QPoint(textLeft + leftDelta + spaceBetweenSceneNumberAndText,
                                         cursorR.top());
                            const QPoint bottomRight
                                = QPoint(textRight + leftDelta - spaceBetweenSceneNumberAndText,
                                         cursorR.bottom());
                            const QRect rect(topLeft, bottomRight);
                            painter.drawText(rect, block.blockFormat().alignment(),
                                             placeholderText);
                        }
                        //
                        // В остальных случаях рисуем индикатор пустой строки
                        //
                        else {
                            painter.setFont(block.charFormat().font());
                            const QString emptyLineMark = "» ";
                            //
                            // Определим область для отрисовки и выведем символ в редактор
                            //
                            // ... в тексте или в первой колоке таблички
                            //
                            if (!cursor.inTable() || cursor.inFirstColumn()) {
                                const QPointF topLeft(isLeftToRight ? pageLeft + leftDelta
                                                                    : textRight + leftDelta,
                                                      cursorR.top());
                                const QPointF bottomRight(isLeftToRight ? textLeft + leftDelta
                                                                        : pageRight + leftDelta,
                                                          cursorR.bottom() + 2);
                                const QRectF rect(topLeft, bottomRight);
                                painter.drawText(rect, Qt::AlignRight | Qt::AlignTop,
                                                 emptyLineMark);
                            }
                            //
                            // ... во второй колонке таблички
                            //
                            else {
                                const qreal x
                                    = splitterX - cursor.currentTable()->format().border();
                                const QPointF topLeft(
                                    x - painter.fontMetrics().horizontalAdvance(emptyLineMark),
                                    cursorR.top());
                                const QPointF bottomRight(x, cursorR.bottom() + 2);
                                const QRectF rect(topLeft, bottomRight);
                                painter.drawText(rect, Qt::AlignRight | Qt::AlignTop,
                                                 emptyLineMark);
                            }
                        }
                    }
                    //
                    // Прорисовка декораций непустых строк
                    //
                    else {
                        //
                        // Прорисовка декораций страницы
                        //
                        if (blockType == ComicBookParagraphType::Page) {
                            //
                            // Прорисовка количества панелей
                            //
                            int panelsCountWidth = 0;
                            if (const auto blockData
                                = static_cast<BusinessLayer::ComicBookTextBlockData*>(
                                    block.userData());
                                blockData != nullptr && blockData->item()->parent() != nullptr) {
                                const auto itemParent = blockData->item()->parent();
                                if (itemParent->type()
                                    == BusinessLayer::ComicBookTextModelItemType::Page) {
                                    const auto pageItem = static_cast<
                                        const BusinessLayer::ComicBookTextModelPageItem*>(
                                        itemParent);

                                    auto font = block.charFormat().font();
                                    font.setCapitalization(QFont::MixedCase);
                                    painter.setFont(
                                        currentTemplate
                                            .paragraphStyle(
                                                BusinessLayer::ComicBookParagraphType::Description)
                                            .font());
                                    const auto panelsCountText = QString(" (%1)").arg(
                                        tr("%n panels", "", pageItem->panelsCount()));
                                    panelsCountWidth
                                        = painter.fontMetrics().horizontalAdvance(panelsCountText);
                                    const QPoint topLeft = isLeftToRight
                                        ? cursorREnd.topLeft()
                                        : cursorREnd.topRight() - QPoint(panelsCountWidth, 0);
                                    const QPoint bottomRight = isLeftToRight
                                        ? cursorREnd.bottomRight() + QPoint(panelsCountWidth, 0)
                                        : cursorREnd.bottomLeft();
                                    const QRect rect(topLeft, bottomRight);
                                    painter.drawText(rect, Qt::AlignRight | Qt::AlignVCenter,
                                                     panelsCountText);
                                }
                            }

                            //
                            // Иконка положения страницы
                            //
                            {
                                painter.setFont(DesignSystem::font().iconsForEditors());
                                painter.setPen(palette().text().color());

                                auto paintLeftPageIcon = [&](const QString& _icon) {
                                    QPointF topLeft(isLeftToRight
                                                        ? pageLeft + leftDelta
                                                        : textRight + panelsCountWidth + leftDelta,
                                                    cursorR.top());
                                    QPointF bottomRight(isLeftToRight ? textLeft + leftDelta
                                                                      : pageRight + leftDelta,
                                                        cursorR.bottom());
                                    QRectF rect(topLeft, bottomRight);
                                    const auto textFontMetrics
                                        = QFontMetricsF(cursor.charFormat().font());
                                    const auto iconFontMetrics
                                        = QFontMetricsF(DesignSystem::font().iconsForEditors());
                                    const auto yDelta = (textFontMetrics.lineSpacing()
                                                         - iconFontMetrics.lineSpacing())
                                        / 2;
                                    rect.adjust(0, yDelta,
                                                -textFontMetrics.horizontalAdvance(".") / 2, 0);
                                    painter.drawText(rect, Qt::AlignRight | Qt::AlignTop, _icon);
                                };
                                auto paintRightPageIcon = [&](const QString& _icon) {
                                    const int spaceWidth
                                        = painter.fontMetrics().horizontalAdvance(" ");
                                    const QPoint topLeft(isLeftToRight
                                                             ? cursorREnd.left() + panelsCountWidth
                                                                 + spaceWidth
                                                             : cursorREnd.right() - spaceWidth,
                                                         cursorREnd.top());
                                    const QPoint bottomRight(isLeftToRight ? pageRight - leftDelta
                                                                           : 0 - leftDelta,
                                                             cursorREnd.bottom());
                                    QRectF rect(topLeft, bottomRight);
                                    const auto textFontMetrics
                                        = QFontMetricsF(cursor.charFormat().font());
                                    const auto iconFontMetrics
                                        = QFontMetricsF(DesignSystem::font().iconsForEditors());
                                    const auto yDelta = (textFontMetrics.lineSpacing()
                                                         - iconFontMetrics.lineSpacing())
                                        / 2;
                                    rect.adjust(0, yDelta,
                                                -textFontMetrics.horizontalAdvance(".") / 2, 0);
                                    painter.drawText(rect, Qt::AlignLeft | Qt::AlignTop, _icon);
                                };

                                const auto pageNumberText = d->document.pageNumber(block);
                                const auto leftPageIcon = u8"\U000F10AA";
                                const auto rightPageIcon = u8"\U000F10AB";

                                //
                                // Два номера
                                //
                                if (pageNumberText.contains('-')) {
                                    auto numbers = pageNumberText.split('-');
                                    const int firstNumber = numbers.constFirst().toInt();
                                    if (firstNumber % 2 == 0) {
                                        painter.setPen(DesignSystem::color().error());
                                        paintLeftPageIcon(rightPageIcon);
                                        paintRightPageIcon(leftPageIcon);
                                        painter.setPen(palette().text().color());
                                    } else {
                                        paintLeftPageIcon(leftPageIcon);
                                        paintRightPageIcon(rightPageIcon);
                                    }
                                } else {
                                    const auto pageNumber = pageNumberText.toInt();
                                    //
                                    // Правая страница
                                    //
                                    if (pageNumber % 2 == 0) {
                                        paintRightPageIcon(rightPageIcon);
                                    }
                                    //
                                    // Левая страница
                                    //
                                    else {
                                        paintLeftPageIcon(leftPageIcon);
                                    }
                                }
                            }
                        }
                    }
                }

                lastSceneBlockBottom = cursorREnd.bottom();

                block = block.next();
            }
        }

        //        //
        //        // Курсоры соавторов
        //        //
        //        {
        //            //
        //            // Ширина области курсора, для отображения имени автора курсора
        //            //
        //            const unsigned cursorAreaWidth = 20;

        //            if (!m_additionalCursors.isEmpty()
        //                && m_document != nullptr) {
        //                QPainter painter(viewport());
        //                painter.setFont(QFont("Sans", 8));
        //                painter.setPen(Qt::white);

        //                const QRectF viewportGeometry = viewport()->geometry();
        //                QPoint mouseCursorPos = mapFromGlobal(QCursor::pos());
        //                mouseCursorPos.setY(mouseCursorPos.y() +
        //                viewport()->mapFromParent(QPoint(0,0)).y()); int cursorIndex = 0; foreach
        //                (const QString& username, m_additionalCursorsCorrected.keys()) {
        //                    QTextCursor cursor(m_document);
        //                    m_document->setCursorPosition(cursor,
        //                    m_additionalCursorsCorrected.value(username)); const QRect cursorR =
        //                    cursorRect(cursor).adjusted(0, 0, 1, 0);

        //                    //
        //                    // Если курсор на экране
        //                    //
        //                    // ... ниже верхней границы
        //                    if ((cursorR.top() > 0 || cursorR.bottom() > 0)
        //                        // ... и выше нижней
        //                        && cursorR.top() < viewportGeometry.bottom()) {
        //                        //
        //                        // ... рисуем его
        //                        //
        //                        painter.fillRect(cursorR, ColorHelper::cursorColor(cursorIndex));

        //                        //
        //                        // ... декорируем
        //                        //
        //                        {
        //                            //
        //                            // Если мышь около него, то выводим имя соавтора
        //                            //
        //                            QRect extandedCursorR = cursorR;
        //                            extandedCursorR.setLeft(extandedCursorR.left() -
        //                            cursorAreaWidth/2); extandedCursorR.setWidth(cursorAreaWidth);
        //                            if (extandedCursorR.contains(mouseCursorPos)) {
        //                                const QRect usernameRect(
        //                                    cursorR.left() - 1,
        //                                    cursorR.top() - painter.fontMetrics().height() - 2,
        //                                    painter.fontMetrics().width(username) + 2,
        //                                    painter.fontMetrics().height() + 2);
        //                                painter.fillRect(usernameRect,
        //                                ColorHelper::cursorColor(cursorIndex));
        //                                painter.drawText(usernameRect, Qt::AlignCenter, username);
        //                            }
        //                            //
        //                            // Если нет, то рисуем небольшой квадратик
        //                            //
        //                            else {
        //                                painter.fillRect(cursorR.left() - 2, cursorR.top() - 5, 5,
        //                                5,
        //                                    ColorHelper::cursorColor(cursorIndex));
        //                            }
        //                        }
        //                    }

        //                    ++cursorIndex;
        //                }
        //            }
        //        }
    }
}

ContextMenu* ComicBookTextEdit::createContextMenu(const QPoint& _position, QWidget* _parent)
{
    auto menu = BaseTextEdit::createContextMenu(_position, _parent);

    auto splitAction = new QAction;
    const BusinessLayer::ComicBookTextCursor cursor = textCursor();
    if (cursor.inTable()) {
        splitAction->setText(tr("Merge paragraph"));
        splitAction->setIconText(u8"\U000f10e7");
    } else {
        splitAction->setText(tr("Split paragraph"));
        splitAction->setIconText(u8"\U000f10e7");

        //
        // Запрещаем разделять некоторые блоки
        //
        const auto blockType = ComicBookBlockStyle::forBlock(cursor.block());
        splitAction->setEnabled(blockType != ComicBookParagraphType::Page
                                && blockType != ComicBookParagraphType::Panel
                                && blockType != ComicBookParagraphType::PanelShadow
                                && blockType != ComicBookParagraphType::FolderHeader
                                && blockType != ComicBookParagraphType::FolderFooter);
    }
    connect(splitAction, &QAction::triggered, this, [this] {
        BusinessLayer::ComicBookTextCursor cursor = textCursor();
        if (cursor.inTable()) {
            d->document.mergeParagraph(cursor);
        } else {
            d->document.splitParagraph(cursor);

            //
            // После разделения, возвращаемся в первую ячейку таблицы
            //
            moveCursor(QTextCursor::PreviousBlock);
            moveCursor(QTextCursor::PreviousBlock);
            moveCursor(QTextCursor::PreviousBlock);
            moveCursor(QTextCursor::EndOfBlock);
        }
    });

    auto actions = menu->actions().toVector();
    actions.first()->setSeparator(true);
    actions.prepend(splitAction);
    menu->setActions(actions);

    return menu;
}

bool ComicBookTextEdit::canInsertFromMimeData(const QMimeData* _source) const
{
    return _source->formats().contains(d->model->mimeTypes().first()) || _source->hasText();
}

QMimeData* ComicBookTextEdit::createMimeDataFromSelection() const
{
    if (!textCursor().hasSelection()) {
        return {};
    }

    QMimeData* mimeData = new QMimeData;
    BusinessLayer::ComicBookTextCursor cursor = textCursor();
    const auto selection = cursor.selectionInterval();

    //
    // Сформируем в текстовом виде, для вставки наружу
    // TODO: экспорт в фонтан
    //
    {
        QByteArray text;
        auto cursor = textCursor();
        cursor.setPosition(selection.from);
        do {
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            if (cursor.position() > selection.to) {
                cursor.setPosition(selection.to, QTextCursor::KeepAnchor);
            }
            if (!text.isEmpty()) {
                text.append("\r\n");
            }
            text.append(cursor.blockCharFormat().fontCapitalization() == QFont::AllUppercase
                            ? TextHelper::smartToUpper(cursor.selectedText()).toUtf8()
                            : cursor.selectedText().toUtf8());
        } while (cursor.position() < textCursor().selectionEnd() && !cursor.atEnd()
                 && cursor.movePosition(QTextCursor::NextBlock));

        mimeData->setData("text/plain", text);
    }

    //
    // Поместим в буфер данные о тексте в специальном формате
    //
    {
        mimeData->setData(d->model->mimeTypes().first(),
                          d->document.mimeFromSelection(selection.from, selection.to).toUtf8());
    }

    return mimeData;
}

void ComicBookTextEdit::insertFromMimeData(const QMimeData* _source)
{
    if (isReadOnly()) {
        return;
    }

    //
    // Удаляем выделенный текст
    //
    BusinessLayer::ComicBookTextCursor cursor = textCursor();
    if (cursor.hasSelection()) {
        cursor.removeCharacters(this);
    }

    //
    // Если в моменте входа мы в состоянии редактирования (такое возможно в момент дропа),
    // то запомним это состояние, чтобы восстановить после дропа, а для вставки важно,
    // чтобы режим редактирования был выключен, т.к. данные будут загружаться через модель
    //
    const bool wasInEditBlock = cursor.isInEditBlock();
    if (wasInEditBlock) {
        cursor.endEditBlock();
    }

    //
    // Вставляем сценарий из майм-данных
    //
    QString textToInsert;

    //
    // Если вставляются данные в сценарном формате, то вставляем как положено
    //
    if (_source->formats().contains(d->model->mimeTypes().constFirst())) {
        textToInsert = _source->data(d->model->mimeTypes().constFirst());
    }
    //
    // Если простой текст, то вставляем его, импортировав построчно
    //
    else if (_source->hasText()) {
        BusinessLayer::ComicBookPlainTextImporter plainTextImporter;
        textToInsert = plainTextImporter.importComicBook(_source->text()).text;
    }

    //
    // Собственно вставка данных
    //
    d->document.insertFromMime(textCursor().position(), textToInsert);

    //
    // Восстанавливаем режим редактирования, если нужно
    //
    if (wasInEditBlock) {
        cursor.beginEditBlock();
    }
}

void ComicBookTextEdit::dropEvent(QDropEvent* _event)
{
    //
    // Если в момент вставки было выделение
    //
    if (textCursor().hasSelection()) {
        BusinessLayer::ComicBookTextCursor cursor = textCursor();
        //
        // ... и это перемещение содержимого внутри редактора
        //
        if (_event->source() == this) {
            //
            // ... то удалим выделенный текст
            //
            cursor.removeCharacters(this);
        }
        //
        // ... а если контент заносят снаружи
        //
        else {
            //
            // ... то очистим выделение, чтобы оставить контент
            //
            cursor.clearSelection();
        }
    }

    PageTextEdit::dropEvent(_event);
}

} // namespace Ui
