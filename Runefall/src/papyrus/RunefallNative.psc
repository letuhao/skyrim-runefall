Scriptname RunefallNative Hidden
{SKSE native bridge exposed by Runefall.dll (API v1).}

int Function GetApiVersion() Global Native
int Function GetPluginVersion() Global Native
int Function GetProbeFailures() Global Native
string Function GetProbeSummary() Global Native

int Function GetAttribute(string a_attribute) Global Native
int Function GetAttributePoints() Global Native
int Function GetSkillPoints() Global Native
int Function GetRuneTier() Global Native
int Function GetSkillRank(string a_skill) Global Native
int Function GetRuneCount(int a_rank) Global Native
int Function GetHuntFirstKill(string a_group) Global Native
int Function SpendAttributePoint(string a_attribute) Global Native
int Function SpendSkillPoint(string a_skill) Global Native
string Function GetMenuSnapshot() Global Native
