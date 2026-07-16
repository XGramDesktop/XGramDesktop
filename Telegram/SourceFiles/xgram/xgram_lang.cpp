// This is the source code of XGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "xgram/xgram_lang.h"

#include "qjsondocument.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "lang/lang_instance.h"
#include "storage/localstorage.h"

#include <QDir>
#include <QFile>

// hard-coded languages
std::map<QString, QString> langMapping = {
	{"pt-br", "pt"},
	{"zh-hans-beta", "zh-hans"},
	{"zh-hant-beta", "zh-hant"},
	{"zh-hans-raw", "zh-hans"},
	{"zh-hant-raw", "zh-hant"},
};

constexpr auto postfixes = {
	"zero",
	"one",
	"two",
	"few",
	"many",
	"other"
};

namespace {

[[nodiscard]] bool IsRussianLanguage() {
	const auto id = Lang::GetInstance().id().toLower();
	const auto baseId = Lang::GetInstance().baseId().toLower();
	return (id == u"ru"_q)
		|| id.startsWith(u"ru-"_q)
		|| (baseId == u"ru"_q)
		|| baseId.startsWith(u"ru-"_q);
}

void ApplyRussianXGramTranslations() {
	if (!IsRussianLanguage()) {
		return;
	}

	static const auto translations = std::map<QString, QString>{
		{ u"xgram_XGramPreferences"_q, u"Настройки XGram"_q },
		{ u"xgram_CryptoHeader"_q, u"Криптовалюта"_q },
		{ u"xgram_CryptoTickerEnabled"_q, u"Показывать курс криптовалюты"_q },
		{ u"xgram_CryptoTickerSymbol"_q, u"Криптовалюта"_q },
		{ u"xgram_CryptoTickerChoose"_q, u"Выберите криптовалюту"_q },
		{ u"xgram_CryptoTickerChange"_q, u"Сменить"_q },
		{ u"xgram_CryptoTickerHide"_q, u"Скрыть курс"_q },
		{ u"xgram_AccountHubHeader"_q, u"Хаб аккаунтов"_q },
		{ u"xgram_AccountHubMenu"_q, u"Хаб аккаунтов"_q },
		{ u"xgram_AccountHubManage"_q, u"Управление аккаунтами"_q },
		{ u"xgram_AccountHubLogoutTitle"_q, u"Выход из выбранных аккаунтов"_q },
		{ u"xgram_AccountHubSelectLogout"_q, u"Выбрать аккаунты для выхода"_q },
		{ u"xgram_AccountHubLogoutDescription"_q, u"Выберите аккаунты Telegram, из которых нужно выйти на этом устройстве."_q },
		{ u"xgram_AccountHubLogoutAction"_q, u"Выйти из выбранных"_q },
		{ u"xgram_AccountHubLogoutConfirm"_q, u"Выйти из выбранных аккаунтов на этом устройстве?"_q },
		{ u"xgram_AccountHubChooseAccount"_q, u"Выберите хотя бы один аккаунт."_q },
		{ u"xgram_AccountHubEmpty"_q, u"Чаты ещё не загружены."_q },
		{ u"xgram_RegistrationDateUnavailable"_q, u"Примерная дата регистрации недоступна."_q },
		{ u"xgram_GlobalPinsHeader"_q, u"Общие закрепы"_q },
		{ u"xgram_GlobalPinsEnabled"_q, u"Включить общие закрепы"_q },
		{ u"xgram_GlobalPinsMenu"_q, u"Общие закрепы"_q },
		{ u"xgram_GlobalPin"_q, u"Общий пин"_q },
		{ u"xgram_GlobalUnpin"_q, u"Убрать общий пин"_q },
		{ u"xgram_GlobalPinsEmpty"_q, u"Общих закрепов пока нет."_q },
		{ u"xgram_OfficialPurchasesHeader"_q, u"Официальные покупки Telegram"_q },
		{ u"xgram_OfficialPurchasesEnabled"_q, u"Показывать официальные покупки"_q },
		{ u"xgram_BuyStars"_q, u"Купить звёзды Telegram"_q },
		{ u"xgram_BuyPremium"_q, u"Купить Telegram Premium"_q },
		{ u"xgram_ProfilePreviewHeader"_q, u"Предпросмотр профиля"_q },
		{ u"xgram_ProfilePreviewDescription"_q, u"Локально показывает выбранный баланс Stars и подарки, не меняя настоящий баланс Telegram и не создавая транзакций."_q },
		{ u"xgram_ProfilePreviewEnabled"_q, u"Включить предпросмотр профиля"_q },
		{ u"xgram_PreviewStarsBalance"_q, u"Эмуляция баланса Stars"_q },
		{ u"xgram_PreviewGiftsCount"_q, u"Эмуляция подарков"_q },
		{ u"xgram_OpenProfilePreview"_q, u"Открыть предпросмотр профиля"_q },
		{ u"xgram_PersistDeletedMedia"_q, u"Сохранять удалённые медиа после перезапуска"_q },
		{ u"xgram_SuppressBlockedGroupNotifications"_q, u"Отключать уведомления от заблокированных в группах"_q },
		{ u"xgram_PerChatGhostMode"_q, u"Режим призрака для отдельных чатов"_q },
		{ u"xgram_EnableGhostModeInChat"_q, u"Включить режим призрака в этом чате"_q },
		{ u"xgram_DisableGhostModeInChat"_q, u"Выключить режим призрака в этом чате"_q },
		{ u"xgram_ClearSavedDataMenuText"_q, u"Очистить сохранённые данные"_q },
		{ u"xgram_OpenSavedMedia"_q, u"Открыть сохранённые медиа"_q },
		{ u"xgram_ClearSavedDataText"_q, u"Удалить все локально сохранённые удалённые сообщения, одноразовые фотографии и видео?"_q },
		{ u"xgram_SendAsOneTimeMedia"_q, u"Отправить как одноразовое медиа"_q },
		{ u"xgram_AddContactShort"_q, u"Добавить"_q },
		{ u"xgram_DeleteSelectedContacts"_q, u"Удалить выбранные"_q },
		{ u"xgram_DeleteSelectedContactsText"_q, u"Удалить выбранные контакты из этого аккаунта Telegram?"_q },
		{ u"xgram_ChooseContacts"_q, u"Выберите хотя бы один контакт."_q },
		{ u"xgram_WebviewHeader"_q, u"Веб-приложения"_q },
		{ u"xgram_DisableGreetingSticker"_q, u"Отключить приветственный стикер"_q },
		{ u"xgram_FiltersSheetDialogsToResolve#one"_q, u"**{count}** диалог для обработки"_q },
		{ u"xgram_FiltersSheetDialogsToResolve#few"_q, u"**{count}** диалога для обработки"_q },
		{ u"xgram_FiltersSheetDialogsToResolve#many"_q, u"**{count}** диалогов для обработки"_q },
		{ u"xgram_FiltersSheetDialogsToResolve#other"_q, u"**{count}** диалогов для обработки"_q },
		{ u"xgram_FiltersSheetNewExclusions#one"_q, u"**{count}** новое исключение"_q },
		{ u"xgram_FiltersSheetNewExclusions#few"_q, u"**{count}** новых исключения"_q },
		{ u"xgram_FiltersSheetNewExclusions#many"_q, u"**{count}** новых исключений"_q },
		{ u"xgram_FiltersSheetNewExclusions#other"_q, u"**{count}** новых исключений"_q },
		{ u"xgram_FiltersSheetNewFilters#one"_q, u"**{count}** новый фильтр"_q },
		{ u"xgram_FiltersSheetNewFilters#few"_q, u"**{count}** новых фильтра"_q },
		{ u"xgram_FiltersSheetNewFilters#many"_q, u"**{count}** новых фильтров"_q },
		{ u"xgram_FiltersSheetNewFilters#other"_q, u"**{count}** новых фильтров"_q },
		{ u"xgram_FiltersSheetRemovedExclusions#one"_q, u"**{count}** удалённое исключение"_q },
		{ u"xgram_FiltersSheetRemovedExclusions#few"_q, u"**{count}** удалённых исключения"_q },
		{ u"xgram_FiltersSheetRemovedExclusions#many"_q, u"**{count}** удалённых исключений"_q },
		{ u"xgram_FiltersSheetRemovedExclusions#other"_q, u"**{count}** удалённых исключений"_q },
		{ u"xgram_FiltersSheetRemovedFilters#one"_q, u"**{count}** удалённый фильтр"_q },
		{ u"xgram_FiltersSheetRemovedFilters#few"_q, u"**{count}** удалённых фильтра"_q },
		{ u"xgram_FiltersSheetRemovedFilters#many"_q, u"**{count}** удалённых фильтров"_q },
		{ u"xgram_FiltersSheetRemovedFilters#other"_q, u"**{count}** удалённых фильтров"_q },
		{ u"xgram_FiltersSheetUpdatedFilters#one"_q, u"**{count}** обновлённый фильтр"_q },
		{ u"xgram_FiltersSheetUpdatedFilters#few"_q, u"**{count}** обновлённых фильтра"_q },
		{ u"xgram_FiltersSheetUpdatedFilters#many"_q, u"**{count}** обновлённых фильтров"_q },
		{ u"xgram_FiltersSheetUpdatedFilters#other"_q, u"**{count}** обновлённых фильтров"_q },
	};

	for (const auto &[key, value] : translations) {
		Lang::GetInstance().resetValue(key.toUtf8());
		Lang::GetInstance().applyValue(key.toUtf8(), value.toUtf8());
	}
}

} // namespace

