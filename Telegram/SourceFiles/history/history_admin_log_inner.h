/*
This file is part of Telegram Desktop,
the official desktop version of Telegram messaging app, see https://telegram.org

Telegram Desktop is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

It is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

In addition, as a special exception, the copyright holders give permission
to link the code of portions of this program with the OpenSSL library.

Full license: https://github.com/telegramdesktop/tdesktop/blob/master/LICENSE
Copyright (c) 2014-2017 John Preston, https://desktop.telegram.org
*/
#pragma once

#include "history/history_admin_log_item.h"
#include "history/history_admin_log_section.h"
#include "ui/widgets/tooltip.h"
#include "mtproto/sender.h"
#include "base/timer.h"

namespace Ui {
class PopupMenu;
} // namespace Ui

namespace Window {
class Controller;
} // namespace Window

namespace AdminLog {

class SectionMemento;

class InnerWidget final : public TWidget, public Ui::AbstractTooltipShower, private MTP::Sender, private base::Subscriber {
public:
	InnerWidget(QWidget *parent, gsl::not_null<Window::Controller*> controller, gsl::not_null<ChannelData*> channel, base::lambda<void(int top)> scrollTo);

	gsl::not_null<ChannelData*> channel() const {
		return _channel;
	}

	// Updates the area that is visible inside the scroll container.
	void setVisibleTopBottom(int visibleTop, int visibleBottom) override;

	// Set the correct scroll position after being resized.
	void restoreScrollPosition();

	void resizeToWidth(int newWidth, int minHeight) {
		_minHeight = minHeight;
		return TWidget::resizeToWidth(newWidth);
	}

	void saveState(gsl::not_null<SectionMemento*> memento);
	void restoreState(gsl::not_null<SectionMemento*> memento);
	void setCancelledCallback(base::lambda<void()> callback) {
		_cancelledCallback = std::move(callback);
	}

	// Empty "flags" means all events.
	void applyFilter(FilterValue &&value);
	FilterValue filter() const {
		return _filter;
	}

	// AbstractTooltipShower interface
	QString tooltipText() const override;
	QPoint tooltipPos() const override;

	~InnerWidget();

protected:
	void paintEvent(QPaintEvent *e) override;
	void keyPressEvent(QKeyEvent *e) override;
	void mousePressEvent(QMouseEvent *e) override;
	void mouseMoveEvent(QMouseEvent *e) override;
	void mouseReleaseEvent(QMouseEvent *e) override;
	void mouseDoubleClickEvent(QMouseEvent *e) override;
	void enterEventHook(QEvent *e) override;
	void leaveEventHook(QEvent *e) override;
	void contextMenuEvent(QContextMenuEvent *e) override;

	// Resizes content and counts natural widget height for the desired width.
	int resizeGetHeight(int newWidth) override;

private:
	enum class Direction {
		Up,
		Down,
	};
	enum class MouseAction {
		None,
		PrepareDrag,
		Dragging,
		Selecting,
	};
	enum class EnumItemsDirection {
		TopToBottom,
		BottomToTop,
	};

	void mouseActionStart(const QPoint &screenPos, Qt::MouseButton button);
	void mouseActionUpdate(const QPoint &screenPos);
	void mouseActionFinish(const QPoint &screenPos, Qt::MouseButton button);
	void mouseActionCancel();
	void updateSelected();
	void performDrag();
	int itemTop(gsl::not_null<const HistoryItem*> item) const;
	void repaintItem(const HistoryItem *item);
	QPoint mapPointToItem(QPoint point, const HistoryItem *item) const;
	void handlePendingHistoryResize();

	void showContextMenu(QContextMenuEvent *e, bool showFromTouch = false);
	void savePhotoToFile(PhotoData *photo);
	void saveDocumentToFile(DocumentData *document);
	void copyContextImage(PhotoData *photo);
	void showStickerPackInfo();
	void copyContextUrl();
	void cancelContextDownload();
	void showContextInFolder();
	void openContextGif();
	void copyContextText();
	void copySelectedText();
	TextWithEntities getSelectedText() const;
	void setToClipboard(const TextWithEntities &forClipboard, QClipboard::Mode mode = QClipboard::Clipboard);

