#include "panda_qt/config_window.hpp"

#include "version.hpp"

ConfigWindow::ConfigWindow(ConfigCallback configCallback, MainWindowCallback windowCallback, const EmulatorConfig& emuConfig, QWidget* parent)
	: QDialog(parent), config(emuConfig), updateConfig(std::move(configCallback)), getMainWindow(std::move(windowCallback)) {
	setWindowTitle(tr("Configuration"));

	// Set up theme selection
	setTheme(config.frontendSettings.theme);
	setIcon(config.frontendSettings.icon);

	// Set the window title of the main window appropriately if we enable showing the app version on the window
	if (config.windowSettings.showAppVersion) {
		getMainWindow()->setWindowTitle(tr("Alber v%1").arg(PANDA3DS_VERSION));
	}

	// Initialize the widget list and the widget container widgets
	widgetList = new QListWidget(this);
	widgetContainer = new QStackedWidget(this);

	helpText = new QTextEdit(this);
	helpText->setReadOnly(true);

	helpText->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	helpText->setFixedHeight(50);

	widgetList->setMinimumWidth(100);
	widgetList->setMaximumWidth(100);
	widgetList->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	widgetList->setPalette(QPalette(QColor(25, 25, 25)));

	widgetList->setCurrentRow(0);
	widgetContainer->setCurrentIndex(0);

	connect(widgetList, &QListWidget::currentRowChanged, this, [&](int row) {
		widgetContainer->setCurrentIndex(row);
		helpText->setText(helpTexts[row]);
	});

	auto connectCheckbox = [&](QCheckBox* checkbox, bool& setting) {
		checkbox->setChecked(setting);

		connect(checkbox, &QCheckBox::toggled, this, [&](bool checked) {
			setting = checked;
			updateConfig();
		});
	};

	QVBoxLayout* mainLayout = new QVBoxLayout();
	QHBoxLayout* hLayout = new QHBoxLayout();

	// Set up widget layouts
	setLayout(mainLayout);
	mainLayout->addLayout(hLayout);
	mainLayout->addWidget(helpText);

	hLayout->setAlignment(Qt::AlignLeft);
	hLayout->addWidget(widgetList);
	hLayout->addWidget(widgetContainer);

	// Interface settings
	QGroupBox* guiGroupBox = new QGroupBox(tr("Interface Settings"), this);
	QFormLayout* guiLayout = new QFormLayout(guiGroupBox);
	guiLayout->setHorizontalSpacing(20);
	guiLayout->setVerticalSpacing(10);

	QComboBox* themeSelect = new QComboBox();
	themeSelect->addItem(tr("System"));
	themeSelect->addItem(tr("Light"));
	themeSelect->addItem(tr("Dark"));
	themeSelect->addItem(tr("Greetings Cat"));
	themeSelect->addItem(tr("Cream"));
	themeSelect->addItem(tr("OLED"));
	themeSelect->setCurrentIndex(static_cast<int>(config.frontendSettings.theme));
	connect(themeSelect, &QComboBox::currentIndexChanged, this, [&](int index) {
		config.frontendSettings.theme = static_cast<Theme>(index);
		setTheme(static_cast<Theme>(index));

		updateConfig();
	});
	guiLayout->addRow(tr("Color theme"), themeSelect);

	QComboBox* iconSelect = new QComboBox();
	iconSelect->addItem(tr("Happy panda"));
	iconSelect->addItem(tr("Happy panda (colourful)"));
	iconSelect->addItem(tr("Sleepy panda"));
	iconSelect->addItem(tr("Cow panda"));
	iconSelect->addItem(tr("The penguin from SkyEmu"));
	iconSelect->addItem(tr("Unpog"));
	iconSelect->setCurrentIndex(static_cast<int>(config.frontendSettings.icon));

	connect(iconSelect, &QComboBox::currentIndexChanged, this, [&](int index) {
		config.frontendSettings.icon = static_cast<WindowIcon>(index);
		setIcon(static_cast<WindowIcon>(index));

		updateConfig();
	});
	guiLayout->addRow(tr("Window icon"), iconSelect);

	QComboBox* languageSelect = createLanguageSelect();
	guiLayout->addRow(tr("Language"), languageSelect);

	QCheckBox* showAppVersion = new QCheckBox(tr("Show version on window title"));
	showAppVersion->setChecked(config.windowSettings.showAppVersion);
	connect(showAppVersion, &QCheckBox::toggled, this, [&](bool checked) {
		config.windowSettings.showAppVersion = checked;
		updateConfig();

		// Update main window title
		getMainWindow()->setWindowTitle(checked ? tr("Alber v%1").arg(PANDA3DS_VERSION) : tr("Alber"));
	});
	connectCheckbox(showAppVersion, config.windowSettings.showAppVersion);
	guiLayout->addRow(showAppVersion);

	QCheckBox* rememberPosition = new QCheckBox(tr("Remember window position"));
	connectCheckbox(rememberPosition, config.windowSettings.rememberPosition);
	guiLayout->addRow(rememberPosition);

	// General settings
	QGroupBox* genGroupBox = new QGroupBox(tr("General Settings"), this);
	QFormLayout* genLayout = new QFormLayout(genGroupBox);
	genLayout->setHorizontalSpacing(20);
	genLayout->setVerticalSpacing(10);

	QLineEdit* defaultRomPath = new QLineEdit;
	defaultRomPath->setText(QString::fromStdU16String(config.defaultRomPath.u16string()));
	connect(defaultRomPath, &QLineEdit::textChanged, this, [&](const QString& text) {
		config.defaultRomPath = text.toStdString();
		updateConfig();
	});
	QPushButton* browseRomPath = new QPushButton(tr("Browse..."));
	browseRomPath->setAutoDefault(false);
	connect(browseRomPath, &QPushButton::pressed, this, [&, defaultRomPath]() {
		QString newPath = QFileDialog::getExistingDirectory(
			this, tr("Select Directory"), QString::fromStdU16String(config.defaultRomPath.u16string()),
			QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
		);
		if (!newPath.isEmpty()) {
			defaultRomPath->setText(newPath);
		}
	});

	QHBoxLayout* romLayout = new QHBoxLayout();
	romLayout->setSpacing(4);
	romLayout->addWidget(defaultRomPath);
	romLayout->addWidget(browseRomPath);
	genLayout->addRow(tr("Default ROMs path"), romLayout);

	QComboBox* systemLanguage = new QComboBox();
	systemLanguage->addItem(tr("Japanese"));
	systemLanguage->addItem(tr("English"));
	systemLanguage->addItem(tr("French"));
	systemLanguage->addItem(tr("German"));
	systemLanguage->addItem(tr("Italian"));
	systemLanguage->addItem(tr("Spanish"));
	systemLanguage->addItem(tr("Chinese"));
	systemLanguage->addItem(tr("Korean"));
	systemLanguage->addItem(tr("Dutch"));
	systemLanguage->addItem(tr("Portuguese"));
	systemLanguage->addItem(tr("Russian"));
	systemLanguage->addItem(tr("Taiwanese"));

	systemLanguage->setCurrentIndex(static_cast<int>(config.systemLanguage));
	connect(systemLanguage, &QComboBox::currentIndexChanged, this, [&](int index) {
		config.systemLanguage = static_cast<LanguageCodes>(index);
		updateConfig();
	});
	genLayout->addRow(tr("System language"), systemLanguage);

	QCheckBox* circlePadProEnabled = new QCheckBox(tr("Enable CirclePad Pro"));
	connectCheckbox(circlePadProEnabled, config.circlePadProEnabled);
	genLayout->addRow(circlePadProEnabled);

	QCheckBox* discordRpcEnabled = new QCheckBox(tr("Enable Discord RPC"));
	connectCheckbox(discordRpcEnabled, config.discordRpcEnabled);
	genLayout->addRow(discordRpcEnabled);

	QCheckBox* usePortableBuild = new QCheckBox(tr("Use portable build"));
	connectCheckbox(usePortableBuild, config.usePortableBuild);
	genLayout->addRow(usePortableBuild);

	QCheckBox* printAppVersion = new QCheckBox(tr("Print version in console output"));
	connectCheckbox(printAppVersion, config.printAppVersion);
	genLayout->addRow(printAppVersion);

	// Graphics settings
	QGroupBox* gpuGroupBox = new QGroupBox(tr("Graphics Settings"), this);
	QFormLayout* gpuLayout = new QFormLayout(gpuGroupBox);
	gpuLayout->setHorizontalSpacing(20);
	gpuLayout->setVerticalSpacing(10);

	QComboBox* rendererType = new QComboBox();
	rendererType->addItem(tr("Null"));
	rendererType->addItem(tr("OpenGL"));
	rendererType->addItem(tr("Vulkan"));
	rendererType->setCurrentIndex(static_cast<int>(config.rendererType));
	connect(rendererType, &QComboBox::currentIndexChanged, this, [&](int index) {
		auto type = static_cast<RendererType>(index);

		if (type == RendererType::Vulkan) {
			QMessageBox messageBox(
				QMessageBox::Icon::Critical, tr("Vulkan renderer unavailable"),
				tr("Qt UI doesn't currently support Vulkan, try again at a later time")
			);
			messageBox.exec();
		} else {
			config.rendererType = type;
			updateConfig();
		}
	});
	gpuLayout->addRow(tr("GPU renderer"), rendererType);

	QCheckBox* enableRenderdoc = new QCheckBox(tr("Enable Renderdoc"));
	connectCheckbox(enableRenderdoc, config.enableRenderdoc);
	gpuLayout->addRow(enableRenderdoc);

	QCheckBox* shaderJitEnabled = new QCheckBox(tr("Enable shader JIT"));
	connectCheckbox(shaderJitEnabled, config.shaderJitEnabled);
	gpuLayout->addRow(shaderJitEnabled);

	QCheckBox* vsyncEnabled = new QCheckBox(tr("Enable VSync"));
	connectCheckbox(vsyncEnabled, config.vsyncEnabled);
	gpuLayout->addRow(vsyncEnabled);

	QCheckBox* useUbershaders = new QCheckBox(tr("Use ubershaders (No stutter, maybe slower)"));
	connectCheckbox(useUbershaders, config.useUbershaders);
	gpuLayout->addRow(useUbershaders);

	QCheckBox* accurateShaderMul = new QCheckBox(tr("Accurate shader multiplication"));
	connectCheckbox(accurateShaderMul, config.accurateShaderMul);
	gpuLayout->addRow(accurateShaderMul);

	QCheckBox* accelerateShaders = new QCheckBox(tr("Accelerate shaders"));
	connectCheckbox(accelerateShaders, config.accelerateShaders);
	gpuLayout->addRow(accelerateShaders);

	QCheckBox* hashTextures = new QCheckBox(tr("Hash textures"));
	hashTextures->setToolTip(tr("Enable this to reduce texture mismatches at the cost of slightly lower performance"));
	connectCheckbox(hashTextures, config.hashTextures);
	gpuLayout->addRow(hashTextures);

	QComboBox* screenLayout = new QComboBox();
	screenLayout->addItem(tr("Default"));
	screenLayout->addItem(tr("Default (Flipped)"));
	screenLayout->addItem(tr("Side-by-Side"));
	screenLayout->addItem(tr("Side-by-Side (Flipped)"));
	screenLayout->setCurrentIndex(static_cast<int>(config.screenLayout));
	connect(screenLayout, &QComboBox::currentIndexChanged, this, [&](int index) {
		config.screenLayout = static_cast<ScreenLayout::Layout>(index);
		updateConfig();
	});
	gpuLayout->addRow(tr("Screen Layout"), screenLayout);

	// Screen size slider widgets
	QLabel* topScreenSizeLabel = new QLabel(QString::number(int(config.topScreenSize * 100)));
	QSlider* topScreenSizeSlider = new QSlider(Qt::Horizontal);

	topScreenSizeSlider->setRange(0, 100);
	topScreenSizeSlider->setValue(int(config.topScreenSize * 100));
	connect(topScreenSizeSlider, &QSlider::valueChanged, this, [this, topScreenSizeLabel](int value) {
		config.topScreenSize = float(value) / 100.0f;
		topScreenSizeLabel->setText(QString::number(value));
		updateConfig();
	});

	QHBoxLayout* screenSizeLayout = new QHBoxLayout();
	screenSizeLayout->setSpacing(4);
	screenSizeLayout->addWidget(topScreenSizeSlider);
	screenSizeLayout->addWidget(topScreenSizeLabel);
	gpuLayout->addRow(tr("Top screen size (%)"), screenSizeLayout);

	QCheckBox* forceShadergenForLights = new QCheckBox(tr("Force shadergen when rendering lights"));
	connectCheckbox(forceShadergenForLights, config.forceShadergenForLights);
	gpuLayout->addRow(forceShadergenForLights);

	QSpinBox* lightShadergenThreshold = new QSpinBox;
	lightShadergenThreshold->setRange(1, 8);
	lightShadergenThreshold->setValue(config.lightShadergenThreshold);
	connect(lightShadergenThreshold, &QSpinBox::valueChanged, this, [&](int value) {
		config.lightShadergenThreshold = static_cast<int>(value);
		updateConfig();
	});
	gpuLayout->addRow(tr("Light threshold for forcing shadergen"), lightShadergenThreshold);

	// Audio settings
	QGroupBox* dspGroupBox = new QGroupBox(tr("Audio Settings"), this);
	QFormLayout* audioLayout = new QFormLayout(dspGroupBox);
	audioLayout->setHorizontalSpacing(20);
	audioLayout->setVerticalSpacing(10);

	QComboBox* dspType = new QComboBox();
	dspType->addItem(tr("Null"));
	dspType->addItem(tr("LLE"));
	dspType->addItem(tr("HLE"));
	dspType->setCurrentIndex(static_cast<int>(config.dspType));
	connect(dspType, &QComboBox::currentIndexChanged, this, [&](int index) {
		config.dspType = static_cast<Audio::DSPCore::Type>(index);
		updateConfig();
	});
	audioLayout->addRow(tr("DSP emulation"), dspType);

	QCheckBox* audioEnabled = new QCheckBox(tr("Enable audio"));
	connectCheckbox(audioEnabled, config.audioEnabled);
	audioLayout->addRow(audioEnabled);

	QCheckBox* aacEnabled = new QCheckBox(tr("Enable AAC audio"));
	connectCheckbox(aacEnabled, config.aacEnabled);
	audioLayout->addRow(aacEnabled);

	QCheckBox* printDSPFirmware = new QCheckBox(tr("Print DSP firmware"));
	connectCheckbox(printDSPFirmware, config.printDSPFirmware);
	audioLayout->addRow(printDSPFirmware);

	QCheckBox* muteAudio = new QCheckBox(tr("Mute audio device"));
	connectCheckbox(muteAudio, config.audioDeviceConfig.muteAudio);
	audioLayout->addRow(muteAudio);

	QComboBox* volumeCurveType = new QComboBox();
	volumeCurveType->addItem(tr("Cubic"));
	volumeCurveType->addItem(tr("Linear"));
	volumeCurveType->setCurrentIndex(static_cast<int>(config.audioDeviceConfig.volumeCurve));
	connect(volumeCurveType, &QComboBox::currentIndexChanged, this, [&](int index) {
		config.audioDeviceConfig.volumeCurve = static_cast<AudioDeviceConfig::VolumeCurve>(index);
		updateConfig();
	});
	audioLayout->addRow(tr("Volume curve"), volumeCurveType);

	QLabel* volumeLabel = new QLabel(QString::number(int(config.audioDeviceConfig.volumeRaw * 100)));

	QSlider* volumeSlider = new QSlider(Qt::Horizontal);
	volumeSlider->setRange(0, 200);
	volumeSlider->setValue(int(config.audioDeviceConfig.volumeRaw * 100));
	connect(volumeSlider, &QSlider::valueChanged, this, [this, volumeLabel](int value) {
		config.audioDeviceConfig.volumeRaw = float(value) / 100.0f;
		volumeLabel->setText(QString::number(value));

		updateConfig();
	});

	QHBoxLayout* volumeLayout = new QHBoxLayout();
	volumeLayout->setSpacing(4);
	volumeLayout->addWidget(volumeSlider);
	volumeLayout->addWidget(volumeLabel);
	audioLayout->addRow(tr("Audio device volume"), volumeLayout);

	inputWindow = new InputWindow(this);

	// Battery settings
	QGroupBox* batGroupBox = new QGroupBox(tr("Battery Settings"), this);
	QFormLayout* batLayout = new QFormLayout(batGroupBox);
	batLayout->setHorizontalSpacing(20);
	batLayout->setVerticalSpacing(10);

	QSpinBox* batteryPercentage = new QSpinBox;
	batteryPercentage->setRange(1, 100);
	batteryPercentage->setValue(config.batteryPercentage);
	connect(batteryPercentage, &QSpinBox::valueChanged, this, [&](int value) {
		config.batteryPercentage = static_cast<int>(value);
		updateConfig();
	});
	batLayout->addRow(tr("Battery percentage"), batteryPercentage);

	QCheckBox* chargerPlugged = new QCheckBox(tr("Charger plugged"));
	connectCheckbox(chargerPlugged, config.chargerPlugged);
	batLayout->addRow(chargerPlugged);

	// SD Card settings
	QGroupBox* sdcGroupBox = new QGroupBox(tr("SD Card Settings"), this);
	QFormLayout* sdcLayout = new QFormLayout(sdcGroupBox);
	sdcLayout->setHorizontalSpacing(20);
	sdcLayout->setVerticalSpacing(10);

	QCheckBox* sdCardInserted = new QCheckBox(tr("Enable virtual SD card"));
	connectCheckbox(sdCardInserted, config.sdCardInserted);
	sdcLayout->addRow(sdCardInserted);

	QCheckBox* sdWriteProtected = new QCheckBox(tr("Write protect virtual SD card"));
	connectCheckbox(sdWriteProtected, config.sdWriteProtected);
	sdcLayout->addRow(sdWriteProtected);

	// Add all our settings widgets to our widget list
	addWidget(guiGroupBox, tr("Interface"), ":/docs/img/sparkling_icon.png", tr("User Interface settings"));
	addWidget(genGroupBox, tr("General"), ":/docs/img/settings_icon.png", tr("General emulator settings"));
	addWidget(gpuGroupBox, tr("Graphics"), ":/docs/img/display_icon.png", tr("Graphics emulation and output settings"));
	addWidget(dspGroupBox, tr("Audio"), ":/docs/img/speaker_icon.png", tr("Audio emulation and output settings"));
	addWidget(inputWindow, tr("Input"), ":/docs/img/gamepad_icon.png", tr("Keyboard & controller input settings"));
	addWidget(batGroupBox, tr("Battery"), ":/docs/img/battery_icon.png", tr("Battery emulation settings"));
	addWidget(sdcGroupBox, tr("SD Card"), ":/docs/img/sdcard_icon.png", tr("SD Card emulation settings"));

	widgetList->setCurrentRow(0);
}

