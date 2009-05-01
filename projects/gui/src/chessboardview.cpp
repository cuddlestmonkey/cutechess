#include "chessboardview.h"
#include <QPainter>
#include <QSvgRenderer>
#include <QResizeEvent>
#include <QtDebug>


ChessboardView::ChessboardView(QWidget* parent)
	: QAbstractItemView(parent),
	  m_squareSize(40),
	  m_needsUpdate(true)
{
	m_squaresRect = QRect(0, 0, 0, 0);

	setSelectionMode(QAbstractItemView::SingleSelection);
	setSelectionBehavior(QAbstractItemView::SelectItems);

	m_lightSquareColor = QColor("#ffce9e");
	m_darkSquareColor = QColor("#d18b47");
	m_moveColor = QColor(255, 255, 0, 127);

	m_pieceRenderer = new QSvgRenderer(QString(":/default.svg"), this);
	m_resizeTimer.setSingleShot(true);
	connect(&m_resizeTimer, SIGNAL(timeout()), this, SLOT(onResizeTimeout()));
}

bool ChessboardView::edit(const QModelIndex& index, EditTrigger trigger, QEvent* event)
{
	Q_UNUSED(index)
	Q_UNUSED(trigger)
	Q_UNUSED(event)

	// Items cannot be edited
	return false;
}

QModelIndex ChessboardView::indexAt(const QPoint& point) const
{
	if (model() != 0 && m_squaresRect.contains(point))
	{
		int row = (point.y() - m_squaresRect.y()) / m_squareSize;
		int column = (point.x() - m_squaresRect.x()) / m_squareSize;
		return model()->index(row, column);
	}

	return QModelIndex();
}

void ChessboardView::scrollTo(const QModelIndex& index, ScrollHint hint)
{
	Q_UNUSED(index);
	Q_UNUSED(hint);
}

QRect ChessboardView::visualRect(const QModelIndex& index) const
{
	if (index.isValid())
	{
		return QRect (m_squaresRect.x() + (index.column() * m_squareSize),
			m_squaresRect.y() + (index.row() * m_squareSize),
			m_squareSize, m_squareSize);
	}

	return QRect();
}

void ChessboardView::renderPiece(const QModelIndex& index, QPainter& painter, QRectF bounds)
{
	QVariant data = model()->data(index);
	if (data.isNull())
		return;

	QString id = data.toString();
	QRectF pieceBounds(m_pieceRenderer->boundsOnElement(id));

	qreal aspect = pieceBounds.width() / pieceBounds.height();
	if (aspect > 1.0)
	{
		qreal height = bounds.height() / aspect;
		qreal adjust = (bounds.height() - height) / 2;
		bounds.adjust(0, adjust, 0, -adjust);
	}
	else if (aspect < 1.0)
	{
		qreal width = bounds.width() * aspect;
		qreal adjust = (bounds.width() - width) / 2;
		bounds.adjust(adjust, 0, -adjust, 0);
	}
	qreal a = bounds.width() / 10;
	bounds.adjust(a, a, -a, -a);

	m_pieceRenderer->render(&painter, id, bounds);
}

void ChessboardView::renderSquare(const QModelIndex& index, QPainter& painter)
{
	int row = index.row();
	int column = index.column();
	QRectF sqBounds(column * m_squareSize, row * m_squareSize,
			m_squareSize, m_squareSize);

	if ((row % 2) == (column % 2))
		painter.fillRect(sqBounds, m_lightSquareColor);
	else
		painter.fillRect(sqBounds, m_darkSquareColor);

	if (index == m_sourceSquare || index == m_targetSquare)
		painter.fillRect(sqBounds, m_moveColor);

	renderPiece(index, painter, sqBounds);
}

void ChessboardView::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);
	if (!model())
		return;

	QPainter painter(viewport());
	painter.setBackgroundMode(Qt::OpaqueMode);

	if (!m_needsUpdate)
	{
		painter.drawPixmap(m_squaresRect, m_background);
		return;
	}
	m_needsUpdate = false;

	if (m_background.size() != m_squaresRect.size())
		m_background = QPixmap(m_squaresRect.size());
	QPainter bgPainter(&m_background);
	bgPainter.setBackgroundMode(Qt::OpaqueMode);

	int columnCount = model()->columnCount();
	int rowCount = model()->rowCount();

	for (int row = 0; row < rowCount; row++)
	{
		for (int column = 0; column < columnCount; column++)
		{
			QModelIndex index = model()->index(row, column);
			renderSquare(index, bgPainter);
		}
	}

	painter.drawPixmap(m_squaresRect, m_background);
}

bool ChessboardView::isIndexHidden(const QModelIndex& index) const
{
	Q_UNUSED(index);
	return false;
}

int ChessboardView::horizontalOffset() const
{
	return 0;
}

int ChessboardView::verticalOffset() const
{
	return 0;
}

QModelIndex ChessboardView::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
	// TODO
	Q_UNUSED(cursorAction);
	Q_UNUSED(modifiers);
	return QModelIndex();
}

void ChessboardView::setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags flags)
{
	QModelIndex selected = indexAt(QPoint(rect.x(), rect.y()));

	if (selected.isValid())
	{
		selectionModel()->select(selected, flags);
	}
}

QRegion ChessboardView::visualRegionForSelection(const QItemSelection& selection) const
{
	Q_UNUSED(selection);
	return QRegion();
}

void ChessboardView::dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
	QPainter painter(&m_background);
	painter.setBackgroundMode(Qt::OpaqueMode);

	m_needsUpdate = true;
	for (int row = topLeft.row(); row <= bottomRight.row(); row++)
	{
		for (int column = topLeft.column(); column <= bottomRight.column(); column++)
		{
			QModelIndex index = model()->index(row, column);
			renderSquare(index, painter);
		}
	}
	QAbstractItemView::dataChanged(topLeft, bottomRight);
}

void ChessboardView::resizeBoard(const QSize& size)
{
	if (!model())
		return;

	int width = size.width();
	int height = size.height();
	int columnCount = model()->columnCount();
	int rowCount = model()->rowCount();
	m_squareSize = qMin(width / columnCount, height / rowCount);

	m_squaresRect.setWidth(columnCount * m_squareSize);
	m_squaresRect.setHeight(rowCount * m_squareSize);
	m_squaresRect.moveCenter(viewport()->rect().center());
}

void ChessboardView::onResizeTimeout()
{
	m_needsUpdate = true;
	viewport()->update();
}

void ChessboardView::resizeEvent(QResizeEvent* event)
{
	QAbstractItemView::resizeEvent(event);
	resizeBoard(event->size());
	m_resizeTimer.start(100);
}

void ChessboardView::onMoveMade(const QModelIndex& source, const QModelIndex& target)
{
	QModelIndex oldSource = m_sourceSquare;
	QModelIndex oldTarget = m_targetSquare;

	m_sourceSquare = source;
	m_targetSquare = target;

	update(oldSource);
	update(oldTarget);
}

void ChessboardView::setLightSquareColor(const QColor& color)
{
	m_lightSquareColor = color;
}

void ChessboardView::setDarkSquareColor(const QColor& color)
{
	m_darkSquareColor = color;
}

QColor ChessboardView::lightSquareColor() const
{
	return m_lightSquareColor;
}

QColor ChessboardView::darkSquareColor() const
{
	return m_darkSquareColor;
}

void ChessboardView::reset()
{
	m_sourceSquare = QModelIndex();
	m_targetSquare = QModelIndex();
	resizeBoard(size());
	m_needsUpdate = true;
	QAbstractItemView::reset();
}
