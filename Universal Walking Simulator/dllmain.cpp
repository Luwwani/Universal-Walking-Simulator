#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <iostream>

#include <Net/nethooks.h>
#include <MinHook/MinHook.h>

#include "patterns.h"
#include "gui.h"
#include <hooks.h>
#include <Gameplay/abilities.h>

void InitializePatterns()
{
    static const auto FnVerDouble = std::stod(FN_Version);

    static auto ReallocAddr = FindPattern(Patterns::Realloc);
    // FMemory::Realloc = decltype(FMemory::Realloc)(ReallocAddr); // we don't need this I think
    CheckPattern(_("FMemory::Realloc"), ReallocAddr, &FMemory::Realloc);

    if (Engine_Version < 425)
    {
        GetNetModeAddr = FindPattern(Patterns::GetNetMode);

        if (!GetNetModeAddr)
            GetNetModeAddr = FindPattern(_("48 89 5C 24 ? 57 48 83 EC 20 48 8B 01 48 8B D9 FF 90 ? ? ? ? 4C 8B 83 ? ? ? ? 48 8B F8 49 8B C8 48 C1 E9 20 85 C9 0F 94 C2 41 81 F8 ? ? ? ?"));

        CheckPattern(_("GetNetMode"), GetNetModeAddr, &GetNetMode);
    }

    LP_SpawnPlayActorAddr = FindPattern(Patterns::LocalPlayerSpawnPlayActor);
    CheckPattern(_("LocalPlayer::SpawnPlayActor"), LP_SpawnPlayActorAddr, &LP_SpawnPlayActor);

    // CollectGarbage = decltype(CollectGarbage)(FindPattern(Patterns::CollectGarbage));

    InitListen = decltype(InitListen)(FindPattern(Patterns::InitListen));

    TickFlushAddr = FindPattern(Patterns::TickFlush);

    if (!TickFlushAddr)
        TickFlushAddr = FindPattern(_("4C 8B DC 55 49 8D AB ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 49 89 5B 18 49 89 73 F0 48 8B F1 49 89 7B E8 48 8D 0D ? ? ? ? 4D 89 73 D0"));
    
    CheckPattern(_("TickFlush"), TickFlushAddr, &TickFlush);

    ReceiveFStringAddr = FindPattern(Patterns::ReceiveFString);
    CheckPattern(_("ReceiveFString"), ReceiveFStringAddr, &ReceiveFString);

    ReceiveUniqueIdReplAddr = FindPattern(Patterns::ReceiveUniqueIdRepl);

    if (!ReceiveUniqueIdReplAddr)
        ReceiveUniqueIdReplAddr = FindPattern(_("48 89 5C 24 ? 55 56 57 48 8B EC 48 83 EC 40 F6 41 28 40 48 8B FA 48 8B D9"));

    CheckPattern(_("ReceiveUniqueIdRepl"), ReceiveUniqueIdReplAddr, &ReceiveUniqueIdRepl);

    WelcomePlayerAddr = FindPattern(Patterns::WelcomePlayer);

    if (!WelcomePlayerAddr) // s6
        WelcomePlayerAddr = FindPattern(_("48 8B C4 55 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 48 89 70 20 48 8B F1 48 89 78 F0 48 8B CA 4C 89 70 E8 48 8B FA"));

    CheckPattern(_("WelcomePlayer"), WelcomePlayerAddr, &WelcomePlayer);

    World_NotifyControlMessageAddr = FindPattern(Patterns::World_NotifyControlMessage);

    if (!World_NotifyControlMessageAddr)
        World_NotifyControlMessageAddr = FindPattern(_("48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 48 89 4C 24 ? 55 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 33 FF 49 8B D9 89 7C 24 60"));

    CheckPattern(_("World_NotifyControlMessage"), World_NotifyControlMessageAddr, &World_NotifyControlMessage);

    SpawnPlayActorAddr = FindPattern(Patterns::SpawnPlayActor);
    CheckPattern(_("SpawnPlayActor"), SpawnPlayActorAddr, &SpawnPlayActor);

    NetDebugAddr = FindPattern(Patterns::NetDebug);
    CheckPattern(_("NetDebug"), NetDebugAddr, &NetDebug);

    if (Engine_Version == 420)
    {
        CollectGarbageAddr = FindPattern(Patterns::CollectGarbage, true, 1);
        CheckPattern(_("CollectGarbage"), CollectGarbageAddr, &CollectGarbage);
    }

    if (Engine_Version != 421) // todo get the patterns
    {
        GiveAbilityAddr = FindPattern(Patterns::GiveAbility);

        if (!GiveAbilityAddr)
            GiveAbilityAddr = FindPattern(_("48 89 5C 24 10 48 89 6C 24 18 48 89 7C 24 20 41 56 48 83 EC ? 83 B9 60 05"));

        CheckPattern(_("GiveAbility"), GiveAbilityAddr, &GiveAbility);

        InternalTryActivateAbilityAddr = FindPattern(Patterns::InternalTryActivateAbility);
        CheckPattern(_("InternalTryActivateAbility"), InternalTryActivateAbilityAddr, &InternalTryActivateAbility);

        MarkAbilitySpecDirtyAddr = FindPattern(Patterns::MarkAbilitySpecDirty);
        CheckPattern(_("MarkAbilitySpecDirty"), MarkAbilitySpecDirtyAddr, &MarkAbilitySpecDirtyNew);
    }

    if (Engine_Version >= 423)
    {
        FixCrashAddr = FindPattern(Patterns::FixCrash);
        CheckPattern(_("FixCrash"), FixCrashAddr, &FixCrash);
    }

    GetPlayerViewpointAddr = FindPattern(Patterns::GetPlayerViewpoint);

    if (!GetPlayerViewpointAddr)
        GetPlayerViewpointAddr = FindPattern(_("48 89 5C 24 ? 48 89 74 24 ? 55 41 56 41 57 48 8B EC 48 83 EC 40 48 8B F2 48 C7 45 ? ? ? ? ? 48 8B 55 38 4D 8B F0 48 8B D9 45 33 FF E8 ? ? ? ? 84 C0 74 4A 80 BB"));

    CheckPattern(_("GetPlayerViewPoint"), GetPlayerViewpointAddr, &GetPlayerViewPoint);

    if ((FnVerDouble >= 5 && FnVerDouble < 7) || Engine_Version == 423)
    {
        if (FnVerDouble >= 5 && FnVerDouble < 7)
        {
            IdkfAddr = FindPattern(Patterns::Idkf);
            CheckPattern(_("Idkf"), IdkfAddr, &Idkf);

            SendChallengeAddr = FindPattern(Patterns::SendChallenge);

            if (!SendChallengeAddr)
                SendChallengeAddr = FindPattern(_("48 89 5C 24 ? 55 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B DA 48 85 D2 0F 84 ? ? ? ? 83 BA ? ? ? ? ? 0F 86 ? ? ? ? 48 83 7A ? ? 0F 84 ? ? ? ? 48 8D 4D 68 48 89 BC 24 ? ? ? ? FF 15 ? ? ? ? 44 8B 45 68 48 8D 15 ? ? ? ? 48 8D 4C 24 ? E8 ? ? ? ? 48 8D 44 24 ? 48 8D BB ? ? ? ? 48 3B F8 74 25 48 8B 0F 48 85 C9 74 05 E8 ? ? ? ? 48 8B 44 24 ? 48 89 07 8B 44 24 38 89 47 08 8B 44 24 3C 89 47 0C EB 0F 48 8B 4C 24 ? 48 85 C9 74 05 E8 ? ? ? ? B2 05 48 8B CB E8 ? ? ? ? 48 8B 83 ? ? ? ?"));
            
            CheckPattern(_("SendChallenge"), SendChallengeAddr, &SendChallenge);
        }

        SetWorldAddr = FindPattern(Patterns::SetWorld);
        CheckPattern(_("SetWorld"), SetWorldAddr, &SetWorld);

        CreateNetDriverAddr = FindPattern(Patterns::CreateNetDriver);
        CheckPattern(_("CreateNetDriver"), CreateNetDriverAddr, &CreateNetDriver);

        if (Engine_Version == 423)
        {
            HasClientLoadedCurrentWorldAddr = FindPattern(Patterns::HasClientLoadedCurrentWorld);
            CheckPattern(_("HasClientLoadedCurrentWorld"), HasClientLoadedCurrentWorldAddr, &HasClientLoadedCurrentWorld);

            malformedAddr = FindPattern(Patterns::malformed);
            CheckPattern(_("malformed"), malformedAddr, &malformed);
        }

        /* if(false) // FnVerDouble >= 8)
        {
            ClientTravelAddr = FindPattern(Patterns::ClientTravel);
            CheckPattern(_("ClientTravel"), ClientTravelAddr, &ClientTravel);
        } */
    }

    if (Engine_Version >= 424)
    {
        SetReplicationDriverAddr = FindPattern(Patterns::SetReplicationDriver);
        CheckPattern(_("SetReplicationDriver"), SetReplicationDriverAddr, &SetReplicationDriver);

        ValidationFailureAddr = FindPattern(Patterns::ValidationFailure);
        CheckPattern(_("ValidationFailure"), ValidationFailureAddr, &ValidationFailure);
    }

    // if (!CreateNetDriver) // This means we are not using beacons
    {
        PauseBeaconRequestsAddr = FindPattern(Patterns::PauseBeaconRequests);
        CheckPattern(_("PauseBeaconRequests"), PauseBeaconRequestsAddr, &PauseBeaconRequests);

        InitHostAddr = FindPattern(Patterns::InitHost);
        CheckPattern(_("InitHost"), InitHostAddr, &InitHost);

        Beacon_NotifyControlMessageAddr = FindPattern(Patterns::Beacon_NotifyControlMessage);

        if (!Beacon_NotifyControlMessageAddr)
            Beacon_NotifyControlMessageAddr = FindPattern(_("4C 8B DC 49 89 5B 18 49 89 73 20 49 89 4B 08 55 57 41 54 41 56 41 57 49 8D AB ? ? ? ? 48 81 EC ? ? ? ? 33 FF 49 8B F1"));

        CheckPattern(_("Beacon_NotifyControlMessage"), Beacon_NotifyControlMessageAddr, &Beacon_NotifyControlMessage);
    }

    // NetDebugAddr = FindPattern(Patterns::NetDebug);
    // CheckPattern(_("NetDebug"), NetDebugAddr, &NetDebug);

    KickPlayerAddr = FindPattern(Patterns::KickPlayer);
    CheckPattern(_("KickPlayer"), KickPlayerAddr, &KickPlayer);

    // static const auto FnVerDouble = std::stod(FN_Version);
}

