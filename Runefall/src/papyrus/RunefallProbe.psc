Scriptname RunefallProbe extends Quest
{Phase-0/test bridge probe. Call with: cqf RunefallProbe RunProbe}

Function RunProbe() Global
    int api = RunefallNative.GetApiVersion()
    Debug.Trace("[RunefallProbe] api=" + api)

    string snapshot = RunefallNative.GetMenuSnapshot()
    Debug.Trace("[RunefallProbe] snapshot=" + snapshot)

    int might = RunefallNative.GetAttribute("Might")
    int points = RunefallNative.GetAttributePoints()
    int runeTier = RunefallNative.GetRuneTier()
    int probeFailures = RunefallNative.GetProbeFailures()
    string probeSummary = RunefallNative.GetProbeSummary()
    Debug.Trace("[RunefallProbe] might=" + might + " points=" + points + " tier=" + runeTier)
    Debug.Trace("[RunefallProbe] probeFailures=" + probeFailures + " summary=" + probeSummary)
EndFunction
