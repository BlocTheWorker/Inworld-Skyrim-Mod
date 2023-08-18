#include <cpr/cpr.h>

#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <thread>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

#include "PhonemeUtility.cpp"

using namespace RE::BSScript;
using json = nlohmann::json;

static class InworldUtility {
public:
    static const void StartQuest(const char* questName) {
        auto quest = RE::TESForm::LookupByEditorID<RE::TESQuest>(questName);
        if (quest) quest->Start();
    }

    static const void MoveQuestToStage(const char* questName, int stage) {
        auto quest = RE::TESForm::LookupByEditorID<RE::TESQuest>(questName);
        if (quest) {
            quest->currentStage = stage;
            quest->GetMustUpdate();
        }
    }

    static void MakeCharacterStopHereAndListen(RE::Actor* conversationActor, bool canMove) {
        SKSE::ModCallbackEvent modEvent{"BLC_SetActorStopState", "", canMove ? 0.0f : 1.0f, conversationActor};
        SKSE::GetModCallbackEventSource()->SendEvent(&modEvent);
    }
};

using namespace RE::BSScript;

static class InworldCaller {
public:
    inline static RE::Actor* conversationActor;
    static void MoveToPlayerWithMargin(RE::Actor* conversationActor) {
        SKSE::ModCallbackEvent modEvent{"BLC_SetActorMoveToPlayer", 0, 0.0f, conversationActor};
        SKSE::GetModCallbackEventSource()->SendEvent(&modEvent);
    }

    static void MakeFacialAnimations(RE::Actor* conversationActor, std::string str) {
        if (str == "") return;  //
        auto splitted = PhonemeUtility::get_instance()->string_to_phonemes(str);
        // SKSE::ModCallbackEvent modEventClear{"BLC_ClearFacialExpressionEvent", "", 1.0f, conversationActor};
        // SKSE::GetModCallbackEventSource()->SendEvent(&modEventClear);
        SKSE::ModCallbackEvent modEvent{"BLC_SetFacialExpressionEvent", splitted, 0.0075f, conversationActor};
        SKSE::GetModCallbackEventSource()->SendEvent(&modEvent);
        InworldUtility::MakeCharacterStopHereAndListen(conversationActor, false);
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

    static void ShowReplyMessage(std::string message) {
        auto messageNew = DisplayMessage(message, 22, 1920);
        SKSE::ModCallbackEvent modEvent{"BLC_CreateSubTitleEvent", messageNew, 5.0f, nullptr};
        SKSE::GetModCallbackEventSource()->SendEvent(&modEvent);
    }

    static void DoGameVisuals(std::string phoneme, std::string message_str) {
        if (InworldCaller::conversationActor) {
            // MoveToPlayerWithMargin(InworldCaller::conversationActor);
            MakeFacialAnimations(InworldCaller::conversationActor, phoneme);
            ShowReplyMessage(message_str);
        }
    }
};

#include "SocketManager.cpp"


class InworldEventSink : public RE::BSTEventSink<SKSE::CrosshairRefEvent>, public RE::BSTEventSink<RE::InputEvent*> {
    InworldEventSink() = default;
    InworldEventSink(const InworldEventSink&) = delete;
    InworldEventSink(InworldEventSink&&) = delete;
    InworldEventSink& operator=(const InworldEventSink&) = delete;
    InworldEventSink& operator=(InworldEventSink&&) = delete;

    

class OpenTextboxCallback : public RE::BSScript::IStackCallbackFunctor {
        virtual inline void operator()(RE::BSScript::Variable a_result) override {
            InworldEventSink::GetSingleton()->trigger_result_menu("UITextEntryMenu");
        }
        virtual inline void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>& a_object){};

    public:
        OpenTextboxCallback() = default;
        bool operator==(const OpenTextboxCallback& other) const { return false; }
    };

    class TextboxResultCallback : public RE::BSScript::IStackCallbackFunctor {
    public:
        RE::Actor* conversationActor;
        TextboxResultCallback(std::function<void()> callback, RE::Actor* form) : callback_(callback) {
            conversationActor = form;
        }

