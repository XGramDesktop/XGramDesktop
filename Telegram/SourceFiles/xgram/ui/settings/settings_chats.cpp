// This is the source code of XGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "xgram/ui/settings/settings_chats.h"

#include "lang_auto.h"
#include "xgram/xgram_settings.h"
#include "xgram/ui/boxes/edit_mark_box.h"
#include "xgram/ui/components/message_preview.h"
#include "xgram/ui/settings/xgram_builder.h"
#include "xgram/ui/settings/settings_xgram_utils.h"
#include "xgram/ui/settings/settings_main.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "styles/style_xgram_icons.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"

#include <memory>

namespace Settings {

using namespace Builder;
using namespace XGramBuilder;

namespace {

struct PreviewState {
	MessagePreview *widget = nullptr;
};

void BuildStickersAndEmoji(SectionBuilder &builder, XGramSectionBuilder &xgram) {
	builder.addSubsectionTitle(tr::lng_settings_stickers_emoji());

	xgram.addSettingToggle({
		.id = u"xgram/showOnlyAddedEmojisAndStickers"_q,
		.title = tr::xgram_ShowOnlyAddedEmojisAndStickers(),
		.getter = &XGramSettings::showOnlyAddedEmojisAndStickers,
		.setter = &XGramSettings::setShowOnlyAddedEmojisAndStickers,
	});

	xgram.addCollapsibleToggle({
		.id = u"xgram/hideReactions"_q,
		.title = tr::xgram_HideReactions(),
		.checkboxes = {
			NestedEntry{
				tr::xgram_HideReactionsInChannels(tr::now),
				[] { return !XGramSettings::getInstance().showChannelReactions(); },
				[](bool v) { XGramSettings::getInstance().setShowChannelReactions(!v); }
			},
			NestedEntry{
				tr::xgram_HideReactionsInGroups(tr::now),
				[] { return !XGramSettings::getInstance().showGroupReactions(); },
				[](bool v) { XGramSettings::getInstance().setShowGroupReactions(!v); }
			},
			NestedEntry{
				tr::xgram_HideReactionsInPrivateChats(tr::now),
				[] { return !XGramSettings::getInstance().showPrivateChatReactions(); },
				[](bool v) { XGramSettings::getInstance().setShowPrivateChatReactions(!v); }
			}
		},
		.toggledWhenAll = false,
	});

	xgram.addSectionDivider();
}

void BuildRecentStickersLimit(SectionBuilder &builder, XGramSectionBuilder &xgram) {
	auto *settings = &XGramSettings::getInstance();

	xgram.addSlider({
		.id = u"xgram/recentStickersCount"_q,
		.title = tr::xgram_SettingsRecentStickersCount(),
		.steps = 200,
		.current = settings->recentStickersCount(),
		.indexToValue = [](int index) { return index + 1; },
		.onChanged = nullptr,
		.onFinalChanged = [](int amount) {
			XGramSettings::getInstance().setRecentStickersCount(amount);
		},
		.formatLabel = [](int amount) { return QString::number(amount); },
	});

	xgram.addSectionDivider();
}

void BuildGroupsAndChannels(SectionBuilder &builder, XGramSectionBuilder &xgram) {
	auto *settings = &XGramSettings::getInstance();

	builder.addSubsectionTitle(tr::lng_premium_double_limits_subtitle_channels());

	xgram.addChooseButton({
		.id = u"xgram/channelBottomButton"_q,
		.altIds = { u"xgram/bottomButton"_q },
		.title = tr::xgram_ChannelBottomButton(),
		.boxTitle = tr::xgram_ChannelBottomButton(),
		.initialSelection = static_cast<int>(settings->channelBottomButton()),
		.options = {
			tr::xgram_ChannelBottomButtonHide(tr::now),
			tr::xgram_ChannelBottomButtonMute(tr::now),
			tr::xgram_ChannelBottomButtonDiscuss(tr::now),
		},
		.setter = [](int index) {
			XGramSettings::getInstance().setChannelBottomButton(
				static_cast<ChannelBottomButton>(index));
		},
	});

	xgram.addSettingToggle({
		.id = u"xgram/quickAdminShortcuts"_q,
		.title = tr::xgram_QuickAdminShortcuts(),
		.getter = &XGramSettings::quickAdminShortcuts,
		.setter = &XGramSettings::setQuickAdminShortcuts,
	});
	xgram.addSettingToggle({
		.id = u"xgram/disableGreetingSticker"_q,
		.title = tr::xgram_DisableGreetingSticker(),
		.getter = &XGramSettings::disableGreetingSticker,
		.setter = &XGramSettings::setDisableGreetingSticker,
	});
	xgram.addSettingToggle({
		.id = u"xgram/showMessageShot"_q,
		.title = tr::xgram_SettingsShowMessageShot(),
		.getter = &XGramSettings::showMessageShot,
		.setter = &XGramSettings::setShowMessageShot,
	});

	builder.addSkip();
	builder.addDividerText(tr::xgram_SettingsShowMessageShotDescription());
	builder.addSkip();
}

void BuildMarks(
		SectionBuilder &builder,
		XGramSectionBuilder &xgram,
		std::shared_ptr<PreviewState> previewState) {
	auto *settings = &XGramSettings::getInstance();
	const auto controller = builder.controller();

	builder.addSubsectionTitle(tr::lng_settings_messages());

	builder.add([=](const WidgetContext &ctx) -> SectionBuilder::WidgetToAdd {
		auto preview = object_ptr<MessagePreview>(ctx.container, controller);
		previewState->widget = preview.data();
		return {
			.widget = std::move(preview),
			.margin = style::margins(
				0,
				st::defaultVerticalListSkip,
				0,
				st::settingsPrivacySkipTop),
		};
	});

	xgram.addSettingToggle({
		.id = u"xgram/replaceBottomInfoWithIcons"_q,
		.altIds = { u"xgram/replaceEditedWithIcon"_q },
		.title = tr::xgram_ReplaceMarksWithIcons(),
		.getter = &XGramSettings::replaceBottomInfoWithIcons,
		.setter = &XGramSettings::setReplaceBottomInfoWithIcons,
	});

	builder.scope([&] {
		builder.addButton({
			.id = u"xgram/deletedMark"_q,
			.title = tr::xgram_DeletedMarkText(),
			.st = &st::settingsButtonNoIcon,
			.label = XGramSettings::getInstance().deletedMarkValue(),
			.onClick = [=] {
				auto box = Box<EditMarkBox>(
					tr::xgram_DeletedMarkText(),
					settings->deletedMark(),
					QString("🧹"),
					[=](const QString &value) {
						XGramSettings::getInstance().setDeletedMark(value);
					});
				Ui::show(std::move(box));
			},
		});

		builder.addButton({
			.id = u"xgram/editedMark"_q,
			.title = tr::xgram_EditedMarkText(),
			.st = &st::settingsButtonNoIcon,
			.label = XGramSettings::getInstance().editedMarkValue(),
			.onClick = [=] {
				auto box = Box<EditMarkBox>(
					tr::xgram_EditedMarkText(),
					settings->editedMark(),
					tr::lng_edited(tr::now),
					[=](const QString &value) {
						XGramSettings::getInstance().setEditedMark(value);
					});
				Ui::show(std::move(box));
			},
		});
	}, XGramSettings::getInstance().replaceBottomInfoWithIconsValue()
		| rpl::map([](bool v) { return !v; }));

	xgram.addSettingToggle({
		.id = u"xgram/removeMessageTail"_q,
		.title = tr::xgram_RemoveMessageTail(),
		.getter = &XGramSettings::removeMessageTail,
		.setter = &XGramSettings::setRemoveMessageTail,
	});

	xgram.addSettingToggle({
		.id = u"xgram/hideFastShare"_q,
		.altIds = { u"xgram/hideShareButton"_q },
		.title = tr::xgram_HideShareButton(),
		.getter = &XGramSettings::hideFastShare,
		.setter = &XGramSettings::setHideFastShare,
	});
	xgram.addSettingToggle({
		.id = u"xgram/simpleQuotesAndReplies"_q,
		.altIds = { u"xgram/disableColorfulReplies"_q, u"xgram/replyElements"_q },
		.title = tr::xgram_SimpleQuotesAndReplies(),
		.getter = &XGramSettings::simpleQuotesAndReplies,
		.setter = &XGramSettings::setSimpleQuotesAndReplies,
	});

	const auto semiTransparent = xgram.addSettingToggle({
		.id = u"xgram/semiTransparentDeletedMessages"_q,
		.altIds = { u"xgram/translucentDeletedMessages"_q },
		.title = tr::xgram_SemiTransparentDeletedMessages(),
		.getter = &XGramSettings::semiTransparentDeletedMessages,
		.setter = &XGramSettings::setSemiTransparentDeletedMessages,
	});
	if (semiTransparent) {
		xgram.addBetaBadge(semiTransparent);
	}

	xgram.addSectionDivider();
}

void BuildWideMessagesMultiplier(
		SectionBuilder &builder,
		XGramSectionBuilder &xgram,
		std::shared_ptr<PreviewState> previewState) {
	auto *settings = &XGramSettings::getInstance();

	constexpr auto kMinSize = 1.00;
	constexpr auto kStep = 0.05;

	const auto valueToIndex = [=](double value) {
		return static_cast<int>(std::round((value - kMinSize) / kStep));
	};

	const auto controller = builder.controller();
	xgram.addSlider({
		.id = u"xgram/messageBubbleRadius"_q,
		.title = tr::xgram_MessageBubbleRadius(),
		.steps = 17,
		.current = settings->messageBubbleRadius(),
		.indexToValue = [](int index) { return index; },
		.onChanged = [=](int index) {
			if (previewState->widget) {
				previewState->widget->setBubbleRadius(index);
			}
		},
		.onFinalChanged = [=](int index) {
			if (previewState->widget) {
				previewState->widget->setBubbleRadius(index);
			}
			XGramSettings::getInstance().setMessageBubbleRadius(index);
			ShowRestartPrompt(controller);
		},
		.formatLabel = [](int index) {
			return QString::number(index);
		},
	});

	xgram.addSectionDivider();

	xgram.addSlider({
		.id = u"xgram/wideMultiplier"_q,
		.title = tr::xgram_SettingsWideMultiplier(),
		.steps = 61, // (4.00 - 1.00) / 0.05 + 1
		.current = valueToIndex(settings->wideMultiplier()),
		.indexToValue = [](int index) { return index; },
		.onChanged = nullptr,
		.onFinalChanged = [=](int index) {
			XGramSettings::getInstance().setWideMultiplier(
				kMinSize + index * kStep);
			ShowRestartPrompt(controller);
		},
		.formatLabel = [=](int index) {
			return QString::number(kMinSize + index * kStep, 'f', 2);
		},
	});

	builder.addSkip();
	builder.addDividerText(tr::xgram_SettingsWideMultiplierDescription());
	builder.addSkip();
}

void BuildContextMenuElements(SectionBuilder &builder, XGramSectionBuilder &xgram) {
	auto *settings = &XGramSettings::getInstance();

	builder.addSubsectionTitle(tr::xgram_ContextMenuElementsHeader());

	const auto options = std::vector{
		tr::xgram_SettingsContextMenuItemHidden(tr::now),
		tr::xgram_SettingsContextMenuItemShown(tr::now),
		tr::xgram_SettingsContextMenuItemExtended(tr::now),
	};

	xgram.addChooseButton({
		.id = u"xgram/showReactionsPanelInContextMenu"_q,
		.title = tr::xgram_SettingsContextMenuReactionsPanel(),
		.boxTitle = tr::xgram_SettingsContextMenuTitle(),
		.initialSelection = static_cast<int>(settings->showReactionsPanelInContextMenu()),
		.options = options,
		.setter = [](int i) { XGramSettings::getInstance().setShowReactionsPanelInContextMenu(static_cast<ContextMenuVisibility>(i)); },
		.icon = { &st::menuIconReactions },
	});
	xgram.addChooseButton({
		.id = u"xgram/showViewsPanelInContextMenu"_q,
		.title = tr::xgram_SettingsContextMenuViewsPanel(),
		.boxTitle = tr::xgram_SettingsContextMenuTitle(),
		.initialSelection = static_cast<int>(settings->showViewsPanelInContextMenu()),
		.options = options,
		.setter = [](int i) { XGramSettings::getInstance().setShowViewsPanelInContextMenu(static_cast<ContextMenuVisibility>(i)); },
		.icon = { &st::menuIconShowInChat },
	});
	xgram.addChooseButton({
		.id = u"xgram/showHideMessageInContextMenu"_q,
		.title = tr::xgram_ContextHideMessage(),
		.boxTitle = tr::xgram_SettingsContextMenuTitle(),
		.initialSelection = static_cast<int>(settings->showHideMessageInContextMenu()),
		.options = options,
		.setter = [](int i) { XGramSettings::getInstance().setShowHideMessageInContextMenu(static_cast<ContextMenuVisibility>(i)); },
		.icon = { &st::menuIconClear },
	});
	xgram.addChooseButton({
		.id = u"xgram/showUserMessagesInContextMenu"_q,
		.title = tr::xgram_UserMessagesMenuText(),
		.boxTitle = tr::xgram_SettingsContextMenuTitle(),
		.initialSelection = static_cast<int>(settings->showUserMessagesInContextMenu()),
		.options = options,
		.setter = [](int i) { XGramSettings::getInstance().setShowUserMessagesInContextMenu(static_cast<ContextMenuVisibility>(i)); },
		.icon = { &st::menuIconTTL },
	});
	xgram.addChooseButton({
		.id = u"xgram/showMessageDetailsInContextMenu"_q,
		.title = tr::xgram_MessageDetailsPC(),
		.boxTitle = tr::xgram_SettingsContextMenuTitle(),
		.initialSelection = static_cast<int>(settings->showMessageDetailsInContextMenu()),
		.options = options,
		.setter = [](int i) { XGramSettings::getInstance().setShowMessageDetailsInContextMenu(static_cast<ContextMenuVisibility>(i)); },
		.icon = { &st::menuIconInfo },
	});
	xgram.addChooseButton({
		.id = u"xgram/showRepeatMessageInContextMenu"_q,
		.title = tr::xgram_RepeatMessage(),
		.boxTitle = tr::xgram_SettingsContextMenuTitle(),
		.initialSelection = static_cast<int>(settings->showRepeatMessageInContextMenu()),
		.options = options,
		.setter = [](int i) { XGramSettings::getInstance().setShowRepeatMessageInContextMenu(static_cast<ContextMenuVisibility>(i)); },
		.icon = { &st::xgramRepeatMenuIcon },
	});
	if (settings->filtersEnabled()) {
		xgram.addChooseButton({
			.id = u"xgram/showAddFilterInContextMenu"_q,
			.title = tr::xgram_RegexFilterQuickAdd(),
			.boxTitle = tr::xgram_SettingsContextMenuTitle(),
			.initialSelection = static_cast<int>(settings->showAddFilterInContextMenu()),
			.options = options,
			.setter = [](int i) { XGramSettings::getInstance().setShowAddFilterInContextMenu(static_cast<ContextMenuVisibility>(i)); },
			.icon = { &st::menuIconAddToFolder },
		});
	}

	builder.addSkip();
	builder.addDividerText(tr::xgram_SettingsContextMenuDescription());
	builder.addSkip();
}

void BuildMessageFieldElements(SectionBuilder &builder, XGramSectionBuilder &xgram) {
	builder.addSubsectionTitle(tr::xgram_MessageFieldElementsHeader());

	xgram.addSettingToggle({
		.id = u"xgram/showAttachButtonInMessageField"_q,
		.title = tr::xgram_MessageFieldElementAttach(),
		.getter = &XGramSettings::showAttachButtonInMessageField,
		.setter = &XGramSettings::setShowAttachButtonInMessageField,
		.icon = { &st::messageFieldAttachIcon },
	});
	xgram.addSettingToggle({
		.id = u"xgram/showCommandsButtonInMessageField"_q,
		.title = tr::xgram_MessageFieldElementCommands(),
		.getter = &XGramSettings::showCommandsButtonInMessageField,
		.setter = &XGramSettings::setShowCommandsButtonInMessageField,
		.icon = { &st::messageFieldCommandsIcon },
	});
	xgram.addSettingToggle({
		.id = u"xgram/showAutoDeleteButtonInMessageField"_q,
		.title = tr::xgram_MessageFieldElementTTL(),
		.getter = &XGramSettings::showAutoDeleteButtonInMessageField,
		.setter = &XGramSettings::setShowAutoDeleteButtonInMessageField,
		.icon = { &st::messageFieldTTLIcon },
	});
	xgram.addSettingToggle({
		.id = u"xgram/showEmojiButtonInMessageField"_q,
		.title = tr::xgram_MessageFieldElementEmoji(),
		.getter = &XGramSettings::showEmojiButtonInMessageField,
		.setter = &XGramSettings::setShowEmojiButtonInMessageField,
		.icon = { &st::messageFieldEmojiIcon },
	});
	xgram.addSettingToggle({
		.id = u"xgram/showMicrophoneButtonInMessageField"_q,
		.title = tr::xgram_MessageFieldElementVoice(),
		.getter = &XGramSettings::showMicrophoneButtonInMessageField,
		.setter = &XGramSettings::setShowMicrophoneButtonInMessageField,
		.icon = { &st::messageFieldVoiceIcon },
	});
	xgram.addSettingToggle({
		.id = u"xgram/showGiftButtonInMessageField"_q,
		.title = tr::lng_profile_action_short_gift(),
		.getter = &XGramSettings::showGiftButtonInMessageField,
		.setter = &XGramSettings::setShowGiftButtonInMessageField,
		.icon = { &st::settingsButtonIconGift },
	});
	xgram.addSettingToggle({
		.id = u"xgram/showAiEditorButtonInMessageField"_q,
		.title = tr::lng_ai_compose_title(),
		.getter = &XGramSettings::showAiEditorButtonInMessageField,
		.setter = &XGramSettings::setShowAiEditorButtonInMessageField,
		.icon = { &st::messageFieldCocoonAiIcon },
	});

	xgram.addSectionDivider();
}

void BuildMessageFieldPopups(SectionBuilder &builder, XGramSectionBuilder &xgram) {
	builder.addSubsectionTitle(tr::xgram_MessageFieldPopupsHeader());

	xgram.addSettingToggle({
		.id = u"xgram/showAttachPopup"_q,
		.title = tr::xgram_MessageFieldElementAttach(),
		.getter = &XGramSettings::showAttachPopup,
		.setter = &XGramSettings::setShowAttachPopup,
		.icon = { &st::messageFieldAttachIcon },
	});
	xgram.addSettingToggle({
		.id = u"xgram/showEmojiPopup"_q,
		.title = tr::xgram_MessageFieldElementEmoji(),
		.getter = &XGramSettings::showEmojiPopup,
		.setter = &XGramSettings::setShowEmojiPopup,
		.icon = { &st::messageFieldEmojiIcon },
	});
}

const auto kMeta = BuildHelper({
	.id = XGramChats::Id(),
	.parentId = XGramMain::Id(),
	.title = &tr::xgram_CategoryChats,
	.icon = &st::menuIconChatBubble,
}, [](SectionBuilder &builder) {
	auto xgram = XGramSectionBuilder(builder);
	const auto previewState = std::make_shared<PreviewState>();

	builder.addSkip();
	BuildStickersAndEmoji(builder, xgram);
	BuildRecentStickersLimit(builder, xgram);
	BuildGroupsAndChannels(builder, xgram);
	BuildMarks(builder, xgram, previewState);
	BuildWideMessagesMultiplier(builder, xgram, previewState);
	BuildContextMenuElements(builder, xgram);
	BuildMessageFieldElements(builder, xgram);
	BuildMessageFieldPopups(builder, xgram);
	builder.addSkip();
});

} // namespace

rpl::producer<QString> XGramChats::title() {
	return tr::xgram_CategoryChats();
}

XGramChats::XGramChats(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void XGramChats::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type XGramChatsId() {
	return XGramChats::Id();
}

} // namespace Settings
