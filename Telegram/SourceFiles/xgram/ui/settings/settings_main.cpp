// This is the source code of XGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "xgram/ui/settings/settings_main.h"

#include "settings/sections/settings_main.h"
#include "lang_auto.h"
#include "xgram/xgram_settings.h"
#include "xgram/xgram/xgram_branding.h"
#include "xgram/ui/xgram_logo.h"
#include "xgram/ui/settings/settings_appearance.h"
#include "xgram/ui/settings/settings_xgram.h"
#include "xgram/ui/settings/settings_chats.h"
#include "xgram/ui/settings/settings_filters.h"
#include "xgram/ui/settings/settings_general.h"
#include "xgram/ui/settings/settings_other.h"
#include "core/version.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "styles/style_xgram_settings.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"
#include "ui/painter.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"
#include "window/window_session_controller_link_info.h"

#include <QDesktopServices>

namespace Settings {

using namespace Builder;

namespace {

void BuildLogo(SectionBuilder &builder) {
	builder.add([](const WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		auto logo = object_ptr<Ui::RpWidget>(ctx.container);
		const auto logoRaw = logo.data();
		logoRaw->resize(
			QSize(st::settingsCloudPasswordIconSize,
				st::settingsCloudPasswordIconSize));
		logoRaw->setNaturalWidth(st::settingsCloudPasswordIconSize);
		logoRaw->paintRequest(
		) | rpl::on_next([=] {
			auto p = QPainter(logoRaw);
			const auto image = XGramAssets::currentAppLogoPad();
			if (!image.isNull()) {
				const auto size = st::settingsCloudPasswordIconSize;
				const auto scaled = image.scaled(
					size * style::DevicePixelRatio(),
					size * style::DevicePixelRatio(),
					Qt::KeepAspectRatio,
					Qt::SmoothTransformation);
				p.drawImage(QRect(0, 0, size, size), scaled);
			}
		}, logoRaw->lifetime());
		return { .widget = std::move(logo), .align = style::al_top };
	});
}

void BuildVersionInfo(SectionBuilder &builder) {
	builder.add([](const WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		return {
			.widget = object_ptr<Ui::FlatLabel>(
				ctx.container,
				rpl::single(
					XGram::Branding::kDesktopName + QStringLiteral(" v")
					+ QString::fromLatin1(AppVersionStr)),
				st::boxTitle),
			.align = style::al_top,
		};
	});

	builder.addSkip();

	builder.add([](const WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		return {
			.widget = object_ptr<Ui::FlatLabel>(
				ctx.container,
				tr::xgram_SettingsDescription(),
				st::centeredBoxLabel),
			.align = style::al_top,
		};
	});
}

void BuildCategories(SectionBuilder &builder) {
	builder.addSkip();
	builder.addSkip();
	builder.addSkip();
	builder.addSkip();
	builder.addDivider();
	builder.addSkip();

	builder.addSubsectionTitle(tr::xgram_CategoriesHeader());

	builder.addSectionButton({
		.title = rpl::single(XGram::Branding::kName),
		.targetSection = XGramGhost::Id(),
		.icon = { &st::menuIconGroupReactions },
	});
	builder.addSectionButton({
		.title = tr::xgram_CategoryFilters(),
		.targetSection = XGramFilters::Id(),
		.icon = { &st::menuIconTagFilter },
	});
	builder.addSectionButton({
		.title = tr::xgram_CategoryGeneral(),
		.targetSection = XGramGeneral::Id(),
		.icon = { &st::menuIconShowAll },
	});
	builder.addSectionButton({
		.title = tr::xgram_CategoryAppearance(),
		.targetSection = XGramAppearance::Id(),
		.icon = { &st::menuIconPalette },
	});
	builder.addSectionButton({
		.title = tr::xgram_CategoryChats(),
		.targetSection = XGramChats::Id(),
		.icon = { &st::menuIconChatBubble },
	});
	builder.addSectionButton({
		.title = tr::xgram_CategoryOther(),
		.targetSection = XGramOther::Id(),
		.icon = { &st::menuIconFave },
	});
}

void BuildLinks(SectionBuilder &builder) {
	builder.addSkip();
	builder.addDivider();
	builder.addSkip();

	builder.addSubsectionTitle(tr::xgram_LinksHeader());

	const auto controller = builder.controller();

	builder.addButton({
		.id = u"xgram/channel"_q,
		.title = tr::xgram_LinksChannel(),
		.icon = { &st::menuIconChannel },
		.label = rpl::single(
			QString("@") + XGram::Branding::kChannelUsername),
		.onClick = [=] {
			controller->showPeerByLink(Window::PeerByLinkInfo{
				.usernameOrId = XGram::Branding::kChannelUsername,
			});
		},
	});
	builder.addButton({
		.id = u"xgram/chat"_q,
		.title = tr::xgram_LinksChats(),
		.icon = { &st::menuIconChats },
		.label = rpl::single(XGram::Branding::kCommunityChatLabel),
		.onClick = [=] {
			if (XGram::Branding::kCommunityChatUsername.isEmpty()) {
				return;
			}
			controller->showPeerByLink(Window::PeerByLinkInfo{
				.usernameOrId = XGram::Branding::kCommunityChatUsername,
			});
		},
	});
	builder.addButton({
		.id = u"xgram/crowdin"_q,
		.title = tr::xgram_LinksTranslate(),
		.icon = { &st::menuIconTranslate },
		.label = rpl::single(XGram::Branding::kTranslationLabel),
		.onClick = [=] {
			QDesktopServices::openUrl(
				XGram::Branding::kTranslationUrl);
		},
	});
	builder.addButton({
		.id = u"xgram/website"_q,
		.title = tr::xgram_LinksDocumentation(),
		.icon = { &st::menuIconIpAddress },
		.label = rpl::single(XGram::Branding::kDocumentationLabel),
		.onClick = [=] {
			if (XGram::Branding::kDocumentationUrl.isEmpty()) {
				return;
			}
			QDesktopServices::openUrl(
				XGram::Branding::kDocumentationUrl);
		},
	});

	builder.addSkip();
}

const auto kMeta = BuildHelper({
	.id = XGramMain::Id(),
	.parentId = MainId(),
	.title = &tr::xgram_XGramPreferences,
	.icon = &st::menuIconPremium,
}, [](SectionBuilder &builder) {
	BuildLogo(builder);
	builder.addSkip();
	BuildVersionInfo(builder);
	BuildCategories(builder);
	BuildLinks(builder);
});

} // namespace

rpl::producer<QString> XGramMain::title() {
	return rpl::single(QString(""));
}

XGramMain::XGramMain(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void XGramMain::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type XGramMainId() {
	return XGramMain::Id();
}

} // namespace Settings
