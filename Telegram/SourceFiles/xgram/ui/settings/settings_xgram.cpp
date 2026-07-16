// This is the source code of XGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "xgram/ui/settings/settings_xgram.h"

#include "lang_auto.h"
#include "xgram/xgram_settings.h"
#include "xgram/ui/xgram_userpic.h"
#include "xgram/ui/settings/xgram_builder.h"
#include "xgram/ui/settings/settings_xgram_utils.h"
#include "xgram/ui/settings/settings_main.h"
#include "boxes/peer_list_box.h"
#include "core/application.h"
#include "data/data_user.h"
#include "main/main_account.h"
#include "main/main_domain.h"
#include "main/main_session.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "settings/sections/settings_credits.h"
#include "settings/sections/settings_premium.h"
#include "styles/style_xgram_icons.h"
#include "styles/style_xgram_styles.h"
#include "styles/style_chat.h"
#include "styles/style_chat_helpers.h"
#include "styles/style_layers.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"
#include "styles/style_window.h"
#include "ui/painter.h"
#include "ui/boxes/confirm_box.h"
#include "ui/vertical_list.h"
#include "ui/boxes/single_choice_box.h"
#include "ui/text/text.h"
#include "ui/toast/toast.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/checkbox.h"
#include "ui/widgets/labels.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/widgets/popup_menu.h"
#include "ui/widgets/menu/menu_item_base.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"

namespace Settings {

using namespace Builder;
using namespace XGramBuilder;

namespace {

struct GhostPickerState {
	rpl::variable<uint64> selectedUserId;
	base::unique_qptr<Ui::PopupMenu> menu;
	Ui::LinkButton *pickerButton = nullptr;
	Fn<void()> refreshCheckboxes;
};

struct AccountUserpicGeometry {
	QRect outer;
	QRect inner;
};

[[nodiscard]] AccountUserpicGeometry AccountUserpic(int height) {
	const auto line = st::mainMenuAccountLine;
	const auto skip = 2 * line + st::lineWidth;
	const auto full = st::mainMenuAccountSize + 2 * skip;
	const auto outer = QRect(
		st::defaultWhoRead.photoLeft
			+ (st::defaultWhoRead.photoSize - full) / 2,
		(height - full) / 2,
		full,
		full);
	return {
		.outer = outer,
		.inner = QRect(
			outer.x() + skip,
			outer.y() + skip,
			st::mainMenuAccountSize,
			st::mainMenuAccountSize),
	};
}

void PaintAccountOutline(Painter &p, QRect outer) {
	const auto line = st::mainMenuAccountLine;
	const auto shift = st::lineWidth + (line * 0.5);
	const auto rect = QRectF(outer).marginsRemoved(QMarginsF(
		shift,
		shift,
		shift,
		shift));
	auto hq = PainterHighQualityEnabler(p);
	auto pen = st::windowBgActive->p;
	pen.setWidthF(line);
	p.setPen(pen);
	p.setBrush(Qt::NoBrush);
	XGramUserpic::PaintShape(p, rect);
}

class AccountAction final : public Ui::Menu::ItemBase {
public:
	AccountAction(
		not_null<Ui::Menu::Menu*> parent,
		const style::Menu &st,
		UserData *user,
		bool active,
		Fn<void()> callback)
	: ItemBase(parent, st)
	, _dummyAction(Ui::CreateChild<QAction>(parent.get()))
	, _user(user)
	, _active(active)
	, _st(st)
	, _height(st::defaultWhoRead.photoSkip * 2 + st::defaultWhoRead.photoSize) {
		setAcceptBoth(true);
		fitToMenuWidth();

		_text.setText(_st.itemStyle, user->name());
		const auto goodWidth = st::defaultWhoRead.nameLeft
			+ _text.maxWidth()
			+ _st.itemPadding.right();
		setMinWidth(std::clamp(goodWidth, _st.widthMin, _st.widthMax));

		setActionTriggered(std::move(callback));

		paintRequest(
		) | rpl::on_next([=] {
			paint(Painter(this));
		}, lifetime());

		enableMouseSelecting();
	}

