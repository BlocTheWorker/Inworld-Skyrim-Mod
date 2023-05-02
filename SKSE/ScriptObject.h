#pragma once

namespace ScriptObject {
    ScriptObjectPtr FromForm(const RE::TESForm* a_form, const std::string& a_scriptName);

    auto GetVariable(ScriptObjectPtr a_object, std::string_view a_variableName) -> RE::BSScript::Variable*;

    bool IsType(ScriptObjectPtr a_object, const char* a_scriptName);

    bool GetBool(ScriptObjectPtr a_object, std::string_view a_variableName);

    void SetBool(ScriptObjectPtr a_object, std::string_view a_variableName, bool a_value);

    std::int32_t GetInt(ScriptObjectPtr a_object, std::string_view a_variableName);

    void SetInt(ScriptObjectPtr a_object, std::string_view a_variableName, std::int32_t a_value);

    float GetFloat(ScriptObjectPtr a_object, std::string_view a_variableName);

    void SetFloat(ScriptObjectPtr a_object, std::string_view a_variableName, float a_value);

    std::string GetString(ScriptObjectPtr a_object, std::string_view a_variableName);

    void SetString(ScriptObjectPtr a_object, std::string_view a_variableName, std::string_view a_value);

    ScriptArrayPtr GetArray(ScriptObjectPtr a_object, std::string_view a_variableName);

    void RegisterForModEvent(ScriptObjectPtr a_object, RE::BSFixedString a_eventName, RE::BSFixedString a_callbackName);

    void UnregisterForModEvent(ScriptObjectPtr a_object, RE::BSFixedString a_eventName);
}