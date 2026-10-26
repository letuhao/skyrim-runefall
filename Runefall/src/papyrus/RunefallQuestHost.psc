Scriptname RunefallQuestHost extends Quest
{Runefall attunement quest host. Exposes RF tier/state to Papyrus and logs quest startup.}

Event OnInit()
    int tier = RunefallNative.GetRuneTier()
    int points = RunefallNative.GetAttributePoints()
    Debug.Trace("[Runefall] quest host init: tier=" + tier + " attributePoints=" + points)
EndEvent

int Function GetTier() Global
    return RunefallNative.GetRuneTier()
EndFunction

int Function GetAttributePoints() Global
    return RunefallNative.GetAttributePoints()
EndFunction

string Function GetSnapshot() Global
    return RunefallNative.GetMenuSnapshot()
EndFunction
