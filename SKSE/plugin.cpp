#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include "PhonemeUtility.cpp"
#include "ScriptObject.h"
#include <string>
#include <sstream>
#include <thread>

using namespace RE::BSScript;
using json = nlohmann::json;

static class InworldUtility {
public:
    static const char* Merge(const char* s1, const char* s2) {
        // Get the lengths of the strings
        size_t len1 = strlen(s1);
        size_t len2 = strlen(s2);

        // Allocate memory for the result
        char* result = new char[len1 + len2 + 1];

        // Copy the first string to the result
        memcpy(result, s1, len1);

        // Copy the second string to the result
        memcpy(result + len1, s2, len2);

        // Add a null terminator
        result[len1 + len2] = '\0';

        // Return the result as a const char*
        return static_cast<const char*>(result);
    }
    

   static std::string get_cat_fact() {
        try {
            cpr::Response r = cpr::Get(cpr::Url{"https://catfact.ninja/fact"});
            if (r.status_code == 200) {
                json root = json::parse(r.text);  // parse the JSON string
                std::string fact = root["fact"];  // get the fact value as a string
                return fact;
            }
            return "Error: Could not make GET request.";
        } catch (...) {
            return "oopsie";
        }
    }

   static std::string say_to_character(std::string name, std::string message) {
        try {
            // Create a JSON object with the name and message fields
            json data;
            data["id"] = name;
            data["message"] = message;

            // Make a POST request to the URL with the JSON object as the body
            cpr::Response r = cpr::Post(cpr::Url{"http://127.0.0.1:3000/say"}, cpr::Body{data.dump()},
                cpr::Header{{"Content-Type", "application/json"}});
            if (r.status_code == 200) {
                // Return the response text if successful
                json root = json::parse(r.text);  // parse the JSON string
                std::string responseMessage = root["message"];  // get the fact value as a string
                return responseMessage;
            }
            return "Error: Could not make POST request.";
        } catch (...) {
            return "oopsie";
        }
    }

    static void OpenDebugMessageBox(const char* menuID) {
        const auto skyrimVM = RE::SkyrimVM::GetSingleton();
        auto vm = skyrimVM ? skyrimVM->impl : nullptr;
        if (vm) {
            RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
            auto args = RE::MakeFunctionArguments(std::move(menuID));
            vm->DispatchStaticCall("Debug", "MessageBox", args, callback);
        }
    }

    static void MakeCharacterStopHereAndListen(RE::Actor* conversationActor, bool canMove) {
        SKSE::ModCallbackEvent modEvent{"BLC_SetActorStopState", "", canMove? 0.0f : 1.0f, conversationActor};
        SKSE::GetModCallbackEventSource()->SendEvent(&modEvent);
    }
};

static class InworldCaller {
public:
    static void MakeCall(RE::Actor* conversationActor, std::string message_str) {
        auto name = conversationActor->GetName();
        auto fullName = conversationActor->GetDisplayFullName();
        auto response = InworldUtility::say_to_character(name, message_str);
        MoveToPlayerWithMargin(conversationActor);
        MakeFacialAnimations(conversationActor, response);
        ShowReplyMessage(response);
    }

    static void MoveToPlayerWithMargin(RE::Actor* conversationActor) { 
        SKSE::ModCallbackEvent modEvent{"BLC_SetActorMoveToPlayer", 0, 0.0f, conversationActor};
        SKSE::GetModCallbackEventSource()->SendEvent(&modEvent);
    }

    static void MakeFacialAnimations(RE::Actor* conversationActor, std::string str) {
        auto splitted = PhonemeUtility::get_instance()->generate_random_phonemes((int)(str.length() / 5));
        SKSE::ModCallbackEvent modEvent{"BLC_SetFacialExpressionEvent", splitted, 0.5f, conversationActor};
        SKSE::GetModCallbackEventSource()->SendEvent(&modEvent);

        InworldUtility::MakeCharacterStopHereAndListen(conversationActor, false);
    }

