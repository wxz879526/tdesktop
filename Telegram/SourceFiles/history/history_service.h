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

struct HistoryServiceDependentData {
	MsgId msgId = 0;
	HistoryItem *msg = nullptr;
	ClickHandlerPtr lnk;
};

struct HistoryServicePinned : public RuntimeComponent<HistoryServicePinned>, public HistoryServiceDependentData {
};

struct HistoryServiceGameScore : public RuntimeComponent<HistoryServiceGameScore>, public HistoryServiceDependentData {
	int score = 0;
};

struct HistoryServicePayment : public RuntimeComponent<HistoryServicePayment>, public HistoryServiceDependentData {
	QString amount;
};

namespace HistoryLayout {
class ServiceMessagePainter;
} // namespace HistoryLayout

class HistoryService : public HistoryItem, private HistoryItemInstantiated<HistoryService> {
public:
	struct PreparedText {
		QString text;
		QList<ClickHandlerPtr> links;
	};

	static gsl::not_null<HistoryService*> create(gsl::not_null<History*> history, const MTPDmessageService &message) {
		return _create(history, message);
	}
	static gsl::not_null<HistoryService*> create(gsl::not_null<History*> history, MsgId msgId, QDateTime date, const PreparedText &message, MTPDmessage::Flags flags = 0, UserId from = 0, PhotoData *photo = nullptr) {
		return _create(history, msgId, date, message, flags, from, photo);
	}

	bool updateDependencyItem() override;
	MsgId dependencyMsgId() const override {
		if (auto dependent = GetDependentData()) {
			return dependent->msgId;
		}
		return 0;
	}
	bool notificationReady() const override {
		if (auto dependent = GetDependentData()) {
			return (dependent->msg || !dependent->msgId);
		}
		return true;
	}

	QRect countGeometry() const;

	void draw(Painter &p, QRect clip, TextSelection selection, TimeMs ms) const override;
	bool hasPoint(QPoint point) const override;
	HistoryTextState getState(QPoint point, HistoryStateRequest request) const override;

	TextSelection adjustSelection(TextSelection selection, TextSelectType type) const override WARN_UNUSED_RESULT {
		return _text.adjustSelection(selection, type);
	}

	void clickHandlerActiveChanged(const ClickHandlerPtr &p, bool active) override;
	void clickHandlerPressedChanged(const ClickHandlerPtr &p, bool pressed) override;

	void applyEdition(const MTPDmessageService &message) override;

	int32 addToOverview(AddToOverviewMethod method) override;
	void eraseFromOverview() override;

	bool needCheck() const override {
		return false;
	}
	bool serviceMsg() const override {
		return true;
	}
	TextWithEntities selectedText(TextSelection selection) const override;
	QString inDialogsText() const override;
	QString inReplyText() const override;

	~HistoryService();

protected:
	friend class HistoryLayout::ServiceMessagePainter;

	HistoryService(gsl::not_null<History*> history, const MTPDmessageService &message);
	HistoryService(gsl::not_null<History*> history, MsgId msgId, QDateTime date, const PreparedText &message, MTPDmessage::Flags flags = 0, UserId from = 0, PhotoData *photo = 0);
	friend class HistoryItemInstantiated<HistoryService>;

	void initDimensions() override;
	int resizeContentGetHeight() override;

	void setServiceText(const PreparedText &prepared);

	QString fromLinkText() const {
		return textcmdLink(1, _from->name);
	};
	ClickHandlerPtr fromLink() const {
		return peerOpenClickHandler(_from);
	};

	void removeMedia();

private:
	HistoryServiceDependentData *GetDependentData() {
		if (auto pinned = Get<HistoryServicePinned>()) {
			return pinned;
		} else if (auto gamescore = Get<HistoryServiceGameScore>()) {
			return gamescore;
		} else if (auto payment = Get<HistoryServicePayment>()) {
			return payment;
		}
		return nullptr;
	}
	const HistoryServiceDependentData *GetDependentData() const {
		return const_cast<HistoryService*>(this)->GetDependentData();
	}
	bool updateDependent(bool force = false);
	void updateDependentText();
	void clearDependency();

	void createFromMtp(const MTPDmessageService &message);
	void setMessageByAction(const MTPmessageAction &action);

	PreparedText preparePinnedText();
	PreparedText prepareGameScoreText();
	PreparedText preparePaymentSentText();

};

class HistoryJoined : public HistoryService, private HistoryItemInstantiated<HistoryJoined> {
public:
	static gsl::not_null<HistoryJoined*> create(gsl::not_null<History*> history, const QDateTime &inviteDate, gsl::not_null<UserData*> inviter, MTPDmessage::Flags flags) {
		return _create(history, inviteDate, inviter, flags);
	}

protected:
	HistoryJoined(gsl::not_null<History*> history, const QDateTime &inviteDate, gsl::not_null<UserData*> inviter, MTPDmessage::Flags flags);
	using HistoryItemInstantiated<HistoryJoined>::_create;
	friend class HistoryItemInstantiated<HistoryJoined>;

private:
	static PreparedText GenerateText(gsl::not_null<History*> history, gsl::not_null<UserData*> inviter);

};

extern TextParseOptions _historySrvOptions;
