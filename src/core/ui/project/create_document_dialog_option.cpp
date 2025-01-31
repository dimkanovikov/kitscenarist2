#include "create_document_dialog_option.h"

#include <domain/document_object.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/animations/click_animation.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>

#include <QPaintEvent>
#include <QPainter>


namespace Ui {

class CreateDocumentDialogOption::Implementation
{
public:
    explicit Implementation(const Domain::DocumentObjectType& _documentType);


    bool isChecked = false;
    Domain::DocumentObjectType documentType;
    QString documentName;

    /**
     * @brief  Декорации при клике
     */
    ClickAnimation decorationAnimation;
};

CreateDocumentDialogOption::Implementation::Implementation(
    const Domain::DocumentObjectType& _documentType)
    : documentType(_documentType)
{
    decorationAnimation.setFast(false);
}


// ****


CreateDocumentDialogOption::CreateDocumentDialogOption(
    const Domain::DocumentObjectType& _documentType, QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(_documentType))
{

    setAttribute(Qt::WA_Hover);
    setFocusPolicy(Qt::StrongFocus);

    connect(&d->decorationAnimation, &ClickAnimation::valueChanged, this,
            qOverload<>(&CreateDocumentDialogOption::update));
}

CreateDocumentDialogOption::~CreateDocumentDialogOption() = default;

const Domain::DocumentObjectType& CreateDocumentDialogOption::documentType() const
{
    return d->documentType;
}

bool CreateDocumentDialogOption::isChecked() const
{
    return d->isChecked;
}

void CreateDocumentDialogOption::setChecked(bool _checked)
{
    if (d->isChecked == _checked) {
        return;
    }

    d->isChecked = _checked;
    update();
    emit checkedChanged(d->isChecked);
}

QSize CreateDocumentDialogOption::sizeHint() const
{
    return QSize(
        contentsMargins().left() + Ui::DesignSystem::layout().px(124) + contentsMargins().right(),
        contentsMargins().top() + Ui::DesignSystem::layout().px(124) + contentsMargins().bottom());
}

void CreateDocumentDialogOption::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);

    //
    // Заливаем фон
    //
    painter.fillRect(_event->rect(), backgroundColor());

    //
    // Заливаем подложку и рисуем рамку
    //
    painter.setPen(
        d->isChecked
            ? QPen(Ui::DesignSystem::color().accent(), Ui::DesignSystem::layout().px2())
            : (underMouse()
                   ? QPen(Ui::DesignSystem::color().accent(), Ui::DesignSystem::layout().px())
                   : QPen(ColorHelper::transparent(textColor(),
                                                   Ui::DesignSystem::elevationEndOpacity()),
                          Ui::DesignSystem::layout().px())));
    painter.setBrush(underMouse() ? ColorHelper::transparent(
                         textColor(), Ui::DesignSystem::elevationEndOpacity())
                                  : backgroundColor());
    painter.drawRoundedRect(contentsRect(), Ui::DesignSystem::card().borderRadius(),
                            Ui::DesignSystem::card().borderRadius());

    //
    // Иконка
    //
    const QRectF iconRect(contentsRect().left(), contentsRect().top(), contentsRect().width(),
                          DesignSystem::layout().px(60));
    painter.setPen(textColor());
    painter.setFont(Ui::DesignSystem::font().iconsMid());
    painter.drawText(iconRect, Qt::AlignHCenter | Qt::AlignBottom,
                     Domain::iconForType(d->documentType));

    //
    // Текст
    //
    const QRectF textRect(contentsRect().left() + DesignSystem::layout().px8(),
                          contentsRect().top() + DesignSystem::layout().px(75),
                          contentsRect().width() - DesignSystem::layout().px16(),
                          contentsRect().height() - DesignSystem::layout().px(75));
    painter.setFont(DesignSystem::font().subtitle2());
    painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, d->documentName);

    //
    // Декорация
    //
    if (d->decorationAnimation.state() == ClickAnimation::Running) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(Ui::DesignSystem::color().accent());
        painter.setClipRect(d->decorationAnimation.clipRect());
        painter.setOpacity(d->decorationAnimation.opacity());
        const auto radius = d->decorationAnimation.radius();
        painter.drawEllipse(d->decorationAnimation.clickPosition(), radius, radius);
        painter.setClipRect(QRectF(), Qt::NoClip);
        painter.setOpacity(1.0);
    }
}

void CreateDocumentDialogOption::mousePressEvent(QMouseEvent* _event)
{
    Widget::mousePressEvent(_event);

    if (!contentsRect().contains(_event->pos())) {
        return;
    }

    d->decorationAnimation.setClickPosition(_event->pos());
    d->decorationAnimation.setClipRect(contentsRect());
    d->decorationAnimation.setRadiusInterval(0.0, contentsRect().width());
    d->decorationAnimation.start();
}

void CreateDocumentDialogOption::mouseReleaseEvent(QMouseEvent* _event)
{
    if (!contentsRect().contains(_event->pos())) {
        return;
    }

    if (d->isChecked) {
        return;
    }

    d->isChecked = true;
    emit checkedChanged(d->isChecked);
}

void CreateDocumentDialogOption::updateTranslations()
{
    using namespace Domain;
    switch (d->documentType) {
    case DocumentObjectType::Folder: {
        d->documentName = tr("Folder");
        break;
    }

    case DocumentObjectType::SimpleText: {
        d->documentName = tr("Text");
        break;
    }

    case DocumentObjectType::Character: {
        d->documentName = tr("Character");
        break;
    }

    case DocumentObjectType::Location: {
        d->documentName = tr("Location");
        break;
    }

    case DocumentObjectType::World: {
        d->documentName = tr("World");
        break;
    }

    case DocumentObjectType::Screenplay: {
        d->documentName = tr("Screenplay");
        break;
    }

    case DocumentObjectType::ComicBook: {
        d->documentName = tr("Comic book");
        break;
    }

    case DocumentObjectType::Audioplay: {
        d->documentName = tr("Audioplay");
        break;
    }

    case DocumentObjectType::Stageplay: {
        d->documentName = tr("Stageplay");
        break;
    }

    case DocumentObjectType::Novel: {
        d->documentName = tr("Novel");
        break;
    }

    case DocumentObjectType::MindMap: {
        d->documentName = tr("Mind map");
        break;
    }

    case DocumentObjectType::ImagesGallery: {
        d->documentName = tr("Image gallery");
        break;
    }

    case DocumentObjectType::Presentation: {
        d->documentName = tr("Presentation");
        break;
    }

    default: {
        Q_ASSERT(false);
        break;
    }
    }
}

} // namespace Ui
