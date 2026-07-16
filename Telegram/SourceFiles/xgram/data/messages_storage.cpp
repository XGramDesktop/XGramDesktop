// This is the source code of XGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "xgram/data/messages_storage.h"

#include "xgram/xgram_settings.h"
#include "xgram/data/xgram_database.h"
#include "xgram/utils/xgram_mapper.h"
#include "xgram/utils/telegram_helpers.h"
#include "base/unixtime.h"
#include "data/data_document.h"
#include "data/data_forum_topic.h"
#include "data/data_media_types.h"
#include "data/data_photo.h"
#include "data/data_session.h"
#include "history/history.h"
#include "history/history_item.h"
#include "history/history_item_components.h"
#include "main/main_session.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace XGramMessages {

namespace {

[[nodiscard]] QString SavedMediaDirectory(
		const ID userId,
		const ID dialogId,
		const ID topicId) {
	return cWorkingDir()
		+ u"tdata/xgram-saved-media/%1/%2/%3/"_q
			.arg(userId)
			.arg(dialogId)
			.arg(topicId);
}

void CopyDeletedMedia(not_null<HistoryItem*> item, XGramMessageBase &message) {
	if (!XGramSettings::getInstance().persistDeletedMedia()) {
		return;
	}

	const auto media = item->media();
	if (!media) {
		return;
	}

	auto source = QString();
	auto fallbackExtension = u"bin"_q;
	if (const auto document = media->document()) {
		source = document->filepath(true);
		message.documentType = document->type;
		message.mimeType = document->mimeString().toStdString();
		if (document->isVideoFile()) {
			fallbackExtension = u"mp4"_q;
		}
	} else if (const auto photo = media->photo()) {
		source = photo->location(true).name();
		message.documentType = -1; // A locally saved photo, not a document.
		message.mimeType = "image/jpeg";
		fallbackExtension = u"jpg"_q;
	} else {
		return;
	}

	const auto sourceInfo = QFileInfo(source);
	if (!sourceInfo.isFile()) {
		return;
	}

	const auto directory = SavedMediaDirectory(
		message.userId,
		message.dialogId,
		message.topicId);
	if (!QDir().mkpath(directory)) {
		return;
	}
	const auto extension = sourceInfo.suffix().isEmpty()
		? fallbackExtension
		: sourceInfo.suffix();
	const auto target = directory
		+ QString::number(message.messageId)
		+ u"."_q
		+ extension;

	QFile::remove(target);
	if (QFile::copy(source, target)) {
		message.mediaPath = target.toStdString();
	}
}

void RemoveSavedMediaDirectory(
		const ID userId,
		const ID dialogId,
		const ID topicId) {
	QDir(SavedMediaDirectory(userId, dialogId, topicId)).removeRecursively();
}

void RemoveSavedMediaDirectory(const ID userId) {
	QDir(cWorkingDir() + u"tdata/xgram-saved-media/%1/"_q.arg(userId))
		.removeRecursively();
}

} // namespace

template<typename DerivedMessage>
std::vector<XGramMessageBase> convertToBase(const std::vector<DerivedMessage> &messages) {
	std::vector<XGramMessageBase> based;
	based.reserve(messages.size());
	for (const auto &msg : messages) {
		based.push_back(static_cast<XGramMessageBase>(msg));
	}
	return based;
}

void map(not_null<HistoryItem*> item, XGramMessageBase &message) {
	const ID userId = item->history()->owner().session().userId().bare & PeerId::kChatTypeMask;

	message.userId = userId;
	message.dialogId = getDialogIdFromPeer(item->history()->peer);
	message.groupedId = item->groupId().raw();
	message.peerId = item->history()->peer->id.value & PeerId::kChatTypeMask;
	message.fromId = item->from()->id.value & PeerId::kChatTypeMask;
	if (item->topic()) {
		message.topicId = item->topicRootId().bare;
	} else {
		message.topicId = 0;
	}
	message.messageId = item->id.bare;
	message.date = item->date();
	message.flags = XGramMapper::mapItemFlagsToMTPFlags(item);

	if (const auto edited = item->Get<HistoryMessageEdited>()) {
		message.editDate = edited->date;
	} else {
		message.editDate = base::unixtime::now();
	}

	message.views = item->viewsCount();
	message.fwdFlags = 0;
	message.fwdFromId = 0;
	// message.fwdName
	message.fwdDate = 0;
	// message.fwdPostAuthor
	if (const auto msgsigned = item->Get<HistoryMessageSigned>()) {
		message.postAuthor = msgsigned->author.toStdString();
	}
	message.replyFlags = 0;
	message.replyMessageId = 0;
	message.replyPeerId = 0;
	message.replyTopId = 0;
	message.replyForumTopic = false;
	// message.replySerialized
	// message.replyMarkupSerialized
	message.entityCreateDate = base::unixtime::now();

	auto serializedText = XGramMapper::serializeTextWithEntities(item);
	message.text = serializedText.first;
	message.textEntities = serializedText.second;

	// Locally retained media is mapped only for deleted messages.
	message.mediaPath = "/";
	// message.hqThumbPath
	message.documentType = 0; // document type none
	// message.documentSerialized
	// message.thumbsSerialized
	// message.documentAttributesSerialized
	// message.mimeType
}

