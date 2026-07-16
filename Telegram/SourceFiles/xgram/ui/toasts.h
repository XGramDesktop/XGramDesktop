#pragma once

#include "ui/toast/toast.h"

namespace XGram::Ui {

void ShowToastWithAction(
	::Ui::Toast::Config &&config,
	const QString &buttonText,
	Fn<void()> callback);

}
