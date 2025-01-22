#include "file_name_delegate.h"

#include "import_dialog.h"

#include <business_layer/model/text/text_model_folder_item.h>
#include <business_layer/model/text/text_model_group_item.h>
#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>

#include <QAbstractItemView>
#include <QFileInfo>
#include <QPainter>


namespace Ui {

FileNameDelegate::FileNameDelegate(QObject* _parent)
    : QStyledItemDelegate(_parent)
{
}

FileNameDelegate::~FileNameDelegate() = default;

void FileNameDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option,
                             const QModelIndex& _index) const
{
    //
    // Получим настройки стиля
    //
    QStyleOptionViewItem opt = _option;
    initStyleOption(&opt, _index);

    //
    // Рисуем ручками
    //
    _painter->setRenderHint(QPainter::Antialiasing, true);

    auto backgroundColor = _option.palette.color(QPalette::Base);
    auto textColor = _option.palette.color(QPalette::Text);
    auto fileNameTextColor = _option.palette.color(QPalette::Text);
    const auto isLeftToRight = QLocale().textDirection() == Qt::LeftToRight;

    //
    // ... фон
    //
    const QRectF backgroundRect = _option.rect;
    if (_option.state.testFlag(QStyle::State_Selected)) {
        //
        // ... для выделенных элементов
        //
        backgroundColor = _option.palette.color(QPalette::Highlight);
        textColor = _option.palette.color(QPalette::HighlightedText);
    } else if (_option.state.testFlag(QStyle::State_MouseOver)) {
        //
        // ... для элементов на которые наведена мышь
        //
        backgroundColor = _option.palette.color(QPalette::AlternateBase);
    } else {
        //
        // ... для остальных элементов
        //
        textColor.setAlphaF(DesignSystem::inactiveTextOpacity());
    }
    _painter->fillRect(backgroundRect, backgroundColor);

    //
    // ... иконка
    //
    _painter->setPen(textColor);
    QRectF iconRect;
    if (isLeftToRight) {
        iconRect = QRectF(
            QPointF(
                std::max(backgroundRect.left(), DesignSystem::treeOneLineItem().margins().left()),
                backgroundRect.top()),
            QSizeF(DesignSystem::treeOneLineItem().iconSize().width(), backgroundRect.height()));
    } else {
        iconRect = QRectF(
            QPointF(backgroundRect.right() - DesignSystem::treeOneLineItem().iconSize().width(),
                    backgroundRect.top()),
            QSizeF(DesignSystem::treeOneLineItem().iconSize().width(), backgroundRect.height()));
    }
    _painter->setFont(DesignSystem::font().iconsMid());
    _painter->drawText(iconRect, Qt::AlignLeft | Qt::AlignVCenter, u8"\U000F0214");

    //
    // ... имя файла
    //
    _painter->setPen(fileNameTextColor);
    _painter->setFont(DesignSystem::font().subtitle2());
    qreal fileNameLeft = 0.0;
    qreal fileNameWidth = 0.0;
    if (isLeftToRight) {
        fileNameLeft = iconRect.right() + DesignSystem::treeOneLineItem().spacing();
        fileNameWidth = backgroundRect.right() - DesignSystem::treeOneLineItem().margins().right()
            - fileNameLeft - DesignSystem::treeOneLineItem().spacing();
    } else {
        fileNameLeft = backgroundRect.left() + DesignSystem::treeOneLineItem().margins().left();
        fileNameWidth = iconRect.left() - fileNameLeft - DesignSystem::treeOneLineItem().spacing();
    }
    const QRectF fileNameRect(
        QPointF(fileNameLeft,
                backgroundRect.top() + DesignSystem::treeOneLineItem().margins().top()),
        QSizeF(fileNameWidth, DesignSystem::treeOneLineItem().contentHeight()));

    QString filePath = _index.data().toString();
    QFileInfo fileInfo(filePath);
    auto fileName = fileInfo.fileName();
    fileName = _painter->fontMetrics().elidedText(fileName, Qt::ElideRight,
                                                  static_cast<int>(fileNameRect.width()));
    _painter->drawText(fileNameRect, Qt::AlignLeft | Qt::AlignVCenter, fileName);

    //
    // ... текст под именем файла (в какой тип будет импортироваться)
    //
    if (!_index.data(ImportDialog::ImportEnabledRole).toBool()) {
        textColor = QColor(Qt::red);
    }
    _painter->setPen(textColor);
    auto underNameText = _index.data(ImportDialog::ImportTypeRole).toString();
    if (underNameText.isEmpty()) {
        return;
    }
    QRectF textRect;
    _painter->setFont(DesignSystem::font().body2());
    if (isLeftToRight) {
        const qreal textLeft = iconRect.right() + DesignSystem::treeOneLineItem().spacing();
        const qreal textWidth
            = backgroundRect.right() - textLeft - DesignSystem::treeOneLineItem().margins().right();
        textRect
            = QRectF(QPointF(textLeft, fileNameRect.bottom() + DesignSystem::compactLayout().px8()),
                     QSizeF(textWidth, TextHelper::fineLineSpacing(_painter->font())));
    } else {
        const qreal textLeft
            = backgroundRect.left() + DesignSystem::treeOneLineItem().margins().left();
        const qreal textWidth = iconRect.left() - textLeft;
        textRect
            = QRectF(QPointF(textLeft, fileNameRect.bottom() + DesignSystem::compactLayout().px8()),
                     QSizeF(textWidth, TextHelper::fineLineSpacing(_painter->font())));
    }
    underNameText = TextHelper::elidedText(underNameText, DesignSystem::font().body2(), textRect);
    _painter->drawText(textRect, Qt::TextWordWrap, underNameText);
}

QSize FileNameDelegate::sizeHint(const QStyleOptionViewItem& _option,
                                 const QModelIndex& _index) const
{
    if (_option.widget == nullptr) {
        return QStyledItemDelegate::sizeHint(_option, _index);
    }

    //
    // Ширина
    //
    int width = _option.widget->width();
    if (const QAbstractItemView* view = qobject_cast<const QAbstractItemView*>(_option.widget)) {
        width = view->viewport()->width();
    }
    width -= DesignSystem::layout().px8() + DesignSystem::layout().px16()
        + DesignSystem::layout().px16();

    //
    // Высота
    //
    int height = DesignSystem::treeOneLineItem().height() + DesignSystem::compactLayout().px8()
        + TextHelper::fineLineSpacing(DesignSystem::font().body2());

    return { width, height };
}

} // namespace Ui
