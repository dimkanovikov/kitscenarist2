#pragma once

#include <QStyledItemDelegate>


namespace Ui {

/**
 * @brief Делегат отрисовки имен файлов в списке на импорт
 */
class FileNameDelegate : public QStyledItemDelegate
{
public:
    explicit FileNameDelegate(QObject* _parent = nullptr);
    ~FileNameDelegate() override;

    /**
     * @brief Реализуем собственную отрисовку
     */
    void paint(QPainter* _painter, const QStyleOptionViewItem& _option,
               const QModelIndex& _index) const override;
    QSize sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;
};

} // namespace Ui