void ConfigWindow::setTheme(Theme theme) {
	switch (theme) {
		case Theme::Dark: {
			QApplication::setStyle(QStyleFactory::create("Fusion"));

			QPalette p;
			p.setColor(QPalette::Window, QColor(53, 53, 53));
			p.setColor(QPalette::WindowText, Qt::white);
			p.setColor(QPalette::Base, QColor(25, 25, 25));
			p.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
			p.setColor(QPalette::ToolTipBase, Qt::white);
			p.setColor(QPalette::ToolTipText, Qt::white);
			p.setColor(QPalette::Text, Qt::white);
			p.setColor(QPalette::Button, QColor(53, 53, 53));
			p.setColor(QPalette::ButtonText, Qt::white);
			p.setColor(QPalette::BrightText, Qt::red);
			p.setColor(QPalette::Link, QColor(42, 130, 218));

			p.setColor(QPalette::Highlight, QColor(42, 130, 218));
			p.setColor(QPalette::HighlightedText, Qt::black);
			qApp->setPalette(p);
			break;
		}

		case Theme::Light: {
			QApplication::setStyle(QStyleFactory::create("Fusion"));

			QPalette p;
			p.setColor(QPalette::Window, Qt::white);
			p.setColor(QPalette::WindowText, Qt::black);
			p.setColor(QPalette::Base, QColor(243, 243, 243));
			p.setColor(QPalette::AlternateBase, Qt::white);
			p.setColor(QPalette::ToolTipBase, Qt::black);
			p.setColor(QPalette::ToolTipText, Qt::black);
			p.setColor(QPalette::Text, Qt::black);
			p.setColor(QPalette::Button, Qt::white);
			p.setColor(QPalette::ButtonText, Qt::black);
			p.setColor(QPalette::BrightText, Qt::red);
			p.setColor(QPalette::Link, QColor(42, 130, 218));

			p.setColor(QPalette::Highlight, QColor(42, 130, 218));
			p.setColor(QPalette::HighlightedText, Qt::white);
			qApp->setPalette(p);
			break;
		}

		case Theme::GreetingsCat: {
			QApplication::setStyle(QStyleFactory::create("Fusion"));

			QPalette p;
			p.setColor(QPalette::Window, QColor(250, 207, 228));
			p.setColor(QPalette::WindowText, QColor(225, 22, 137));
			p.setColor(QPalette::Base, QColor(250, 207, 228));
			p.setColor(QPalette::AlternateBase, QColor(250, 207, 228));
			p.setColor(QPalette::ToolTipBase, QColor(225, 22, 137));
			p.setColor(QPalette::ToolTipText, QColor(225, 22, 137));
			p.setColor(QPalette::Text, QColor(225, 22, 137));
			p.setColor(QPalette::Button, QColor(250, 207, 228));
			p.setColor(QPalette::ButtonText, QColor(225, 22, 137));
			p.setColor(QPalette::BrightText, Qt::black);
			p.setColor(QPalette::Link, QColor(42, 130, 218));

			p.setColor(QPalette::Highlight, QColor(42, 130, 218));
			p.setColor(QPalette::HighlightedText, Qt::black);
			qApp->setPalette(p);
			break;
		}

		case Theme::Cream: {
			QApplication::setStyle(QStyleFactory::create("Fusion"));

			QPalette p;
			p.setColor(QPalette::Window, QColor(255, 229, 180));
			p.setColor(QPalette::WindowText, QColor(33, 37, 41));
			p.setColor(QPalette::Base, QColor(255, 229, 180));
			p.setColor(QPalette::AlternateBase, QColor(255, 229, 180));
			p.setColor(QPalette::ToolTipBase, QColor(33, 37, 41));
			p.setColor(QPalette::ToolTipText, QColor(33, 37, 41));
			p.setColor(QPalette::Text, QColor(33, 37, 41));
			p.setColor(QPalette::Button, QColor(255, 229, 180));
			p.setColor(QPalette::ButtonText, QColor(33, 37, 41));
			p.setColor(QPalette::BrightText, QColor(217, 113, 103));
			p.setColor(QPalette::Link, QColor(248, 148, 150));

			p.setColor(QPalette::Highlight, QColor(217, 113, 103));
			p.setColor(QPalette::HighlightedText, QColor(63, 33, 29));
			qApp->setPalette(p);
			break;
		}

		case Theme::Oled: {
			QApplication::setStyle(QStyleFactory::create("Fusion"));

			QPalette p;
			p.setColor(QPalette::Window, Qt::black);
			p.setColor(QPalette::WindowText, Qt::white);
			p.setColor(QPalette::Base, Qt::black);
			p.setColor(QPalette::AlternateBase, Qt::black);
			p.setColor(QPalette::ToolTipBase, Qt::black);
			p.setColor(QPalette::ToolTipText, Qt::white);
			p.setColor(QPalette::Text, Qt::white);
			p.setColor(QPalette::Button, QColor(5, 5, 5));
			p.setColor(QPalette::ButtonText, Qt::white);
			p.setColor(QPalette::BrightText, Qt::red);
			p.setColor(QPalette::Link, QColor(42, 130, 218));

			p.setColor(QPalette::Highlight, QColor(42, 130, 218));
			p.setColor(QPalette::HighlightedText, Qt::black);
			qApp->setPalette(p);
			qApp->setStyleSheet(
				"QLineEdit {"
				"background-color: #000000; color: #ffffff; border: 1px solid #a0a0a0; "
				"border-radius: 4px; padding: 5px; }"

				"QCheckBox::indicator:unchecked {"
				"border: 1px solid #808080; border-radius: 4px; }"
			);
			break;
		}

		case Theme::System: {
			qApp->setPalette(this->style()->standardPalette());
			qApp->setStyle(QStyleFactory::create("WindowsVista"));
			qApp->setStyleSheet("");
			break;
		}
	}
}