XGramLanguage *XGramLanguage::instance = nullptr;

XGramLanguage::XGramLanguage() = default;

void XGramLanguage::init() {
	if (!instance) instance = new XGramLanguage;
	instance->loadCachedLanguage();
	ApplyRussianXGramTranslations();
	Lang::GetInstance().updatePluralRules();
}

XGramLanguage *XGramLanguage::currentInstance() {
	return instance;
}

QString XGramLanguage::getCacheDir() const {
	return cWorkingDir() + u"tdata/xgram/languages/"_q;
}

QString XGramLanguage::getCachePath(const QString &langId) const {
	return getCacheDir() + langId + u".json"_q;
}

void XGramLanguage::loadCachedLanguage() {
	const auto langPackId = Lang::GetInstance().id();
	const auto langPackBaseId = Lang::GetInstance().baseId();
	auto finalLangPackId = langMapping.contains(langPackId) ? langMapping[langPackId] : langPackId;

	if (finalLangPackId.isEmpty()) {
		finalLangPackId = langPackBaseId;
	}
	if (finalLangPackId.isEmpty()) {
		return;
	}

	const auto legacyCachePath = [&](const QString &id) {
		return cWorkingDir() + u"tdata/xgram/languages/"_q + id + u".json"_q;
	};
	const auto cachePath = getCachePath(finalLangPackId);
	QFile file(cachePath);
	auto fromLegacyPath = false;
	if (!file.exists()) {
		const auto basePath = getCachePath(langPackBaseId);
		if (QFile::exists(basePath)) {
			file.setFileName(basePath);
		} else if (QFile::exists(legacyCachePath(finalLangPackId))) {
			file.setFileName(legacyCachePath(finalLangPackId));
			fromLegacyPath = true;
		} else if (QFile::exists(legacyCachePath(langPackBaseId))) {
			file.setFileName(legacyCachePath(langPackBaseId));
			fromLegacyPath = true;
		} else {
			return;
		}
	}

	if (file.open(QIODevice::ReadOnly)) {
		const auto data = file.readAll();
		file.close();

		QJsonParseError error{};
		const auto doc = QJsonDocument::fromJson(data, &error);
		if (error.error == QJsonParseError::NoError) {
			LOG(("Loading cached XGram language: %1").arg(finalLangPackId));
			applyLanguageJson(doc);
			if (fromLegacyPath) {
				saveCachedLanguage(data, finalLangPackId);
				QFile::remove(file.fileName());
			}
		}
	}
}