	void checkPreloadMore();
	void updateVisibleTopItem();
	void preloadMore(Direction direction);
	void itemsAdded(Direction direction);
	void updateSize();
	void updateMinMaxIds();
	void updateEmptyText();
	void paintEmpty(Painter &p);
	void clearAfterFilterChange();

	void toggleScrollDateShown();
	void repaintScrollDateCallback();
	bool displayScrollDate() const;
	void scrollDateHide();
	void scrollDateCheck();
	void scrollDateHideByTimer();

	// This function finds all history items that are displayed and calls template method
	// for each found message (in given direction) in the passed history with passed top offset.
	//
	// Method has "bool (*Method)(HistoryItem *item, int itemtop, int itembottom)" signature
	// if it returns false the enumeration stops immidiately.
	template <EnumItemsDirection direction, typename Method>
	void enumerateItems(Method method);

	// This function finds all userpics on the left that are displayed and calls template method
	// for each found userpic (from the top to the bottom) using enumerateItems() method.
	//
	// Method has "bool (*Method)(gsl::not_null<HistoryMessage*> message, int userpicTop)" signature
	// if it returns false the enumeration stops immidiately.
	template <typename Method>
	void enumerateUserpics(Method method);

	// This function finds all date elements that are displayed and calls template method
	// for each found date element (from the bottom to the top) using enumerateItems() method.
	//
	// Method has "bool (*Method)(gsl::not_null<HistoryItem*> item, int itemtop, int dateTop)" signature
	// if it returns false the enumeration stops immidiately.
	template <typename Method>
	void enumerateDates(Method method);

	gsl::not_null<Window::Controller*> _controller;
	gsl::not_null<ChannelData*> _channel;
	gsl::not_null<History*> _history;
	base::lambda<void()> _cancelledCallback;
	base::lambda<void(int top)> _scrollTo;
	std::vector<HistoryItemOwned> _items;
	std::map<uint64, HistoryItem*> _itemsByIds;
	int _itemsTop = 0;
	int _itemsHeight = 0;

	LocalIdManager _idManager;
	int _minHeight = 0;
	int _visibleTop = 0;
	int _visibleBottom = 0;
	HistoryItem *_visibleTopItem = nullptr;
	int _visibleTopFromItem = 0;

	bool _scrollDateShown = false;
	Animation _scrollDateOpacity;
	SingleQueuedInvokation _scrollDateCheck;
	base::Timer _scrollDateHideTimer;
	HistoryItem *_scrollDateLastItem = nullptr;
	int _scrollDateLastItemTop = 0;

	// Up - max, Down - min.
	uint64 _maxId = 0;
	uint64 _minId = 0;
	mtpRequestId _preloadUpRequestId = 0;
	mtpRequestId _preloadDownRequestId = 0;

	// Don't load anything until the memento was read.
	bool _upLoaded = true;
	bool _downLoaded = true;
	bool _filterChanged = false;
	Text _emptyText;

	MouseAction _mouseAction = MouseAction::None;
	TextSelectType _mouseSelectType = TextSelectType::Letters;
	QPoint _dragStartPosition;
	QPoint _mousePosition;
	HistoryItem *_mouseActionItem = nullptr;
	HistoryCursorState _mouseCursorState = HistoryDefaultCursorState;
	uint16 _mouseTextSymbol = 0;
	bool _pressWasInactive = false;

	HistoryItem *_selectedItem = nullptr;
	TextSelection _selectedText;
	bool _wasSelectedText = false; // was some text selected in current drag action
	Qt::CursorShape _cursor = style::cur_default;

	// context menu
	Ui::PopupMenu *_menu = nullptr;

	QPoint _trippleClickPoint;
	base::Timer _trippleClickTimer;

	ClickHandlerPtr _contextMenuLink;

	FilterValue _filter;

};

} // namespace AdminLog
