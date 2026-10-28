# Runefall Prisma UI shell

Hard dependency: PrismaUI. The native plugin writes `Data/Runefall/ui/view-model.json`
(schemaVersion 1, runefall-hud). This shell only renders that contract; if PrismaUI is absent
the plugin keeps running and the HUD fails closed (no game data is mutated).
