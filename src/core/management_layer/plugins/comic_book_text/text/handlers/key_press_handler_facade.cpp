#include "key_press_handler_facade.h"

#include "../comic_book_text_edit.h"
#include "character_handler.h"
#include "description_handler.h"
#include "dialog_handler.h"
#include "inline_note_handler.h"
#include "page_handler.h"
#include "panel_handler.h"
#include "parenthetical_handler.h"
#include "pre_handler.h"
#include "prepare_handler.h"
#include "sequence_footer_handler.h"
#include "sequence_heading_handler.h"
#include "splitter_handler.h"
#include "unformatted_text_handler.h"

#include <business_layer/templates/comic_book_template.h>

#include <QKeyEvent>
#include <QTextBlock>

using BusinessLayer::TextParagraphType;
using Ui::ComicBookTextEdit;

namespace KeyProcessingLayer {

class KeyPressHandlerFacade::Implementation
{
public:
    explicit Implementation(ComicBookTextEdit* _editor);

    Ui::ComicBookTextEdit* editor = nullptr;

    QScopedPointer<PrepareHandler> prepareHandler;
    QScopedPointer<PreHandler> preHandler;
    QScopedPointer<UnformattedTextHandler> unformattedTextHandler;
    QScopedPointer<PageHandler> pageHandler;
    QScopedPointer<PanelHandler> panelHandler;
    QScopedPointer<DescriptionHandler> actionHandler;
    QScopedPointer<CharacterHandler> characterHandler;
    QScopedPointer<ParentheticalHandler> parentheticalHandler;
    QScopedPointer<DialogHandler> dialogHandler;
    QScopedPointer<InlineNoteHandler> inlineNoteHandler;
    QScopedPointer<SequenceHeadingHandler> sequenceHeadingHandler;
    QScopedPointer<SequenceFooterHandler> sequenceFooterHandler;
    QScopedPointer<SplitterHandler> splitterHandler;
};

KeyPressHandlerFacade::Implementation::Implementation(Ui::ComicBookTextEdit* _editor)
    : editor(_editor)
    , prepareHandler(new PrepareHandler(_editor))
    , preHandler(new PreHandler(_editor))
    , unformattedTextHandler(new UnformattedTextHandler(_editor))
    , pageHandler(new PageHandler(_editor))
    , panelHandler(new PanelHandler(_editor))
    , actionHandler(new DescriptionHandler(_editor))
    , characterHandler(new CharacterHandler(_editor))
    , parentheticalHandler(new ParentheticalHandler(_editor))
    , dialogHandler(new DialogHandler(_editor))
    , inlineNoteHandler(new InlineNoteHandler(_editor))
    , sequenceHeadingHandler(new SequenceHeadingHandler(_editor))
    , sequenceFooterHandler(new SequenceFooterHandler(_editor))
    , splitterHandler(new SplitterHandler(_editor))
{
}


// ****


KeyPressHandlerFacade* KeyPressHandlerFacade::instance(ComicBookTextEdit* _editor)
{
    static QHash<ComicBookTextEdit*, KeyPressHandlerFacade*> instances;
    if (!instances.contains(_editor)) {
        instances.insert(_editor, new KeyPressHandlerFacade(_editor));
    }

    return instances.value(_editor);
}

KeyPressHandlerFacade::~KeyPressHandlerFacade() = default;

void KeyPressHandlerFacade::prepare(QKeyEvent* _event)
{
    d->prepareHandler->handle(_event);
}

void KeyPressHandlerFacade::prepareForHandle(QKeyEvent* _event)
{
    d->preHandler->handle(_event);
}

void KeyPressHandlerFacade::prehandle()
{
    const bool prepare = true;
    handle(nullptr, prepare);
}

void KeyPressHandlerFacade::handle(QEvent* _event, bool _pre)
{
    QTextBlock currentBlock = d->editor->textCursor().block();
    const auto currentType = BusinessLayer::TextBlockStyle::forBlock(currentBlock);
    auto currentHandler = handlerFor(currentType);

    if (currentHandler == nullptr) {
        return;
    }

    if (_pre) {
        currentHandler->prehandle();
    } else {
        currentHandler->handle(_event);
    }
}

bool KeyPressHandlerFacade::needSendEventToBaseClass() const
{
    return d->prepareHandler->needSendEventToBaseClass();
}

bool KeyPressHandlerFacade::needEnsureCursorVisible() const
{
    return d->prepareHandler->needEnsureCursorVisible();
}

bool KeyPressHandlerFacade::needPrehandle() const
{
    return d->prepareHandler->needPrehandle();
}

KeyPressHandlerFacade::KeyPressHandlerFacade(ComicBookTextEdit* _editor)
    : d(new Implementation(_editor))
{
}

AbstractKeyHandler* KeyPressHandlerFacade::handlerFor(TextParagraphType _type)
{
    switch (_type) {
    case TextParagraphType::UnformattedText: {
        return d->unformattedTextHandler.data();
    }

    case TextParagraphType::PageHeading: {
        return d->pageHandler.data();
    }

    case TextParagraphType::PanelHeading: {
        return d->panelHandler.data();
    }

    case TextParagraphType::Description: {
        return d->actionHandler.data();
    }

    case TextParagraphType::Character: {
        return d->characterHandler.data();
    }

    case TextParagraphType::Parenthetical: {
        return d->parentheticalHandler.data();
    }

    case TextParagraphType::Dialogue: {
        return d->dialogHandler.data();
    }

    case TextParagraphType::InlineNote: {
        return d->inlineNoteHandler.data();
    }

    case TextParagraphType::SequenceHeading: {
        return d->sequenceHeadingHandler.data();
    }

    case TextParagraphType::SequenceFooter: {
        return d->sequenceFooterHandler.data();
    }

    case TextParagraphType::PageSplitter: {
        return d->splitterHandler.data();
    }

    default: {
        return nullptr;
    }
    }
}

} // namespace KeyProcessingLayer
