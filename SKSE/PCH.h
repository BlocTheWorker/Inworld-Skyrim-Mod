#pragma once

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

using ScriptObjectPtr = RE::BSTSmartPointer<RE::BSScript::Object>;
using ScriptArrayPtr = RE::BSTSmartPointer<RE::BSScript::Array>;
using ScriptCallbackPtr = RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor>;
using ScriptArgs = std::unique_ptr<RE::BSScript::IFunctionArguments>;

using namespace std::literals;
