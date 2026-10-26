Scriptname RunefallMenuBridge extends Quest
{Debug quest companion script: lets Papyrus read the native menu snapshot and probe summary.}

string Function MenuSnapshot() Global
    return RunefallNative.GetMenuSnapshot()
EndFunction

string Function ProbeSummary() Global
    return RunefallNative.GetProbeSummary()
EndFunction

int Function ProbeFailures() Global
    return RunefallNative.GetProbeFailures()
EndFunction
