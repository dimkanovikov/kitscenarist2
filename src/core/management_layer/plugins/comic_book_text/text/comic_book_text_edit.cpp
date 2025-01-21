#include "comic_book_text_edit.h"

#include "handlers/key_press_handler_facade.h"

#include <business_layer/document/comic_book/text/comic_book_text_corrector.h>
#include <business_layer/document/comic_book/text/comic_book_text_document.h>
#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/export/comic_book/comic_book_export_options.h>
#include <business_layer/export/comic_book/comic_book_fountain_exporter.h>
#include <business_layer/import/comic_book/comic_book_fountain_importer.h>
#include <business_layer/model/characters/character_model.h>
#include <business_layer/model/comic_book/comic_book_information_model.h>
#include <business_layer/model/comic_book/text/comic_book_text_block_parser.h>
#include <business_layer/model/comic_book/text/comic_book_text_model.h>
#include <business_layer/model/comic_book/text/comic_book_text_model_page_item.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/comic_book_template.h>
#include <business_layer/templates/templates_facade.h>
#include <domain/document_object.h>
#include <domain/starcloud_api.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/model_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/tools/debouncer.h>

#include <QAction>
#include <QCoreApplication>
#include <QDir>
#include <QLocale>
#include <QMimeData>
#include <QPainter>
#include <QPointer>
#include <QScrollBar>
#include <QTextTable>

using BusinessLayer::TemplatesFacade;
using BusinessLayer::TextBlockStyle;
using BusinessLayer::TextParagraphType;

namespace Ui {

namespace {
const QLatin1String kMarkdownMimeType("text/markdown");
}

class ComicBookTextEdit::Implementation
{
public:
    explicit Implementation(ComicBookTextEdit* _q);

    /**
     * @brief Текущий шаблон документа
     */
    const BusinessLayer::ComicBookTemplate& comicBookTemplate() const;

    /**
     * @brief Отменить/повторить последнее действие
     */
    void revertAction(bool previous);

    /**
     * @brief Обновить редакторскую заметку в заданном интервале
     */
    void updateReviewMark(QKeyEvent* _event, int _from, int _to);


    ComicBookTextEdit* q = nullptr;

    QPointer<BusinessLayer::ComicBookTextModel> model;
    BusinessLayer::ComicBookTextDocument document;

    bool showSceneNumber = false;
    bool showSceneNumberOnLeft = false;
    bool showSceneNumberOnRight = false;
    bool showDialogueNumber = false;

    struct {
        bool isEnabled = false;
        QColor textColor;
        QColor backgroundColor;
        bool isRevision = false;
    } autoReviewMode;

    QVector<Domain::CursorInfo> collaboratorsCursorInfo;
    QVector<Domain::CursorInfo> pendingCollaboratorsCursorInfo;
    Debouncer collaboratorCursorInfoUpdateDebouncer;
};

ComicBookTextEdit::Implementation::Implementation(ComicBookTextEdit* _q)
    : q(_q)
    , collaboratorCursorInfoUpdateDebouncer(500)
{
    connect(&collaboratorCursorInfoUpdateDebouncer, &Debouncer::gotWork, q, [this] {
        std::swap(collaboratorsCursorInfo, pendingCollaboratorsCursorInfo);
        q->update();
    });
}

const BusinessLayer::ComicBookTemplate& ComicBookTextEdit::Implementation::comicBookTemplate() const
{
    const auto currentTemplateId
        = model && model->informationModel() ? model->informationModel()->templateId() : "";
    return TemplatesFacade::comicBookTemplate(currentTemplateId);
}

void ComicBookTextEdit::Implementation::revertAction(bool previous)
{
    if (model == nullptr) {
        return;
    }

    auto finalCursorPosition = q->textCursor().position();

    BusinessLayer::ChangeCursor changeCursor;
    if (previous) {
        changeCursor = model->undo();
    } else {
        changeCursor = model->redo();
    }
    //
    if (changeCursor.item != nullptr) {
        const auto item = static_cast<BusinessLayer::TextModelItem*>(changeCursor.item);
        const auto itemIndex = model->indexForItem(item);
        finalCursorPosition = document.itemEndPosition(itemIndex);
        if (changeCursor.position >= 0) {
            finalCursorPosition += changeCursor.position;
        } else if (item->type() == BusinessLayer::TextModelItemType::Text) {
            const auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
            finalCursorPosition += textItem->text().length();
        }
    }

    auto cursor = q->textCursor();
    cursor.setPosition(finalCursorPosition);
    q->setTextCursorAndKeepScrollBars(cursor);
    q->ensureCursorVisible();

    //
    // При отмене/повторе последнего действия позиция курсора могла и не поменяться,
    // но тип параграфа сменился, поэтому перестраховываемся и говорим будто бы
    // сменилась позиция курсора, чтобы обновить состояние панелей
    //
    emit q->cursorPositionChanged();
}

void ComicBookTextEdit::Implementation::updateReviewMark(QKeyEvent* _event, int _from, int _to)
{
    if (!autoReviewMode.isEnabled) {
        return;
    }

    //
    // Если включён режим автоматического добавления редакторских заметок
    // ... и текст добавляется с клавиатуры и это не шорткат
    // ... или вставляется из буфера обмена
    // ... и позиция курсора изменилась после обработки события
    //
    if ((((_event->modifiers().testFlag(Qt::NoModifier)
           || _event->modifiers().testFlag(Qt::ShiftModifier))
          && !_event->text().isEmpty())
         || _event == QKeySequence::Paste)
        && _from < _to) {
        //
        // То автоматически добавим редакторскую заметку
        //
        const auto lastCursor = q->textCursor();
        auto cursor = lastCursor;
        cursor.setPosition(_from);
        cursor.setPosition(_to, QTextCursor::KeepAnchor);
        q->setTextCursor(cursor);
        q->addReviewMark(autoReviewMode.textColor, autoReviewMode.backgroundColor, {},
                         autoReviewMode.isRevision);
        q->setTextCursor(lastCursor);
    }
}


// ****


ComicBookTextEdit::ComicBookTextEdit(QWidget* _parent)
    : ScriptTextEdit(_parent)
    , d(new Implementation(this))
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    setFrameShape(QFrame::NoFrame);
    setShowPageNumbers(true);
    setShowPageNumberAtFirstPage(false);

