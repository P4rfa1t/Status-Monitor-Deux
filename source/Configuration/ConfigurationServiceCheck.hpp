#pragma once

class ConfigurationServiceCheck : public tsl::Gui {
private:
	std::string m_type;
	std::string showType;
	std::string m_name;
	std::string footerBackup;
	static constexpr size_t descriptionSize = 18;

	// Lead byte of a 3- or 4-byte UTF-8 codepoint. Those are the scripts written
	// without spaces (CJK, Hangul), which the word loop below would otherwise
	// accumulate into one unbreakable "word". 2-byte codepoints deliberately stay
	// inside the word so European text keeps breaking at spaces only.
	static bool isWideLeadByte(unsigned char c) {
		return c >= 0xE0;
	}

	static size_t codepointLength(unsigned char lead) {
		if ((lead & 0xF0) == 0xE0) return 3;
		if ((lead & 0xF8) == 0xF0) return 4;
		return 1;
	}

	std::string wrapText(const std::string& text, u32 maxWidth, u32 fontsize) {
		std::string wrappedText;
		std::string currentLine;
		std::string currentWord;
		auto renderer = tsl::gfx::Renderer::getRenderer();

		// Helper lambda to measure string width invisibly
		auto measureWidth = [&](const std::string& str) {
			// Passing 0, 0 for x/y since we only care about the returned width

			const auto [width, height] = renderer.drawString(str.c_str(), false, 0, fontsize, fontsize, renderer.a(0x0000));
			return width;
		};

		// Append one already-whole unit, starting a new line when it stops
		// fitting. A unit is never split, so a multibyte codepoint always
		// survives a line break intact.
		auto appendUnit = [&](const std::string& unit, bool trailingSpace) {
			std::string padded = trailingSpace ? unit + " " : unit;
			if (currentLine.empty()) {
				currentLine = padded;
				return;
			}
			if (measureWidth(currentLine + unit) > maxWidth) {
				while (!currentLine.empty() && currentLine.back() == ' ') currentLine.pop_back();
				wrappedText += currentLine + "\n";
				currentLine = padded;
			} else {
				currentLine += padded;
			}
		};

		// Hand the pending ASCII word to appendUnit and clear it.
		auto flushWord = [&](bool trailingSpace) {
			if (currentWord.empty()) return;
			appendUnit(currentWord, trailingSpace);
			currentWord.clear();
		};

		for (size_t i = 0; i < text.size(); ) {
			unsigned char uc = (unsigned char)text[i];

			if (isWideLeadByte(uc)) {
				// One codepoint per unit, so lines may break between characters.
				size_t len = codepointLength(uc);
				if (i + len > text.size()) len = 1;   // truncated sequence: emit as-is
				flushWord(false);
				appendUnit(text.substr(i, len), false);
				i += len;
				continue;
			}

			char c = text[i++];
			if (c == ' ') {
				if (currentWord.empty()) {
					// A CJK codepoint was just emitted as its own unit, so there
					// is no pending word to flush. Keep the separator or the
					// next Latin word would run into the Chinese text.
					if (!currentLine.empty() && currentLine.back() != ' ') currentLine += ' ';
				} else {
					flushWord(true);
				}
			} else if (c == '\n') {
				// Respect any explicit newlines already in the string
				wrappedText += currentLine + currentWord + "\n";
				currentLine.clear();
				currentWord.clear();
			} else {
				currentWord += c;
			}
		}

		flushWord(false);
		wrappedText += currentLine;

		return wrappedText;
	}

	size_t GetHeight(const std::string& text, size_t fontsize) {
		auto renderer = tsl::gfx::Renderer::getRenderer();
		const auto [width, height] = renderer.drawString(text.c_str(), false, 0, fontsize, fontsize, renderer.a(0x0000));
		return height;
	}

public:
	ConfigurationServiceCheck(std::string name) {
		m_name = name;
		footerBackup = defaultButtonView;
		defaultButtonView = locale["Footer"];
	}

	~ConfigurationServiceCheck() {
		defaultButtonView = footerBackup;
	}

	virtual tsl::elm::Element* createUI() override {
		rootFrame = new tsl::elm::OverlayFrame(APP_TITLE, m_name);
		auto list = new tsl::elm::List();

		for (auto& svc : smseGetAllServices()) {
			std::string descriptionAdjusted = "";
			bool errorFound = false;
			for (size_t i = 0; i < svc.initErrors.size(); i++) {
				if (svc.initErrors[i].ok() == false) {
					if (svc.initErrors[i].detail.length() > 0) descriptionAdjusted = wrapText(resolveHexEscapes(svc.initErrors[i].detail), 360, descriptionSize);
					else descriptionAdjusted = localizedOr("init_error_no_desc", "Init error with no desc");
					errorFound = true;
				}
            }				
			if (errorFound == false && svc.execErrors -> size() > 0) {
				for (size_t i = 0; i < svc.execErrors -> size(); i++) {
					for (size_t x = 0; x < svc.execErrors[i].size(); x++) {
						if (svc.execErrors[i][x].ok() == false) {
							if (svc.execErrors[i][x].detail.size() > 0) descriptionAdjusted = wrapText(resolveHexEscapes(svc.execErrors[i][x].detail), 360, descriptionSize);
							else descriptionAdjusted = localizedOr("exec_error_no_desc", "Exec error with no desc");
							errorFound = true;
							break;
						}
					}
				}
			}

			auto addDescription = [&]() {
				if (descriptionAdjusted.length() > 0) {
					auto height = GetHeight(descriptionAdjusted, descriptionSize) + descriptionSize;
					auto Status = new tsl::elm::CustomDrawer([this, descriptionAdjusted, height](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {
						renderer->drawString(descriptionAdjusted.c_str(), false, x+5, y+descriptionSize+5, descriptionSize, 0xFCCC);
						renderer->drawRect(x, y+5, 2, height-descriptionSize+5, 0xFCCC);
						renderer->drawRect(x, y+5, 5, 2, 0xFCCC);
					});
					list->addItem(Status, height);
				}
			};


            auto Item = new tsl::elm::ListItem(svc.name, svc.connected ? "\uE14B" : "\uE14C", !svc.connected);
            if (errorFound) addDescription();
            list->addItem(Item);
        }
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
