// This is the source code of XGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "xgram/ui/settings/settings_general.h"

#include "lang_auto.h"
#include "xgram/xgram_settings.h"
#include "xgram/ui/settings/xgram_builder.h"
#include "xgram/ui/settings/settings_xgram_utils.h"
#include "xgram/ui/settings/settings_main.h"
#include "base/platform/base_platform_info.h"
#include "core/application.h"
#include "lang/lang_text_entity.h"
#include "platform/platform_translate_provider.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"
#include "ui/boxes/single_choice_box.h"
#include "ui/toast/toast.h"
#include "ui/widgets/buttons.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_controller.h"
#include "window/window_session_controller.h"

namespace Settings {

using namespace Builder;
using namespace XGramBuilder;

namespace {

void BuildTranslator(SectionBuilder &builder, XGramSectionBuilder &xgram) {
	builder.addSubsectionTitle(tr::lng_translate_settings_subtitle());

	auto *settings = &XGramSettings::getInstance();

	const auto options = std::vector{
		std::pair(TranslationProvider::Telegram, QString("Telegram")),
		std::pair(TranslationProvider::Google, QString("Google")),
		std::pair(TranslationProvider::Yandex, QString("Yandex")),
	};
	const auto nativeAvailable = Platform::IsTranslateProviderAvailable();
	auto availableOptions = options;
	if (nativeAvailable) {
		availableOptions.push_back(std::pair(
			TranslationProvider::Native,
			[] {
				if constexpr (Platform::IsMac()) {
					return QString("macOS");
				} else if constexpr (Platform::IsWindows()) {
					return QString("Windows");
				} else {
					return QString("Linux");
				}
			}()));
	}
	auto optionLabels = std::vector<QString>();
	optionLabels.reserve(availableOptions.size());
	for (const auto &option : availableOptions) {
		optionLabels.push_back(option.second);
	}

	const auto getIndex = [=](TranslationProvider val) {
		const auto i = ranges::find(
			availableOptions,
			val,
			&std::pair<TranslationProvider, QString>::first);
		return (i != end(availableOptions))
			? int(i - begin(availableOptions))
			: 0;
	};

	auto currentVal = XGramSettings::getInstance().translationProviderValue()
		| rpl::map(getIndex)
		| rpl::map([=](int val) { return availableOptions[val].second; });

	const auto button = builder.addButton({
		.id = u"xgram/translationProvider"_q,
		.title = tr::xgram_TranslationProvider(),
		.st = &st::settingsButtonNoIcon,
		.label = std::move(currentVal),
		.onClick = [=] {
			if (const auto controller = Core::App().activeWindow()->sessionController()) {
				controller->show(Box(
						[=](not_null<Ui::GenericBox*> box) {
							const auto save = [=](int index) {
								const auto option = availableOptions[index].first;
								XGramSettings::getInstance().setTranslationProvider(option);

								if constexpr (Platform::IsMac()) {
									if (option == TranslationProvider::Native) {
										controller->showToast(Ui::Toast::Config{
											.text = tr::lng_translate_settings_use_platform_mac_about(tr::now, tr::rich),
											.duration = 6 * crl::time(1000)
										});
									}
								}
							};
							SingleChoiceBox(box, {
								.title = tr::xgram_TranslationProvider(),
								.options = optionLabels,
								.initialSelection = getIndex(settings->translationProvider()),
								.callback = save,
							});
						}));
			}
		},
	});
	if (button) {
		xgram.addBetaBadge(button);
	}
}

void BuildShowPeerId(SectionBuilder &builder) {
	auto *settings = &XGramSettings::getInstance();

	const auto options = std::vector{
		QString(tr::xgram_SettingsShowID_Hide(tr::now)),
		QString("Telegram API"),
		QString("Bot API")
	};

	auto currentVal = XGramSettings::getInstance().showPeerIdValue()
		| rpl::map([=](PeerIdDisplay val) {
			return options[static_cast<int>(val)];
		});

	const auto controller = builder.controller();
	builder.addButton({
		.id = u"xgram/showPeerId"_q,
		.altIds = { u"xgram/showIdAndDc"_q },
		.title = tr::xgram_SettingsShowID(),
		.st = &st::settingsButtonNoIcon,
		.label = std::move(currentVal),
		.onClick = [=] {
			controller->show(Box(
				[=](not_null<Ui::GenericBox*> box) {
					const auto save = [=](int index) {
						XGramSettings::getInstance().setShowPeerId(
							static_cast<PeerIdDisplay>(index));
					};
					SingleChoiceBox(box, {
						.title = tr::xgram_SettingsShowID(),
						.options = options,
						.initialSelection = static_cast<int>(settings->showPeerId()),
						.callback = save,
					});
				}));
		},
	});
}

void BuildQoLToggles(SectionBuilder &builder, XGramSectionBuilder &xgram) {
	auto *settings = &XGramSettings::getInstance();

	BuildTranslator(builder, xgram);
	xgram.addSectionDivider();

	builder.addSubsectionTitle(tr::xgram_CategoryGeneral());

	const auto controller = builder.controller();
	xgram.addToggle({
		.id = u"xgram/disableStories"_q,
		.altIds = { u"xgram/hideStories"_q },
		.title = tr::xgram_DisableStories(),
		.getter = [=] { return settings->disableStories(); },
		.setter = [=](bool enabled) {
			XGramSettings::getInstance().setDisableStories(enabled);
			ShowRestartPrompt(controller);
		},
	});

	xgram.addSettingToggle({
		.id = u"xgram/disableOpenLinkWarning"_q,
		.title = tr::xgram_DisableOpenLinkWarning(),
		.getter = &XGramSettings::disableOpenLinkWarning,
		.setter = &XGramSettings::setDisableOpenLinkWarning,
	});

	xgram.addCollapsibleToggle({
		.id = u"xgram/similarChannels"_q,
		.title = tr::xgram_DisableSimilarChannels(),
		.checkboxes = {
			NestedEntry{
				tr::xgram_CollapseSimilarChannels(tr::now),
				[] { return XGramSettings::getInstance().collapseSimilarChannels(); },
				[](bool v) { XGramSettings::getInstance().setCollapseSimilarChannels(v); }
			},
			NestedEntry{
				tr::xgram_HideSimilarChannelsTab(tr::now),
				[] { return XGramSettings::getInstance().hideSimilarChannels(); },
				[](bool v) { XGramSettings::getInstance().setHideSimilarChannels(v); }
			}
		},
		.toggledWhenAll = true,
	});

	xgram.addSettingToggle({
		.id = u"xgram/disableNotificationsDelay"_q,
		.title = tr::xgram_DisableNotificationsDelay(),
		.getter = &XGramSettings::disableNotificationsDelay,
		.setter = &XGramSettings::setDisableNotificationsDelay,
	});

	xgram.addSectionDivider();

	const auto zalgoButton = builder.addButton({
		.id = u"xgram/filterZalgo"_q,
		.title = tr::xgram_FilterZalgo(),
		.st = &st::settingsButtonNoIcon,
		.toggled = rpl::single(settings->filterZalgo()),
	});
	if (zalgoButton) {
		zalgoButton->toggledValue(
		) | rpl::filter(
			[=](bool enabled) {
				return (enabled != settings->filterZalgo());
			}
		) | on_next(
			[=](bool enabled) {
				XGramSettings::getInstance().setFilterZalgo(enabled);
				ShowRestartPrompt(controller);
			},
			zalgoButton->lifetime());
		xgram.addBetaBadge(zalgoButton);
	}

	xgram.addSettingToggle({
		.id = u"xgram/improveLinkPreviews"_q,
		.title = tr::xgram_ImproveLinkPreviews(),
		.getter = &XGramSettings::improveLinkPreviews,
		.setter = &XGramSettings::setImproveLinkPreviews,
	});
	xgram.addSettingToggle({
		.id = u"xgram/showMessageSeconds"_q,
		.altIds = { u"xgram/formatTimeWithSeconds"_q },
		.title = tr::xgram_SettingsShowMessageSeconds(),
		.getter = &XGramSettings::showMessageSeconds,
		.setter = &XGramSettings::setShowMessageSeconds,
	});

	BuildShowPeerId(builder);

	xgram.addSectionDivider();

	builder.addSubsectionTitle(tr::xgram_WebviewHeader());

	xgram.addSettingToggle({
		.id = u"xgram/spoofWebviewAsAndroid"_q,
		.title = tr::xgram_SettingsSpoofWebviewAsAndroid(),
		.getter = &XGramSettings::spoofWebviewAsAndroid,
		.setter = &XGramSettings::setSpoofWebviewAsAndroid,
	});

	xgram.addCollapsibleToggle({
		.id = u"xgram/biggerWindow"_q,
		.title = tr::xgram_SettingsBiggerWindow(),
		.checkboxes = {
			NestedEntry{
				tr::xgram_SettingsIncreaseWebviewHeight(tr::now),
				[] { return XGramSettings::getInstance().increaseWebviewHeight(); },
				[](bool v) { XGramSettings::getInstance().setIncreaseWebviewHeight(v); }
			},
			NestedEntry{
				tr::xgram_SettingsIncreaseWebviewWidth(tr::now),
				[] { return XGramSettings::getInstance().increaseWebviewWidth(); },
				[](bool v) { XGramSettings::getInstance().setIncreaseWebviewWidth(v); }
			}
		},
		.toggledWhenAll = false,
	});

	xgram.addSectionDivider();

	builder.addSubsectionTitle(tr::xgram_ConfirmationsTitle());

	xgram.addSettingToggle({
		.id = u"xgram/stickerConfirmation"_q,
		.title = tr::xgram_StickerConfirmation(),
		.getter = &XGramSettings::stickerConfirmation,
		.setter = &XGramSettings::setStickerConfirmation,
	});
	xgram.addSettingToggle({
		.id = u"xgram/gifConfirmation"_q,
		.title = tr::xgram_GIFConfirmation(),
		.getter = &XGramSettings::gifConfirmation,
		.setter = &XGramSettings::setGifConfirmation,
	});
	xgram.addSettingToggle({
		.id = u"xgram/voiceConfirmation"_q,
		.title = tr::xgram_VoiceConfirmation(),
		.getter = &XGramSettings::voiceConfirmation,
		.setter = &XGramSettings::setVoiceConfirmation,
	});
}

const auto kMeta = BuildHelper({
	.id = XGramGeneral::Id(),
	.parentId = XGramMain::Id(),
	.title = &tr::xgram_CategoryGeneral,
	.icon = &st::menuIconShowAll,
}, [](SectionBuilder &builder) {
	auto xgram = XGramSectionBuilder(builder);

	builder.addSkip();
	BuildQoLToggles(builder, xgram);
	builder.addSkip();
});

} // namespace

rpl::producer<QString> XGramGeneral::title() {
	return tr::xgram_CategoryGeneral();
}

XGramGeneral::XGramGeneral(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void XGramGeneral::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type XGramGeneralId() {
	return XGramGeneral::Id();
}

} // namespace Settings