        virtual inline void operator()(RE::BSScript::Variable a_result) override {
            if (a_result.IsNoneObject()) {
                RE::ConsoleLog::GetSingleton()->Print("Result is empty!");
            } else if (a_result.IsString()) {
                auto playerMessage = std::string(a_result.GetString());
                std::thread(
                    [](RE::Actor* actor, std::string msg) { SocketManager::getInstance().sendMessage(msg, actor); },
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

public:
    bool isLocked;
    RE::Actor* conversationPair;
    bool pressingKey = false;
    bool isOpenedWindow = false;
    bool isMenuInitialized = false;

    static InworldEventSink* GetSingleton() {
        static InworldEventSink singleton;
        return &singleton;
    }

    RE::BSEventNotifyControl ProcessEvent(const SKSE::CrosshairRefEvent* event,
                                          RE::BSTEventSource<SKSE::CrosshairRefEvent>*) {
        if (conversationPair != nullptr) {
            InworldUtility::MakeCharacterStopHereAndListen(conversationPair, true);
        }

        if (event->crosshairRef) {
            const char* objectName = event->crosshairRef->GetBaseObject()->GetName();

            try {
                // RE::ConsoleLog::GetSingleton()->Print(objectName);
                auto baseObject = event->crosshairRef->GetBaseObject();
                auto talkingWith = RE::TESForm::LookupByID<RE::TESNPC>(baseObject->formID);
                auto actorObject = event->crosshairRef->As<RE::Actor>();

                if (talkingWith && actorObject) {
                    conversationPair = nullptr;
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

    void ReleaseListener() { InworldEventSink::GetSingleton()->isLocked = false; }

    void OnKeyReleased() {
        if (pressingKey && conversationPair != nullptr) {
            pressingKey = false;
            SocketManager::getInstance().controlVoiceInput(false, conversationPair);
        }
    }

    void OnKeyPressed() {
        if (!pressingKey && conversationPair != nullptr) {
            pressingKey = true;
            SocketManager::getInstance().controlVoiceInput(true, conversationPair);
        }
    }

    void OnPlayerRequestInput(RE::BSFixedString menuID) {
        const auto skyrimVM = RE::SkyrimVM::GetSingleton();
        auto vm = skyrimVM ? skyrimVM->impl : nullptr;
        if (vm) {
            isOpenedWindow = true;
            RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callbackOpenTextbox(new OpenTextboxCallback());
            RE::TESForm* emptyForm = NULL;
            RE::TESForm* emptyForm2 = NULL;
            auto args2 = RE::MakeFunctionArguments(std::move(menuID), std::move(emptyForm), std::move(emptyForm2));
            vm->DispatchStaticCall("UIExtensions", "OpenMenu", args2, callbackOpenTextbox);
        }
    }

    void InitMenu(RE::BSFixedString menuID) {
        const auto skyrimVM = RE::SkyrimVM::GetSingleton();
        auto vm = skyrimVM ? skyrimVM->impl : nullptr;
        if (vm) {
            RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
            auto args = RE::MakeFunctionArguments(std::move(menuID));
            vm->DispatchStaticCall("UIExtensions", "InitMenu", args, callback);
        }
    }

    void trigger_result_menu(RE::BSFixedString menuID) {
        const auto skyrimVM = RE::SkyrimVM::GetSingleton();
        auto vm = skyrimVM ? skyrimVM->impl : nullptr;
        if (vm) {
            RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback(new TextboxResultCallback(
                []() {
                    InworldEventSink::GetSingleton()->ReleaseListener();
                },
                conversationPair));
            auto args = RE::MakeFunctionArguments(std::move(menuID));
            vm->DispatchStaticCall("UIExtensions", "GetMenuResultString", args, callback);
            isOpenedWindow = false;
        }
    }

    RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* eventPtr, RE::BSTEventSource<RE::InputEvent*>*) {
        if (!eventPtr) return RE::BSEventNotifyControl::kContinue;
        auto* event = *eventPtr;
        if (!event) return RE::BSEventNotifyControl::kContinue;

        if (!isMenuInitialized) {
            isMenuInitialized = true;
            InitMenu("UITextEntryMenu");
        }

        try {
            if (event->GetEventType() == RE::INPUT_EVENT_TYPE::kButton) {
                auto* buttonEvent = event->AsButtonEvent();
                auto dxScanCode = buttonEvent->GetIDCode();
                // Press V key to speak.
                if (dxScanCode == 47) {
                    if (!isOpenedWindow) {
                        if (buttonEvent->IsUp()) {
                            OnKeyReleased();
                        } else {
                            OnKeyPressed();
                        }
                    }
                    // U key
                } else if (dxScanCode == 22) {
                    if (!isOpenedWindow)
                        OnPlayerRequestInput("UITextEntryMenu");
                } else if (dxScanCode == 21) {
                    // Y key. Connect to character
                    if (buttonEvent->IsDown() && conversationPair != nullptr) {
                        SocketManager::getInstance().connectTo(conversationPair);
                    }
                }
                /* // funbit
                else if (dxScanCode == 71) {
                    // Start
                    InworldUtility::StartQuest("InworldNazeemDestroyer");
                } else if (dxScanCode == 71) {
                    // Proceed
                    InworldUtility::MoveQuestToStage("InworldNazeemDestroyer",10);
                } 
                */
            }
        } catch (...) {
        }

        return RE::BSEventNotifyControl::kContinue;
    }
};



void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kInputLoaded) {
        SocketManager::getInstance().initSocket();
        RE::BSInputDeviceManager::GetSingleton()->AddEventSink(InworldEventSink::GetSingleton());
    }
}

void writeInworldLog(const std::string& message) {
    std::ofstream logFile("InworldSkyrim.txt", std::ios::app);
    if (logFile.is_open()) {
        logFile << message << std::endl;
        logFile.close();
    }
}


#include <ShellAPI.h>

void StartAudioBus() {
    auto mainPath = std::filesystem::current_path();
    auto clientPath = mainPath / "Inworld" / "Audio" / "AudioBloc.exe";
    writeInworldLog("Opening: " + clientPath.string());
    LPCWSTR exePath = clientPath.c_str();
    HINSTANCE result = ShellExecute(NULL, L"open", exePath, NULL, clientPath.parent_path().c_str(), SW_SHOWNORMAL);
}

void StartClient() { 
    auto mainPath = std::filesystem::current_path(); 
    auto clientPath = mainPath / "Inworld" / "SkyrimClient.exe";
    writeInworldLog("Opening: " + clientPath.string());
    LPCWSTR exePath = clientPath.c_str();
    HINSTANCE result = ShellExecute(NULL, L"open", exePath, NULL, clientPath.parent_path().c_str(), SW_SHOWNORMAL);
    StartAudioBus();
}



SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);
    
    StartClient();

    auto* eventSink = InworldEventSink::GetSingleton();

    // ScriptSource
    auto* eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();

    // SKSE
    SKSE::GetCrosshairRefEventSource()->AddEventSink(eventSink);

    // Input Device
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);

    return true;
}