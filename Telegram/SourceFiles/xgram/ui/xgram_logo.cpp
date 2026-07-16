// This is the source code of XGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "xgram/ui/xgram_logo.h"

#include "xgram/xgram_settings.h"
#include "styles/style_xgram_styles.h"
#include "ui/rect.h"

#include <QSvgRenderer>

#include <array>

static QString LAST_LOADED_NAME;
static QImage LAST_LOADED;
static QImage LAST_LOADED_PAD;
static QImage LAST_LOADED_INTERNAL;
static QImage LAST_LOADED_INTERNAL_PAD;

namespace XGramAssets {

void RemoveLegacyIconFiles() {
	const auto names = std::array{
		DEFAULT_ICON,
		ALT_ICON,
		DISCORD_ICON,
		SPOTIFY_ICON,
		EXTERA_ICON,
		NOTHING_ICON,
		BARD_ICON,
		YAPLUS_ICON,
		WIN95_ICON,
		CHIBI_ICON,
		CHIBI2_ICON,
		EXTERA2_ICON,
	};
	for (const auto &name : names) {
		QFile::remove(cWorkingDir() + u"tdata/XGram-"_q + name + u".ico"_q);
	}
	QFile::remove(cWorkingDir() + u"tdata/XGram-custom.ico"_q);
	QFile::remove(cWorkingDir() + u"tdata/XGram-custom-icon.png"_q);
}

QString appIcoPath() {
	const auto &settings = XGramSettings::getInstance();
	return cWorkingDir()
		+ u"tdata/XGram-"_q
		+ settings.appIcon()
		+ u".ico"_q;
}

void loadAppIco() {
	const auto &settings = XGramSettings::getInstance();
	const auto iconPath = appIcoPath();
	RemoveLegacyIconFiles();

	auto f = QFile(iconPath);
	if (f.exists()) {
		f.setPermissions(QFile::WriteOther);
		f.remove();
	}
	f.close();
	QFile::copy(
		qsl(":/gui/art/xgram/%1/app_icon.ico").arg(settings.appIcon()),
		iconPath);
}

QImage CreateImage(const QString &name, const QSize resultImageSize, const int padding = 0) {
	const auto iconSize = resultImageSize.shrunkBy(QMargins(padding, padding, padding, padding));
	const auto pngPath = qsl(":/gui/art/xgram/%1/app.png").arg(name);
	if (QFile::exists(pngPath)) {
		const auto loaded = QImage(pngPath).scaled(iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
		auto res = QImage(
			resultImageSize * style::DevicePixelRatio(),
			QImage::Format_ARGB32_Premultiplied);
		res.setDevicePixelRatio(style::DevicePixelRatio());
		res.fill(Qt::transparent);
		{
			auto p = QPainter(&res);
			p.drawImage(QRect(padding, padding, iconSize.width(), iconSize.height()), loaded);
		}
		return res;
	}

	const auto svgPath = qsl(":/gui/art/xgram/%1/app.svg").arg(name);
	if (!QFile::exists(svgPath)) {
		return {};
	}

	auto svg = QSvgRenderer(svgPath);
	auto image = QImage(
		resultImageSize * style::DevicePixelRatio(),
		QImage::Format_ARGB32_Premultiplied);
	image.setDevicePixelRatio(style::DevicePixelRatio());
	image.fill(Qt::transparent);
	{
		auto p = QPainter(&image);

		QPainterPath path;
		path.addRoundedRect(
			QRect(padding, padding, iconSize.width(), iconSize.height()),
			iconSize.width() / 2.0f,
			iconSize.height() / 2.0f
		);

		p.save();

		p.setRenderHint(QPainter::Antialiasing, true);
		p.setClipPath(path);
		p.setRenderHint(QPainter::Antialiasing, false);

		svg.render(&p, QRect(padding, padding, iconSize.width(), iconSize.height()));

		p.restore();
	}
	return image;
}

void loadIcons() {
	const auto &settings = XGramSettings::getInstance();
	if (LAST_LOADED_NAME != settings.appIcon()) {
		LAST_LOADED_NAME = settings.appIcon();
		LAST_LOADED = CreateImage(settings.appIcon(), Size(256));
		LAST_LOADED_PAD = CreateImage(settings.appIcon(), Size(256), 12);
		LAST_LOADED_INTERNAL = CreateImage(settings.appIcon(), Size(256));
		LAST_LOADED_INTERNAL_PAD = CreateImage(settings.appIcon(), Size(256), 12);
	}
}

QImage loadPreview(const QString &name) {
	return CreateImage(name, Size(st::iconPickerIconSize), st::iconPickerImagePadding);
}

QString currentAppLogoName() {
	return LAST_LOADED_NAME;
}

QImage currentAppLogo() {
	loadIcons();
	return LAST_LOADED_INTERNAL;
}

QImage currentAppLogoPad() {
	loadIcons();
	return LAST_LOADED_INTERNAL_PAD;
}

QImage currentApplicationIcon() {
	loadIcons();
	return LAST_LOADED;
}

}
