#include "screenplay_text_comment_view.h"

#include "screenplay_text_comment_delegate.h"

#include <ui/design_system/design_system.h>

#include <QPainter>


namespace Ui {

class ScreenplayTextCommentView::Implementation
{
public:
    QStyleOptionViewItem option(const Widget* _widget) const;

    ScreenplayTextCommentDelegate delegate;
    QModelIndex commentIndex;
};

QStyleOptionViewItem ScreenplayTextCommentView::Implementation::option(const Widget* _widget) const
{
    QStyleOptionViewItem option;
    option.widget = _widget;
    option.rect = _widget->rect();
    option.palette.setColor(QPalette::Base, _widget->backgroundColor());
    option.palette.setColor(QPalette::Text, _widget->textColor());
    return option;
}


// ****


ScreenplayTextCommentView::ScreenplayTextCommentView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
    auto sizePolicy = this->sizePolicy();
    sizePolicy.setHeightForWidth(true);
    setSizePolicy(sizePolicy);
    d->delegate.setSingleCommentMode(true);
}

ScreenplayTextCommentView::~ScreenplayTextCommentView() = default;

void ScreenplayTextCommentView::setCommentIndex(const QModelIndex& _index)
{
    if (d->commentIndex == _index) {
        return;
    }

    d->commentIndex = _index;
    updateGeometry();
    update();
}

int ScreenplayTextCommentView::heightForWidth(int _width) const
{
    auto option = d->option(this);
    option.rect.setWidth(_width);
    return d->delegate.sizeHint(option, d->commentIndex).height();
}

void ScreenplayTextCommentView::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event)

    QPainter painter(this);
    d->delegate.paint(&painter, d->option(this), d->commentIndex);
}

void ScreenplayTextCommentView::mouseReleaseEvent(QMouseEvent* _event)
{
    Widget::mouseReleaseEvent(_event);

    emit clicked();
}
} // namespace Ui