	not_null<QAction*> action() const override { return _dummyAction; }
	bool isEnabled() const override { return true; }

protected:
	int contentHeight() const override { return _height; }

private:
	void paint(Painter &&p) {
		const auto selected = isSelected();
		if (selected && _st.itemBgOver->c.alpha() < 255) {
			p.fillRect(0, 0, width(), _height, _st.itemBg);
		}
		const auto bg = selected ? _st.itemBgOver : _st.itemBg;
		p.fillRect(0, 0, width(), _height, bg);
		if (isEnabled()) {
			paintRipple(p, 0, 0);
		}

		const auto userpic = AccountUserpic(_height);
		_user->paintUserpicLeft(
			p,
			_userpicView,
			userpic.inner.x(),
			userpic.inner.y(),
			width(),
			userpic.inner.width());
		if (_active) {
			PaintAccountOutline(p, userpic.outer);
		}

		p.setPen(selected ? _st.itemFgOver : _st.itemFg);
		_text.drawLeftElided(
			p,
			st::defaultWhoRead.nameLeft,
			(_height - _st.itemStyle.font->height) / 2,
			width() - st::defaultWhoRead.nameLeft - _st.itemPadding.right(),
			width());
	}

	const not_null<QAction*> _dummyAction;
	UserData *_user = nullptr;
	mutable Ui::PeerUserpicView _userpicView;
	const bool _active = false;
	const style::Menu &_st;
	Ui::Text::String _text;
	const int _height;
};

class GlobalAction final : public Ui::Menu::ItemBase {
public:
	GlobalAction(
		not_null<Ui::Menu::Menu*> parent,
		const style::Menu &st,
		const QString &text,
		bool active,
		Fn<void()> callback)
	: ItemBase(parent, st)
	, _dummyAction(Ui::CreateChild<QAction>(parent.get()))
	, _active(active)
	, _st(st)
	, _height(st::defaultWhoRead.photoSkip * 2 + st::defaultWhoRead.photoSize) {
		setAcceptBoth(true);
		fitToMenuWidth();

		_text.setText(_st.itemStyle, text);
		const auto goodWidth = st::defaultWhoRead.nameLeft
			+ _text.maxWidth()
			+ _st.itemPadding.right();
		setMinWidth(std::clamp(goodWidth, _st.widthMin, _st.widthMax));

		setActionTriggered(std::move(callback));

		paintRequest(
		) | rpl::on_next([=] {
			paint(Painter(this));
		}, lifetime());

		enableMouseSelecting();
	}

	not_null<QAction*> action() const override { return _dummyAction; }
	bool isEnabled() const override { return true; }

protected:
	int contentHeight() const override { return _height; }

private:
	void paint(Painter &&p) {
		const auto selected = isSelected();
		if (selected && _st.itemBgOver->c.alpha() < 255) {
			p.fillRect(0, 0, width(), _height, _st.itemBg);
		}
		const auto bg = selected ? _st.itemBgOver : _st.itemBg;
		p.fillRect(0, 0, width(), _height, bg);
		if (isEnabled()) {
			paintRipple(p, 0, 0);
		}

		const auto userpic = AccountUserpic(_height);
		{
			auto hq = PainterHighQualityEnabler(p);
			auto rect = QRectF(userpic.inner);
			auto gradient = QLinearGradient(rect.topLeft(), rect.bottomLeft());
			gradient.setStops({
				{ 0., st::historyPeer5UserpicBg->c },
				{ 1., st::historyPeer5UserpicBg2->c },
			});
			p.setPen(Qt::NoPen);
			p.setBrush(gradient);
			XGramUserpic::PaintShape(p, rect);
		}
		{
			auto hq = PainterHighQualityEnabler(p);
			p.drawImage(
				userpic.inner,
				st::xgramGhostModeGlobalIcon.instance(st::historyPeerUserpicFg->c));
		}
		if (_active) {
			PaintAccountOutline(p, userpic.outer);
		}

		p.setPen(selected ? _st.itemFgOver : _st.itemFg);
		_text.drawLeftElided(
			p,
			st::defaultWhoRead.nameLeft,
			(_height - _st.itemStyle.font->height) / 2,
			width() - st::defaultWhoRead.nameLeft - _st.itemPadding.right(),
			width());
	}

