//
// Created by PC-SAMUEL on 23/11/2024.
//

#include "config.h"
#include "globals.h"

#include <regex>
#include <set>
#include <filesystem>
#include <toml.hpp>

namespace souls_vision {

Components Config::components;
BarSettings Config::statBarSettings;
float Config::bestEffectIconSize = 39.0f;
float Config::dmgTypeIconSize = 30.0f;
float Config::effectBarIconSize = 48.0f;
int Config::bestEffects = 7;
int Config::statBarSpacing = 0;
float Config::fontSize = 18.0f;
float Config::opacity;
int Config::delay = 0;
bool Config::debug = false;
bool Config::dragOverlay = false;
bool Config::configUpdated = false;
bool Config::opacityUpdated = false;
bool Config::fontSizeUpdated = false;
bool Config::hideBlightMadness = false;
int Config::maxEffectBars = 7;

void Config::SaveConfig(const std::string& configFilePath) {
    try {
        toml::table configToml;

        configToml.insert_or_assign("general", toml::table{
                {"debug", debug},
                {"dragOverlay", dragOverlay},
                {"fontSize", fontSize},
                {"opacity", opacity},
                {"delay", delay},
        });

        configToml.insert_or_assign("appearance", toml::table{
                {"bestEffects", bestEffects},
                {"maxEffectBars", maxEffectBars},
                {"bestEffectIconSize", bestEffectIconSize},
                {"dmgTypeIconSize", dmgTypeIconSize},
                {"statBarSpacing", statBarSpacing},
                {"hideBlightMadness", hideBlightMadness}
        });

        toml::table statBar;
        statBar.insert_or_assign("position", toml::table{
                {"x", statBarSettings.position.x},
                {"y", statBarSettings.position.y}
        });
        statBar.insert_or_assign("size", toml::table{
                {"width", statBarSettings.size.x},
                {"height", statBarSettings.size.y}
        });
        configToml.insert_or_assign("statBar", statBar);

        toml::table componentsTable;
        componentsTable.insert_or_assign("hp", toml::table{{"visible", components.hp.visible}, {"hideText", components.hp.hideText}});
        componentsTable.insert_or_assign("fp", toml::table{{"visible", components.fp.visible}, {"hideText", components.fp.hideText}});
        componentsTable.insert_or_assign("stamina", toml::table{{"visible", components.stamina.visible}, {"hideText", components.stamina.hideText}});
        componentsTable.insert_or_assign("stagger", toml::table{{"visible", components.stagger.visible}, {"hideText", components.stagger.hideText}});
        componentsTable.insert_or_assign("poison", toml::table{{"visible", components.poison.visible}, {"hideText", components.poison.hideText}});
        componentsTable.insert_or_assign("scarletRot", toml::table{{"visible", components.scarletRot.visible}, {"hideText", components.scarletRot.hideText}});
        componentsTable.insert_or_assign("hemorrhage", toml::table{{"visible", components.hemorrhage.visible}, {"hideText", components.hemorrhage.hideText}});
        componentsTable.insert_or_assign("deathBlight", toml::table{{"visible", components.deathBlight.visible}, {"hideText", components.deathBlight.hideText}});
        componentsTable.insert_or_assign("frostbite", toml::table{{"visible", components.frostbite.visible}, {"hideText", components.frostbite.hideText}});
        componentsTable.insert_or_assign("sleep", toml::table{{"visible", components.sleep.visible}, {"hideText", components.sleep.hideText}});
        componentsTable.insert_or_assign("madness", toml::table{{"visible", components.madness.visible}, {"hideText", components.madness.hideText}});
        componentsTable.insert_or_assign("bestEffects", components.bestEffects);
        componentsTable.insert_or_assign("immuneEffects", components.immuneEffects);
        componentsTable.insert_or_assign("dmgTypes", components.dmgTypes);
        componentsTable.insert_or_assign("neutralDmgTypes", components.neutralDmgTypes);
        configToml.insert_or_assign("components", componentsTable);

        std::ofstream configFile(configFilePath);
        if (!configFile.is_open()) {
            throw std::runtime_error("Failed to open " + configFilePath + " for writing.");
        }

        configFile << configToml;
        configFile.close();

        AddComments(configFilePath);
    } catch (const std::exception& e) {
        Logger::Error(std::string("Config::SaveConfig - Error: ") + e.what());
    }
}

void Config::LoadConfig(const std::string& configFilePath) {
    try {
        toml::table configToml;

        if (!std::filesystem::exists(configFilePath)) {
            Logger::Info("Config file not found. Creating a new one...");
            CreateConfig(configFilePath);

            std::ifstream configFile(configFilePath);
            if (!configFile.is_open()) {
                throw std::runtime_error("Failed to open " + configFilePath + " after creation.");
            }
            configToml = toml::parse(configFile);
        } else {
            std::ifstream configFile(configFilePath);
            if (!configFile.is_open()) {
                throw std::runtime_error("Failed to open " + configFilePath);
            }
            configToml = toml::parse(configFile);
        }

        debug = configToml["general"]["debug"].value_or(false);
        dragOverlay = configToml["general"]["dragOverlay"].value_or(false);
        hideBlightMadness = configToml["general"]["hideBlightMadness"].value_or(false);
        fontSize = configToml["general"]["fontSize"].value_or(18.0f);
        opacity = configToml["general"]["opacity"].value_or(0.9f);
        delay = configToml["general"]["delay"].value_or(0);

        fontSizeUpdated = (fontSize != configToml["general"]["fontSize"].value_or(fontSize));
        opacityUpdated = (opacity != configToml["general"]["opacity"].value_or(opacity));

        bestEffects = configToml["appearance"]["bestEffects"].value_or(7);
        bestEffectIconSize = configToml["appearance"]["bestEffectIconSize"].value_or(39.0f);
        dmgTypeIconSize = configToml["appearance"]["dmgTypeIconSize"].value_or(30.0f);
        statBarSpacing = configToml["appearance"]["statBarSpacing"].value_or(0);
        maxEffectBars = configToml["appearance"]["maxEffectBars"].value_or(7);

        auto statBarTable = configToml["statBar"].as_table();
        if (statBarTable) {
            auto positionTable = statBarTable->at("position").as_table();
            if (positionTable) {
                statBarSettings.position.x = positionTable->at("x").value_or(gGameWindowSize.width - 555 - 5);
                statBarSettings.position.y = positionTable->at("y").value_or(10);
            }

            auto sizeTable = statBarTable->at("size").as_table();
            if (sizeTable) {
                statBarSettings.size.x = sizeTable->at("width").value_or(555);
                statBarSettings.size.y = sizeTable->at("height").value_or(40);
            }
        }

        float iconWidth = statBarSettings.size.y * 1.70f;
        effectBarIconSize = iconWidth * 0.85f;

        auto componentsTable = configToml["components"].as_table();
        if (componentsTable) {
            components.hp.visible = componentsTable->at("hp").as_table()->at("visible").value_or(true);
            components.hp.hideText = componentsTable->at("hp").as_table()->at("hideText").value_or(false);

            components.fp.visible = componentsTable->at("fp").as_table()->at("visible").value_or(true);
            components.fp.hideText = componentsTable->at("fp").as_table()->at("hideText").value_or(false);

            components.stamina.visible = componentsTable->at("stamina").as_table()->at("visible").value_or(true);
            components.stamina.hideText = componentsTable->at("stamina").as_table()->at("hideText").value_or(false);

            components.stagger.visible = componentsTable->at("stagger").as_table()->at("visible").value_or(true);
            components.stagger.hideText = componentsTable->at("stagger").as_table()->at("hideText").value_or(false);

            components.poison.visible = componentsTable->at("poison").as_table()->at("visible").value_or(true);
            components.poison.hideText = componentsTable->at("poison").as_table()->at("hideText").value_or(false);

            components.scarletRot.visible = componentsTable->at("scarletRot").as_table()->at("visible").value_or(true);
            components.scarletRot.hideText = componentsTable->at("scarletRot").as_table()->at("hideText").value_or(false);

            components.hemorrhage.visible = componentsTable->at("hemorrhage").as_table()->at("visible").value_or(true);
            components.hemorrhage.hideText = componentsTable->at("hemorrhage").as_table()->at("hideText").value_or(false);

            components.deathBlight.visible = componentsTable->at("deathBlight").as_table()->at("visible").value_or(true);
            components.deathBlight.hideText = componentsTable->at("deathBlight").as_table()->at("hideText").value_or(false);

            components.frostbite.visible = componentsTable->at("frostbite").as_table()->at("visible").value_or(true);
            components.frostbite.hideText = componentsTable->at("frostbite").as_table()->at("hideText").value_or(false);

            components.sleep.visible = componentsTable->at("sleep").as_table()->at("visible").value_or(true);
            components.sleep.hideText = componentsTable->at("sleep").as_table()->at("hideText").value_or(false);

            components.madness.visible = componentsTable->at("madness").as_table()->at("visible").value_or(true);
            components.madness.hideText = componentsTable->at("madness").as_table()->at("hideText").value_or(false);

            components.bestEffects = componentsTable->at("bestEffects").value_or(true);
            components.immuneEffects = componentsTable->at("immuneEffects").value_or(true);
            components.dmgTypes = componentsTable->at("dmgTypes").value_or(true);
            components.neutralDmgTypes = componentsTable->at("neutralDmgTypes").value_or(false);
        }

        Logger::Info("Config loaded successfully.");
    } catch (const std::exception& e) {
        Logger::Error(std::string("Config::LoadConfig - Error: ") + e.what());
    }
}

void Config::CreateConfig(const std::string& configFilePath) {
    Size barSize = {600, 40};

    toml::table configToml;

    configToml.insert_or_assign("general", toml::table{
            {"debug", false},
            {"dragOverlay", false},
            {"fontSize", 18.0f},
            {"opacity", 0.9f},
            {"delay", 0},
    });

    configToml.insert_or_assign("appearance", toml::table{
            {"bestEffects", 7},
            {"maxEffectBars", 7},
            {"bestEffectIconSize", 39},
            {"dmgTypeIconSize", 30},
            {"statBarSpacing", 0},
            {"hideBlightMadness", false}
    });

    toml::table statBar;
    statBar.insert_or_assign("position", toml::table{
            {"x", 0},
            {"y", 0}
    });
    statBar.insert_or_assign("size", toml::table{
            {"width", barSize.width},
            {"height", barSize.height}
    });
    configToml.insert_or_assign("statBar", statBar);

    toml::table componentsTable;
    componentsTable.insert_or_assign("hp", toml::table{{"visible", true}, {"hideText", false}});
    componentsTable.insert_or_assign("fp", toml::table{{"visible", true}, {"hideText", false}});
    componentsTable.insert_or_assign("stamina", toml::table{{"visible", true}, {"hideText", false}});
    componentsTable.insert_or_assign("stagger", toml::table{{"visible", true}, {"hideText", false}});
    componentsTable.insert_or_assign("poison", toml::table{{"visible", true}, {"hideText", false}});
    componentsTable.insert_or_assign("scarletRot", toml::table{{"visible", true}, {"hideText", false}});
    componentsTable.insert_or_assign("hemorrhage", toml::table{{"visible", true}, {"hideText", false}});
    componentsTable.insert_or_assign("deathBlight", toml::table{{"visible", true}, {"hideText", false}});
    componentsTable.insert_or_assign("frostbite", toml::table{{"visible", true}, {"hideText", false}});
    componentsTable.insert_or_assign("sleep", toml::table{{"visible", true}, {"hideText", false}});
    componentsTable.insert_or_assign("madness", toml::table{{"visible", true}, {"hideText", false}});
    componentsTable.insert_or_assign("bestEffects", true);
    componentsTable.insert_or_assign("immuneEffects", true);
    componentsTable.insert_or_assign("dmgTypes", true);
    componentsTable.insert_or_assign("neutralDmgTypes", false);
    configToml.insert_or_assign("components", componentsTable);

    std::ofstream configFile(configFilePath);
    if (!configFile.is_open()) {
        throw std::runtime_error("Failed to create config file: " + configFilePath);
    }

    configFile << configToml;
    configFile.close();

    AddComments(configFilePath);
}

void Config::AddComments(const std::string& configFilePath) {
    try {
        std::string content;

        std::ifstream fileIn(configFilePath);
        if (fileIn.is_open()) {
            std::stringstream buffer;
            buffer << fileIn.rdbuf();
            content = buffer.str();
            fileIn.close();
        } else {
            return;
        }

        std::regex pattern("^(\\w+)\\s*=", std::regex::multiline);
        std::sregex_iterator it(content.begin(), content.end(), pattern);
        std::sregex_iterator end;

        std::set<std::string> existingKeys;
        while (it != end) {
            existingKeys.insert(it->str(1));
            ++it;
        }

        std::map<std::string, std::string> comments = {
                {"general", "# [General]\n# General settings\n"},
                {"debug", "# Enable debug mode\n"},
                {"dragOverlay", "# Enable overlay dragging\n"},
                {"fontSize", "# Font size for overlay text\n"},
                {"opacity", "# Overlay opacity (0.0-1.0)\n"},
                {"delay", "# Delay in milliseconds before overlay starts\n"},
                {"appearance", "\n# [Appearance]\n# Visual appearance settings\n"},
                {"bestEffects", "# Number of best effects to display\n"},
                {"maxEffectBars", "# Maximum effect bars to display\n"},
                {"bestEffectIconSize", "# Size of effect icons\n"},
                {"dmgTypeIconSize", "# Size of damage type icons\n"},
                {"statBarSpacing", "# Spacing between stat bars\n"},
                {"hideBlightMadness", "# Hide death blight and madness\n"},
                {"statBar", "\n# [StatBar]\n# Stat bar position and size\n"},
                {"position", "# Position of stat bars\n"},
                {"x", "# X coordinate\n"},
                {"y", "# Y coordinate\n"},
                {"size", "# Size of stat bars\n"},
                {"width", "# Bar width\n"},
                {"height", "# Bar height\n"},
                {"components", "\n# [Components]\n# Component visibility and settings\n"},
                {"hp", "# Health points\n"},
                {"fp", "# Focus points\n"},
                {"stamina", "# Stamina\n"},
                {"stagger", "# Stagger\n"},
                {"poison", "# Poison status\n"},
                {"scarletRot", "# Scarlet rot status\n"},
                {"hemorrhage", "# Hemorrhage status\n"},
                {"deathBlight", "# Death blight status\n"},
                {"frostbite", "# Frostbite status\n"},
                {"sleep", "# Sleep status\n"},
                {"madness", "# Madness status\n"},
                {"bestEffects", "# Show best effects\n"},
                {"immuneEffects", "# Show immunity effects\n"},
                {"dmgTypes", "# Show damage types\n"},
                {"neutralDmgTypes", "# Show neutral damage types\n"},
                {"visible", "# Visibility\n"},
                {"hideText", "# Hide text\n"}
        };

        std::stringstream newContent;
        std::string::iterator contentIt = content.begin();
        bool newLine = true;
        bool insertedGeneral = false;
        bool insertedAppearance = false;
        bool insertedStatBar = false;
        bool insertedComponents = false;

        for (size_t i = 0; i < content.size(); ++i) {
            if (newLine) {
                size_t lineEnd = content.find('\n', i);
                if (lineEnd == std::string::npos) lineEnd = content.size();
                std::string line = content.substr(i, lineEnd - i);

                std::regex lineRegex("^(\\w+)");
                std::smatch match;
                if (std::regex_search(line, match, lineRegex)) {
                    std::string key = match.str(1);

                    if (key == "general" && !insertedGeneral) {
                        newContent << comments["general"];
                        insertedGeneral = true;
                    } else if (key == "appearance" && !insertedAppearance) {
                        newContent << comments["appearance"];
                        insertedAppearance = true;
                    } else if (key == "statBar" && !insertedStatBar) {
                        newContent << comments["statBar"];
                        insertedStatBar = true;
                    } else if (key == "components" && !insertedComponents) {
                        newContent << comments["components"];
                        insertedComponents = true;
                    } else if (comments.find(key) != comments.end() && comments[key].length() > 1) {
                        newContent << comments[key];
                    }
                }
                newLine = false;
            }

            if (content[i] == '\n') {
                newLine = true;
            }
            newContent << content[i];
        }

        std::ofstream fileOut(configFilePath);
        if (fileOut.is_open()) {
            fileOut << newContent.str();
            fileOut.close();
        }
    } catch (const std::exception& e) {
        Logger::Error(std::string("Config::AddComments - Error: ") + e.what());
    }
}

} // namespace souls_vision