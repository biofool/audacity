/*
* Audacity: A Digital Audio Editor
*/
#include "recordcommandsregister.h"

#include "framework/ui/view/iconcodes.h"
#include "framework/global/types/translatablestring.h"

#include "../recordcommands.h"

using namespace au::record;
using namespace muse;
using namespace muse::rcommand;
using namespace muse::ui;

namespace {
const std::vector<CommandInfo> s_commandInfos = {
    CommandInfo{
        RECORD_START_COMMAND,
        //: Action title: shown as a menu item or a button label; keep it short
        TranslatableString("action", "Record"),
        //: Action description: shown as a tooltip; can be a full sentence
        TranslatableString("action_description", "Start recording at the playhead — adds a new clip on the selected track; existing audio is kept"),
        InputSchema(),
        Decoration(IconCode::Code::RECORD_FILL)
    },
    CommandInfo{
        RECORD_ON_CURRENT_TRACK_COMMAND,
        //: Action title: shown as a menu item or a button label; keep it short
        TranslatableString("action", "Record on current track"),
        //: Action description: shown as a tooltip; can be a full sentence
        TranslatableString("action_description", "Record at the playhead on the selected track — adds a new clip; existing audio is kept"),
        InputSchema(),
        Decoration()
    },
    CommandInfo{
        RECORD_ON_NEW_TRACK_COMMAND,
        //: Action title: shown as a menu item or a button label; keep it short
        TranslatableString("action", "Record on new track"),
        //: Action description: shown as a tooltip; can be a full sentence
        TranslatableString("action_description", "Record on a new track — existing tracks and clips are left untouched"),
        InputSchema(),
        Decoration()
    },
    CommandInfo{
        RECORD_PAUSE_COMMAND,
        //: Action title: shown as a menu item or a button label; keep it short
        TranslatableString("action", "Pause"),
        //: Action description: shown as a tooltip; can be a full sentence
        TranslatableString("action_description", "Pause"),
        InputSchema(),
        Decoration(IconCode::Code::PAUSE_FILL)
    },
    CommandInfo{
        RECORD_STOP_COMMAND,
        //: Action title: shown as a menu item or a button label; keep it short
        TranslatableString("action", "Stop"),
        //: Action description: shown as a tooltip; can be a full sentence
        TranslatableString("action_description", "Stop record"),
        InputSchema(),
        Decoration(IconCode::Code::STOP_FILL)
    },
    CommandInfo{
        RECORD_LEVEL_COMMAND,
        //: Action title: shown as a menu item or a button label; keep it short
        TranslatableString("action", "Record level"),
        //: Action description: shown as a tooltip; can be a full sentence
        TranslatableString("action_description", "Recording input level and meter — click to adjust input volume, input monitoring and mic metering"),
        InputSchema(),
        Decoration(IconCode::Code::MICROPHONE)
    },
    CommandInfo{
        RECORD_TOGGLE_MIC_METERING_COMMAND,
        //: Action title: shown as a menu item or a button label; keep it short
        TranslatableString("action", "Show mic metering"),
        //: Action description: shown as a tooltip; can be a full sentence
        TranslatableString("action_description", "Show the input level on the record meter even when not recording — use it to check levels before you record"),
        InputSchema(),
        Decoration(rcommand::Checkable::Yes)
    },
    CommandInfo{
        RECORD_TOGGLE_INPUT_MONITORING_COMMAND,
        //: Action title: shown as a menu item or a button label; keep it short
        TranslatableString("action", "Turn on input monitoring"),
        //: Action description: shown as a tooltip; can be a full sentence
        TranslatableString("action_description", "Hear your input through the playback device while recording — use headphones to avoid feedback, and expect some latency"),
        InputSchema(),
        Decoration(rcommand::Checkable::Yes)
    },
    CommandInfo{
        RECORD_LEAD_IN_RECORDING_COMMAND,
        //: Action title: shown as a menu item or a button label; keep it short
        TranslatableString("action", "Lead-in Recording"),
        //: Action description: shown as a tooltip; can be a full sentence
        TranslatableString("action_description", "Play a few seconds before the playhead for context, then record — existing audio is kept and crossfaded into"),
        InputSchema(),
        Decoration(IconCode::Code::RECORD_FILL)
    },
};
}

std::string RecordCommandsRegister::moduleName() const
{
    return "record";
}

const std::vector<Command>& RecordCommandsRegister::commandList() const
{
    static std::vector<muse::rcommand::Command> commands;
    if (commands.empty()) {
        commands.reserve(s_commandInfos.size());
        for (const auto& info : s_commandInfos) {
            commands.push_back(info.command);
        }
    }
    return commands;
}

const std::vector<CommandInfo>& RecordCommandsRegister::commandInfoList() const
{
    return s_commandInfos;
}