void ConfigWindow::setIcon(WindowIcon icon) {
	auto updateIcon = [&](const QString& iconPath) { getMainWindow()->setWindowIcon(QIcon(iconPath)); };

	switch (icon) {
		case WindowIcon::Rsyn: updateIcon(":/docs/img/rsyn_icon.png"); break;
		case WindowIcon::Rnap: updateIcon(":/docs/img/rnap_icon.png"); break;
		case WindowIcon::Rcow: updateIcon(":/docs/img/rcow_icon.png"); break;
		case WindowIcon::SkyEmu: updateIcon(":/docs/img/skyemu_icon.png"); break;
		case WindowIcon::Runpog: updateIcon(":/docs/img/runpog_icon.png"); break;

		case WindowIcon::Rpog:
		default: updateIcon(":/docs/img/rpog_icon.png"); break;
	}
}

void ConfigWindow::addWidget(QWidget* widget, QString title, QString icon, QString helpText) {
	const int index = widgetList->count();

	QListWidgetItem* item = new QListWidgetItem(widgetList);
	item->setText(title);
	if (!icon.isEmpty()) {
		item->setIcon(QIcon::fromTheme(icon));
	}

	widgetContainer->addWidget(widget);

	if (index >= settingWidgetCount) {
		Helpers::panic("Qt: ConfigWindow::settingWidgetCount has not been updated correctly!");
	}
	helpTexts[index] = std::move(helpText);
}

ConfigWindow::~ConfigWindow() {
	delete helpText;
	delete widgetList;
	delete widgetContainer;
}
