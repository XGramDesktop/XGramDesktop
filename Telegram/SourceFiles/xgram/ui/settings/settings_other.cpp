// This is the source code of XGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "xgram/ui/settings/settings_other.h"

#include "lang_auto.h"
#include "xgram/xgram_settings.h"
#include "xgram/ui/boxes/donate_qr_box.h"
#include "xgram/ui/settings/xgram_builder.h"
#include "xgram/ui/settings/settings_xgram_utils.h"
#include "xgram/ui/settings/settings_main.h"
#include "boxes/abstract_box.h"
#include "core/application.h"
#include "lang/lang_text_entity.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"
#include "ui/integration.h"
#include "ui/painter.h"
#include "ui/rect.h"
#include "ui/vertical_list.h"
#include "ui/boxes/confirm_box.h"
#include "ui/text/text_utilities.h"
#include "ui/widgets/buttons.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"
#include "window/themes/window_theme.h"

#include <QDesktopServices>
#include <QGuiApplication>
#include <QSvgRenderer>

namespace Settings {

using namespace Builder;
using namespace XGramBuilder;

namespace {

struct Asset {
	QString icon;
	QColor background;
};

Asset getAsset(const QString &name) {
	const auto isNightMode = Window::Theme::IsNightMode();
	const auto normalized = name.toLower();
	QString icon = QString(":/gui/icons/xgram/donates/%1.svg").arg(normalized);
	QColor background = isNightMode ? QColor(0xEEEEEE) : QColor(0x242B2C);
	return {
		.icon = std::move(icon),
		.background = std::move(background)
	};
}

QImage getImage(const QString &name) {
	const auto iconData = getAsset(name);

	const auto factor = style::DevicePixelRatio();
	const auto size = st::menuIconLink.size();
	auto image = QImage(
		size * factor,
		QImage::Format_ARGB32_Premultiplied);
	image.setDevicePixelRatio(factor);
	image.fill(Qt::transparent);
	{
		auto p = QPainter(&image);
		auto hq = PainterHighQualityEnabler(p);

		p.setPen(Qt::NoPen);
		p.setBrush(iconData.background);
		p.drawRoundedRect(Rect(size), size.width() / 4., size.height() / 4.);
		p.setBrush(Qt::transparent);

		auto svgIcon = QSvgRenderer(iconData.icon);
		svgIcon.render(&p, Rect(size));
	}

	return image;
}

[[nodiscard]] not_null<Ui::SettingsButton*> AddDonate(
		not_null<Ui::SettingsButton*> button,
		const QString &name) {
	const auto btnContainer = Ui::CreateChild<Ui::RpWidget>(button);
	const auto &buttonSt = button->st();
	const auto fullHeight = buttonSt.height
		+ rect::m::sum::v(buttonSt.padding);

	const auto iconWidget = Ui::CreateChild<Ui::RpWidget>(button.get());

	auto icon = getImage(name);
	iconWidget->resize(icon.size() / style::DevicePixelRatio());
	iconWidget->paintRequest(
	) | rpl::on_next([=] {
		auto p = QPainter(iconWidget);
		p.drawImage(0, 0, icon);
	}, iconWidget->lifetime());

	button->sizeValue(
	) | rpl::on_next([=](const QSize &s) {
		iconWidget->moveToLeft(
			button->st().iconLeft
				+ (st::menuIconShop.width() - iconWidget->width()) / 2,
			(s.height() - iconWidget->height()) / 2);
		btnContainer->moveToLeft(
			iconWidget->x() - (fullHeight - iconWidget->height()) / 2,
			0);
	}, iconWidget->lifetime());

	btnContainer->resize(fullHeight, fullHeight);

	return button;
}

void AddCryptoDonate(
		const QString &name,
		const QString &address,
		not_null<Ui::VerticalLayout*> container) {
	const auto button = AddDonate(
		AddButtonWithIcon(
			container,
			rpl::single(name),
			st::settingsButton),
		name);
	button->setClickedCallback([=] {
		auto box = Box(
			Ui::FillDonateQrBox,
			address,
			getAsset(name).icon);
		Ui::show(std::move(box));
	});
}

void BuildDonations(SectionBuilder &builder) {
	builder.add([](const BuildContext &ctx) {
		v::match(ctx, [&](const WidgetContext &wctx) {
			const auto container = wctx.container;

			AddSubsectionTitle(container, tr::xgram_SupportHeader());
			AddDonate(
				AddButtonWithIcon(
					container,
					rpl::single(QString("Boosty")),
					st::settingsButton),
				"boosty"
			)->setClickedCallback([=] {
				QDesktopServices::openUrl(
					QString("https://boosty.to/killmeifilose"));
			});
			AddCryptoDonate("TON", QString("UQBoJLE-FCKb1J69ckWUbSgmNgzKObrhqfM85E5_19fGU4Hv"), container);
			AddCryptoDonate("Bitcoin", QString("bc1qrl65vls8df3g3le4gmzy7t5f3zw5uchuucz2ed"), container);
			AddCryptoDonate("Ethereum", QString("0x583AED945e2Cffc437B42742672bbb2e20B58934"), container);
			AddCryptoDonate("Solana", QString("7Wd23ddxutwj5zUFxmsVtJPKYStK898ZRutUUiryoSHp"), container);
			AddCryptoDonate("Tron", QString("TW84rGw7LSs5qnFfipTeARVd5A9sYXX39o"), container);
			AddSkip(container);

			AddDividerText(container,
				tr::xgram_SupportDescription2(
					lt_item,
					rpl::single(
						Ui::Text::Link(tr::xgram_SupportDescription1(tr::now), QString("tg://support"))
					),
					tr::marked));
		}, [&](const SearchContext &sctx) {
			sctx.entries->push_back({
				.id = u"xgram/donate"_q,
				.title = tr::xgram_SupportHeader(tr::now),
				.section = sctx.sectionId,
			});
		});
	});
}

void BuildCrashReporting(SectionBuilder &builder, XGramSectionBuilder &xgram) {
#ifndef TDESKTOP_DISABLE_AUTOUPDATE
	builder.addSkip();
	builder.addSubsectionTitle(tr::xgram_CategoryOther());

	xgram.addSettingToggle({
		.id = u"xgram/crashReporting"_q,
		.altIds = { u"xgram/crashlytics"_q },
		.title = tr::xgram_CrashReporting(),
		.getter = &XGramSettings::crashReporting,
		.setter = &XGramSettings::setCrashReporting,
		.icon = { &st::menuIconReport },
	});
	builder.addSkip();
	builder.addDividerText(tr::xgram_CrashReportingDescription());
#endif
}

void BuildOtherThings(SectionBuilder &builder) {
	const auto controller = builder.controller();

	builder.addSkip();
	builder.addButton({
		.id = u"xgram/registerUrlScheme"_q,
		.title = tr::xgram_RegisterURLScheme(),
		.icon = { &st::menuIconLink },
		.onClick = [=] {
			Core::Application::RegisterUrlScheme();
			controller->showToast(tr::lng_box_done(tr::now));
		},
	});
	builder.addButton({
		.id = u"xgram/resetSettings"_q,
		.title = tr::xgram_ResetSettings(),
		.icon = { &st::menuIconRestore },
		.onClick = [=] {
			controller->show(Ui::MakeConfirmBox({
				.text = tr::xgram_ResetSettingsConfirmation(tr::rich),
				.confirmed = [=](Fn<void()> &&close) {
					XGramSettings::reset();
					controller->showToast(tr::lng_box_done(tr::now));
					close();
				},
				.confirmText = tr::lng_box_yes(),
			}));
		},
	});
	builder.addSkip();
}

const auto kMeta = BuildHelper({
	.id = XGramOther::Id(),
	.parentId = XGramMain::Id(),
	.title = &tr::xgram_CategoryOther,
	.icon = &st::menuIconFave,
}, [](SectionBuilder &builder) {
	auto xgram = XGramSectionBuilder(builder);

	builder.addSkip();
	BuildDonations(builder);
	BuildCrashReporting(builder, xgram);
	BuildOtherThings(builder);
});

} // namespace

rpl::producer<QString> XGramOther::title() {
	return tr::xgram_CategoryOther();
}

XGramOther::XGramOther(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void XGramOther::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type XGramOtherId() {
	return XGramOther::Id();
}

} // namespace Settings
