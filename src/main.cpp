#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/modify/ProfilePage.hpp>

using namespace geode::prelude;

std::map<std::string, ccColor3B> colors_map = {
	{"online", cc3bFromHexString("#00FF2A").value()},
	{"idle", cc3bFromHexString("#FFEE00").value()},
	{"dnd", cc3bFromHexString("#FF0000").value()},
	{"offline", cc3bFromHexString("#BEBEBE").value()},
	{"null", cc3bFromHexString("#BEBEBE").value()}
};


class $modify(StatusProfilePage, ProfilePage) {
	bool status_own_profile;
	bool status_loaded;
	std::string status_string;


	void roundRobinStatus() {
		if (m_fields->status_string == "null") {
			m_fields->status_string = "online";
			return;
		}

		if (m_fields->status_string == "online") {
			m_fields->status_string = "idle";
			return;
		}

		if (m_fields->status_string == "idle") {
			m_fields->status_string = "dnd";
			return;
		}

		if (m_fields->status_string == "dnd") {
			m_fields->status_string = "offline";
			return;
		}

		if (m_fields->status_string == "offline") {
			m_fields->status_string = "online";
			return;
		}
	}


	void onStatusClick(CCObject* sender) {
		if (!m_fields->status_loaded) return;

		if (m_accountID != GJAccountManager::get()->m_accountID) {
			const char* user_status = m_fields->status_string == "null" ? "Not set" : m_fields->status_string.c_str();

			FLAlertLayer::create(
				std::string(std::string(m_usernameLabel->getString()) + std::string("'s status")).c_str(),
				user_status,
				"OK"
			)->show();
			return;
		}

		log::debug("Status string: {}", m_fields->status_string);

		roundRobinStatus();

		log::debug("Updated status string: {}", m_fields->status_string);

		updateStatusDot(m_fields->status_string);
		web::AsyncWebRequest()
			.fetch(
				"https://gdstatus.7m.pl/updateStatus.php?accountID=" 
				+ std::to_string(m_accountID) 
				+ "&status=" 
				+ m_fields->status_string
			)
			.text()
			.then([&](std::string const& result) {
				log::info("Updated status");
				Notification::create(
					"Updated status to " + m_fields->status_string,
					CCSprite::create("GJ_completesIcon_001.png")
				)->show();
			})
			.expect([](std::string const& error) {
				Notification::create(
					"Error updating status: " + error,
					CCSprite::create("GJ_deleteIcon_001.png")
				)->show();
			});
	}

	void addStatusDot(bool own_profile) {
		m_fields->status_own_profile = own_profile;

		auto status_dot = CCMenuItemLabel::create(
			CCLabelBMFont::create(".", "bigFont.fnt"), 
			this, menu_selector(StatusProfilePage::onStatusClick)
		);

		auto status_menu = CCMenu::create();

		status_menu->addChild(status_dot);
		addChild(status_menu);

		status_dot->setID("status_dot"_spr);

		status_menu->setPosition({116, 239});
		status_dot->setScale(1.825f);

		status_dot->setColor(cc3bFromHexString("#BEBEBE").value());
	}

	void updateStatusDot(std::string status) {
		m_fields->status_string = status;

		auto status_dot = static_cast<CCMenuItemLabel*>(getChildByIDRecursive("status_dot"_spr));
		status_dot->setColor(colors_map[status]);
	}

	void setupStatus(int account_id, bool own_profile) {
		addStatusDot(own_profile);

		log::info("Account ID: {}", account_id);

		web::AsyncWebRequest()
			.fetch("https://gdstatus.7m.pl/getStatus.php?accountID=" + std::to_string(account_id))
			.text()
			.then([&](std::string const& result) {
				m_fields->status_loaded = true;
				updateStatusDot(result);
			})
			.expect([](std::string const& error) {
				Notification::create(
					std::string("Error fetching status: ") + error,
					CCSprite::create("GJ_deleteIcon_001.png")
				)->show();
			});
		
	}


	bool init(int account_id, bool own_profile) {
		if (!ProfilePage::init(account_id, own_profile)) return false;

		m_fields->status_loaded = false;
		setupStatus(account_id, own_profile);

		return true;
	}
};