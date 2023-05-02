#include "FormUtil.h"

RE::TESForm* FormUtil::GetFormFromIdentifier(const std::string& a_identifier) {
    std::istringstream ss{a_identifier};
    std::string plugin, id;

    std::getline(ss, plugin, '|');
    std::getline(ss, id);
    RE::FormID relativeID;
    std::istringstream(id) >> std::hex >> relativeID;
    const auto dataHandler = RE::TESDataHandler::GetSingleton();
    return dataHandler ? dataHandler->LookupForm(relativeID, plugin) : nullptr;
}

std::string FormUtil::GetIdentifierFromForm(const RE::TESForm* a_form) {
    if (!a_form) {
        return ""s;
    }

    auto file = a_form->GetFile();
    auto plugin = file ? file->fileName : "";

    RE::FormID formID = a_form->GetFormID();
    RE::FormID relativeID = formID & 0x00FFFFFF;

#ifndef SKYRIMVR
    if (file && file->recordFlags.all(RE::TESFile::RecordFlag::kSmallFile)) {
        relativeID &= 0x00000FFF;
    }
#endif

    std::ostringstream ss;
    ss << plugin << "|" << std::hex << relativeID;
    return ss.str();
}

std::string FormUtil::GetModName(const RE::TESForm* a_form) {
    if (!a_form) return ""s;

    const auto array = a_form->sourceFiles.array;
    if (!array || array->empty()) {
        return ""s;
    }

    const auto file = array->front();
    const auto filename = file ? file->GetFilename() : ""sv;
    return std::filesystem::path(filename).stem().string();
}