void XGramLanguage::saveCachedLanguage(const QByteArray &json, const QString &langId) {
	const auto cacheDir = getCacheDir();
	QDir().mkpath(cacheDir);

	const auto cachePath = getCachePath(langId);
	QFile file(cachePath);
	if (file.open(QIODevice::WriteOnly)) {
		auto branded = json;
		branded.replace("exteraGram", "XGram");
		file.write(branded);
		file.close();
		LOG(("Cached XGram language: %1").arg(langId));
	}
}

void XGramLanguage::fetchLanguage(const QString &id, const QString &baseId) {
	auto finalLangPackId = langMapping.contains(id) ? langMapping[id] : id;
	_currentLangId = finalLangPackId.isEmpty() ? baseId : finalLangPackId;

	if (Core::App().settings().proxy().isEnabled()) {
		const auto proxy = Core::App().settings().proxy().selected();
		if (proxy.type == MTP::ProxyData::Type::Socks5 || proxy.type == MTP::ProxyData::Type::Http) {
			const auto networkProxy = ToNetworkProxy(ToDirectIpProxy(Core::App().settings().proxy().selected()));
			networkManager.setProxy(networkProxy);
		}
	}

	// using `jsdelivr` since China (...and maybe other?) users have some problems with GitHub
	// https://crowdin.com/project/xgram/discussions/6
	QUrl url;
	if (!finalLangPackId.isEmpty() && !baseId.isEmpty() && !needFallback) {
		url.setUrl(qsl("https://cdn.jsdelivr.net/gh/XGram/Languages@l10n_main/values/langs/%1/Shared.json").arg(
			finalLangPackId));
	} else {
		url.setUrl(qsl("https://cdn.jsdelivr.net/gh/XGram/Languages@l10n_main/values/langs/%1/Shared.json").arg(
			needFallback ? baseId : finalLangPackId));
	}
	_chkReply = networkManager.get(QNetworkRequest(url));
	connect(_chkReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(fetchError(QNetworkReply::NetworkError)));
	connect(_chkReply, SIGNAL(finished()), this, SLOT(fetchFinished()));
}