    setDocument(&d->document);
    setCapitalizeWords(false);


    connect(document(), &QTextDocument::contentsChange, this,
            [this](int _position, int _charsRemoved, int _charsAdded) {
                auto updateCursors = [_position, _charsRemoved,
                                      _charsAdded](QVector<Domain::CursorInfo>& _cursors) {
                    for (auto& collaboratorCursor : _cursors) {
                        int collaboratorCursorPosition = collaboratorCursor.cursorData.toInt();
                        if (collaboratorCursorPosition >= _position) {
                            collaboratorCursorPosition += _charsAdded - _charsRemoved;
                            collaboratorCursor.cursorData
                                = QByteArray::number(collaboratorCursorPosition);
                        }
                    }
                };
                updateCursors(d->collaboratorsCursorInfo);

                d->collaboratorCursorInfoUpdateDebouncer.abortWork();
            });
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

void ComicBookTextEdit::setCorrectionOptions(bool _needToCorrectCharactersNames,
                                             bool _showDialogueNumber,
                                             bool _needToCorrectPageBreaks)
{
    d->document.setCorrectionOptions(_needToCorrectCharactersNames, _showDialogueNumber,
                                     _needToCorrectPageBreaks);
}

void ComicBookTextEdit::initWithModel(BusinessLayer::ComicBookTextModel* _model)
{
    if (d->model) {
        d->model->disconnect(this);
        if (d->model->informationModel()) {
            d->model->informationModel()->disconnect(this);
        }
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
        const auto currentTemplate = d->comicBookTemplate();
        setPageFormat(currentTemplate.pageSizeId());
        setPageMarginsMm(currentTemplate.pageMargins());
        setPageNumbersAlignment(currentTemplate.pageNumbersAlignment());
        setShowPageNumberAtFirstPage(currentTemplate.isFirstPageNumberVisible());
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

        connect(d->model, &BusinessLayer::ComicBookTextModel::dataChanged, this,
                qOverload<>(&ComicBookTextEdit::update));
        connect(d->model->informationModel(),
                &BusinessLayer::ComicBookInformationModel::headerChanged, this,
                &ComicBookTextEdit::setHeader);
        connect(d->model->informationModel(),
                &BusinessLayer::ComicBookInformationModel::footerChanged, this,
                &ComicBookTextEdit::setFooter);
    }

    emit cursorPositionChanged();
}

void ComicBookTextEdit::reinit()
{
    //
    // Перенастроим всё, что зависит от шаблона
    //
    initWithModel(d->model);

    //
    // Пересчитаем всё, что считается во время выполнения
    //
    if (d->model != nullptr) {
        d->model->updateRuntimeDictionaries();
    }
}

const BusinessLayer::ComicBookTemplate& ComicBookTextEdit::comicBookTemplate() const
{
    return d->comicBookTemplate();
}

BusinessLayer::ComicBookDictionariesModel* ComicBookTextEdit::dictionaries() const
{
    if (d->model == nullptr) {
        return nullptr;
    }

    return d->model->dictionariesModel();
}

QAbstractItemModel* ComicBookTextEdit::characters() const
{
    if (d->model == nullptr) {
        return nullptr;
    }

    return d->model->charactersList();
}

void ComicBookTextEdit::createCharacter(const QString& _name)
{
    if (d->model == nullptr) {
        return;
    }

    d->model->createCharacter(_name);
}

void ComicBookTextEdit::undo()
{
    d->revertAction(true);
}

void ComicBookTextEdit::redo()
{
    d->revertAction(false);
}

void ComicBookTextEdit::addParagraph(BusinessLayer::TextParagraphType _type)
{
    QString mimeDataToMove;

    //
    // Выводим курсор за пределы таблицы, чтобы вставка происходила за её пределами и не создавались
    // многоуровневые таблицы
    //
    if (BusinessLayer::TextCursor cursor = textCursor(); cursor.inTable()) {
        //
        // Курсор обязательно должен быть во второй колонке
        //
        Q_ASSERT(!cursor.inFirstColumn());

        //
        // Если до конца блока есть текст вырезаем его
        //
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        if (cursor.hasSelection()) {
            mimeDataToMove = d->document.mimeFromSelection(cursor.selectionInterval().from,
                                                           cursor.selectionInterval().to);
            cursor.removeSelectedText();
        }

        //
        // Выходим из таблицы
        //
        cursor.movePosition(QTextCursor::NextBlock);
        Q_ASSERT(!cursor.inTable());

        setTextCursorForced(cursor);
    }

    //
    // Если изменяется заголовок изолированного элемента, то снимаем изоляцию на время
    // операции, а после изолируем предшествующий текущему элемент, либо его родителя
    //
    const QSet<TextParagraphType> headerTypes = {
        TextParagraphType::PageHeading,     TextParagraphType::PanelHeading,
        TextParagraphType::SequenceHeading, TextParagraphType::SequenceFooter,
        TextParagraphType::ActHeading,      TextParagraphType::ActFooter,
    };

    const auto currentTypeIsHeader = headerTypes.contains(currentParagraphType());
    const auto targetTypeIsHeader = headerTypes.contains(_type);
    const auto needReisolate = (currentTypeIsHeader || targetTypeIsHeader)
        && d->document.visibleTopLeveLItem().isValid();
    if (needReisolate) {
        d->document.setVisibleTopLevelItem({});
    }

    //
    // Вставляем параграф на уровне модели
    //
    d->document.addParagraph(_type, textCursor());

    //
    // ... при необходимости восстанавливаем режим изоляции
    //
    if (needReisolate) {
        d->document.setVisibleTopLevelItem(d->document.itemIndex(textCursor().block()));
    }

    //
    // Если диалоги нужно размещать в таблице, переметим добавляемый диалог в таблицу
    //
    if (d->comicBookTemplate().placeDialoguesInTable()) {
        //
        // Если вставляется персонаж, то разделяем страницу, для добавления реплики
        //
        if (_type == BusinessLayer::TextParagraphType::Character) {
            const auto cursorPosition = textCursor().position();
            d->document.splitParagraph(textCursor());
            auto cursor = textCursor();
            cursor.setPosition(cursorPosition + 1); // +1 чтобы войти внутрь таблицы
            setTextCursor(cursor);
            cursor.movePosition(BusinessLayer::TextCursor::NextBlock);
            d->document.setParagraphType(BusinessLayer::TextParagraphType::Dialogue, cursor);
        }
        //
        // Если вставляется ремарка, или реплика, то разделяем страницу и ставим курсор во вторую
        // колонку
        //
        else if (_type == BusinessLayer::TextParagraphType::Parenthetical
                 || _type == BusinessLayer::TextParagraphType::Dialogue) {
            const auto cursorPosition = textCursor().position();
            d->document.splitParagraph(textCursor());
            auto cursor = textCursor();
            cursor.setPosition(cursorPosition + 1); // +1 чтобы войти внутрь таблицы
            setTextCursor(cursor);
            d->document.setParagraphType(BusinessLayer::TextParagraphType::Character, cursor);
            cursor.movePosition(BusinessLayer::TextCursor::NextBlock);
            d->document.setParagraphType(BusinessLayer::TextParagraphType::Dialogue, cursor);
            setTextCursor(cursor);
        }
    }

    //
    // Вставляем вырезанные данные
    //
    if (!mimeDataToMove.isEmpty()) {
        d->document.insertFromMime(textCursor().position(), mimeDataToMove);
    }

    emit paragraphTypeChanged();
}

void ComicBookTextEdit::setCurrentParagraphType(TextParagraphType _type)
{
    if (currentParagraphType() == _type) {
        return;
    }

    BusinessLayer::TextCursor cursor = textCursor();

    //
    // Если изменяется заголовок изолированного элемента, то снимаем изоляцию на время
    // операции, а после изолируем предшествующий текущему элемент, либо его родителя
    //
    const QSet<TextParagraphType> headerTypes = {
        TextParagraphType::PageHeading,     TextParagraphType::PanelHeading,
        TextParagraphType::SequenceHeading, TextParagraphType::SequenceFooter,
        TextParagraphType::ActHeading,      TextParagraphType::ActFooter,
    };

    const auto currentTypeIsHeader = headerTypes.contains(currentParagraphType());
    const auto targetTypeIsHeader = headerTypes.contains(_type);
    const auto needReisolate = (currentTypeIsHeader || targetTypeIsHeader)
        && d->document.visibleTopLeveLItem().isValid();
    if (needReisolate) {
        d->document.setVisibleTopLevelItem({});
    }

    //
    // Меняем тип блока на персонажа
    //
    if (d->comicBookTemplate().placeDialoguesInTable() && _type == TextParagraphType::Character) {
        //
        // Если текущий блок не в таблице, то создаём её и текущий блок помещаем в неё как персонажа
        //
        if (!cursor.inTable()) {
            d->document.setParagraphType(_type, cursor);
            const auto cursorPosition = cursor.position();
            d->document.splitParagraph(cursor);
            auto otherCursor = textCursor();
            otherCursor.setPosition(cursorPosition + 1); // +1 чтобы войти внутрь таблицы
            setTextCursor(otherCursor);
            otherCursor.movePosition(QTextCursor::NextBlock);
            d->document.setParagraphType(BusinessLayer::TextParagraphType::Dialogue, otherCursor);
        }
        //
        // Если текущий блок в таблице, то ничего не делаем
        //
        else {
        }
    }
    //
    // На реплику
    //
    else if (d->comicBookTemplate().placeDialoguesInTable()
             && (_type == TextParagraphType::Parenthetical
                 || _type == TextParagraphType::Dialogue)) {
        //
        // Если текущий блок не в таблице, то создаём её и текущий блок помещаем в неё как персонажа
        //
        if (!cursor.inTable()) {
            d->document.setParagraphType(_type, cursor);
            cursor.movePosition(QTextCursor::StartOfBlock);
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            const auto blockMimeData = d->document.mimeFromSelection(
                cursor.selectionInterval().from, cursor.selectionInterval().to);
            cursor.removeSelectedText();

            const auto cursorPosition = cursor.position();
            d->document.splitParagraph(cursor);
            auto otherCursor = textCursor();
            otherCursor.setPosition(cursorPosition + 1); // +1 чтобы войти внутрь таблицы
            d->document.setParagraphType(BusinessLayer::TextParagraphType::Character, otherCursor);
            otherCursor.movePosition(QTextCursor::NextBlock);
            d->document.insertFromMime(otherCursor.position(), blockMimeData);
            setTextCursor(otherCursor);
        }
        //
        //
        //
        else {
            return;
        }
    }
    //
    // На любой другой
    //
    else {
        //
        // ... в таблице
        //
        if (cursor.inTable()) {
            //
            // ... если таблица пуста, то удаляем таблицу и применяем оставшемуся блоку новый тип
            //
            const bool skipCurrentBlockEmptynessCheck = true;
            if (cursor.isTableEmpty(skipCurrentBlockEmptynessCheck)) {
                d->document.mergeParagraph(cursor);
                d->document.setParagraphType(_type, cursor);
            }
            //
            // ... если таблица не пуста, ничего не делаем
            //
            else {
            }
        }
        //
        // ... за пределами таблицы - устанавливаем заданный тип
        //
        else {
            d->document.setParagraphType(_type, cursor);
        }
    }

    //
    // ... при необходимости восстанавливаем режим изоляции
    //
    if (needReisolate) {
        d->document.setVisibleTopLevelItem(d->document.itemIndex(textCursor().block()));
    }

    //
    // Если вставили папку, то нужно перейти к предыдущему блоку (из футера к хидеру)
    //
    if (_type == TextParagraphType::ActHeading || _type == TextParagraphType::SequenceHeading) {
        moveCursor(QTextCursor::PreviousBlock);
    }

    emit paragraphTypeChanged();
}

BusinessLayer::TextParagraphType ComicBookTextEdit::currentParagraphType() const
{
    return TextBlockStyle::forBlock(textCursor().block());
}

QModelIndex ComicBookTextEdit::currentModelIndex() const
{
    if (d->model == nullptr || d->document.isEditTransactionActive()) {
        return {};
    }

    auto userData = textCursor().block().userData();
    if (userData == nullptr) {
        return {};
    }

    auto comicBookBlockData = static_cast<BusinessLayer::TextBlockData*>(userData);
    return d->model->indexForItem(comicBookBlockData->item());
}

void ComicBookTextEdit::setCurrentModelIndex(const QModelIndex& _index)
{
    if (!_index.isValid() || _index.model() != d->model || _index == currentModelIndex()) {
        return;
    }

    const auto cursorPosition = d->document.itemStartPosition(_index);
    if (cursorPosition == -1) {
        return;
    }

    BusinessLayer::TextCursor textCursor(document());
    textCursor.setPosition(cursorPosition);
    ensureCursorVisible(textCursor);
}

void ComicBookTextEdit::setVisibleTopLevelItemIndex(const QModelIndex& _index)
{
    d->document.setVisibleTopLevelItem(_index);
}

int ComicBookTextEdit::positionForModelIndex(const QModelIndex& _index)
{
    return d->document.itemStartPosition(_index);
}

void ComicBookTextEdit::addReviewMark(const QColor& _textColor, const QColor& _backgroundColor,
                                      const QString& _comment, bool _isRevision)
{
    BusinessLayer::TextCursor cursor(textCursor());
    if (!cursor.hasSelection()) {
        return;
    }

    d->document.addReviewMark(_textColor, _backgroundColor, _comment, _isRevision, cursor);
}

void ComicBookTextEdit::setAutoReviewModeEnabled(bool _enabled)
{
    d->autoReviewMode.isEnabled = _enabled;
}

void ComicBookTextEdit::setAutoReviewMode(const QColor& _textColor, const QColor& _backgroundColor,
                                          bool _isRevision)
{
    d->autoReviewMode.textColor
        = _textColor.isValid() ? _textColor : ColorHelper::contrasted(_backgroundColor);
    d->autoReviewMode.backgroundColor = _backgroundColor;
    d->autoReviewMode.isRevision = _isRevision;
}

void ComicBookTextEdit::setCursors(const QVector<Domain::CursorInfo>& _cursors)
{
    d->pendingCollaboratorsCursorInfo = _cursors;
    d->collaboratorCursorInfoUpdateDebouncer.orderWork();
}

void ComicBookTextEdit::keyPressEvent(QKeyEvent* _event)
{
    if (isReadOnly()) {
        ScriptTextEdit::keyPressEvent(_event);
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
        const int positionBeforeEventHandling = textCursor().position();
        if (keyPressEventReimpl(_event)) {
            _event->accept();
        } else {
            ScriptTextEdit::keyPressEvent(_event);
            _event->ignore();
        }
        const int positionAfterEventHandling = textCursor().position();

        updateEnteredText(_event);
        d->updateReviewMark(_event, positionBeforeEventHandling, positionAfterEventHandling);
    }

    //
    // Обработка
    //
    if (!_event->isAccepted()) {
        handler->handle(_event);
        updateTypewriterScroll();
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
    // ... вставить текст
    //
    else if (_event == QKeySequence::Paste) {
        paste();
        d->model->saveChanges();
    }
    //
    // Обрабатываем в базовом классе
    //
    else {
        isEventHandled = ScriptTextEdit::keyPressEventReimpl(_event);
        if (_event == QKeySequence::Cut) {
            d->model->saveChanges();
        }
    }

    return isEventHandled;
}

void ComicBookTextEdit::paintEvent(QPaintEvent* _event)
{
    ScriptTextEdit::paintEvent(_event);

    //
    // Определить область прорисовки по краям от текста
    //
    const bool isLeftToRight = QLocale().textDirection() == Qt::LeftToRight;
    const qreal pageLeft = 0;
    const qreal pageRight
        = viewport()->width() - verticalScrollBar()->width() - DesignSystem::layout().px8();
    const qreal spaceBetweenSceneNumberAndText = DesignSystem::layout().px(10);
    const qreal textLeft = pageLeft - (isLeftToRight ? 0 : horizontalScrollBar()->maximum())
        + document()->rootFrame()->frameFormat().leftMargin() - spaceBetweenSceneNumberAndText;
    const qreal textRight = pageRight + (isLeftToRight ? horizontalScrollBar()->maximum() : 0)
        - document()->rootFrame()->frameFormat().rightMargin() + spaceBetweenSceneNumberAndText;
    const qreal leftDelta = (isLeftToRight ? -1 : 1) * horizontalScrollBar()->value();
    //    int colorRectWidth = 0;
    qreal verticalMargin = 0;
    const auto currentTemplate = d->comicBookTemplate();
    const qreal splitterX = leftDelta + textLeft
        + (textRight - textLeft) * currentTemplate.leftHalfOfPageWidthPercents() / 100;


    //
    // Определим начальный блок на экране
    //
    QTextBlock topBlock = document()->lastBlock();
    {
        const auto topCursor = cursorForPositionReimpl(viewport()->mapFromParent(QPoint(0, 0)));
        if (topBlock.blockNumber() > topCursor.block().blockNumber()) {
            topBlock = topCursor.block();
        }
    }
    //
    // ... идём до начала сцены
    //
    while (TextBlockStyle::forBlock(topBlock) != TextParagraphType::PageHeading
           && TextBlockStyle::forBlock(topBlock) != TextParagraphType::PanelHeading
           && TextBlockStyle::forBlock(topBlock) != TextParagraphType::SequenceHeading
           && topBlock != document()->firstBlock()) {
        topBlock = topBlock.previous();
    }

    //
    // Определим последний блок на экране
    //
    QTextBlock bottomBlock = document()->firstBlock();
    {
        const auto bottomCursor
            = cursorForPositionReimpl(viewport()->mapFromParent(QPoint(0, viewport()->height())));
        if (bottomBlock.blockNumber() < bottomCursor.block().blockNumber()) {
            bottomBlock = bottomCursor.block();
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
        BusinessLayer::TextCursor bottomCursor(document());
        bottomCursor.setPosition(bottomBlock.position());
        while (bottomCursor.inTable() && bottomCursor.movePosition(QTextCursor::NextBlock)) {
            bottomBlock = bottomCursor.block();
        }
    }

    //
    // Прорисовка дополнительных элементов редактора
    //
    QPainter painter(viewport());

    //
    // Декорации текста
    //
    {
        clipPageDecorationRegions(&painter);

        //
        // Проходим блоки на экране и декорируем их
        //
        QTextBlock block = topBlock;
        const QRectF viewportGeometry = viewport()->geometry();
        int previousSceneBlockBottom = 0;
        int lastSceneBlockBottom = 0;
        QVector<QColor> lastSceneColors;
        bool isLastBlockSceneHeadingWithNumberAtRight = false;
        int lastCharacterBlockBottom = 0;
        QColor lastCharacterColor;

        auto setPainterPen = [&painter, &block, this](const QColor& _color) {
            painter.setPen(ColorHelper::transparent(
                _color,
                1.0
                    - (isFocusCurrentParagraph() && block != textCursor().block()
                           ? Ui::DesignSystem::inactiveTextOpacity()
                           : 0.0)));
        };

        BusinessLayer::TextCursor cursor(document());
        while (block.isValid() && block != bottomBlock) {
            //
            // Стиль текущего блока
            //
            const auto blockType = TextBlockStyle::forBlock(block);

            cursor.setPosition(block.position());
            const QRect cursorR = cursorRect(cursor);
            cursor.movePosition(QTextCursor::EndOfBlock);
            const QRect cursorREnd = cursorRect(cursor);
            //
            verticalMargin = cursorR.height() / 2;

            //
            // Определим цвет сцены
            //
            switch (blockType) {
            case TextParagraphType::PageHeading:
            case TextParagraphType::PanelHeading:
            case TextParagraphType::SequenceHeading:
            case TextParagraphType::ActHeading:
            case TextParagraphType::SequenceFooter:
            case TextParagraphType::ActFooter: {
                previousSceneBlockBottom = lastSceneBlockBottom;
                lastSceneBlockBottom = cursorR.top();
                lastSceneColors = d->document.itemColors(block);
                break;
            }
            default: {
                break;
            }
            }

            //
            // Нарисуем цвета
            //
            if (!lastSceneColors.isEmpty()) {
                const QPointF topLeft(isLeftToRight
                                          ? pageRight + leftDelta - DesignSystem::layout().px4()
                                          : pageLeft + leftDelta,
                                      lastSceneBlockBottom - verticalMargin);
                const QPointF bottomRight(isLeftToRight
                                              ? pageRight + leftDelta
                                              : pageLeft + leftDelta + DesignSystem::layout().px4(),
                                          cursorREnd.bottom() + verticalMargin);
                QRectF rect(topLeft, bottomRight);
                for (const auto& color : std::as_const(lastSceneColors)) {
                    if (!color.isValid()) {
                        continue;
                    }

                    auto colorRect = rect;
                    if (color != lastSceneColors.constLast()) {
                        colorRect.setTop(previousSceneBlockBottom);
                    }

                    painter.setPen(Qt::NoPen);
                    painter.setBrush(color);
                    painter.drawRect(colorRect);
                    rect.moveLeft(rect.left() - DesignSystem::layout().px12());
                }
            }

            //
            // Определим цвет персонажа
            //
            if (blockType == TextParagraphType::Character && d->model
                && d->model->charactersList() != nullptr) {
                lastCharacterColor = QColor();
                const QString characterName
                    = BusinessLayer::ComicBookCharacterParser::name(block.text());
                if (auto character = d->model->character(characterName)) {
                    if (character->color().isValid()) {
                        lastCharacterColor = character->color();
                    }
                }
            } else if (blockType != TextParagraphType::Parenthetical
                       && blockType != TextParagraphType::Dialogue
                       && blockType != TextParagraphType::Lyrics) {
                lastCharacterColor = QColor();
            }

            //
            // Нарисуем цвет персонажа
            //
            if (lastCharacterColor.isValid()) {
                const auto isBlockCharacterWithNumber
                    = blockType == TextParagraphType::Character && d->showDialogueNumber;
                if (!isBlockCharacterWithNumber) {
                    QPointF topLeft(isLeftToRight
                                        ? textLeft + leftDelta - spaceBetweenSceneNumberAndText
                                            + DesignSystem::layout().px4()
                                        : textRight + leftDelta + spaceBetweenSceneNumberAndText,
                                    cursorR.top());
                    const QPointF bottomRight(topLeft.x() + DesignSystem::layout().px4(),
                                              cursorREnd.bottom());
                    const QRectF rect(topLeft, bottomRight);
                    painter.fillRect(rect, lastCharacterColor);
                }
            }

            //
            // Курсор на экране
            //
            // ... ниже верхней границы
            if ((cursorR.top() > 0 || cursorR.bottom() > 0)
                // ... и выше нижней
                && cursorR.top() < viewportGeometry.bottom()) {
                //
                // Прорисовка закладок
                //
                const auto bookmark = d->document.bookmark(block);
                if (bookmark.isValid()) {
                    setPainterPen(bookmark.color);
                    painter.setFont(DesignSystem::font().iconsForEditors());

                    //
                    // Определим область для отрисовки
                    //
                    QPointF topLeft(isLeftToRight
                                        ? (pageLeft + leftDelta
                                           + Ui::DesignSystem::card().shadowMargins().left())
                                        : (textRight + leftDelta),
                                    cursorR.top());
                    QPointF bottomRight(isLeftToRight
                                            ? textLeft + leftDelta
                                            : (pageRight + leftDelta
                                               - Ui::DesignSystem::card().shadowMargins().right()),
                                        cursorR.bottom());
                    QRectF rect(topLeft, bottomRight);
                    const auto yDelta = Ui::DesignSystem::layout().px(32) - rect.height() / 2.0;
                    //
                    // корректируем размер области, чтобы получить квадрат для отрисовки иконки
                    // закладки
                    //
                    if (yDelta > 0) {
                        rect.adjust(0, -yDelta, 0, yDelta);
                    }
                    if (isLeftToRight) {
                        rect.setWidth(rect.height());
                    } else {
                        rect.setLeft(rect.right() - rect.height());
                    }
                    painter.fillRect(rect,
                                     ColorHelper::transparent(
                                         bookmark.color, Ui::DesignSystem::elevationEndOpacity()));
                    painter.drawText(rect, Qt::AlignCenter, u8"\U000F00C0");
                }

                //
                // Прорисовка декораций пустой строки
                //
                if (!block.blockFormat().boolProperty(TextBlockStyle::PropertyIsCorrection)
                    && blockType != TextParagraphType::PageSplitter
                    && block.text().simplified().isEmpty()) {
                    //
                    // Настроим цвет
                    //
                    setPainterPen(ColorHelper::transparent(
                        palette().text().color(), Ui::DesignSystem::inactiveItemOpacity()));

                    //
                    // Для пустого футера рисуем плейсхолдер
                    //
                    if (blockType == TextParagraphType::SequenceFooter) {
                        painter.setFont(block.charFormat().font());

                        //
                        // Ищем открывающий блок папки
                        //
                        auto headerBlock = block.previous();
                        int openedFolders = 0;
                        while (headerBlock.isValid()) {
                            const auto headerBlockType = TextBlockStyle::forBlock(headerBlock);
                            if (headerBlockType == TextParagraphType::SequenceHeading) {
                                if (openedFolders > 0) {
                                    --openedFolders;
                                } else {
                                    break;
                                }
                            } else if (headerBlockType == TextParagraphType::SequenceFooter) {
                                ++openedFolders;
                            }

                            headerBlock = headerBlock.previous();
                        }

                        //
                        // Определим область для отрисовки плейсхолдера
                        //
                        const auto placeholderText = QString("%1 %2").arg(
                            QCoreApplication::translate("KeyProcessingLayer::FolderFooterHandler",
                                                        "End of"),
                            headerBlock.text());
                        const QPoint topLeft = QPoint(
                            textLeft + leftDelta + spaceBetweenSceneNumberAndText, cursorR.top());
                        const QPoint bottomRight
                            = QPoint(textRight + leftDelta - spaceBetweenSceneNumberAndText,
                                     cursorR.bottom());
                        const QRect rect(topLeft, bottomRight);
                        painter.drawText(rect, block.blockFormat().alignment(), placeholderText);
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
                            painter.drawText(rect, Qt::AlignRight | Qt::AlignTop, emptyLineMark);
                        }
                        //
                        // ... во второй колонке таблички
                        //
                        else {
                            const qreal x = splitterX - cursor.currentTable()->format().border();
                            const QPointF topLeft(
                                x - TextHelper::fineTextWidthF(emptyLineMark, painter.font()),
                                cursorR.top());
                            const QPointF bottomRight(x, cursorR.bottom() + 2);
                            const QRectF rect(topLeft, bottomRight);
                            painter.drawText(rect, Qt::AlignRight | Qt::AlignTop, emptyLineMark);
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
                    if (blockType == TextParagraphType::PageHeading) {
                        //
                        // Прорисовка количества панелей
                        //
                        int panelsCountWidth = 0;
                        if (const auto blockData
                            = static_cast<BusinessLayer::TextBlockData*>(block.userData());
                            blockData != nullptr && blockData->item() != nullptr
                            && blockData->item()->parent() != nullptr) {
                            const auto itemParent = blockData->item()->parent();
                            if (itemParent->type() == BusinessLayer::TextModelItemType::Group
                                && static_cast<BusinessLayer::TextModelGroupItem*>(itemParent)
                                        ->groupType()
                                    == BusinessLayer::TextGroupType::Page) {
                                const auto pageItem
                                    = static_cast<const BusinessLayer::ComicBookTextModelPageItem*>(
                                        itemParent);

                                auto font = block.charFormat().font();
                                font.setCapitalization(QFont::MixedCase);
                                painter.setFont(
                                    currentTemplate
                                        .paragraphStyle(
                                            BusinessLayer::TextParagraphType::Description)
                                        .font());
                                const auto panelsCountText = QString(" (%1)").arg(
                                    tr("%n panels", "", pageItem->panelsCount()));
                                panelsCountWidth
                                    = TextHelper::fineTextWidthF(panelsCountText, painter.font());
                                const QPoint topLeft = isLeftToRight
                                    ? cursorREnd.topLeft()
                                    : cursorREnd.topRight() - QPoint(panelsCountWidth, 0);
                                const QPoint bottomRight = isLeftToRight
                                    ? cursorREnd.bottomRight() + QPoint(panelsCountWidth, 0)
                                    : cursorREnd.bottomLeft();
                                const QRect rect(topLeft, bottomRight);
                                setPainterPen(palette().text().color());
                                painter.drawText(rect, Qt::AlignRight | Qt::AlignVCenter,
                                                 panelsCountText);
                            }
                        }

                        //
                        // Иконка положения страницы
                        //
                        {
                            painter.setFont(DesignSystem::font().iconsForEditors());
                            setPainterPen(palette().text().color());

                            auto paintLeftPageIcon = [&](const QString& _icon) {
                                QPointF topLeft(isLeftToRight
                                                    ? pageLeft + leftDelta
                                                    : textRight + panelsCountWidth + leftDelta,
                                                cursorR.top());
                                QPointF bottomRight(isLeftToRight ? textLeft + leftDelta
                                                                  : pageRight + leftDelta,
                                                    cursorR.bottom());
                                QRectF rect(topLeft, bottomRight);
                                const auto yDelta
                                    = (TextHelper::fineLineSpacing(cursor.charFormat().font())
                                       - TextHelper::fineLineSpacing(
                                           DesignSystem::font().iconsForEditors()))
                                    / 2;
                                rect.adjust(
                                    0, yDelta,
                                    -TextHelper::fineTextWidthF(".", cursor.charFormat().font())
                                        / 2,
                                    0);
                                painter.drawText(rect, Qt::AlignRight | Qt::AlignTop, _icon);
                            };
                            auto paintRightPageIcon = [&](const QString& _icon) {
                                const int spaceWidth
                                    = TextHelper::fineTextWidthF(" ", painter.font());
                                const QPoint topLeft(isLeftToRight
                                                         ? cursorREnd.left() + panelsCountWidth
                                                             + spaceWidth
                                                         : cursorREnd.right() - spaceWidth,
                                                     cursorREnd.top());
                                const QPoint bottomRight(isLeftToRight ? pageRight - leftDelta
                                                                       : 0 - leftDelta,
                                                         cursorREnd.bottom());
                                QRectF rect(topLeft, bottomRight);
                                const auto yDelta
                                    = (TextHelper::fineLineSpacing(cursor.charFormat().font())
                                       - TextHelper::fineLineSpacing(
                                           DesignSystem::font().iconsForEditors()))
                                    / 2;
                                rect.adjust(
                                    0, yDelta,
                                    -TextHelper::fineTextWidthF(".", cursor.charFormat().font())
                                        / 2,
                                    0);
                                painter.drawText(rect, Qt::AlignLeft | Qt::AlignTop, _icon);
                            };

                            const auto pageNumberText = d->document.pageNumber(block);
                            const auto leftPageIcon = u8"\U000F0BB3";
                            const auto rightPageIcon = u8"\U000F0BB1";

                            //
                            // Два номера
                            //
                            if (pageNumberText.contains('-')) {
                                auto numbers = pageNumberText.split('-');
                                const int firstNumber = numbers.constFirst().toInt();
                                if (firstNumber % 2 == 1) {
                                    setPainterPen(DesignSystem::color().error());
                                    paintLeftPageIcon(rightPageIcon);
                                    paintRightPageIcon(leftPageIcon);
                                    setPainterPen(palette().text().color());
                                } else {
                                    paintLeftPageIcon(leftPageIcon);
                                    paintRightPageIcon(rightPageIcon);
                                }
                            } else {
                                const auto pageNumber = pageNumberText.toInt();
                                //
                                // Правая страница
                                //
                                if (pageNumber % 2 == 1) {
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
                    //
                    // Прорисовка ревизий
                    //
                    {
                        //
                        // Собираем ревизии для отображения
                        //
                        QVector<QPair<QRectF, QColor>> revisionMarks;
                        for (const auto& format : block.textFormats()) {
                            if (const auto revision
                                = format.format.property(TextBlockStyle::PropertyCommentsIsRevision)
                                      .toStringList();
                                !revision.isEmpty() && revision.constFirst() == "true") {
                                auto revisionCursor = cursor;
                                revisionCursor.setPosition(revisionCursor.block().position()
                                                           + format.start);
                                do {
                                    if (revisionCursor.positionInBlock() != format.start) {
                                        revisionCursor.movePosition(QTextCursor::NextCharacter);
                                    }
                                    const auto revisionCursorR = cursorRect(revisionCursor);
                                    QPointF topLeft(isLeftToRight ? (textRight + leftDelta)
                                                                  : (pageLeft - leftDelta),
                                                    revisionCursorR.top());
                                    QPointF bottomRight(isLeftToRight ? pageRight
                                                                      : textLeft - leftDelta,
                                                        revisionCursorR.bottom());
                                    const QRectF rect(topLeft, bottomRight);
                                    const auto revisionColor = format.format.foreground().color();
                                    //
                                    // ... первая звёздочка, или звёздочка на следующей строке
                                    //
                                    if (revisionMarks.isEmpty()
                                        || revisionMarks.constLast().first != rect) {
                                        revisionMarks.append({ rect, revisionColor });
                                    }
                                    //
                                    // ... звёздочка на той же строке - проверяем уровень
                                    //
                                    else if (ColorHelper::revisionLevel(
                                                 revisionMarks.constLast().second)
                                             < ColorHelper::revisionLevel(revisionColor)) {
                                        revisionMarks.last().second = revisionColor;
                                    }
                                    if (!revisionCursor.movePosition(QTextCursor::EndOfLine)) {
                                        revisionCursor.movePosition(QTextCursor::EndOfBlock);
                                    }
                                } while (revisionCursor.positionInBlock()
                                             < (format.start + format.length)
                                         && !revisionCursor.atBlockEnd());
                            }
                        }

                        painter.setFont(cursor.charFormat().font());
                        for (const auto& reviewMark : std::as_const(revisionMarks)) {
                            setPainterPen(reviewMark.second);
                            painter.drawText(reviewMark.first, Qt::AlignHCenter | Qt::AlignTop,
                                             "*");
                        }
                    }
                }
            }

            block = block.next();
        }
    }

    //
    // Курсоры соавторов
    //
    painter.setClipRect(QRectF(), Qt::NoClip);
    if (!d->collaboratorsCursorInfo.isEmpty()) {
        for (const auto& cursorInfo : std::as_const(d->collaboratorsCursorInfo)) {
            //
            // Пропускаем курсоры из других документов
            //
            if (cursorInfo.documentUuid != d->model->document()->uuid()) {
                continue;
            }

            //
            // Пропускаем курсоры, которые находятся за пределами экрана
            //
            const auto cursorPosition = cursorInfo.cursorData.toInt();
            if (bottomBlock.isValid()
                && (cursorPosition < topBlock.position()
                    || cursorPosition > (bottomBlock.position() + bottomBlock.length()))) {
                continue;
            }


            QTextCursor cursor(document());
            cursor.setPosition(cursorPosition);
            if (!cursor.block().isVisible()) {
                continue;
            }

            const auto cursorR = cursorRect(cursor).adjusted(0, 0, 1, 0);
            const auto backgroundColor = ColorHelper::forText(cursorInfo.name);

            //
            // ... рисуем его
            //
            painter.fillRect(cursorR, backgroundColor);

            //
            // ... выводим имя соавтора
            //
            painter.setFont(DesignSystem::font().subtitle2());
            const QRect usernameRect(cursorR.left() - Ui::DesignSystem::layout().px4(),
                                     cursorR.top() - Ui::DesignSystem::layout().px24(),
                                     TextHelper::fineTextWidth(cursorInfo.name, painter.font())
                                         + Ui::DesignSystem::layout().px12(),
                                     Ui::DesignSystem::layout().px24());
            painter.setPen(Qt::NoPen);
            painter.setBrush(backgroundColor);
            painter.drawRoundedRect(usernameRect, Ui::DesignSystem::button().borderRadius(),
                                    Ui::DesignSystem::button().borderRadius());
            painter.setPen(ColorHelper::contrasted(backgroundColor));
            painter.drawText(usernameRect, Qt::AlignCenter, cursorInfo.name);
        }
    }
}

ContextMenu* ComicBookTextEdit::createContextMenu(const QPoint& _position, QWidget* _parent)
{
    //
    // Сначала нужно создать контекстное меню в базовом классе, т.к. в этот момент может
    // измениться курсор, который установлен в текстовом редакторе, и использовать его
    //
    auto menu = ScriptTextEdit::createContextMenu(_position, _parent);
    if (isReadOnly() || hasSpellingMenu(_position)) {
        return menu;
    }

    const BusinessLayer::TextCursor cursor = textCursor();

    //
    // Работа с закладками
    //
    auto bookmarkAction = new QAction(this);
    bookmarkAction->setText(tr("Bookmark"));
    bookmarkAction->setIconText(u8"\U000F00C3");
    if (!d->document.bookmark(cursor.block()).isValid()) {
        auto createBookmark = new QAction(bookmarkAction);
        createBookmark->setText(tr("Add"));
        createBookmark->setIconText(u8"\U000F00C4");
        connect(createBookmark, &QAction::triggered, this,
                &ComicBookTextEdit::addBookmarkRequested);
    } else {
        auto editBookmark = new QAction(bookmarkAction);
        editBookmark->setText(tr("Edit"));
        editBookmark->setIconText(u8"\U000F03EB");
        connect(editBookmark, &QAction::triggered, this, &ComicBookTextEdit::editBookmarkRequested);
        //
        auto removeBookmark = new QAction(bookmarkAction);
        removeBookmark->setText(tr("Remove"));
        removeBookmark->setIconText(u8"\U000F01B4");
        connect(removeBookmark, &QAction::triggered, this,
                &ComicBookTextEdit::removeBookmarkRequested);
    }
    //
    auto showBookmarks = new QAction(bookmarkAction);
    showBookmarks->setText(tr("Show/hide list"));
    showBookmarks->setIconText(u8"\U000F0E16");
    connect(showBookmarks, &QAction::triggered, this, &ComicBookTextEdit::showBookmarksRequested);

    auto actions = menu->actions().toVector();
    actions.first()->setSeparator(true);
    actions.prepend(bookmarkAction);
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
    BusinessLayer::TextCursor cursor = textCursor();
    const auto selection = cursor.selectionInterval();

    //
    // Сформируем в текстовом виде, для вставки наружу
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
    // Добавим фонтан
    //
    {
        //
        // Подготавливаем опции для экспорта в фонтан
        //
        BusinessLayer::ComicBookExportOptions options;
        options.filePath = QDir::temp().absoluteFilePath("clipboard.fountain");
        options.includeTitlePage = false;
        options.includeSynopsis = false;
        //        options.showScenesNumbers = d->model->informationModel()->showSceneNumbers();
        //
        // ... сохраняем в формате фонтана
        //
        BusinessLayer::ComicBookFountainExporter().exportTo(d->model, selection.from, selection.to,
                                                            options);
        //
        // ... читаем сохранённый экспорт из файла
        //
        QFile file(options.filePath);
        QByteArray text;
        if (file.open(QIODevice::ReadOnly)) {
            text = file.readAll();
            file.close();
        }

        if (!text.isEmpty()) {
            mimeData->setData(kMarkdownMimeType, text);
        }
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
    using namespace BusinessLayer;

    if (isReadOnly()) {
        return;
    }

    //
    // Удаляем выделенный текст
    //
    TextCursor cursor = textCursor();
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
    // Если вставляются данные в сценарном формате
    //
    if (_source->formats().contains(d->model->mimeTypes().constFirst())) {
        textToInsert = _source->data(d->model->mimeTypes().constFirst());
    }
    //
    // Если простой текст
    //
    else if (_source->hasFormat(kMarkdownMimeType) || _source->hasText()) {
        const auto text = _source->hasFormat(kMarkdownMimeType) ? _source->data(kMarkdownMimeType)
                                                                : _source->text();

        //
        // ... если строк несколько, то вставляем его, импортировав с фонтана
        // NOTE: Перед текстом нужно обязательно добавить перенос строки, чтобы он
        //       не воспринимался как титульная страница
        //
        ComicBookFountainImporter fountainImporter;
        textToInsert = fountainImporter.comicbookText("\n" + text).text;
    }

    //
    // Если курсор установлен в таблице
    //
    if (cursor.inTable()) {
        const auto mimeInfo = ModelHelper::isMimeHasJustOneBlock(textToInsert);
        const auto isMimeContainsJustOneBlock = mimeInfo.first;
        const auto isMimeContainsFolderOrSequence = mimeInfo.second;
        //
        // ... если вставляется несколько блоков
        //
        if (!isMimeContainsJustOneBlock || isMimeContainsFolderOrSequence) {
            bool isTableEmpty = true;
            while (TextBlockStyle::forCursor(cursor) != TextParagraphType::PageSplitter) {
                cursor.movePosition(TextCursor::PreviousBlock);
            }
            const auto tableBeginningPosition = cursor.position();
            cursor.movePosition(TextCursor::NextBlock);
            while (TextBlockStyle::forCursor(cursor) != TextParagraphType::PageSplitter) {
                if (!cursor.block().text().isEmpty()) {
                    isTableEmpty = false;
                    break;
                }
                cursor.movePosition(TextCursor::NextBlock);
            }

            //
            // ... если таблица пуста, то удалим её
            //
            if (isTableEmpty) {
                cursor.setPosition(tableBeginningPosition, TextCursor::KeepAnchor);
                cursor.removeSelectedText();
                setCurrentParagraphType(TextParagraphType::Cue);
            }
            //
            // ... а если не пуста, то выходим из таблицы и будем производить вставку после неё
            //
            else {
                while (TextBlockStyle::forCursor(cursor) != TextParagraphType::PageSplitter) {
                    cursor.movePosition(TextCursor::NextBlock);
                }
                setTextCursor(cursor);
                addParagraph(TextParagraphType::Cue);
            }
        }
    }

    //
    // Собственно вставка данных
    //
    auto cursorPosition = d->document.insertFromMime(textCursor().position(), textToInsert);

    //
    // Восстанавливаем режим редактирования, если нужно
    //
    if (wasInEditBlock) {
        cursor.beginEditBlock();
    }

    //
    // Позиционируем курсор
    //
    if (cursorPosition >= 0) {
        cursor.setPosition(cursorPosition);
        setTextCursor(cursor);
    }
}

void ComicBookTextEdit::dropEvent(QDropEvent* _event)
{
    //
    // Если в момент вставки было выделение
    //
    if (textCursor().hasSelection()) {
        BusinessLayer::TextCursor cursor = textCursor();
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
