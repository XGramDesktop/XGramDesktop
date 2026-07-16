// This is the source code of XGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "xgram/ui/utils/xgram_profile_values.h"

#include "xgram/xgram_settings.h"
#include "xgram/utils/telegram_helpers.h"
#include "data/data_peer.h"
#include "lang/lang_tag.h"
#include "lang/lang_text_entity.h"
#include "ui/text/text_utilities.h"

constexpr auto kMaxChannelId = -1000000000000;

QString IDString(const not_null<PeerData*> peer) {
	auto resultId = QString::number(getBareID(peer));

	const auto &settings = XGramSettings::getInstance();
	if (settings.showPeerId() == PeerIdDisplay::BotApi) {
		if (peer->isChannel()) {
			resultId = QString::number(peerToChannel(peer->id).bare - kMaxChannelId).prepend("-");
		} else if (peer->isChat()) {
			resultId = resultId.prepend("-");
		}
	}

	return resultId;
}

QString IDString(MsgId topicRootId) {
	return QString::number(topicRootId.bare);
}

rpl::producer<TextWithEntities> IDValue(not_null<PeerData*> peer) {
	return XGramSettings::getInstance().showPeerIdValue(
	) | rpl::map([=](PeerIdDisplay display) {
		if (display == PeerIdDisplay::Hidden) {
			return TextWithEntities();
		}
		using namespace Ui::Text;
		const auto id = IDString(peer);
		auto ok = false;
		const auto raw = id.toLongLong(&ok);
		auto result = Italic(u"id: "_q);
		result.append(Link(
			Italic(ok ? Lang::FormatCountDecimal(raw) : id),
			1));
		return result;
	});
}

rpl::producer<TextWithEntities> IDValue(MsgId topicRootId) {
	return XGramSettings::getInstance().showPeerIdValue(
	) | rpl::map([=](PeerIdDisplay display) {
		return (display == PeerIdDisplay::Hidden)
			? TextWithEntities()
			: tr::marked(IDString(topicRootId));
	});
}
