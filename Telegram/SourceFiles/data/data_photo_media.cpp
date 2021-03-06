/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "data/data_photo_media.h"

#include "data/data_photo.h"
#include "data/data_session.h"
#include "data/data_file_origin.h"
#include "data/data_auto_download.h"
#include "main/main_session.h"
#include "history/history_item.h"
#include "history/history.h"
#include "storage/file_download.h"
#include "ui/image/image.h"

namespace Data {

PhotoMedia::PhotoMedia(not_null<PhotoData*> owner)
: _owner(owner) {
}

// NB! Right now DocumentMedia can outlive Main::Session!
// In DocumentData::collectLocalData a shared_ptr is sent on_main.
// In case this is a problem the ~Gif code should be rewritten.
PhotoMedia::~PhotoMedia() = default;

not_null<PhotoData*> PhotoMedia::owner() const {
	return _owner;
}

Image *PhotoMedia::thumbnailInline() const {
	if (!_inlineThumbnail) {
		const auto bytes = _owner->inlineThumbnailBytes();
		if (!bytes.isEmpty()) {
			auto image = Images::FromInlineBytes(bytes);
			if (image.isNull()) {
				_owner->clearInlineThumbnailBytes();
			} else {
				_inlineThumbnail = std::make_unique<Image>(std::move(image));
			}
		}
	}
	return _inlineThumbnail.get();
}

Image *PhotoMedia::image(PhotoSize size) const {
	if (const auto image = _images[PhotoSizeIndex(size)].get()) {
		return image;
	}
	return _images[_owner->validSizeIndex(size)].get();
}

void PhotoMedia::wanted(PhotoSize size, Data::FileOrigin origin) {
	const auto index = _owner->validSizeIndex(size);
	if (!_images[index]) {
		_owner->load(size, origin);
	}
}

QSize PhotoMedia::size(PhotoSize size) const {
	const auto index = PhotoSizeIndex(size);
	if (const auto image = _images[index].get()) {
		return image->size();
	}
	const auto &location = _owner->location(size);
	return { location.width(), location.height() };
}

void PhotoMedia::set(PhotoSize size, QImage image) {
	const auto index = PhotoSizeIndex(size);
	const auto limit = PhotoData::SideLimit();
	if (image.width() > limit || image.height() > limit) {
		image = image.scaled(
			limit,
			limit,
			Qt::KeepAspectRatio,
			Qt::SmoothTransformation);
	}
	_images[index] = std::make_unique<Image>(std::move(image));
	_owner->session().downloaderTaskFinished().notify();
}

bool PhotoMedia::loaded() const {
	const auto index = PhotoSizeIndex(PhotoSize::Large);
	return (_images[index] != nullptr);
}

float64 PhotoMedia::progress() const {
	return (_owner->uploading() || _owner->loading())
		? _owner->progress()
		: (loaded() ? 1. : 0.);
}

void PhotoMedia::automaticLoad(
		Data::FileOrigin origin,
		const HistoryItem *item) {
	const auto index = PhotoSizeIndex(PhotoSize::Large);
	if (!item || loaded() || _owner->cancelled()) {
		return;
	}
	const auto loadFromCloud = Data::AutoDownload::Should(
		_owner->session().settings().autoDownload(),
		item->history()->peer,
		_owner);
	_owner->load(
		origin,
		loadFromCloud ? LoadFromCloudOrLocal : LoadFromLocalOnly,
		true);
}

void PhotoMedia::collectLocalData(not_null<PhotoMedia*> local) {
	if (const auto image = local->_inlineThumbnail.get()) {
		_inlineThumbnail = std::make_unique<Image>(image->original());
	}
	for (auto i = 0; i != kPhotoSizeCount; ++i) {
		if (const auto image = local->_images[i].get()) {
			_images[i] = std::make_unique<Image>(image->original());
		}
	}
}

} // namespace Data
