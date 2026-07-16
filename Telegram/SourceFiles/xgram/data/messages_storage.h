// This is the source code of XGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include "xgram/data/entities.h"

namespace Main {
class Session;
} // namespace Main

namespace XGramMessages {

void addEditedMessage(not_null<HistoryItem *> item);
std::vector<XGramMessageBase> getEditedMessages(not_null<HistoryItem*> item, ID minId, ID maxId, int totalLimit);
bool hasRevisions(not_null<HistoryItem*> item);

void addDeletedMessage(not_null<HistoryItem*> item);
std::vector<XGramMessageBase> getDeletedMessages(not_null<PeerData*> peer, ID topicId, ID minId, ID maxId, int totalLimit, const QString &searchQuery = QString());
bool hasDeletedMessages(not_null<PeerData*> peer, ID topicId);
void clearDeletedMessages(not_null<PeerData*> peer, ID topicId);
void clearAllSavedMessages(not_null<Main::Session*> session);

}
