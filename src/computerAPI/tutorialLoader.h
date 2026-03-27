#ifndef tutorialloader_h
#define tutorialloader_h

#include "gui/viewportManager/circuitView/tutorial.h"

constexpr std::string STEP_TOKEN = "Step:";

constexpr std::string ACTION_TOKEN = "Action:";
constexpr std::string MESSAGE_TOKEN = "Message:";
constexpr std::string BLOCK_PREVIEW_TOKEN = "Block";
constexpr std::string CONNECTION_PREVIEW_TOKEN = "Connection";
constexpr std::string VIEW_CENTER_TOKEN = "View";
constexpr std::string PLACE_BLOCK_TOKEN = "Block:";
constexpr std::string PLACE_CONNECTION_TOKEN = "Connection:";
constexpr std::string ACTIONS[] = {
	MESSAGE_TOKEN, BLOCK_PREVIEW_TOKEN, CONNECTION_PREVIEW_TOKEN, VIEW_CENTER_TOKEN, PLACE_BLOCK_TOKEN, PLACE_CONNECTION_TOKEN,
};

constexpr std::string CONDITION_TOKEN = "Condition:";
constexpr std::string BLOCK_CONDITION_TOKEN = "Block:";
constexpr std::string CONNECTION_CONDITION_TOKEN = "Connection:";
constexpr std::string LOGIC_STATE_CONDITION_TOKEN = "State:";
constexpr std::string TRUTH_TABLE_TOKEN = "Truth";
constexpr std::string CONDITIONS[] = {
	BLOCK_CONDITION_TOKEN,
	CONNECTION_CONDITION_TOKEN,
	LOGIC_STATE_CONDITION_TOKEN,
	TRUTH_TABLE_TOKEN,
};

std::vector<TutorialStep> parseTutorialFile(std::string fileName);

#endif