    static void ShowReplyMessage(std::string message) {
        auto messageNew = DisplayMessage(message, 22, 1920);
        SKSE::ModCallbackEvent modEvent{"BLC_CreateSubTitleEvent", messageNew, 5.0f, nullptr};
        SKSE::GetModCallbackEventSource()->SendEvent(&modEvent);
    }

    static std::string DisplayMessage(std::string str, int fontSize, int width) {
        std::stringstream ss(str);
        std::string word;
        std::string combined = "";
        std::string tracker = "";

        while (ss >> word) {
            if (((tracker.length() + word.length()) * fontSize) >= width) {
                combined += ";;;" + word;
                tracker = " " + word;
            } else {
                combined += " " + word;
                tracker += " " + word;
            }
        }
        return combined;
    }
};


class InworldEventSink : public RE::BSTEventSink<SKSE::CrosshairRefEvent>,
                     public RE::BSTEventSink<RE::InputEvent*> {
    InworldEventSink() = default;
    InworldEventSink(const InworldEventSink&) = delete;
    InworldEventSink(InworldEventSink&&) = delete;
    InworldEventSink& operator=(const InworldEventSink&) = delete;
    InworldEventSink& operator=(InworldEventSink&&) = delete;

#pragma region Internal_Classes
    class ConversationCallbackFunctor : public RE::BSScript::IStackCallbackFunctor {
    public:
        RE::Actor* conversationActor;
        ConversationCallbackFunctor(std::function<void()> callback, RE::Actor* form) : callback_(callback) {
            conversationActor = form;
        }

        virtual inline void operator()(RE::BSScript::Variable a_result) override {
            if (a_result.IsNoneObject()) {
                RE::ConsoleLog::GetSingleton()->Print("Result is empty!");
            } else if (a_result.IsString()) {
                auto playerMessage = std::string(a_result.GetString());

                std::thread([](RE::Actor* actor, std::string msg) { InworldCaller::MakeCall(actor, msg); },
                            conversationActor, playerMessage)
                    .detach();
            }
            callback_();
        }

        virtual inline void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>& a_object){};

    private:
        // Member variable to store the callback function
        std::function<void()> callback_;
    };

    class ConversationOpenMenuCallbackFunctor : public RE::BSScript::IStackCallbackFunctor {
        virtual inline void operator()(RE::BSScript::Variable a_result) override {
            InworldEventSink::GetSingleton()->trigger_result_menu("UITextEntryMenu");
        }

        virtual inline void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>& a_object){};
    };
#pragma endregion