	const not_null<QAction*> _dummyAction;
	const bool _active = false;
	const style::Menu &_st;
	Ui::Text::String _text;
	const int _height;
};

QString GetAccountName(uint64 userId) {
	for (const auto &account : Core::App().domain().orderedAccounts()) {
		if (account->sessionExists()
			&& account->session().userId().bare == userId) {
			return account->session().user()->name();
		}
	}
	return QString("Unknown");
}

QString PickerLabel(uint64 userId) {
	return (userId == 0)
		? tr::xgram_GhostModeGlobalSettings(tr::now)
		: GetAccountName(userId);
}

void selectGhostProfile(GhostPickerState *state, uint64 userId) {
	if (state->selectedUserId.current() == userId) {
		return;
	}

	auto wasGlobal = (state->selectedUserId.current() == 0);
	auto nowGlobal = (userId == 0);

	XGramSettings::getInstance().setUseGlobalGhostMode(nowGlobal);

	state->selectedUserId = userId;

	state->pickerButton->setText(PickerLabel(userId));

	state->refreshCheckboxes();

	if (wasGlobal != nowGlobal) {
		Ui::Toast::Show(nowGlobal
			? tr::xgram_GhostModeSwitchedToGlobalSettings(tr::now)
			: tr::xgram_GhostModeSwitchedToIndividualSettings(tr::now));
	}
}

void BuildGhostEssentials(SectionBuilder &builder) {
	builder.add([](const BuildContext &ctx) {
		v::match(ctx, [&](const WidgetContext &wctx) {
			const auto container = wctx.container;
			const auto controller = wctx.controller;

			auto activeCount = 0;
			for (const auto &account : Core::App().domain().orderedAccounts()) {
				if (account->sessionExists()) {
					++activeCount;
				}
			}

			if (activeCount <= 1 && !XGramSettings::getInstance().useGlobalGhostMode()) {
				auto userId = controller->session().userId().bare;
				auto &src = XGramSettings::ghost(userId);
				auto &dst = XGramSettings::ghost(0);
				dst.setSendReadMessages(src.sendReadMessages());
				dst.setSendReadStories(src.sendReadStories());
				dst.setSendOnlinePackets(src.sendOnlinePackets());
				dst.setSendUploadProgress(src.sendUploadProgress());
				dst.setSendOfflinePacketAfterOnline(src.sendOfflinePacketAfterOnline());
				dst.setMarkReadAfterAction(src.markReadAfterAction());
				dst.setUseScheduledMessages(src.useScheduledMessages());
				dst.setSendWithoutSound(src.sendWithoutSound());
				dst.setSuggestGhostModeBeforeViewingStory(src.suggestGhostModeBeforeViewingStory());
				dst.setSendReadMessagesLocked(src.sendReadMessagesLocked());
				dst.setSendReadStoriesLocked(src.sendReadStoriesLocked());
				dst.setSendOnlinePacketsLocked(src.sendOnlinePacketsLocked());
				dst.setSendUploadProgressLocked(src.sendUploadProgressLocked());
				dst.setSendOfflinePacketAfterOnlineLocked(src.sendOfflinePacketAfterOnlineLocked());
				XGramSettings::getInstance().setUseGlobalGhostMode(true);
			}

			const auto isGlobal = XGramSettings::getInstance().useGlobalGhostMode();
			auto initialUserId = isGlobal
				? uint64(0)
				: controller->session().userId().bare;

			const auto state = container->lifetime().make_state<GhostPickerState>();
			state->selectedUserId = initialUserId;

			const auto title = AddSubsectionTitle(container, tr::xgram_GhostEssentialsHeader());

			const auto pickerButton = Ui::CreateChild<Ui::LinkButton>(
				container.get(),
				PickerLabel(initialUserId),
				st::ghostPickerButton);
			state->pickerButton = pickerButton;

			const auto arrow = Ui::CreateChild<Ui::AbstractButton>(container.get());
			{
				const auto &icon = st::ghostPickerArrow;
				arrow->resize(icon.size());
				arrow->paintRequest(
				) | rpl::on_next([=, &icon] {
					auto p = QPainter(arrow);
					icon.paint(p, 0, 0, arrow->width());
				}, arrow->lifetime());
			}
			arrow->setCursor(style::cur_pointer);

			const auto showPicker = activeCount > 1;
			pickerButton->setVisible(showPicker);
			arrow->setVisible(showPicker);

			rpl::combine(
				title->geometryValue(),
				container->widthValue(),
				pickerButton->naturalWidthValue()
			) | rpl::on_next([=](QRect r, int width, int natural) {
				pickerButton->resizeToNaturalWidth(width / 2);
				pickerButton->moveToRight(
					st::defaultSubsectionTitlePadding.right() + arrow->width() + st::normalFont->spacew / 2,
					r.y() + (r.height() - pickerButton->height()) / 2,
					width);
				arrow->moveToLeft(
					pickerButton->x() + pickerButton->width() + st::normalFont->spacew / 2,
					r.y() + (r.height() - arrow->height()) / 2);
			}, pickerButton->lifetime());

			std::vector checkboxes{
				NestedEntry{
					tr::xgram_DontReadMessages(tr::now),
					[state] { return !XGramSettings::ghost(state->selectedUserId.current()).sendReadMessages(); },
					[state](bool v) { XGramSettings::ghost(state->selectedUserId.current()).setSendReadMessages(!v); },
					[state] { return XGramSettings::ghost(state->selectedUserId.current()).sendReadMessagesLocked(); },
					[state](bool v) { XGramSettings::ghost(state->selectedUserId.current()).setSendReadMessagesLocked(v); }
				},
				NestedEntry{
					tr::xgram_DontReadStories(tr::now),
					[state] { return !XGramSettings::ghost(state->selectedUserId.current()).sendReadStories(); },
					[state](bool v) { XGramSettings::ghost(state->selectedUserId.current()).setSendReadStories(!v); },
					[state] { return XGramSettings::ghost(state->selectedUserId.current()).sendReadStoriesLocked(); },
					[state](bool v) { XGramSettings::ghost(state->selectedUserId.current()).setSendReadStoriesLocked(v); }
				},
				NestedEntry{
					tr::xgram_DontSendOnlinePackets(tr::now),
					[state] { return !XGramSettings::ghost(state->selectedUserId.current()).sendOnlinePackets(); },
					[state](bool v) { XGramSettings::ghost(state->selectedUserId.current()).setSendOnlinePackets(!v); },
					[state] { return XGramSettings::ghost(state->selectedUserId.current()).sendOnlinePacketsLocked(); },
					[state](bool v) { XGramSettings::ghost(state->selectedUserId.current()).setSendOnlinePacketsLocked(v); }
				},
				NestedEntry{
					tr::xgram_DontSendUploadProgress(tr::now),
					[state] { return !XGramSettings::ghost(state->selectedUserId.current()).sendUploadProgress(); },
					[state](bool v) { XGramSettings::ghost(state->selectedUserId.current()).setSendUploadProgress(!v); },
					[state] { return XGramSettings::ghost(state->selectedUserId.current()).sendUploadProgressLocked(); },
					[state](bool v) { XGramSettings::ghost(state->selectedUserId.current()).setSendUploadProgressLocked(v); }
				},
				NestedEntry{
					tr::xgram_SendOfflinePacketAfterOnline(tr::now),
					[state] { return XGramSettings::ghost(state->selectedUserId.current()).sendOfflinePacketAfterOnline(); },
					[state](bool v) { XGramSettings::ghost(state->selectedUserId.current()).setSendOfflinePacketAfterOnline(v); },
					[state] { return XGramSettings::ghost(state->selectedUserId.current()).sendOfflinePacketAfterOnlineLocked(); },
					[state](bool v) { XGramSettings::ghost(state->selectedUserId.current()).setSendOfflinePacketAfterOnlineLocked(v); }
				},
			};

			auto collapsible = AddCollapsibleToggle(
				container,
				tr::xgram_GhostModeToggle(),
				std::move(checkboxes),
				true,
				tr::xgram_GhostModeOptionShiftDescription(tr::now));
			state->refreshCheckboxes = std::move(collapsible.refresh);
			if (wctx.highlights && collapsible.widget) {
				wctx.highlights->push_back(std::make_pair(
					u"xgram/ghostModeToggle"_q,
					HighlightEntry{ collapsible.widget, {} }));
			}

			const auto markReadButton = AddButtonWithIcon(
				container,
				tr::xgram_MarkReadAfterAction(),
				st::settingsButtonNoIcon
			);
			if (wctx.highlights) {
				wctx.highlights->push_back(std::make_pair(
					u"xgram/markReadAfterAction"_q,
					HighlightEntry{ markReadButton.get(), {} }));
			}
			markReadButton->toggleOn(
				state->selectedUserId.value()
				| rpl::map([](uint64 id) {
					return XGramSettings::ghost(id).markReadAfterActionValue();
				}) | rpl::flatten_latest()
			)->toggledValue(
			) | rpl::filter(
				[=](bool enabled) {
					return enabled != XGramSettings::ghost(state->selectedUserId.current()).markReadAfterAction();
				}
			) | on_next(
				[=](bool enabled) {
					auto &ghost = XGramSettings::ghost(state->selectedUserId.current());
					ghost.setMarkReadAfterAction(enabled);
					if (enabled) {
						ghost.setUseScheduledMessages(false);
					}
				},
				container->lifetime());
			AddSkip(container);
			AddDividerText(container, tr::xgram_MarkReadAfterActionDescription());

			AddSkip(container);
			const auto scheduleButton = AddButtonWithIcon(
				container,
				tr::xgram_UseScheduledMessages(),
				st::settingsButtonNoIcon
			);
			if (wctx.highlights) {
				wctx.highlights->push_back(std::make_pair(
					u"xgram/useScheduledMessages"_q,
					HighlightEntry{ scheduleButton.get(), {} }));
			}
			scheduleButton->toggleOn(
				state->selectedUserId.value()
				| rpl::map([](uint64 id) {
					return XGramSettings::ghost(id).useScheduledMessagesValue();
				}) | rpl::flatten_latest()
			)->toggledValue(
			) | rpl::filter(
				[=](bool enabled) {
					return enabled != XGramSettings::ghost(state->selectedUserId.current()).useScheduledMessages();
				}
			) | on_next(
				[=](bool enabled) {
					auto &ghost = XGramSettings::ghost(state->selectedUserId.current());
					ghost.setUseScheduledMessages(enabled);
					if (enabled) {
						ghost.setMarkReadAfterAction(false);
					}
				},
				container->lifetime());
			AddSkip(container);
			AddDividerText(container, tr::xgram_UseScheduledMessagesDescription());

			AddSkip(container);
			const auto silentOptions = std::vector<QString>{
				tr::xgram_SendWithoutSoundByDefaultNever(tr::now),
				tr::xgram_SendWithoutSoundByDefaultInGhostMode(tr::now),
				tr::xgram_SendWithoutSoundByDefaultAlways(tr::now),
			};
			const auto silentOptionText = state->selectedUserId.value(
			) | rpl::map([=](uint64 id) {
				return XGramSettings::ghost(id).sendWithoutSoundValue(
				) | rpl::map([=](SendWithoutSoundOption value) {
					return silentOptions[static_cast<int>(value)];
				});
			}) | rpl::flatten_latest();
			const auto silentButton = AddButtonWithLabel(
				container,
				tr::xgram_SendWithoutSoundByDefault(),
				std::move(silentOptionText),
				st::settingsButtonNoIcon);
			if (wctx.highlights) {
				wctx.highlights->push_back(std::make_pair(
					u"xgram/sendWithoutSound"_q,
					HighlightEntry{ silentButton.get(), {} }));
			}
			silentButton->addClickHandler([=] {
				controller->show(Box([=](not_null<Ui::GenericBox*> box) {
					const auto save = [=](int index) {
						XGramSettings::ghost(state->selectedUserId.current()
						).setSendWithoutSound(
							static_cast<SendWithoutSoundOption>(index));
					};
					SingleChoiceBox(box, {
						.title = tr::xgram_SendWithoutSoundByDefault(),
						.options = silentOptions,
						.initialSelection = static_cast<int>(
							XGramSettings::ghost(state->selectedUserId.current()
							).sendWithoutSound()),
						.callback = save,
					});
				}));
			});
			AddSkip(container);
			AddDividerText(container, tr::xgram_SendWithoutSoundByDefaultDescription());

			AddSkip(container);
			const auto suggestGhostModeButton = AddButtonWithIcon(
				container,
				tr::xgram_SuggestGhostModeBeforeViewingStory(),
				st::settingsButtonNoIcon);
			if (wctx.highlights) {
				wctx.highlights->push_back(std::make_pair(
					u"xgram/suggestGhostModeBeforeViewingStory"_q,
					HighlightEntry{ suggestGhostModeButton.get(), {} }));
			}
			suggestGhostModeButton->toggleOn(
				state->selectedUserId.value()
				| rpl::map([](uint64 id) {
					return XGramSettings::ghost(id).suggestGhostModeBeforeViewingStoryValue();
				}) | rpl::flatten_latest()
			)->toggledValue(
			) | rpl::filter(
				[=](bool enabled) {
					return enabled != XGramSettings::ghost(state->selectedUserId.current()).suggestGhostModeBeforeViewingStory();
				}
			) | on_next(
				[=](bool enabled) {
					XGramSettings::ghost(state->selectedUserId.current()).setSuggestGhostModeBeforeViewingStory(enabled);
				},
				container->lifetime());
			AddSkip(container);
			AddDividerText(container, tr::xgram_SuggestGhostModeBeforeViewingStoryDescription());

			auto showMenu = [=] {
				state->menu = base::make_unique_q<Ui::PopupMenu>(
					pickerButton,
					st::defaultPopupMenu);

				state->menu->addAction(
					base::make_unique_q<GlobalAction>(
						state->menu->menu(),
						st::defaultPopupMenu.menu,
						tr::xgram_GhostModeGlobalSettings(tr::now),
						state->selectedUserId.current() == 0,
						[=] { selectGhostProfile(state, 0); }));

				for (const auto &account : Core::App().domain().orderedAccounts()) {
					if (!account->sessionExists()) {
						continue;
					}
					auto user = account->session().user();
					auto id = account->session().userId().bare;
					state->menu->addAction(
						base::make_unique_q<AccountAction>(
							state->menu->menu(),
							st::defaultPopupMenu.menu,
							user,
							state->selectedUserId.current() == id,
							[=] { selectGhostProfile(state, id); }));
				}

				state->menu->popup(
					pickerButton->mapToGlobal(
						QPoint(pickerButton->width(), pickerButton->height())));
			};
			pickerButton->setClickedCallback(showMenu);
			arrow->setClickedCallback(showMenu);
		}, [&](const SearchContext &sctx) {
			sctx.entries->push_back({
				.id = u"xgram/ghostModeToggle"_q,
				.title = tr::xgram_GhostModeToggle(tr::now),
				.section = sctx.sectionId,
			});
			sctx.entries->push_back({
				.id = u"xgram/markReadAfterAction"_q,
				.title = tr::xgram_MarkReadAfterAction(tr::now),
				.section = sctx.sectionId,
			});
			sctx.entries->push_back({
				.id = u"xgram/useScheduledMessages"_q,
				.title = tr::xgram_UseScheduledMessages(tr::now),
				.section = sctx.sectionId,
			});
			sctx.entries->push_back({
				.id = u"xgram/sendWithoutSound"_q,
				.title = tr::xgram_SendWithoutSoundByDefault(tr::now),
				.section = sctx.sectionId,
			});
			sctx.entries->push_back({
				.id = u"xgram/suggestGhostModeBeforeViewingStory"_q,
				.title = tr::xgram_SuggestGhostModeBeforeViewingStory(tr::now),
				.section = sctx.sectionId,
			});
		});
	});
}

void BuildSpyEssentials(SectionBuilder &builder, XGramSectionBuilder &xgram) {
	builder.addSubsectionTitle(tr::xgram_SpyEssentialsHeader());

	xgram.addSettingToggle({
		.id = u"xgram/saveDeletedMessages"_q,
		.title = tr::xgram_SaveDeletedMessages(),
		.getter = &XGramSettings::saveDeletedMessages,
		.setter = &XGramSettings::setSaveDeletedMessages,
	});
	xgram.addSettingToggle({
		.id = u"xgram/perChatGhostMode"_q,
		.title = tr::xgram_PerChatGhostMode(),
		.getter = &XGramSettings::perChatGhostMode,
		.setter = &XGramSettings::setPerChatGhostMode,
	});
	xgram.addSettingToggle({
		.id = u"xgram/persistDeletedMedia"_q,
		.title = tr::xgram_PersistDeletedMedia(),
		.getter = &XGramSettings::persistDeletedMedia,
		.setter = &XGramSettings::setPersistDeletedMedia,
	});
	xgram.addSettingToggle({
		.id = u"xgram/saveMessagesHistory"_q,
		.title = tr::xgram_SaveMessagesHistory(),
		.getter = &XGramSettings::saveMessagesHistory,
		.setter = &XGramSettings::setSaveMessagesHistory,
	});

	xgram.addSectionDivider();

	xgram.addSettingToggle({
		.id = u"xgram/saveForBots"_q,
		.title = tr::xgram_MessageSavingSaveForBots(),
		.getter = &XGramSettings::saveForBots,
		.setter = &XGramSettings::setSaveForBots,
	});
}

void BuildOther(SectionBuilder &builder, XGramSectionBuilder &xgram) {
	builder.addSubsectionTitle(tr::xgram_MessageSavingOtherHeader());

	xgram.addSettingToggle({
		.id = u"xgram/localPremium"_q,
		.title = tr::xgram_LocalPremium(),
		.getter = &XGramSettings::localPremium,
		.setter = &XGramSettings::setLocalPremium,
	});
	xgram.addSettingToggle({
		.id = u"xgram/disableAds"_q,
		.title = tr::xgram_DisableAds(),
		.getter = &XGramSettings::disableAds,
		.setter = &XGramSettings::setDisableAds,
	});
}

void BuildCrypto(SectionBuilder &builder, XGramSectionBuilder &xgram) {
	builder.addSubsectionTitle(tr::xgram_CryptoHeader());

	xgram.addSettingToggle({
		.id = u"xgram/cryptoTickerEnabled"_q,
		.title = tr::xgram_CryptoTickerEnabled(),
		.getter = &XGramSettings::cryptoTickerEnabled,
		.setter = &XGramSettings::setCryptoTickerEnabled,
	});

	const auto symbols = std::vector<QString>{
		u"BTC"_q,
		u"ETH"_q,
		u"TON"_q,
		u"SOL"_q,
		u"BNB"_q,
		u"DOGE"_q,
	};
	const auto options = std::vector<QString>{
		u"Bitcoin (BTC)"_q,
		u"Ethereum (ETH)"_q,
		u"Toncoin (TON)"_q,
		u"Solana (SOL)"_q,
		u"BNB"_q,
		u"Dogecoin (DOGE)"_q,
	};
	const auto symbol = XGramSettings::getInstance().cryptoTickerSymbol();
	auto initial = 0;
	for (auto i = 0; i != int(symbols.size()); ++i) {
		if (symbols[i] == symbol) {
			initial = i;
			break;
		}
	}
	xgram.addChooseButton({
		.id = u"xgram/cryptoTickerSymbol"_q,
		.title = tr::xgram_CryptoTickerSymbol(),
		.boxTitle = tr::xgram_CryptoTickerChoose(),
		.initialSelection = initial,
		.options = options,
		.setter = [symbols](int index) {
			if (index >= 0 && index < int(symbols.size())) {
				XGramSettings::getInstance().setCryptoTickerSymbol(symbols[index]);
			}
		},
	});
}

void ShowAccountHubLogoutBoxInternal(
		not_null<Window::SessionController*> controller) {
	controller->show(Box([=](not_null<Ui::GenericBox*> box) {
		struct State {
			std::vector<not_null<Main::Account*>> accounts;
			std::vector<not_null<Ui::Checkbox*>> checks;
		};
		const auto state = box->lifetime().make_state<State>();

		box->setTitle(tr::xgram_AccountHubLogoutTitle());
		box->addRow(object_ptr<Ui::FlatLabel>(
			box,
			tr::xgram_AccountHubLogoutDescription(tr::now),
			st::boxLabel), st::boxPadding);
		box->addSkip(st::boxMediumSkip);

		for (const auto &account : Core::App().domain().orderedAccounts()) {
			if (!account->sessionExists()) {
				continue;
			}
			const auto check = box->addRow(object_ptr<Ui::Checkbox>(
				box,
				account->session().user()->name(),
				false,
				st::defaultBoxCheckbox), st::settingsCheckboxPadding);
			state->accounts.push_back(account);
			state->checks.push_back(check);
		}

		box->addButton(tr::xgram_AccountHubLogoutAction(), [=] {
			auto selected = std::vector<not_null<Main::Account*>>();
			for (auto i = 0; i != int(state->accounts.size()); ++i) {
				if (state->checks[i]->checked()) {
					selected.push_back(state->accounts[i]);
				}
			}
			if (selected.empty()) {
				controller->showToast(tr::xgram_AccountHubChooseAccount(tr::now));
				return;
			}
			controller->show(Ui::MakeConfirmBox({
				.text = tr::xgram_AccountHubLogoutConfirm(),
				.confirmed = [=](Fn<void()> &&close) {
					close();
					box->closeBox();
					const auto active = &controller->session().account();
					for (const auto account : selected) {
						if (account != active && account->sessionExists()) {
							Core::App().logoutWithChecks(account);
						}
					}
					if (ranges::any_of(selected, [=](const auto account) {
						return account.get() == active;
					})
						&& active->sessionExists()) {
						Core::App().logoutWithChecks(active);
					}
				},
				.confirmText = tr::xgram_AccountHubLogoutAction(),
				.confirmStyle = &st::attentionBoxButton,
			}));
		}, st::attentionBoxButton);
		box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
	}));
}

void BuildAccountHub(SectionBuilder &builder) {
	builder.addSubsectionTitle(tr::xgram_GlobalPinsHeader());
	auto xgram = XGramSectionBuilder(builder);
	xgram.addSettingToggle({
		.id = u"xgram/globalPinsEnabled"_q,
		.title = tr::xgram_GlobalPinsEnabled(),
		.getter = &XGramSettings::globalPinsEnabled,
		.setter = &XGramSettings::setGlobalPinsEnabled,
	});
}

void ShowPreviewNumberBox(
		not_null<Window::SessionController*> controller,
		rpl::producer<QString> title,
		int64 current,
		int64 maximum,
		Fn<void(int64)> save) {
	controller->show(Box([=, title = std::move(title)](
			not_null<Ui::GenericBox*> box) mutable {
		box->setTitle(std::move(title));
		const auto field = box->addRow(object_ptr<Ui::InputField>(
			box,
			st::defaultInputField,
			rpl::single(u"0"_q),
			QString::number(current)));
		field->setMaxLength(12);
		field->selectAll();
		box->setFocusCallback([=] { field->setFocusFast(); });
		const auto apply = [=] {
			const auto text = field->getLastText().trimmed();
			auto ok = false;
			const auto value = text.toLongLong(&ok);
			if (!ok || value < 0 || value > maximum) {
				field->showError();
				return;
			}
			save(value);
			box->closeBox();
		};
		field->submits() | rpl::on_next(apply, field->lifetime());
		box->addButton(tr::lng_settings_save(), apply);
		box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
	}));
}

void BuildProfilePreview(SectionBuilder &builder, XGramSectionBuilder &xgram) {
	const auto controller = builder.controller();
	const auto &settings = XGramSettings::getInstance();
	builder.addSubsectionTitle(tr::xgram_ProfilePreviewHeader());
	builder.addDividerText(tr::xgram_ProfilePreviewDescription());
	xgram.addSettingToggle({
		.id = u"xgram/profilePreviewEnabled"_q,
		.title = tr::xgram_ProfilePreviewEnabled(),
		.getter = &XGramSettings::profilePreviewEnabled,
		.setter = &XGramSettings::setProfilePreviewEnabled,
	});
	builder.addButton({
		.id = u"xgram/previewStarsBalance"_q,
		.title = tr::xgram_PreviewStarsBalance(),
		.icon = { &st::menuIconPremium },
		.label = settings.previewStarsBalanceValue() | rpl::map([](int64 value) {
			return Lang::FormatCountDecimal(value);
		}),
		.onClick = [=] {
			ShowPreviewNumberBox(
				controller,
				tr::xgram_PreviewStarsBalance(),
				XGramSettings::getInstance().previewStarsBalance(),
				999999999,
				[](int64 value) {
					XGramSettings::getInstance().setPreviewStarsBalance(value);
				});
		},
		.shown = settings.profilePreviewEnabledValue(),
	});
	builder.addButton({
		.id = u"xgram/previewGiftsCount"_q,
		.title = tr::xgram_PreviewGiftsCount(),
		.icon = { &st::menuIconGiftPremium },
		.label = settings.previewGiftsCountValue() | rpl::map([](int value) {
			return QString::number(value);
		}),
		.onClick = [=] {
			ShowPreviewNumberBox(
				controller,
				tr::xgram_PreviewGiftsCount(),
				XGramSettings::getInstance().previewGiftsCount(),
				9999,
				[](int64 value) {
					XGramSettings::getInstance().setPreviewGiftsCount(int(value));
				});
		},
		.shown = settings.profilePreviewEnabledValue(),
	});
	builder.addButton({
		.id = u"xgram/openProfilePreview"_q,
		.title = tr::xgram_OpenProfilePreview(),
		.icon = { &st::menuIconProfile },
		.onClick = [=] {
			controller->showPeerInfo(controller->session().user());
		},
		.shown = settings.profilePreviewEnabledValue(),
	});
}

void BuildOfficialPurchases(SectionBuilder &builder, XGramSectionBuilder &xgram) {
	const auto controller = builder.controller();
	builder.addSubsectionTitle(tr::xgram_OfficialPurchasesHeader());
	xgram.addSettingToggle({
		.id = u"xgram/officialPurchasesEnabled"_q,
		.title = tr::xgram_OfficialPurchasesEnabled(),
		.getter = &XGramSettings::officialPurchasesEnabled,
		.setter = &XGramSettings::setOfficialPurchasesEnabled,
	});
	builder.addButton({
		.id = u"xgram/buyStars"_q,
		.title = tr::xgram_BuyStars(),
		.onClick = [=] {
			controller->setPremiumRef(u"xgram"_q);
			controller->showSettings(Settings::CreditsId());
		},
		.shown = XGramSettings::getInstance().officialPurchasesEnabledValue(),
	});
	builder.addButton({
		.id = u"xgram/buyPremium"_q,
		.title = tr::xgram_BuyPremium(),
		.onClick = [=] {
			controller->setPremiumRef(u"xgram"_q);
			controller->showSettings(Settings::PremiumId());
		},
		.shown = XGramSettings::getInstance().officialPurchasesEnabledValue(),
	});
}

const auto kMeta = BuildHelper({
	.id = XGramGhost::Id(),
	.parentId = XGramMain::Id(),
	.title = u"XGram"_q,
	.icon = &st::menuIconGroupReactions,
}, [](SectionBuilder &builder) {
	auto xgram = XGramSectionBuilder(builder);

	builder.addSkip();
	BuildGhostEssentials(builder);

	builder.addSkip();
	BuildSpyEssentials(builder, xgram);

	xgram.addSectionDivider();
	BuildOther(builder, xgram);

	xgram.addSectionDivider();
	BuildCrypto(builder, xgram);

	xgram.addSectionDivider();
	BuildAccountHub(builder);

	xgram.addSectionDivider();
	BuildOfficialPurchases(builder, xgram);

	xgram.addSectionDivider();
	BuildProfilePreview(builder, xgram);
	builder.addSkip();
});

} // namespace

void ShowAccountHubLogoutBox(
		not_null<Window::SessionController*> controller) {
	ShowAccountHubLogoutBoxInternal(controller);
}

rpl::producer<QString> XGramGhost::title() {
	return rpl::single(QString("XGram"));
}

XGramGhost::XGramGhost(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller)
, _controller(controller) {
	setupContent();
}

void XGramGhost::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type XGramGhostId() {
	return XGramGhost::Id();
}

} // namespace Settings