void addEditedMessage(not_null<HistoryItem *> item) {
	EditedMessage message;
	map(item, message);

	if (message.text.empty()) {
		return;
	}

	XGramDatabase::addEditedMessage(message);
}

std::vector<XGramMessageBase> getEditedMessages(not_null<HistoryItem*> item, ID minId, ID maxId, int totalLimit) {
	const ID userId = item->history()->owner().session().userId().bare & PeerId::kChatTypeMask;
	const auto dialogId = getDialogIdFromPeer(item->history()->peer);
	const auto msgId = item->id.bare;

	return convertToBase(XGramDatabase::getEditedMessages(userId, dialogId, msgId, minId, maxId, totalLimit));
}

bool hasRevisions(not_null<HistoryItem*> item) {
	const ID userId = item->history()->owner().session().userId().bare & PeerId::kChatTypeMask;
	const auto dialogId = getDialogIdFromPeer(item->history()->peer);
	const auto msgId = item->id.bare;

	return XGramDatabase::hasRevisions(userId, dialogId, msgId);
}

void addDeletedMessage(not_null<HistoryItem*> item) {
	DeletedMessage message;
	map(item, message);
	CopyDeletedMedia(item, message);

	if (message.text.empty() && message.mediaPath == "/") {
		return;
	}

	XGramDatabase::addDeletedMessage(message);
}

std::vector<XGramMessageBase>
getDeletedMessages(not_null<PeerData*> peer, ID topicId, ID minId, ID maxId, int totalLimit, const QString &searchQuery) {
	const ID userId = peer->session().userId().bare & PeerId::kChatTypeMask;
	return convertToBase(
		XGramDatabase::getDeletedMessages(userId, getDialogIdFromPeer(peer), topicId, minId, maxId, totalLimit, searchQuery.toStdString()));
}

bool hasDeletedMessages(not_null<PeerData*> peer, ID topicId) {
	const ID userId = peer->session().userId().bare & PeerId::kChatTypeMask;
	return XGramDatabase::hasDeletedMessages(userId, getDialogIdFromPeer(peer), topicId);
}

void clearDeletedMessages(not_null<PeerData*> peer, ID topicId) {
	const ID userId = peer->session().userId().bare & PeerId::kChatTypeMask;
	const auto dialogId = getDialogIdFromPeer(peer);
	XGramDatabase::clearDeletedMessages(userId, dialogId, topicId);
	if (topicId == 0) {
		QDir(cWorkingDir()
			+ u"tdata/xgram-saved-media/%1/%2/"_q
				.arg(userId)
				.arg(dialogId)).removeRecursively();
	} else {
		RemoveSavedMediaDirectory(userId, dialogId, topicId);
	}
}

void clearAllSavedMessages(not_null<Main::Session*> session) {
	const ID userId = session->userId().bare & PeerId::kChatTypeMask;
	XGramDatabase::clearAllDeletedMessages(userId);
	RemoveSavedMediaDirectory(userId);

	auto items = std::vector<not_null<HistoryItem*>>();
	for (const auto item : session->data().allMessages()) {
		const auto media = item->media();
		if (item->isDeleted()
			|| item->unsupportedTTL() > 0
			|| (media && media->ttlSeconds() > 0)) {
			items.push_back(item);
		}
	}
	if (!items.empty()) {
		session->data().notifyItemsAboutToBeDestroyed(items);
	}
	for (const auto item : items) {
		item->destroy();
	}
}

}