public:
    bool isLocked;
    RE::Actor* conversationPair;

    static InworldEventSink* GetSingleton() {
        static InworldEventSink singleton;
        return &singleton;
    }

    RE::BSEventNotifyControl ProcessEvent(const SKSE::CrosshairRefEvent* event, RE::BSTEventSource<SKSE::CrosshairRefEvent>*) {

        if (conversationPair != nullptr) {
            InworldUtility::MakeCharacterStopHereAndListen(conversationPair, true);
        }
        conversationPair = nullptr;
        if (event->crosshairRef) {
            const char* objectName = event->crosshairRef->GetBaseObject()->GetName();

            try {
                RE::ConsoleLog::GetSingleton()->Print(objectName);
                auto baseObject = event->crosshairRef->GetBaseObject();
                auto talkingWith = RE::TESForm::LookupByID<RE::TESNPC>(baseObject->formID);
                auto actorObject = event->crosshairRef->As<RE::Actor>();
                
                if (talkingWith && actorObject) {
                    auto className = talkingWith->npcClass->fullName;
                    auto raceName = talkingWith->race->fullName;

                    if (className == "Prey" || className == "Predator") return RE::BSEventNotifyControl::kContinue;
                    if (raceName.contains("Rabbit") || raceName.contains("Wolf") || raceName.contains("Cow") ||
                        raceName.contains("Fox"))
                        return RE::BSEventNotifyControl::kContinue;
                    
                    conversationPair = actorObject;
                }
            } catch (...) {
            }
        }
        return RE::BSEventNotifyControl::kContinue;
    }

    void ReleaseListener() { 
        InworldEventSink::GetSingleton()->isLocked = false;
    }

    void trigger_result_menu(RE::BSFixedString menuID) {
        const auto skyrimVM = RE::SkyrimVM::GetSingleton();
        auto vm = skyrimVM ? skyrimVM->impl : nullptr;
        if (vm) {
            RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback(new ConversationCallbackFunctor(
                []() { InworldEventSink::GetSingleton()->ReleaseListener(); }, conversationPair));
            auto args = RE::MakeFunctionArguments(std::move(menuID));
            vm->DispatchStaticCall("UIExtensions", "GetMenuResultString", args, callback);
        }
    }

    inline void open_menu(RE::BSFixedString menuID) {
        const auto skyrimVM = RE::SkyrimVM::GetSingleton();
        auto vm = skyrimVM ? skyrimVM->impl : nullptr;
        if (vm) {
            RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback(new ConversationOpenMenuCallbackFunctor());
            RE::TESForm* emptyForm = NULL;
            RE::TESForm* emptyForm2 = NULL;
            auto args = RE::MakeFunctionArguments(std::move(menuID), std::move(emptyForm), std::move(emptyForm2));
            vm->DispatchStaticCall("UIExtensions", "OpenMenu", args, callback);
        }
    }

     void OpenDebugMessageBox(const char* menuID) {
        const auto skyrimVM = RE::SkyrimVM::GetSingleton();
        auto vm = skyrimVM ? skyrimVM->impl : nullptr;
        if (vm) {
            RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
            auto args = RE::MakeFunctionArguments(std::move(menuID));
            vm->DispatchStaticCall("Debug", "MessageBox", args, callback);
        }
    }

    inline void init_menu(RE::BSFixedString menuID) {
        const auto skyrimVM = RE::SkyrimVM::GetSingleton();
        auto vm = skyrimVM ? skyrimVM->impl : nullptr;
        if (vm) {
            RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
            auto args = RE::MakeFunctionArguments(std::move(menuID));
            vm->DispatchStaticCall("UIExtensions", "InitMenu", args, callback);
        }
    }

    inline void make_notification(const char* string) {
        const auto skyrimVM = RE::SkyrimVM::GetSingleton();
        auto vm = skyrimVM ? skyrimVM->impl : nullptr;
        if (vm) {
            RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
            auto args = RE::MakeFunctionArguments(std::move(string));
            vm->DispatchStaticCall("Debug", "Notification", args, callback);
        }
    }

    RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* eventPtr, RE::BSTEventSource<RE::InputEvent*>*) {
        if (!eventPtr) return RE::BSEventNotifyControl::kContinue;

        auto* event = *eventPtr;
        if (!event) return RE::BSEventNotifyControl::kContinue;

        if (event->GetEventType() == RE::INPUT_EVENT_TYPE::kButton) {
            auto* buttonEvent = event->AsButtonEvent();

            if (!buttonEvent->IsUp()) return RE::BSEventNotifyControl::kContinue;
            auto dxScanCode = buttonEvent->GetIDCode();

            if (dxScanCode == 21) {
                if (!InworldEventSink::GetSingleton()->isLocked && conversationPair) {
                    SKSE::ModCallbackEvent modEventX{"BLC_SubtitlePositionEvent", "PositionX", 1280 / 2, nullptr};
                    SKSE::GetModCallbackEventSource()->SendEvent(&modEventX);

                    SKSE::ModCallbackEvent modEventY{"BLC_SubtitlePositionEvent", "PositionY", 650, nullptr};
                    SKSE::GetModCallbackEventSource()->SendEvent(&modEventY);
                    InworldEventSink::GetSingleton()->isLocked = true;
                    init_menu("UITextEntryMenu");
                    open_menu("UITextEntryMenu");
                }
            } 
        }

        return RE::BSEventNotifyControl::kContinue;
    }
};


void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kInputLoaded)
        RE::BSInputDeviceManager::GetSingleton()->AddEventSink(InworldEventSink::GetSingleton());
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);

    auto* eventSink = InworldEventSink::GetSingleton();

    // ScriptSource
    auto* eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();

    // SKSE
    SKSE::GetCrosshairRefEventSource()->AddEventSink(eventSink);

    // Input Device
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);

    return true;
}