void XGramLanguage::fetchFinished() {
	if (!_chkReply) return;

	QString langPackBaseId = Lang::GetInstance().baseId();
	QString langPackId = Lang::GetInstance().id();
	auto statusCode = _chkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

	if (statusCode == 404 && !langPackId.isEmpty() && !langPackBaseId.isEmpty() && !needFallback) {
		LOG(("XGram language not found! Fallback to main language: %1...").arg(langPackBaseId));
		needFallback = true;
		_chkReply->disconnect();
		fetchLanguage("", langPackBaseId);
	} else {
		const auto result = _chkReply->readAll().trimmed();
		QJsonParseError error{};
		const auto doc = QJsonDocument::fromJson(result, &error);
		if (error.error == QJsonParseError::NoError) {
			saveCachedLanguage(result, _currentLangId);
			applyLanguageJson(doc);
		} else {
			LOG(("Incorrect language JSON File."));
		}

		_chkReply = nullptr;
	}
}

void XGramLanguage::fetchError(QNetworkReply::NetworkError e) {
	LOG(("Network error: %1").arg(e));

	if (e == QNetworkReply::NetworkError::ContentNotFoundError) {
		const auto baseId = Lang::GetInstance().baseId();
		const auto id = Lang::GetInstance().id();

		if (!id.isEmpty() && !baseId.isEmpty() && !needFallback) {
			LOG(("XGram language not found! Fallback to main language: %1...").arg(baseId));
			needFallback = true;
			_chkReply->disconnect();
			fetchLanguage("", baseId);
		} else {
			LOG(("XGram language not found!"));
			_chkReply = nullptr;
		}
	}
}

void XGramLanguage::applyLanguageJson(QJsonDocument doc) {
	const auto json = doc.object();
	for (const QString &brokenKey : json.keys()) {
		auto key = qsl("xgram_") + brokenKey;
		auto val = json.value(brokenKey).toString().replace(qsl("&amp;"), qsl("&"));
		// Custom language packs keep the product name in their translated text.
		// Apply the XGram brand consistently for every supported language.
		val.replace(qsl("exteraGram"), qsl("XGram"), Qt::CaseInsensitive);

		if (key.endsWith("_Android")) {
			continue;
		}

		for (const auto &postfix : postfixes) {
			if (key.endsWith(qsl("_") + postfix)) {
				key = key.replace(qsl("_") + postfix, qsl("#") + postfix);
				break;
			}
		}

		if (key.endsWith("_PC")) {
			key = key.replace("_PC", "");
		}

		if (val.contains(qsl("%1$d")) && !val.contains(qsl("%2$d"))) {
			val = val.replace(qsl("%1$d"), qsl("{count}"));
		} else if (val.contains(qsl("%1$d")) && val.contains(qsl("%2$d"))) {
			val = val.replace(qsl("%1$d"), qsl("{count1}")).replace(qsl("%2$d"), qsl("{count2}"));
		} else if (val.contains(qsl("%1$s")) && !val.contains(qsl("%2$s"))) {
			val = val.replace(qsl("%1$s"), qsl("{item}"));
		} else if (val.contains(qsl("%1$s")) && val.contains(qsl("%2$s"))) {
			val = val.replace(qsl("%1$s"), qsl("{item1}")).replace(qsl("%2$s"), qsl("{item2}"));
		}

		Lang::GetInstance().resetValue(key.toUtf8());
		Lang::GetInstance().applyValue(key.toUtf8(), val.toUtf8());
	}
	ApplyRussianXGramTranslations();
	Lang::GetInstance().updatePluralRules();
}
