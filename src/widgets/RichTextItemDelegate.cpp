#include "RichTextItemDelegate.h"

#include <QDebug>
#include <QPainter>
#include <QTextDocument>

RichTextItemDelegate::RichTextItemDelegate(QObject *parent) : QItemDelegate(parent)
{
}

void RichTextItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    _paintingIndex = index;
    QItemDelegate::paint(painter, option, index);
}

void RichTextItemDelegate::drawDisplay(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QString &text) const
{
    Q_UNUSED(option);
    Q_UNUSED(text);
    Q_ASSERT(_paintingIndex.isValid());

    QTextDocument *doc = document(option, _paintingIndex);

    painter->save();
    painter->translate(rect.left(), rect.top());
    doc->drawContents(painter, QRect(0, 0, rect.width(), rect.height()));
    painter->restore();
    delete doc;
}

QSize RichTextItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QVariant value = index.data(Qt::SizeHintRole);
    if (value.isValid())
        return qvariant_cast<QSize>(value);

    QTextDocument *doc = document(option, index);

    QRect decorationRect = rect(option, index, Qt::DecorationRole);
    QRect displayRect = QRect(0, 0, doc->size().width(), doc->size().height());
    QRect checkRect = rect(option, index, Qt::CheckStateRole);

    doLayout(option, &checkRect, &decorationRect, &displayRect, true);

    delete doc;

    return (decorationRect|displayRect|checkRect).size();
}

QTextDocument* RichTextItemDelegate::document(const QStyleOptionViewItem &option, const QModelIndex& index) const
{
    QFont font = qvariant_cast<QFont>(index.data(Qt::FontRole)).resolve(option.font);

    QTextDocument *doc = new QTextDocument;
    doc->setDefaultFont(font);
    doc->setTextWidth(-1);
    doc->setHtml(index.data(Qt::DisplayRole).toString());
    return doc;
}