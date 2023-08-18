Scriptname InworldConversation extends Quest  

iWant_Widgets Property iWidgets Auto
  
Int _posX
Int _posY
bool _isVisible
Int _lastShapeId = -1
Int _fontSize = 22
float _defaultGreetingSetting = 50.0
float _voiceRecoveryTimeForActor = 10.0
Bool _isChattingWithActor = false
Actor _actor
      
Event OnInit()     
    ; Debug.Notification("Inworld Mod Started")
    RegisterForUpdate(4.0)   
    RegisterForSingleUpdateGameTime(10) 
    RegisterForModEvent("iWantWidgetsReset", "OniWantWidgetsReset")
    RegisterForModEvent("BLC_CreateSubTitleEvent", "ShowTextEventHandler")
    RegisterForModEvent("BLC_SubtitlePositionEvent", "SetPositionHandler")
    RegisterForModEvent("BLC_SubtitleToggleEvent", "VisibilityToggleHandler")
    RegisterForModEvent("BLC_SetFacialExpressionEvent", "SetFacialExpressionHandler") 
    RegisterForModEvent("BLC_ClearFacialExpressionEvent", "ClearFacialExpressionHandler") 
    RegisterForModEvent("BLC_SetActorStopState", "SetActorStopHandler")
    RegisterForModEvent("BLC_SetActorMoveToPlayer", "SetActorMoveToPlayer")
EndEvent  
 
Event OnUpdate()
    ; Debug.Notification("Checking..")
    If(_isChattingWithActor)
        _actor.SetRestrained();
        Debug.SendAnimationEvent(_actor, "IdleStopInstant")
    EndIf
Endevent

; Required for hooking to widget  
Event SetActorStopHandler(String eventName, String strArg, Float numArg, Form sender)
    Actor subjectActor = sender as Actor
    Bool dontMove = False
    _actor = subjectActor
    If (numArg as int == 1)
        dontMove = True 
        subjectActor.SetRestrained(true)
        _isChattingWithActor = true
    Else
        subjectActor.SetRestrained(false)
        _isChattingWithActor = false
        ClearActorFixation(subjectActor)
    EndIf
    SetGreetingStuff(subjectActor, dontMove)
EndEvent

Event SetActorMoveToPlayer(String eventName, String strArg, Float numArg, Form sender)
    Actor subjectActor = sender as Actor
    Actor playerActor = Game.GetPlayer()
    subjectActor.SetLookAt(playerActor);
    subjectActor.ClearForcedMovement()
    Debug.SendAnimationEvent(subjectActor, "IdleStop")
    Debug.SendAnimationEvent(subjectActor, "IdleForceDefaultState")
EndEvent
 
Event ClearActorFixation(Actor subjectActor)
    subjectActor.ClearLookAt()
    subjectActor.ClearForcedMovement()
    subjectActor.ClearKeepOffsetFromActor()
    subjectActor.SetDontMove(false)
EndEvent
  
Function SetGreetingStuff(Actor subjectActor, Bool isTurnOff)
    If(isTurnOff)
        _defaultGreetingSetting = Game.GetGameSettingFloat("fAIMinGreetingDistance")
        Debug.Trace("Turning off Min Greeting Distance. It was previously " + _defaultGreetingSetting)
        Game.SetGameSettingFloat("fAIMinGreetingDistance", 0.0)
    Else
        If(_defaultGreetingSetting < 1)
            _defaultGreetingSetting = 50.0 
        endif
        Game.SetGameSettingFloat("fAIMinGreetingDistance", _defaultGreetingSetting)
        ClearPhoneme(subjectActor)
        Debug.Trace("Turning on Min Greeting Distance. And clearing Phoneme of chars ")
    EndIf
EndFunction

