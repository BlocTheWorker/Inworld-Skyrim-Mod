#include "ScriptObject.h"

#include "FormUtil.h"

ScriptObjectPtr ScriptObject::FromForm(const RE::TESForm* a_form, const std::string& a_scriptName) {
    ScriptObjectPtr object;

    if (!a_form) {

        return object;
    }

    const auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
    if (!vm) {
        return object;
    }

    const auto typeID = static_cast<RE::VMTypeID>(a_form->GetFormType());
    const auto policy = vm->GetObjectHandlePolicy();
    const auto handle = policy ? policy->GetHandleForObject(typeID, a_form) : 0;

    if (!a_scriptName.empty()) {
        // Just call the virtual function if we know the script name
        vm->FindBoundObject(handle, a_scriptName.c_str(), object);

        if (!object) {
            std::string formIdentifier = FormUtil::GetIdentifierFromForm(a_form);
        }
    } else {
        // Script name wasn't specified, so look for one in the internal structure
        RE::BSSpinLockGuard lk{vm->attachedScriptsLock};
        if (auto it = vm->attachedScripts.find(handle); it != vm->attachedScripts.end()) {
            auto& scriptArray = it->second;
            const auto size = scriptArray.size();
            if (size == 1) {
                object = ScriptObjectPtr(scriptArray[0].get());
            } else if (size == 0) {
                std::string formIdentifier = FormUtil::GetIdentifierFromForm(a_form);
                return nullptr;
            } else {
                std::string formIdentifier = FormUtil::GetIdentifierFromForm(a_form);
                return nullptr;
            }
        }
    }

    return object;
}

auto ScriptObject::GetVariable(ScriptObjectPtr a_object, std::string_view a_variableName) -> RE::BSScript::Variable* {
    constexpr auto INVALID = static_cast<std::uint32_t>(-1);
    auto idx = INVALID;
    decltype(idx) offset = 0;
    for (auto cls = a_object->type.get(); cls; cls = cls->GetParent()) {
        const auto vars = cls->GetVariableIter();
        if (idx == INVALID) {
            if (vars) {
                for (std::uint32_t i = 0; i < cls->GetNumVariables(); i++) {
                    const auto& var = vars[i];
                    if (var.name == a_variableName) {
                        idx = i;
                        break;
                    }
                }
            }
        } else {
            offset += cls->GetNumVariables();
        }
    }

    if (idx == INVALID) {
        return nullptr;
    }

    return std::addressof(a_object->variables[offset + idx]);
}

bool ScriptObject::IsType(ScriptObjectPtr a_object, const char* a_scriptName) {
    for (auto cls = a_object ? a_object->type.get() : nullptr; cls; cls = cls->GetParent()) {
        if (_stricmp(cls->GetName(), a_scriptName) == 0) {
            return true;
        }
    }

    return false;
}

bool ScriptObject::GetBool(ScriptObjectPtr a_object, std::string_view a_variableName) {
    auto variable = GetVariable(a_object, a_variableName);
    return variable ? variable->GetBool() : false;
}

void ScriptObject::SetBool(ScriptObjectPtr a_object, std::string_view a_variableName, bool a_value) {
    auto variable = GetVariable(a_object, a_variableName);
    if (variable) variable->SetBool(a_value);
}

std::int32_t ScriptObject::GetInt(ScriptObjectPtr a_object, std::string_view a_variableName) {
    auto variable = GetVariable(a_object, a_variableName);
    return variable ? variable->GetSInt() : 0;
}

void ScriptObject::SetInt(ScriptObjectPtr a_object, std::string_view a_variableName, std::int32_t a_value) {
    auto variable = GetVariable(a_object, a_variableName);
    if (variable) variable->SetSInt(a_value);
}

float ScriptObject::GetFloat(ScriptObjectPtr a_object, std::string_view a_variableName) {
    auto variable = GetVariable(a_object, a_variableName);
    return variable ? variable->GetFloat() : 0.0f;
}

void ScriptObject::SetFloat(ScriptObjectPtr a_object, std::string_view a_variableName, float a_value) {
    auto variable = GetVariable(a_object, a_variableName);
    if (variable) variable->SetFloat(a_value);
}

std::string ScriptObject::GetString(ScriptObjectPtr a_object, std::string_view a_variableName) {
    auto variable = GetVariable(a_object, a_variableName);
    return variable ? std::string{variable->GetString()} : ""s;
}

void ScriptObject::SetString(ScriptObjectPtr a_object, std::string_view a_variableName, std::string_view a_value) {
    auto variable = GetVariable(a_object, a_variableName);
    if (variable) variable->SetString(a_value);
}

ScriptArrayPtr ScriptObject::GetArray(ScriptObjectPtr a_object, std::string_view a_variableName) {
    auto variable = GetVariable(a_object, a_variableName);
    return variable ? variable->GetArray() : nullptr;
}

void ScriptObject::RegisterForModEvent(ScriptObjectPtr a_object, RE::BSFixedString a_eventName,
                                       RE::BSFixedString a_callbackName) {
    const auto skyrimVM = RE::SkyrimVM::GetSingleton();
    const auto vm = skyrimVM ? skyrimVM->impl : nullptr;

    if (vm) {
        auto args = RE::MakeFunctionArguments(std::move(a_eventName), std::move(a_callbackName));
        ScriptCallbackPtr nullCallback;
        vm->DispatchMethodCall(a_object, "RegisterForModEvent"sv, args, nullCallback);
        delete args;
    }
}

void ScriptObject::UnregisterForModEvent(ScriptObjectPtr a_object, RE::BSFixedString a_eventName) {
    const auto skyrimVM = RE::SkyrimVM::GetSingleton();
    const auto vm = skyrimVM ? skyrimVM->impl : nullptr;

    if (vm) {
        auto args = RE::MakeFunctionArguments(std::move(a_eventName));
        ScriptCallbackPtr nullCallback;
        vm->DispatchMethodCall(a_object, "UnregisterForModEvent"sv, args, nullCallback);
        delete args;
    }
}