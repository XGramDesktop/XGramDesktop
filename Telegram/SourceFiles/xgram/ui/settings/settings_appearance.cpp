// This is the source code of XGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "xgram/ui/settings/settings_appearance.h"

#include "lang_auto.h"
#include "xgram/xgram_settings.h"
#include "xgram/xgram_ui_settings.h"
#include "xgram/ui/boxes/font_selector.h"
#include "xgram/ui/components/avatar_corners_preview.h"
#include "xgram/ui/components/icon_picker.h"
#include "xgram/ui/settings/xgram_builder.h"
#include "xgram/ui/settings/settings_xgram_utils.h"
#include "xgram/ui/settings/settings_main.h"
#include "inline_bots/bot_attach_web_view.h"
#include "main/main_session.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "styles/style_xgram_icons.h"
#include "styles/style_xgram_styles.h"
#include "styles/style_dialogs.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"
#include "ui/painter.h"
#include "ui/toast/toast.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/padding_wrap.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"

namespace Settings {

using namespace Builder;
using namespace XGramBuilder;

namespace {

bool HasDrawerBots(not_null<Window::SessionController*> controller) {
	// todo: maybe iterate through all accounts
	const auto bots = &controller->session().attachWebView();
	for (const auto &bot : bots->attachBots()) {
		if (!bot.inMainMenu || !bot.media) {
			continue;
		}
		return true;
	}
	return false;
}

void BuildAppIcon(SectionBuilder &builder, XGramSectionBuilder &xgram) {
	builder.addSubsectionTitle({
		.id = u"xgram/appIcon"_q,
		.title = tr::xgram_AppIconHeader(),
	});

	builder.add([](const WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		return {
			.widget = object_ptr<IconPicker>(ctx.container),
			.margin = st::settingsButtonNoIcon.padding,
		};
	});

#if defined Q_OS_WIN || defined Q_OS_MAC
	builder.addDivider();
	builder.addSkip();
	xgram.addSettingToggle({
		.id = u"xgram/hideNotificationBadge"_q,
		.title = tr::xgram_HideNotificationBadge(),
		.getter = &XGramSettings::hideNotificationBadge,
		.setter = &XGramSettings::setHideNotificationBadge,
	});
	builder.addSkip();
	builder.addDividerText(tr::xgram_HideNotificationBadgeDescription());
	builder.addSkip();
#else
    builder.addDivider();
    builder.addSkip();
#endif
}

void BuildAvatarCorners(SectionBuilder &builder, XGramSectionBuilder &xgram) {
	auto *settings = &XGramSettings::getInstance();
	const auto controller = builder.controller();

	const auto mapRadius = [](int val)
	{
		if (val == 0) {
			return tr::xgram_AvatarCornersSquare(tr::now).toUpper();
		} else if (val == XGramUiSettings::kMaxAvatarCorners) {
			return tr::xgram_AvatarCornersCircle(tr::now).toUpper();
		}
		return QString::number(val);
	};

	builder.add([=](const WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		const auto container = ctx.container;
		auto title = object_ptr<Ui::FlatLabel>(
			container,
			tr::xgram_AvatarCorners(),
			st::defaultSubsectionTitle);
		const auto titleRaw = title.data();

		const auto badge = Ui::CreateChild<Ui::PaddingWrap<Ui::FlatLabel>>(
			container,
			object_ptr<Ui::FlatLabel>(
				container,
				settings->avatarCornersValue() | rpl::map(mapRadius),
				st::settingsPremiumNewBadge),
			st::xgramBetaBadgePadding);
		badge->show();
		badge->setAttribute(Qt::WA_TransparentForMouseEvents);
		badge->paintRequest() | rpl::on_next([=] {
			auto p = QPainter(badge);
			auto hq = PainterHighQualityEnabler(p);
			p.setPen(Qt::NoPen);
			p.setBrush(st::windowBgActive);
			const auto r = st::xgramBetaBadgePadding.left();
			p.drawRoundedRect(badge->rect(), r, r);
		}, badge->lifetime());

		titleRaw->geometryValue() | rpl::on_next([=](QRect geometry) {
			badge->moveToLeft(
				geometry.x()
					+ titleRaw->textMaxWidth()
					+ st::settingsPremiumNewBadgePosition.x(),
				geometry.y()
					+ (geometry.height() - badge->height()) / 2);
		}, badge->lifetime());

		return {
			.widget = std::move(title),
			.margin = st::defaultSubsectionTitlePadding,
		};
	}, [] {
		return SearchEntry{
			.id = u"xgram/avatarCorners"_q,
			.title = tr::xgram_AvatarCorners(tr::now),
		};
	});

	auto *previewRaw = static_cast<AvatarCornersPreview*>(nullptr);
	builder.add([&](const Builder::WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		auto preview = object_ptr<AvatarCornersPreview>(
			ctx.container,
			controller);
		previewRaw = preview.data();
		const auto vMargin = st::settingsButtonNoIcon.padding
			- st::defaultDialogRow.padding;
		return {
			.widget = std::move(preview),
			.margin = QMargins(0, vMargin.top(), 0, vMargin.bottom()),
		};
	});

	xgram.addSlider({
		.id = u"xgram/avatarCornersSlider"_q,
		.title = rpl::single(QString()),
		.showTitle = false,
		.steps = XGramUiSettings::kMaxAvatarCorners + 1,
		.current = settings->avatarCorners(),
		.onChanged = [=](int val) {
			XGramSettings::getInstance().setAvatarCorners(val);
			if (previewRaw) {
				previewRaw->update();
			}
		},
		.onFinalChanged = [=](int val) {
			XGramSettings::getInstance().setAvatarCorners(val);
			ShowRestartPrompt(controller);
		},
	});

	xgram.addSettingToggle({
		.id = u"xgram/singleCornerRadius"_q,
		.title = tr::xgram_SingleCornerRadius(),
		.getter = &XGramSettings::singleCornerRadius,
		.setter = &XGramSettings::setSingleCornerRadius,
	});

	builder.addSkip();
	builder.addDividerText(tr::xgram_SingleCornerRadiusDescription());
	builder.addSkip();
}

void BuildAppearance(SectionBuilder &builder, XGramSectionBuilder &xgram) {
	auto *settings = &XGramSettings::getInstance();

	builder.addSubsectionTitle(tr::xgram_CategoryAppearance());

	xgram.addSettingToggle({
		.id = u"xgram/materialSwitches"_q,
		.altIds = { u"xgram/newSwitchStyle"_q },
		.title = tr::xgram_MaterialSwitches(),
		.getter = &XGramSettings::materialSwitches,
		.setter = &XGramSettings::setMaterialSwitches,
	});
	xgram.addSettingToggle({
		.id = u"xgram/disableCustomBackgrounds"_q,
		.altIds = { u"xgram/customThemes"_q },
		.title = tr::xgram_DisableCustomBackgrounds(),
		.getter = &XGramSettings::disableCustomBackgrounds,
		.setter = &XGramSettings::setDisableCustomBackgrounds,
	});
	xgram.addSettingToggle({
		.id = u"xgram/hidePremiumStatuses"_q,
		.title = tr::xgram_HidePremiumStatuses(),
		.getter = &XGramSettings::hidePremiumStatuses,
		.setter = &XGramSettings::setHidePremiumStatuses,
	});

	const auto controller = builder.controller();
	builder.addButton({
		.id = u"xgram/monoFont"_q,
		.title = tr::xgram_MonospaceFont(),
		.st = &st::settingsButtonNoIcon,
		.label = rpl::single(
			settings->monoFont().isEmpty()
				? tr::xgram_FontDefault(tr::now)
				: settings->monoFont()),
		.onClick = [=] {
			XGramUi::FontSelectorBox::Show(
				controller,
				[=](const QString &font) {
					XGramSettings::getInstance().setMonoFont(font);
					ShowRestartPrompt(controller);
				});
		},
	});

	xgram.addSectionDivider();
}

void BuildChatFolders(SectionBuilder &builder, XGramSectionBuilder &xgram) {
	builder.addSubsectionTitle(tr::xgram_ChatFoldersHeader());

	xgram.addSettingToggle({
		.id = u"xgram/hideNotificationCounters"_q,
		.altIds = { u"xgram/tabCounter"_q },
		.title = tr::xgram_HideNotificationCounters(),
		.getter = &XGramSettings::hideNotificationCounters,
		.setter = &XGramSettings::setHideNotificationCounters,
	});
	xgram.addSettingToggle({
		.id = u"xgram/hideAllChatsFolder"_q,
		.altIds = { u"xgram/hideAllChats"_q },
		.title = tr::xgram_HideAllChats(),
		.getter = &XGramSettings::hideAllChatsFolder,
		.setter = &XGramSettings::setHideAllChatsFolder,
	});

	xgram.addSectionDivider();
}

void BuildTrayElements(SectionBuilder &builder, XGramSectionBuilder &xgram) {
	builder.addSubsectionTitle(tr::xgram_TrayElementsHeader());

	xgram.addSettingToggle({
		.id = u"xgram/showGhostToggleInTray"_q,
		.title = tr::xgram_EnableGhostModeTray(),
		.getter = &XGramSettings::showGhostToggleInTray,
		.setter = &XGramSettings::setShowGhostToggleInTray,
	});

#if defined Q_OS_WIN || defined Q_OS_MAC
	xgram.addSettingToggle({
		.id = u"xgram/showStreamerToggleInTray"_q,
		.title = tr::xgram_EnableStreamerModeTray(),
		.getter = &XGramSettings::showStreamerToggleInTray,
		.setter = &XGramSettings::setShowStreamerToggleInTray,
	});
#endif

	xgram.addSectionDivider();
}

void BuildDrawerElements(SectionBuilder &builder, XGramSectionBuilder &xgram) {
	builder.addSubsectionTitle(tr::xgram_DrawerElementsHeader());

	xgram.addSettingToggle({
		.id = u"xgram/showMyProfileInDrawer"_q,
		.title = tr::lng_menu_my_profile(),
		.getter = &XGramSettings::showMyProfileInDrawer,
		.setter = &XGramSettings::setShowMyProfileInDrawer,
		.icon = { &st::menuIconProfile },
	});

	const auto controller = builder.controller();
	if (controller && HasDrawerBots(controller)) {
		xgram.addSettingToggle({
			.id = u"xgram/showBotsInDrawer"_q,
			.title = tr::lng_filters_type_bots(),
			.getter = &XGramSettings::showBotsInDrawer,
			.setter = &XGramSettings::setShowBotsInDrawer,
			.icon = { &st::menuIconBot },
		});
	}

	xgram.addSettingToggle({
		.id = u"xgram/showNewGroupInDrawer"_q,
		.title = tr::lng_create_group_title(),
		.getter = &XGramSettings::showNewGroupInDrawer,
		.setter = &XGramSettings::setShowNewGroupInDrawer,
		.icon = { &st::menuIconGroups },
	});
	xgram.addSettingToggle({
		.id = u"xgram/showNewChannelInDrawer"_q,
		.title = tr::lng_create_channel_title(),
		.getter = &XGramSettings::showNewChannelInDrawer,
		.setter = &XGramSettings::setShowNewChannelInDrawer,
		.icon = { &st::menuIconChannel },
	});
	xgram.addSettingToggle({
		.id = u"xgram/showContactsInDrawer"_q,
		.title = tr::lng_menu_contacts(),
		.getter = &XGramSettings::showContactsInDrawer,
		.setter = &XGramSettings::setShowContactsInDrawer,
		.icon = { &st::menuIconUserShow },
	});
	xgram.addSettingToggle({
		.id = u"xgram/showCallsInDrawer"_q,
		.title = tr::lng_menu_calls(),
		.getter = &XGramSettings::showCallsInDrawer,
		.setter = &XGramSettings::setShowCallsInDrawer,
		.icon = { &st::menuIconPhone },
	});
	xgram.addSettingToggle({
		.id = u"xgram/showSavedMessagesInDrawer"_q,
		.title = tr::lng_saved_messages(),
		.getter = &XGramSettings::showSavedMessagesInDrawer,
		.setter = &XGramSettings::setShowSavedMessagesInDrawer,
		.icon = { &st::menuIconSavedMessages },
	});
	xgram.addSettingToggle({
		.id = u"xgram/showLReadToggleInDrawer"_q,
		.title = tr::xgram_LReadMessages(),
		.getter = &XGramSettings::showLReadToggleInDrawer,
		.setter = &XGramSettings::setShowLReadToggleInDrawer,
		.icon = { &st::xgramLReadMenuIcon },
	});
	xgram.addSettingToggle({
		.id = u"xgram/showSReadToggleInDrawer"_q,
		.title = tr::xgram_SReadMessages(),
		.getter = &XGramSettings::showSReadToggleInDrawer,
		.setter = &XGramSettings::setShowSReadToggleInDrawer,
		.icon = { &st::xgramSReadMenuIcon },
	});
	xgram.addSettingToggle({
		.id = u"xgram/showNightModeToggleInDrawer"_q,
		.title = tr::lng_menu_night_mode(),
		.getter = &XGramSettings::showNightModeToggleInDrawer,
		.setter = &XGramSettings::setShowNightModeToggleInDrawer,
		.icon = { &st::menuIconNightMode },
	});
	xgram.addSettingToggle({
		.id = u"xgram/showGhostToggleInDrawer"_q,
		.title = tr::xgram_GhostModeToggle(),
		.getter = &XGramSettings::showGhostToggleInDrawer,
		.setter = &XGramSettings::setShowGhostToggleInDrawer,
		.icon = { &st::xgramGhostIcon },
	});

#if defined Q_OS_WIN || defined Q_OS_MAC
	xgram.addSettingToggle({
		.id = u"xgram/showStreamerToggleInDrawer"_q,
		.title = tr::xgram_StreamerModeToggle(),
		.getter = &XGramSettings::showStreamerToggleInDrawer,
		.setter = &XGramSettings::setShowStreamerToggleInDrawer,
		.icon = { &st::xgramStreamerModeMenuIcon },
	});
#endif

	builder.addSkip();
}

const auto kMeta = BuildHelper({
	.id = XGramAppearance::Id(),
	.parentId = XGramMain::Id(),
	.title = &tr::xgram_CategoryAppearance,
	.icon = &st::menuIconPalette,
}, [](SectionBuilder &builder) {
	auto xgram = XGramSectionBuilder(builder);

	builder.addSkip();
	BuildAppIcon(builder, xgram);
	BuildAvatarCorners(builder, xgram);
	BuildAppearance(builder, xgram);
	BuildChatFolders(builder, xgram);
	BuildTrayElements(builder, xgram);
	BuildDrawerElements(builder, xgram);
	builder.addSkip();
});

} // namespace

rpl::producer<QString> XGramAppearance::title() {
	return tr::xgram_CategoryAppearance();
}

XGramAppearance::XGramAppearance(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void XGramAppearance::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type XGramAppearanceId() {
	return XGramAppearance::Id();
}

} // namespace Settings
