// This is the source code of XGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "xgram/xgram_infra.h"

#include "xgram/xgram_lang.h"
#include "xgram/xgram_settings.h"
#include "xgram/xgram_ui_settings.h"
#include "xgram/xgram_worker.h"
#include "xgram/data/xgram_database.h"
#include "xgram/ui/xgram_logo.h"
#include "features/translator/xgram_translator.h"
#include "lang/lang_instance.h"
#include "ui/chat/chat_style_radius.h"
#include "utils/rc_manager.h"

#ifdef Q_OS_WIN
#include "xgram/utils/windows_utils.h"
#endif

namespace XGramInfra {

void initLang() {
	QString id = Lang::GetInstance().id();
	QString baseId = Lang::GetInstance().baseId();
	if (id.isEmpty()) {
		LOG(("Language is not loaded"));
		return;
	}
	XGramLanguage::init();
	XGramLanguage::currentInstance()->fetchLanguage(id, baseId);
}

void initUiSettings() {
	const auto &settings = XGramSettings::getInstance();

	XGramUiSettings::setMonoFont(settings.monoFont());
	XGramUiSettings::setWideMultiplier(settings.wideMultiplier());
	XGramUiSettings::setMaterialSwitches(settings.materialSwitches());
	XGramUiSettings::setAvatarCorners(settings.avatarCorners());
	Ui::SetAppliedBubbleRadius(settings.messageBubbleRadius());
}

void initDatabase() {
	XGramDatabase::initialize();
}

void initWorker() {
	XGramWorker::initialize();
}

void initRCManager() {
	RCManager::getInstance().start();
}

void initTranslator() {
	XGram::Translator::TranslateManager::init();
}

void initIcon() {
#ifdef Q_OS_WIN
	XGramAssets::loadAppIco();
	reloadAppIconFromTaskBar();
#endif
}

void init() {
	initLang();
	initDatabase();
	initUiSettings();
	initIcon();
	initWorker();
	initRCManager();
	initTranslator();
}

}