DWORD WINAPI Input(LPVOID)
{
    while (1)
    {
        if (GetAsyncKeyState(VK_F5) & 1)
        {
            LoadInMatch();
        }

        else if (GetAsyncKeyState(VK_F6) & 1)
        {
            initStuff();
        }

        else if (GetAsyncKeyState(VK_F7) & 1)
        {
            // if (MyPawn)
            {
                // Helper::TeleportTo(MyPawn, Helper::GetPlayerStart());
            }
        }

        else if (GetAsyncKeyState(VK_F8) & 1)
        {
            InitializeNetHooks();

            std::cout << _("Initialized NetHooks!\n");
        }

        Sleep(1000 / 30);
    }

    return 0;
}

DWORD WINAPI Main(LPVOID)
{
    AllocConsole();

    FILE* fptr;
    freopen_s(&fptr, _("CONOUT$"), _("w"), stdout);

    auto stat = MH_Initialize();

    if (stat != MH_OK)
    {
        std::cout << std::format("Failed to initialize MinHook! Error: {}\n", MH_StatusToString(stat));
        return 1;
    }

    if (!Setup())
    {
        std::cout << _("Failed setup!\n");
        return 1;
    }

    InitializePatterns();

    std::cout << _("Initialized Patterns!\n");

    InitializeNetUHooks();
    if (GiveAbilityAddr)
        InitializeAbilityHooks();
    FinishInitializeUHooks();

    InitializeHooks();

    CreateThread(0, 0, Input, 0, 0, 0);
    CreateThread(0, 0, GuiHook, 0, 0, 0);
    CreateThread(0, 0, Helper::Console::Setup , 0, 0, 0);

    if (Engine_Version < 422)
        std::cout << _("Press play button to host!\n");
    else
        std::cout << _("Press F5 to host!\n");

    std::cout << dye::aqua(_("[Base Address] ")) << std::format("0x{:x}\n", (uintptr_t)GetModuleHandleW(0));
    std::cout << dye::green(_("[ServerReplicateActors] ")) << std::format("0x{:x}\n", ServerReplicateActorsOffset);

    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(0, 0, Main, 0, 0, 0);
        break;
    case DLL_PROCESS_DETACH:
        std::cout << _("Disabling all Hooks!");
        MH_DisableHook(MH_ALL_HOOKS); // Untested
        break;
    }
    return TRUE;
}
