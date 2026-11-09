#pragma once

class EditConfigLanguage : public tsl::Gui {
private:
	std::string m_type;
	std::string showType;
	std::string m_name;
	std::string footerBackup;
	std::map<std::string, std::map<std::string, std::string>> m_locale;
	struct localeStruct {
		std::string name;
		std::string ietf_code;
	};
	std::vector<localeStruct> languages;
	bool* m_skipSavingConfig;
public:
	EditConfigLanguage(bool* skipSavingConfig) {
		m_skipSavingConfig = skipSavingConfig;
		m_locale = getParsedDataFromIniFile("sdmc:/config/status-monitor-deux/locale.ini");

		footerBackup = defaultButtonView;
		defaultButtonView = locale["Footer"];

		for (const auto& [key_code, map] : m_locale) {
			// A section written before a key was added is still usable: the
			// loader backfills anything missing from the defaults. Only the
			// display name has to be there for the entry to be listable.
			auto nameIt = map.find("language_name");
			if (nameIt == map.end() || nameIt->second.length() == 0) continue;
			languages.emplace_back(nameIt->second, key_code);
		}
	}

	~EditConfigLanguage() {
		defaultButtonView = footerBackup;
	}

	virtual tsl::elm::Element* createUI() override {
		rootFrame = new tsl::elm::OverlayFrame(APP_TITLE, locale["override_language"]);
		auto list = new tsl::elm::List();

		for (const auto& [name, code] : languages) {
			auto Item = new tsl::elm::ListItem(name);
			Item->setClickListener([this, code](uint64_t keys) {
				if (keys & KEY_A) {
					*m_skipSavingConfig = true;
					setIniFileValue("sdmc:/config/status-monitor-deux/config.ini", "status-monitor-deux", "override_language", "true");
					setIniFileValue("sdmc:/config/status-monitor-deux/config.ini", "status-monitor-deux", "override_language_ietf_code", code);
					tsl::setNextOverlay(filepath, "");
					tsl::goBack();
					tsl::goBack();
					tsl::goBack();
					return true;
				}
				return false;
			});		
			list->addItem(Item);
		}

		auto Item = new tsl::elm::ListItem(locale["system_language"]);
		Item->setClickListener([this](uint64_t keys) {
			if (keys & KEY_A) {
				*m_skipSavingConfig = true;
				setIniFileValue("sdmc:/config/status-monitor-deux/config.ini", "status-monitor-deux", "override_language", "false");
				tsl::setNextOverlay(filepath, "");
				tsl::goBack();
				tsl::goBack();
				tsl::goBack();
				return true;
			}
			return false;
		});		
		list->addItem(Item);

		rootFrame->setContent(list);
		return rootFrame;
	}

	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		if (keysDown & KEY_B) {
			tsl::hlp::requestForeground(true);
			tsl::goBack();
			return true;
		}
		return false;
	}
};