;; strArgs should be 1-2-3-
Event SetFacialExpressionHandler(String eventName, String strArg, Float numArg, Form sender)
    Actor subjectActor = sender as Actor
    subjectActor.ClearExpressionOverride()
    Debug.Trace("Args: " + strArg)
    Debug.Trace("FormID" + subjectActor.GetFormID())
    subjectActor.ResetExpressionOverrides();
    String[] splitted = StringUtil.Split(strArg, "-");
    Int index = 0
    While (index < splitted.Length)
      String current = splitted[index]
      Int phonemeIndex = StringToActionIndexConverter(current)
      ;subjectActor.SetExpressionPhoneme(phonemeIndex, 0.4)
      ; Float expressionVal = numArg
      Float strength = 0.1
      While(strength < 0.51)
        subjectActor.SetExpressionPhoneme(phonemeIndex, strength)
        strength = strength + 0.1
        Utility.Wait(numArg)
      EndWhile
      While(strength > 0)
        subjectActor.SetExpressionPhoneme(phonemeIndex, strength)
        strength = strength - 0.1
        Utility.Wait(numArg)
      EndWhile
      ;ClearPhoneme(subjectActor)
      ;subjectActor.ResetExpressionOverrides()
      index = index + 1
    EndWhile
    ;ClearPhoneme(subjectActor)
    subjectActor.ResetExpressionOverrides()
EndEvent

Event ClearFacialExpressionHandler(String eventName, String strArg, Float numArg, Form sender)
    Actor subjectActor = sender as Actor
    subjectActor.ClearExpressionOverride()
    subjectActor.ResetExpressionOverrides();
    ClearPhoneme(subjectActor)
    subjectActor.ResetExpressionOverrides()
EndEvent

function ClearPhoneme(Actor ActorRef) global
    int i
	while (i <= 15)
        ActorRef.SetExpressionPhoneme(i, 0.0)
        i = i + 1
    endWhile
endFunction

Int Function StringToActionIndexConverter(string str)
    If (str == "1")
        return 1
    ElseIf (str == "2")
        return 2
    ElseIf (str == "3")
        return 3
    ElseIf (str == "4")
        return 4
    ElseIf (str == "5")
        return 5
    ElseIf (str == "6")
        return 6
    ElseIf (str == "7")
        return 7
    ElseIf (str == "8")
        return 8
    ElseIf (str == "9")
        return 9
    ElseIf (str == "10")
        return 10
    ElseIf (str == "11")
        return 11
    ElseIf (str == "12")
        return 12
    ElseIf (str == "13")
        return 13
    ElseIf (str == "14")
        return 14
    ElseIf (str == "15")
        return 15
    Else
        return 1
    EndIf
EndFunction
; Required for hooking to widget  
Event OniWantWidgetsReset(String eventName, String strArg, Float numArg, Form sender)
    If eventName == "iWantWidgetsReset"
        iWidgets = sender As iWant_Widgets
    EndIf
EndEvent
 
Event SetPositionHandler(String eventName, String strArg, Float numArg, Form sender) 
    If strArg == "PositionX"
        _posX = numArg as Int
    Elseif strArg == "PositionY"
        _posY = numArg as Int 
    EndIf
EndEvent

Event VisibilityToggleHandler(String eventName, String strArg, Float numArg, Form sender)
    If strArg == "true"
       iWidgets.setVisible(_lastShapeId,1)
    Elseif strArg == "false"
        iWidgets.setVisible(_lastShapeId,0)
    EndIf
EndEvent

Event ShowTextEventHandler(String eventName, String strArg, Float numArg, Form sender)
    If _lastShapeId != -1
        _isVisible = false;
        iWidgets.setVisible(_lastShapeId, 0)
        _lastShapeId = -1
    EndIf

    DisplayMessage(strArg, numArg as Int)
EndEvent

Function DisplayMessage(String str, Int waitTime)
  string[] messageDivided = StringUtil.Split(str, ";;;")
  Int index = 0
  While (index < messageDivided.Length)
    String current = messageDivided[index]
    ShowInternal(current, waitTime)
    index = index + 1
  EndWhile
EndFunction

 
Function ShowInternal(String str, Int waitTime)
    If _lastShapeId != -1
        _isVisible = false;
        iWidgets.setVisible(_lastShapeId, 0)
        _lastShapeId = -1
    EndIf
    _lastShapeId = iWidgets.loadText(str, "Minipax", _fontSize)
	iWidgets.setPos(_lastShapeId, _posX, _posY)
	iWidgets.setVisible(_lastShapeId)
    _isVisible = true
    Utility.Wait(waitTime as Int)
    iWidgets.setVisible(_lastShapeId, 0)
    _lastShapeId = -1
    _isVisible = false;
EndFunction