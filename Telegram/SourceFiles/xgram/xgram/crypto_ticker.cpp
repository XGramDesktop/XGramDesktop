// This is the source code of XGram for Desktop.
#include "xgram/xgram/crypto_ticker.h"

#include "xgram/xgram_settings.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QLocale>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

namespace XGram {
namespace {

constexpr auto kRefreshInterval = 60 * 1000;

} // namespace

CryptoTicker::CryptoTicker(QObject *parent)
: QObject(parent)
, _manager(this) {
	_refreshTimer.setInterval(kRefreshInterval);
	connect(
		&_refreshTimer,
		&QTimer::timeout,
		this,
		&CryptoTicker::refresh);
	_refreshTimer.start();
	refresh();
}

void CryptoTicker::refresh() {
	const auto &settings = XGramSettings::getInstance();
	if (!settings.cryptoTickerEnabled()) {
		_text = QString();
		return;
	}

	const auto symbol = settings.cryptoTickerSymbol();
	if (_reply) {
		_reply->abort();
		_reply->deleteLater();
		_reply = nullptr;
	}
	_text = u"%1  ·  …"_q.arg(symbol);

	auto request = QNetworkRequest(QUrl(
		u"https://api.coinbase.com/v2/prices/%1-USD/spot"_q.arg(symbol)));
	request.setHeader(QNetworkRequest::UserAgentHeader, u"XGram Desktop"_q);
	request.setAttribute(
		QNetworkRequest::RedirectPolicyAttribute,
		QNetworkRequest::NoLessSafeRedirectPolicy);
	const auto reply = _manager.get(request);
	_reply = reply;
	connect(reply, &QNetworkReply::finished, this, [=] {
		if (_reply != reply) {
			reply->deleteLater();
			return;
		}
		_reply = nullptr;

		if (reply->error() != QNetworkReply::NoError) {
			setUnavailable(symbol);
			reply->deleteLater();
			return;
		}
		const auto data = QJsonDocument::fromJson(reply->readAll()).object();
		const auto amount = data.value(u"data"_q).toObject().value(u"amount"_q).toString();
		bool converted = false;
		const auto price = amount.toDouble(&converted);
		if (!converted || price <= 0.) {
			setUnavailable(symbol);
			reply->deleteLater();
			return;
		}
		const auto precision = (price < 1.) ? 5 : (price < 100.) ? 2 : 0;
		_text = u"%1  ·  $%2"_q.arg(
			symbol,
			QLocale().toString(price, 'f', precision));
		reply->deleteLater();
	});
}

rpl::producer<QString> CryptoTicker::textValue() const {
	return _text.value();
}

void CryptoTicker::setUnavailable(const QString &symbol) {
	_text = u"%1  ·  —"_q.arg(symbol);
}

} // namespace XGram
