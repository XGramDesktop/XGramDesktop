// This is the source code of XGram for Desktop.
#pragma once

#include "rpl/producer.h"
#include "rpl/variable.h"

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkAccessManager>

class QNetworkReply;

namespace XGram {

class CryptoTicker final : public QObject {
public:
	explicit CryptoTicker(QObject *parent = nullptr);

	void refresh();
	[[nodiscard]] rpl::producer<QString> textValue() const;

private:
	void setUnavailable(const QString &symbol);

	QNetworkAccessManager _manager;
	QPointer<QNetworkReply> _reply;
	QTimer _refreshTimer;
	rpl::variable<QString> _text;
};

} // namespace XGram
