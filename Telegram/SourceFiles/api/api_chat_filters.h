/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

namespace Main {
class Session;
} // namespace Main

namespace Api {

void SaveNewFilterPinned(
	not_null<Main::Session*> session,
	FilterId filterId);

void SaveNewOrder(
	not_null<Main::Session*> session,
	const std::vector<FilterId> &order);

} // namespace Api
