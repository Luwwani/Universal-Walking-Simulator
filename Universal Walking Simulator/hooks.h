#pragma once

#include <UE/structs.h>
#include <functional>

#include "color.hpp"
#include <Net/server.h>
#include <Gameplay/helper.h>
#include <Net/funcs.h>
#include <Net/nethooks.h>
#include <Gameplay/abilities.h>
#include <Gameplay/events.h>

#include <mutex>
#include "Gameplay/player.h"

#define LOGGING

// HEAVILY INSPIRED BY KEMOS UFUNCTION HOOKING

static bool bStarted = false;
static bool bLogRpcs = false;
static bool bLogProcessEvent = false;

inline void initStuff()
{
	if (!bStarted && bTraveled && bIsReadyToRestart)
	{
		bStarted = true;

		CreateThread(0, 0, Helper::Console::Setup, 0, 0, 0);

		auto world = Helper::GetWorld();
		auto gameState = *world->Member<UObject*>("GameState");

		auto Playlist = FindObject(PlaylistToUse);

		if (gameState)
		{
			auto AuthGameMode = *world->Member<UObject*>(("AuthorityGameMode"));

			*(*AuthGameMode->Member<UObject*>(("GameSession")))->Member<int>(("MaxPlayers")) = 100; // GameState->GetMaxPlaylistPlayers()

			std::cout << "AA: " << *AuthGameMode->Member<bool>("bStartPlayersAsSpectators") << '\n';

			*AuthGameMode->Member<bool>("bStartPlayersAsSpectators") = false;
			
			if (AuthGameMode)
			{
				auto PlayerControllerClass = // bIsSTW ? FindObject("Class /Script/FortniteGame.FortPlayerControllerZone") :
					FindObject(("BlueprintGeneratedClass /Game/Athena/Athena_PlayerController.Athena_PlayerController_C"));
				std::cout << ("PlayerControllerClass: ") << PlayerControllerClass << '\n';
				*AuthGameMode->Member<UObject*>(("PlayerControllerClass")) = PlayerControllerClass;

				static auto PawnClass = FindObject(("BlueprintGeneratedClass /Game/Athena/PlayerPawn_Athena.PlayerPawn_Athena_C"));

				*AuthGameMode->Member<UObject*>(("DefaultPawnClass")) = PawnClass;
			}

			auto WillSkipAircraft = gameState->Member<bool>(("bGameModeWillSkipAircraft"));

			/* if (!WillSkipAircraft) // we are in like frontend or some gamestate that is not athena
				return; */

			if (WillSkipAircraft)
				*WillSkipAircraft = false;

			auto AircraftStartTime = gameState->Member<float>(("AircraftStartTime"));

			if (AircraftStartTime)
			{
				*AircraftStartTime = 99999.0f;
				*gameState->Member<float>(("WarmupCountdownEndTime")) = 99999.0f;
			}

			auto bSkipTeamReplication = gameState->Member<bool>("bSkipTeamReplication");

			if (bSkipTeamReplication)
				*bSkipTeamReplication = false;

			// auto FriendlyFireTypeOffset = GetOffset(gameState, "FriendlyFireType");

			EFriendlyFireType* FFT = gameState->Member<EFriendlyFireType>(("FriendlyFireType"));
			//Super scuffed way of checking if its valid.
			if (FFT && __int64(FFT) != __int64(gameState)) {
				*gameState->Member<EFriendlyFireType>(("FriendlyFireType")) = EFriendlyFireType::On;
			}
			else {
				std::cout << "FriendlyFireType is not valid!\n";
			}

			// if (Engine_Version >= 420)
			{
				*gameState->Member<EAthenaGamePhase>(("GamePhase")) = EAthenaGamePhase::Warmup;

				struct {
					EAthenaGamePhase OldPhase;
				} params2{ EAthenaGamePhase::None };

				static const auto fnGamephase = gameState->Function(("OnRep_GamePhase"));

				if (fnGamephase)
					gameState->ProcessEvent(fnGamephase, &params2);
			}

			if (AuthGameMode)
			{
				// *AuthGameMode->Member<bool>("bAlwaysDBNO") = true;

				AuthGameMode->ProcessEvent(AuthGameMode->Function(("StartPlay")), nullptr);

				auto FNVer = FnVerDouble;

				if (std::floor(FNVer) == 3 || FNVer >= 8.0) 
				{
					//If This is called on Seasons 4, 6, or 7 then Setting The Playlist Crashes.
					AuthGameMode->ProcessEvent("StartMatch");
				}

				if (!bIsSTW)
				{

					bIsPlayground = PlaylistToUse == "FortPlaylistAthena /Game/Athena/Playlists/Playground/Playlist_Playground.Playlist_Playground";

					if (FnVerDouble >= 6.10) // WRONG
					{
						auto OnRepPlaylist = gameState->Function(("OnRep_CurrentPlaylistInfo"));

						static auto BasePlaylistOffset = FindOffsetStruct(("ScriptStruct /Script/FortniteGame.PlaylistPropertyArray"), ("BasePlaylist"));
						static auto PlaylistReplicationKeyOffset = FindOffsetStruct(("ScriptStruct /Script/FortniteGame.PlaylistPropertyArray"), ("PlaylistReplicationKey"));

						if (BasePlaylistOffset && OnRepPlaylist && Playlist)
						{
							auto PlaylistInfo = gameState->Member<UScriptStruct>(("CurrentPlaylistInfo"));

							// PlaylistInfo->MemberStruct<UObject*>("BasePlaylist");
							auto BasePlaylist = (UObject**)(__int64(PlaylistInfo) + BasePlaylistOffset);// *gameState->Member<UObject>(("CurrentPlaylistInfo"))->Member<UObject*>(("BasePlaylist"), true);
							auto PlaylistReplicationKey = (int*)(__int64(PlaylistInfo) + PlaylistReplicationKeyOffset);

							if (BasePlaylist)
							{
								*BasePlaylist = Playlist;
								(*PlaylistReplicationKey)++;
								if (Engine_Version <= 422)
									((FFastArraySerializerOL*)PlaylistInfo)->MarkArrayDirty();
								else
									((FFastArraySerializerSE*)PlaylistInfo)->MarkArrayDirty();
								std::cout << ("Set playlist to: ") << Playlist->GetFullName() << '\n';
							}
							else
								std::cout << ("Base Playlist is null!\n");
						}
						else
						{
							std::cout << ("Missing something related to the Playlist!\n");
							std::cout << ("BasePlaylist Offset: ") << BasePlaylistOffset << '\n';
							std::cout << ("OnRepPlaylist: ") << OnRepPlaylist << '\n';
							std::cout << ("Playlist: ") << Playlist << '\n';
						}

						auto MapInfo = *gameState->Member<UObject*>("MapInfo");

						std::cout << "MapInfo: " << MapInfo << '\n';

						gameState->ProcessEvent(OnRepPlaylist);
					}
					else
					{
						if (Playlist) {
							static auto OnRepPlaylist = gameState->Function(("OnRep_CurrentPlaylistData"));
							*gameState->Member<UObject*>(("CurrentPlaylistData")) = Playlist;

							if (OnRepPlaylist)
								gameState->ProcessEvent(OnRepPlaylist);
						}
						else {
							std::cout << ("Playlist is NULL") << '\n';
						}
					}

					std::cout << "GameSessionName: " << (*AuthGameMode->Member<UObject*>("GameSession"))->GetFullName() << '\n';
				}
			}
			else
			{
				std::cout << dye::yellow(("[WARNING] ")) << ("Failed to find AuthorityGameMode!\n");
			}

			if (Engine_Version != 421)
			{
				auto PlayersLeft = gameState->Member<int>(("PlayersLeft"));

				if (PlayersLeft && *PlayersLeft)
					*PlayersLeft = 0;

				static auto OnRep_PlayersLeft = gameState->Function(("OnRep_PlayersLeft"));

				if (OnRep_PlayersLeft)
					gameState->ProcessEvent(OnRep_PlayersLeft);
			}
		}

		auto GameInstance = *GetEngine()->Member<UObject*>(("GameInstance"));
		auto& LocalPlayers = *GameInstance->Member<TArray<UObject*>>(("LocalPlayers"));
		auto PlayerController = *LocalPlayers.At(0)->Member<UObject*>(("PlayerController"));

		if (PlayerController)
			*PlayerController->Member<UObject*>("CheatManager") = Easy::SpawnObject(FindObject("Class /Script/Engine.CheatManager"), PlayerController);

		Listen(7777);
		// CreateThread(0, 0, MapLoadThread, 0, 0, 0);

		InitializeNetHooks();

		std::cout << ("Initialized NetHooks!\n");

		if (Engine_Version >= 420 && !bIsSTW) {
			Events::LoadEvents();
			Helper::FixPOIs();
		}
		
		if (FnVerDouble != 12.61)
			LootingV2::InitializeWeapons(nullptr);

		/* auto bUseDistanceBasedRelevancy = (*world->Member<UObject*>("NetworkManager"))->Member<bool>("bUseDistanceBasedRelevancy");
		std::cout << "bUseDistanceBasedRelevancy: " << *bUseDistanceBasedRelevancy << '\n';
		*bUseDistanceBasedRelevancy = !(*bUseDistanceBasedRelevancy); */

		/*

		// cursed server pawn if you want to see level names or something

		if (FnVerDouble < 7.4)
		{
			static const auto QuickBarsClass = FindObject("Class /Script/FortniteGame.FortQuickBars", true);
			auto QuickBars = PlayerController->Member<UObject*>("QuickBars");

			if (QuickBars)
			{
				*QuickBars = Easy::SpawnActor(QuickBarsClass, FVector(), FRotator());
				Helper::SetOwner(*QuickBars, PlayerController);
			}
		}

		*PlayerController->Member<char>(("bReadyToStartMatch")) = true;
		*PlayerController->Member<char>(("bClientPawnIsLoaded")) = true;
		*PlayerController->Member<char>(("bHasInitiallySpawned")) = true;

		*PlayerController->Member<bool>(("bHasServerFinishedLoading")) = true;
		*PlayerController->Member<bool>(("bHasClientFinishedLoading")) = true;

		auto PlayerState = *PlayerController->Member<UObject*>(("PlayerState"));

		*PlayerState->Member<char>(("bHasStartedPlaying")) = true;
		*PlayerState->Member<char>(("bHasFinishedLoading")) = true;
		*PlayerState->Member<char>(("bIsReadyToContinue")) = true;

		Helper::InitPawn(PlayerController); */

		// GAMEMODE STUFF HERE

		bRestarting = false;
		bIsReadyToRestart = false;

		if (Helper::IsRespawnEnabled())
		{
			// Enable glider redeploy (idk if this works)
			FString GliderRedeployCmd;
			GliderRedeployCmd.Set(L"Athena.EnableParachuteEverywhere 1");
			// Helper::Console::ExecuteConsoleCommand(GliderRedeployCmd);
		}

		/* if (PlaylistToUse.contains("FortPlaylistAthena /Game/Athena/Playlists/Low/Playlist_Low_Squads.Playlist_Low_"))
		{
			float GravityScale = 50;
			auto WorldSettings = *(*world->Member<UObject*>("PersistentLevel"))->Member<UObject*>("WorldSettings");

			std::cout << "WorldSettings: " << WorldSettings << '\n';

			*WorldSettings->Member<float>("WorldGravityZ") = GravityScale;
			*WorldSettings->Member<float>("GlobalGravityZ") = GravityScale;
		} */
	}
}

bool ServerReviveFromDBNOHook(UObject* DownedPawn, UFunction*, void* Parameters)
{
	if (DownedPawn)
	{
		auto DownedController = *DownedPawn->Member<UObject*>(("Controller"));
		*DownedPawn->Member<bool>(("bIsDBNO")) = false;

		static auto OnRep_IsDBNO = DownedPawn->Function("OnRep_IsDBNO");
		DownedPawn->ProcessEvent(OnRep_IsDBNO);

		struct parms { UObject* EventInstigator; }; // AController*

		auto Params = (parms*)Parameters;

		auto Instigator = Params->EventInstigator;

		static auto ClientOnPawnRevived = DownedController->Function("ClientOnPawnRevived");

		if (ClientOnPawnRevived)
			DownedController->ProcessEvent(ClientOnPawnRevived, &Instigator);

		Helper::SetHealth(DownedPawn, 30);
	}

	return false;
}

bool OnSafeZoneStateChangeHook(UObject* Indicator, UFunction* Function, void* Parameters) // TODO: Change this func
{
	std::cout << "OnSafeZoneStateChange!\n";

	if (Indicator && Parameters && Helper::IsSmallZoneEnabled())
	{
		struct ASafeZoneIndicator_C_OnSafeZoneStateChange_Params
		{
		public:
			EFortSafeZoneState              NewState;                                          // 0x0(0x1)(BlueprintVisible, BlueprintReadOnly, Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
			bool                                       bInitial;                                          // 0x1(0x1)(BlueprintVisible, BlueprintReadOnly, Parm, ZeroConstructor, IsPlainOldData, NoDestructor)
		};

		auto Params = (ASafeZoneIndicator_C_OnSafeZoneStateChange_Params*)Parameters;

		std::random_device rd; // obtain a random number from hardware
		std::mt19937 gen(rd()); // seed the generator
		std::uniform_int_distribution<> distr(300.f, 5000.f);

		std::random_device rd1; // obtain a random number from hardware
		std::mt19937 gen1(rd1()); // seed the generator
		std::uniform_int_distribution<> distr1(300.f, 5000.f);

		std::random_device rd2; // obtain a random number from hardware
		std::mt19937 gen2(rd2()); // seed the generator
		std::uniform_int_distribution<> distr2(300.f, 5000.f);

		auto world = Helper::GetWorld();
		auto AuthGameMode = *world->Member<UObject*>(("AuthorityGameMode"));
		auto SafeZonePhase = *AuthGameMode->Member<int>("SafeZonePhase");

		bool bIsStartZone = SafeZonePhase == 0; // Params->NewState == EFortSafeZoneState::Starting
		
		// *Indicator->Member<float>("SafeZoneStartShrinkTime") // how fast the storm iwll shrink

		auto Radius = *Indicator->Member<float>("Radius");
		*Indicator->Member<float>("NextRadius") = bIsStartZone ? 10000 : Radius / 2;

		auto NextCenter = Indicator->Member<FVector>("NextCenter");

		if (bIsStartZone)
		{
			*Indicator->Member<float>("Radius") = 14000;
			*NextCenter = AircraftLocationToUse;
		}
		else
			*NextCenter += FVector{ (float)distr(gen), (float)distr1(gen1), (float)distr2(gen2) };
	}

	return true;
}

bool ServerLoadingScreenDroppedHook(UObject* PlayerController, UFunction* Function, void* Parameters)
{
	// auto PlayerState = *PlayerController->Member<UObject*>(("PlayerState"));
	// auto Pawn = *PlayerController->Member<UObject*>(("Pawn"));

	return false;
}

// WE NEED TO COMPLETELY SWITCH OFF OF THIS FUNCTION!
bool ServerUpdatePhysicsParamsHook(UObject* Vehicle, UFunction* Function, void* Parameters) // FortAthenaVehicle
{
	if (Vehicle && Parameters)
	{
		struct parms { __int64 InState; };
		auto Params = (parms*)Parameters;

		static auto TranslationOffset = FindOffsetStruct(("ScriptStruct /Script/FortniteGame.ReplicatedAthenaVehiclePhysicsState"), ("Translation"));
		auto Translation = (FVector*)(__int64(&Params->InState) + TranslationOffset);

		static auto RotationOffset = FindOffsetStruct(("ScriptStruct /Script/FortniteGame.ReplicatedAthenaVehiclePhysicsState"), ("Rotation"));
		auto Rotation = (FQuat*)(__int64(&Params->InState) + RotationOffset);

		static auto LinearVelocityOffset = FindOffsetStruct(("ScriptStruct /Script/FortniteGame.ReplicatedAthenaVehiclePhysicsState"), ("LinearVelocity"));
		auto LinearVelocity = (FVector*)(__int64(&Params->InState) + LinearVelocityOffset);

		static auto AngularVelocityOffset = FindOffsetStruct(("ScriptStruct /Script/FortniteGame.ReplicatedAthenaVehiclePhysicsState"), ("AngularVelocity"));
		auto AngularVelocity = (FVector*)(__int64(&Params->InState) + AngularVelocityOffset);

		if (Translation && Rotation)
		{
			/* std::cout << ("X: ") << Translation->X << '\n';
			std::cout << ("Y: ") << Translation->Y << '\n';
			std::cout << ("Z: ") << Translation->Z << '\n';

			auto rot = Rotation->Rotator();

			// rot.Pitch = 0;

			std::cout << ("Pitch: ") << rot.Pitch << '\n';
			std::cout << ("Yaw: ") << rot.Yaw << '\n';
			std::cout << ("Roll: ") << rot.Roll << '\n'; */

			auto OGRot = Helper::GetActorRotation(Vehicle);

			Helper::SetActorLocation(Vehicle, *Translation);
			// Helper::SetActorLocationAndRotation(Vehicle, *Translation, FRotator{ rot.Pitch, OGRot.Yaw, OGRot.Roll });

			UObject* RootComp = nullptr;
			static auto GetRootCompFunc = Vehicle->Function("K2_GetRootComponent");

			if (GetRootCompFunc)
				Vehicle->ProcessEvent(GetRootCompFunc, &RootComp);

			if (RootComp)
			{
				// SetSimulatePhysics

				static auto SetPhysicsLinearVelocity = RootComp->Function("SetPhysicsLinearVelocity");

				struct {
					FVector                                     NewVel;                                                   // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
					bool                                        bAddToCurrent;                                            // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
					FName                                       BoneName;
				} SetPhysicsLinearVelocity_Params{*LinearVelocity, false, FName()};

				if (SetPhysicsLinearVelocity)
					RootComp->ProcessEvent(SetPhysicsLinearVelocity, &SetPhysicsLinearVelocity);

				static auto SetPhysicsAngularVelocity = RootComp->Function("SetPhysicsAngularVelocity");

				struct {
					FVector                                     NewAngVel;                                                // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
					bool                                               bAddToCurrent;                                            // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
					FName                                       BoneName;
				} SetPhysicsAngularVelocity_Params{*AngularVelocity, false, FName()};

				if (SetPhysicsLinearVelocity)
					RootComp->ProcessEvent(SetPhysicsAngularVelocity, &SetPhysicsAngularVelocity_Params);
			}

			// Vehicle->ProcessEvent("OnRep_ServerCorrection");
		}
	}

	return false;
}

bool ServerAttemptAircraftJumpHook(UObject* PlayerController, UFunction* Function, void* Parameters)
{
	if (Engine_Version >= 424)
		PlayerController = Helper::GetOwnerOfComponent(PlayerController);

	struct Param{
		FRotator                                    ClientRotation;
	};

	std::cout << "Called!\n";

	auto Params = (Param*)Parameters;

	if (PlayerController && Params) // PlayerController->IsInAircraft()
	{
		auto world = Helper::GetWorld();
		auto GameState = Helper::GetGameState();

		if (GameState)
		{
			auto Aircrafts = GameState->Member<TArray<UObject*>>(("Aircrafts"));

			if (Aircrafts)
			{
				auto Aircraft = Aircrafts->At(0);

				if (Aircraft)
				{
					if (bClearInventoryOnAircraftJump)
						ClearInventory(PlayerController);

					auto ExitLocation = Helper::GetActorLocation(Aircraft);

					auto Pawn = Helper::InitPawn(PlayerController, false, ExitLocation);

					// ClientSetRotation

					if (Pawn)
					{
						static auto setShieldFn = Pawn->Function(("SetShield"));
						struct { float NewValue; }shieldParams{ 0 };

						if (setShieldFn)
							Pawn->ProcessEvent(setShieldFn, &shieldParams);
						else
							std::cout << ("Unable to find setShieldFn!\n");
					}

					Aircraft->ProcessEvent("PlayEffectsForPlayerJumped");

					/* if (Engine_Version <= 421)
					{
						auto CheatManager = PlayerController->Member<UObject*>("CheatManager");

						static auto God = (*CheatManager)->Function("God");

						if (God)
							(*CheatManager)->ProcessEvent(God);
					} */
				}
			}
		}
	}

	return false;
}

bool ReadyToStartMatchHook(UObject* Object, UFunction* Function, void* Parameters)
{
	// std::cout << "Ready to start match!\n";
	initStuff();
	return false;
}

void LoadInMatch()
{
	auto GameInstance = *GetEngine()->Member<UObject*>(("GameInstance"));
	auto& LocalPlayers = *GameInstance->Member<TArray<UObject*>>(("LocalPlayers"));
	auto PlayerController = *LocalPlayers.At(0)->Member<UObject*>(("PlayerController"));

	if (PlayerController)
	{
		static auto SwitchLevelFn = PlayerController->Function(("SwitchLevel"));
		FString Map;
		Map.Set(GetMapName());
		PlayerController->ProcessEvent(SwitchLevelFn, &Map);
		// Map.Free();
		bTraveled = true;
	}
	else
	{
		std::cout << dye::red(("[ERROR] ")) << ("Unable to find PlayerController!\n");
	}
}

uint8_t GetDeathCause(UObject* PlayerState, FGameplayTagContainer Tags)
{
	// UFortDeathCauseFromTagMapping
	// FortDeathCauseFromTagMapping

	/* struct
	{
		FGameplayTagContainer                       InTags;                                                   // (ConstParm, Parm, OutParm, ReferenceParm, NativeAccessSpecifierPublic)
		bool                                               bWasDBNO;                                                 // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
		uint8_t                                        ReturnValue;                                              // (Parm, OutParm, ZeroConstructor, ReturnParm, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
	} AFortPlayerStateAthena_ToDeathCause_Params{Tags, false};

	static auto ToDeathCause = PlayerState->Function("ToDeathCause");

	if (ToDeathCause)
		PlayerState->ProcessEvent(ToDeathCause, &AFortPlayerStateAthena_ToDeathCause_Params);

	return AFortPlayerStateAthena_ToDeathCause_Params.ReturnValue; */
	return 1;
}

void RequestExitWithStatusHook(bool Force, uint8_t ReturnCode)
{
	std::cout << std::format("Force: {} ReturnCode: {}", Force, std::to_string((int)ReturnCode));
	return;
}

void Restart()
{
	bStarted = false;
	bListening = false;
	bTraveled = false;
	bRestarting = true;
	AmountOfRestarts++;

	DisableNetHooks();

	if (BeaconHost)
		Helper::DestroyActor(BeaconHost);

	ExistingBuildings.clear();

	LoadInMatch();

	bIsReadyToRestart = true;

	Teams::NextTeamIndex = Teams::StartingTeamIndex;
}

DWORD WINAPI AutoRestartThread(LPVOID)
{
	Sleep(6000); // the issue why we cant auto restart is because destroying the beacon actor kicks all players

	Restart();
}

inline bool ClientOnPawnDiedHook(UObject* DeadPC, UFunction* Function, void* Parameters)
{
	if (DeadPC && Parameters)
	{
		struct parms { __int64 DeathReport; };

		auto Params = (parms*)Parameters;

		auto DeadPawn = Helper::GetPawnFromController(DeadPC);
		auto DeadPlayerState = *DeadPC->Member<UObject*>(("PlayerState"));
		auto GameState = Helper::GetGameState();

		static auto KillerPawnOffset = FindOffsetStruct(("ScriptStruct /Script/FortniteGame.FortPlayerDeathReport"), ("KillerPawn"));
		static auto KillerPlayerStateOffset = FindOffsetStruct(("ScriptStruct /Script/FortniteGame.FortPlayerDeathReport"), ("KillerPlayerState"));

		auto KillerPawn = *(UObject**)(__int64(&Params->DeathReport) + KillerPawnOffset);
		auto KillerPlayerState = *(UObject**)(__int64(&Params->DeathReport) + KillerPlayerStateOffset);

		UObject* KillerController = nullptr;

		if (KillerPawn)
			KillerController = *KillerPawn->CachedMember<UObject*>(("Controller"));
			
		static auto DeathLocationOffset = FindOffsetStruct(("ScriptStruct /Script/FortniteGame.DeathInfo"), ("DeathLocation"));	
		auto DeathInfoOffset = GetOffset(DeadPlayerState, "DeathInfo");

		if (DeathInfoOffset == -1) // iirc if u rejoin and die this is invalid idfk why
			return false;

		auto DeathInfo = (__int64*)(__int64(DeadPlayerState) + DeathInfoOffset);

		auto DeathLocation = Helper::GetActorLocation(DeadPawn); // *(FVector*)(__int64(&*DeathInfo) + DeathLocationOffset);

		if (Helper::IsRespawnEnabled()) // || bIsTrickshotting ? !KillerController : false) // basically, if trickshotting, respawn player if they dont die to a player
			Player::RespawnPlayer(DeadPC);
		else
		{
			auto DeathCause = EDeathCause::SniperNoScope;

			auto PlayersLeft = GameState->Member<int>(("PlayersLeft"));

			(*PlayersLeft)--;

			if (*PlayersLeft == 1 && KillerController)
			{
				static auto DamageCauserOffset = FindOffsetStruct(("ScriptStruct /Script/FortniteGame.FortPlayerDeathReport"), ("DamageCauser"));

				auto DamageCauser = *(UObject**)(__int64(&Params->DeathReport) + DamageCauserOffset);

				if (DamageCauser)
					std::cout << "DamageCauser Name: " << DamageCauser->GetFullName() << '\n';

				UObject* FinishingWeaponDefinition = nullptr;

				if (DamageCauser)
				{
					static auto ProjectileClass = FindObject("Class /Script/FortniteGame.FortProjectileBase");
					static auto FortWeaponClass = FindObject("Class /Script/FortniteGame.FortWeapon");

					if (DamageCauser->IsA(ProjectileClass))
						FinishingWeaponDefinition = Helper::GetWeaponData(Helper::GetOwner(DamageCauser));
					else if (DamageCauser->IsA(FortWeaponClass))
						FinishingWeaponDefinition = Helper::GetWeaponData(DamageCauser); // *(*KillerPawn->Member<UObject*>("CurrentWeapon"))->Member<UObject*>("WeaponData");
				}

				struct
				{
					UObject* FinisherPawn;          // APawn                                   // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
					UObject* FinishingWeapon; // UFortWeaponItemDefinition                                          // (ConstParm, Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
					EDeathCause                                        DeathCause;                                               // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
				} AFortPlayerControllerAthena_ClientNotifyWon_Params{ KillerPawn, FinishingWeaponDefinition, DeathCause};

				static auto ClientNotifyWon = KillerController->Function("ClientNotifyWon");

				if (ClientNotifyWon)
					KillerController->ProcessEvent(ClientNotifyWon, &AFortPlayerControllerAthena_ClientNotifyWon_Params);

				// CreateThread(0, 0, AutoRestartThread, 0, 0, 0);

				auto WinningPlayerState = GameState->Member<UObject*>("WinningPlayerState");

				if (WinningPlayerState)
				{
					*WinningPlayerState = KillerPlayerState;
					*GameState->Member<int>("WinningTeam") = *KillerPlayerState->Member<int>("TeamIndex");

					struct FFortWinnerPlayerData { int PlayerId; };

					GameState->Member<TArray<FFortWinnerPlayerData>>("WinningPlayerList")->Add(FFortWinnerPlayerData{ *KillerPlayerState->Member<int>("WorldPlayerId") });

					GameState->ProcessEvent("OnRep_WinningPlayerState");
					GameState->ProcessEvent("OnRep_WinningTeam");
				}
			}

			// DeadPC->ClientSendMatchStatsForPlayer(DeadPC->GetMatchReport()->MatchStats);

			auto ItemInstances = Inventory::GetItemInstances(DeadPC);

			if (ItemInstances)
			{
				for (int i = 1; i < ItemInstances->Num(); i++)
				{
					auto ItemInstance = ItemInstances->At(i);

					if (ItemInstance)
					{
						struct
						{
							FGuid                                       ItemGuid;                                                 // (Parm, ZeroConstructor, IsPlainOldData)
							int                                                Count;                                                    // (Parm, ZeroConstructor, IsPlainOldData)
						} AFortPlayerController_ServerAttemptInventoryDrop_Params{ Inventory::GetItemGuid(ItemInstance), *FFortItemEntry::GetCount(GetItemEntryFromInstance<__int64>(ItemInstance)) };

						// ServerAttemptInventoryDropHook(DeadPC, nullptr, &AFortPlayerController_ServerAttemptInventoryDrop_Params);

						auto Params = &AFortPlayerController_ServerAttemptInventoryDrop_Params;

						auto Definition = Inventory::TakeItem(DeadPC, Params->ItemGuid, Params->Count, true);
						auto Pawn = Helper::GetPawnFromController(DeadPC);

						// bDropOnDBNO

						if (Pawn && Definition)
						{
							auto loc = Helper::GetActorLocation(Pawn);

							auto Pickup = Helper::SummonPickup(Pawn, Definition, DeathLocation, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::PlayerElimination, 
								Params->Count);
						}
					}
				}
			}

			if (Engine_Version >= 423) // wrong
			{
				auto Chip = Helper::SpawnChip(DeadPC, DeathLocation);

				if (Chip)
				{
					// 0x0018
					struct FFortResurrectionData
					{
						bool                                               bResurrectionChipAvailable;                               // 0x0000(0x0001) (ZeroConstructor, IsPlainOldData)
						unsigned char                                      UnknownData00[0x3];                                       // 0x0001(0x0003) MISSED OFFSET
						float                                              ResurrectionExpirationTime;                               // 0x0004(0x0004) (ZeroConstructor, IsPlainOldData)
						float                                              ResurrectionExpirationLength;                             // 0x0008(0x0004) (ZeroConstructor, IsPlainOldData)
						struct FVector                                     WorldLocation;                                            // 0x000C(0x000C) (ZeroConstructor, IsPlainOldData)
					};

					auto FortResurrectionData = DeadPlayerState->Member<FFortResurrectionData>(("ResurrectionChipAvailable"));

					if (FortResurrectionData)
					{
						std::cout << ("FortResurrectionData valid!\n");
						std::cout << ("FortResurrectionData Location X: ") << FortResurrectionData->WorldLocation.X << '\n';
					}
					else
						std::cout << ("No FortResurrectionData!\n");

					static auto ServerTimeForResurrectOffset = FindOffsetStruct(("ScriptStruct /Script/FortniteGame.FortPlayerDeathReport"), ("ServerTimeForResurrect"));

					auto ServerTimeForResurrect = *(float*)(__int64(&Params->DeathReport) + ServerTimeForResurrectOffset);

					std::cout << "ServerTimeForResurrect: " << ServerTimeForResurrect << '\n';

					auto ResurrectionData = FFortResurrectionData{};
					ResurrectionData.bResurrectionChipAvailable = true;
					ResurrectionData.ResurrectionExpirationLength = ServerTimeForResurrect;
					ResurrectionData.ResurrectionExpirationTime = ServerTimeForResurrect;
					ResurrectionData.WorldLocation = Helper::GetActorLocation(Chip);

					if (FortResurrectionData)
						*FortResurrectionData = ResurrectionData;

					std::cout << ("Spawned Chip!\n");
				}
			}
		}

		{
			if (KillerPlayerState)
			{
				// idk whats wrong with the tagcontainer
				/* static auto TagsOffset = FindOffsetStruct(("ScriptStruct /Script/FortniteGame.DeathInfo"), ("Tags"));
				static auto DeathCauseOffset = FindOffsetStruct(("ScriptStruct /Script/FortniteGame.DeathInfo"), ("DeathCause"));
				static auto FinisherOrDownerOffset = FindOffsetStruct(("ScriptStruct /Script/FortniteGame.DeathInfo"), ("FinisherOrDowner"));
				static auto bDBNOOffset = FindOffsetStruct(("ScriptStruct /Script/FortniteGame.DeathInfo"), ("bDBNO"));

				// /Script/FortniteGame.FortPlayerStateAthena.OnRep_TeamKillScore

				FGameplayTagContainer* Tags = nullptr; // (FGameplayTagContainer*)(__int64(&*DeathInfo) + TagsOffset);

				*(uint8_t*)(__int64(&*DeathInfo) + DeathCauseOffset) = Tags ? GetDeathCause(DeadPlayerState, *Tags) : (uint8_t)EDeathCause::Cube;
				*(UObject**)(__int64(&*DeathInfo) + FinisherOrDownerOffset) = KillerPlayerState ? KillerPlayerState : DeadPlayerState;
				*(bool*)(__int64(&*DeathInfo) + bDBNOOffset) = false;

				static auto OnRep_DeathInfo = DeadPlayerState->Function(("OnRep_DeathInfo"));

				if (OnRep_DeathInfo)
					DeadPlayerState->ProcessEvent(OnRep_DeathInfo); */
			}
		}

		if (KillerPlayerState && KillerPlayerState != DeadPlayerState) // make sure if they didnt die of like falling or rejoining they dont get their kill
		{
			if (KillerController)
			{
				static auto ClientReceiveKillNotification = KillerController->Function(("ClientReceiveKillNotification"));

				struct {
					// Both playerstates
					UObject* Killer;
					UObject* Killed;
				} ClientReceiveKillNotification_Params{ KillerPlayerState, DeadPlayerState };

				if (ClientReceiveKillNotification)
					KillerController->ProcessEvent(ClientReceiveKillNotification, &ClientReceiveKillNotification_Params);
			}

			static auto ClientReportKill = KillerPlayerState->Function(("ClientReportKill"));
			struct { UObject* PlayerState; }ClientReportKill_Params{ DeadPlayerState };

			if (ClientReportKill)
				KillerPlayerState->ProcessEvent(ClientReportKill, &ClientReportKill_Params);

			(*KillerPlayerState->Member<int>(("KillScore")))++;

			if (Engine_Version >= 423) // idgaf wrong
				(*KillerPlayerState->Member<int>(("TeamKillScore")))++;

			static auto OnRep_Kills = KillerPlayerState->Function(("OnRep_Kills"));

			if (OnRep_Kills)
				KillerPlayerState->ProcessEvent(OnRep_Kills);
		}

		ProcessEventO(DeadPC, Function, Parameters);

		if (FnVerDouble < 6 && false)
		{
			// void(__fastcall * BeginSpectating)(UObject * OurPlayerState, UObject * PlayerStateToSpectate); // some versions it returns how many spectators are spectating the player (including you)

			// BeginSpectating = decltype(BeginSpectating)(FindPattern("40 53 55 48 83 EC 48 48 8B DA 48 8B E9 48 3B 91 ? ? ? ? 75 40 80 3D ? ? ? ? ? 0F 82 ? ? ? ? 48 8B 05 ? ? ? ? 4C 8D 44 24 ? 48 89 44 24 ? 41 B9"));

			auto World = Helper::GetWorld();

			if (World)
			{
				auto NetDriver = *World->Member<UObject*>(("NetDriver"));
				if (NetDriver)
				{
					auto ClientConnections = NetDriver->Member<TArray<UObject*>>(("ClientConnections"));

					if (ClientConnections)
					{
						for (int i = 0; i < ClientConnections->Num(); i++)
						{
							auto Connection = ClientConnections->At(i);

							if (!Connection)
								continue;

							auto aaController = *Connection->Member<UObject*>(("PlayerController"));

							if (aaController)
							{
								auto aaPlayerState = *aaController->Member<UObject*>(("PlayerState"));
								auto aaPawn = Helper::GetPawnFromController(aaController);

								if (aaPawn)
								{
									static auto IsActorBeingDestroyed = aaPawn->Function(("IsActorBeingDestroyed"));

									bool bIsActorBeingDestroyed = true;

									if (IsActorBeingDestroyed)
										aaPawn->ProcessEvent(IsActorBeingDestroyed, &bIsActorBeingDestroyed);

									if (aaPlayerState && aaPlayerState != DeadPlayerState && aaPawn) // && !bIsActorBeingDestroyed)
									{
										// BeginSpectating(DeadPlayerState, KillerPlayerState ? KillerPlayerState : aaPlayerState);
										std::cout << "wtf!\n";
										break;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return true;
}

inline bool ServerAttemptExitVehicleHook(UObject* Controller, UFunction* Function, void* Parameters)
{
	auto Pawn = Helper::GetPawnFromController(Controller);

	if (Pawn)
	{
		UObject* Vehicle = Helper::GetVehicle(Pawn);

		if (Vehicle)
		{
			Helper::SetLocalRole(Pawn, ENetRole::ROLE_Authority);
			Helper::SetLocalRole(Vehicle, ENetRole::ROLE_Authority);

			UObject* VehicleWeaponDef = nullptr; // Vehicle->Member<UObject*>("CachedWeaponDef");

			// Function /Script/FortniteGame.FortPlayerControllerZone.ServerRequestSeatChange

			if (VehicleWeaponDef)
			{
				VehicleWeaponDef = Helper::GetWeaponData(Helper::GetCurrentWeapon(Pawn)); // scuffed? noooo

				if (VehicleWeaponDef)
				{
					auto VehicleWeaponInstance = Inventory::FindItemInInventory(Controller, VehicleWeaponDef);

					if (VehicleWeaponInstance)
					{
						Inventory::RemoveItem(Controller, Inventory::GetItemGuid(VehicleWeaponInstance));
					}
				}
			}
		}
	}

	return false;
}

inline bool ServerPlayEmoteItemHook(UObject* Controller, UFunction* Function, void* Parameters)
{
	auto Pawn = Helper::GetPawnFromController(Controller);

	struct SPEIParams  { UObject* EmoteAsset; }; // UFortMontageItemDefinitionBase
	auto EmoteParams = (SPEIParams*)Parameters;

	auto EmoteAsset = EmoteParams->EmoteAsset;

	std::cout << "aaaaaaa!\n";

	if (Controller /* && !Controller->IsInAircraft() */ && Pawn && EmoteAsset)
	{
		struct {
			TEnumAsByte<EFortCustomBodyType> BodyType;
			TEnumAsByte<EFortCustomGender> Gender;
			UObject* AnimMontage; // UAnimMontage
		} GAHRParams{EFortCustomBodyType::All, EFortCustomGender::Both}; // (CurrentPawn->CharacterBodyType, CurrentPawn->CharacterGender)
		static auto fn = EmoteAsset->Function(("GetAnimationHardReference"));

		auto EmoteAssetName = EmoteAsset->GetFullName();

		std::cout << "EmoteAsset: " << EmoteAsset->GetFullName() << '\n';

		/* if (EmoteAssetName.contains("Spray"))
		{
			struct
			{
				UObject* SpawningPC;      // AFortPlayerController                                          // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
				UObject* InSprayAsset;            // UAthenaSprayItemDefinition                                  // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
			}AFortSprayDecalInstance_SetSprayParameters_Params{ Controller, EmoteAsset };

			struct FFortSprayDecalRepPayload
			{
				UObject* SprayAsset;             // UAthenaSprayItemDefinition                                   // 0x0000(0x0008) (BlueprintVisible, BlueprintReadOnly, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
				FName                                       BannerName;                                               // 0x0008(0x0008) (BlueprintVisible, BlueprintReadOnly, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
				FName                                       BannerColor;                                              // 0x0010(0x0008) (BlueprintVisible, BlueprintReadOnly, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
				int                                                SavedStatValue;                                           // 0x0018(0x0004) (BlueprintVisible, BlueprintReadOnly, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
				unsigned char                                      UnknownData00[0x4];                                       // 0x001C(0x0004) MISSED OFFSET
			};

			static auto SprayDecalInstanceClass = FindObject("BlueprintGeneratedClass /Game/Athena/Cosmetics/Sprays/BP_SprayDecal.BP_SprayDecal_C");

			auto DecalInstance = Easy::SpawnActor(SprayDecalInstanceClass, Helper::GetActorLocation(Pawn), FRotator());

			std::cout << "Instance: " << DecalInstance << '\n';

			static auto SetSprayParameters = DecalInstance->Function("SetSprayParameters");

			if (SetSprayParameters)
			{
				DecalInstance->ProcessEvent(SetSprayParameters, &AFortSprayDecalInstance_SetSprayParameters_Params);
				DecalInstance->ProcessEvent("OnRep_SprayInfo");
				std::cout << "dababy!\n";
				Controller->Member<TArray<UObject*>>("ActiveSprayInstances")->Add(DecalInstance);
			}
			else
				std::cout << "No SetSprayParameters!\n";

			return false;
		}

		// 	class AActor* SpawnToyInstance(class UClass* ToyClass, struct FTransform SpawnPosition);
		//  SoftClassProperty FortniteGame.AthenaToyItemDefinition.ToyActorClass

		else */ if (EmoteAssetName.contains("Toy"))
		{
			auto ToyActorClass = EmoteAsset->Member<TSoftClassPtr>("ToyActorClass");

			std::cout << "BRUH: " << ToyActorClass->ObjectID.AssetPathName.ToString() << '\n';

			static auto BPC = FindObject("Class /Script/Engine.BlueprintGeneratedClass");

			auto ToyClass = StaticLoadObject(BPC, nullptr, ToyActorClass->ObjectID.AssetPathName.ToString()); // FindObject(ToyActorClass->ObjectID.AssetPathName.ToString());

			if (ToyClass)
			{
				/* auto Toy = Easy::SpawnActor(ToyClass, Helper::GetActorLocation(Pawn));
				std::cout << "Toy: " << Toy->GetFullName() << '\n';
				static auto InitializeToyInstance = Toy->Function("InitializeToyInstance");

				struct { UObject* OwningPC; int32_t NumTimesSummoned; } paafiq23{ Controller, 1 };

				if (InitializeToyInstance)
					Toy->ProcessEvent(InitializeToyInstance, &paafiq23); */

				// bool idk = true;
				// Toy->ProcessEvent("OnReplicatedVelocityStartOrStop", &idk);

				// ^ SEMI WORKING CODE (Removed because they never disappear), below desnt do anything

				/* FTransform transform;
				transform.Scale3D = { 1, 1, 1 };
				transform.Translation = Helper::GetActorLocation(Pawn);
				transform.Rotation = {};

				struct
				{
					UObject* ToyClass;      // UClass                                            // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor, UObjectWrapper, HasGetValueTypeHash, NativeAccessSpecifierPublic)
					FTransform                                  SpawnPosition;                                            // (Parm, IsPlainOldData, NoDestructor, NativeAccessSpecifierPublic)
					UObject* ReturnValue;   // AActor                                            // (Parm, OutParm, ZeroConstructor, ReturnParm, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
				} AFortPlayerController_SpawnToyInstance_Params{ ToyClass, transform };

				static auto SpawnToyInstance = Controller->Function("SpawnToyInstance");

				// InitializeToyInstance

				if (SpawnToyInstance)
					Controller->ProcessEvent(SpawnToyInstance, &AFortPlayerController_SpawnToyInstance_Params); */

				// *AFortPlayerController_SpawnToyInstance_Params.ReturnValue->Member<UObject*>("OwningPawn") = Pawn;
			}
			else
				std::cout << "Unable to find ToyClass!\n";
		}

		if (fn)
		{
			EmoteAsset->ProcessEvent(fn, &GAHRParams);
			auto Montage = GAHRParams.AnimMontage;
			if (Montage && Engine_Version < 426 && Engine_Version >= 420)
			{
				std::cout << ("Playing Montage: ") << Montage->GetFullName() << '\n';

				auto AbilitySystemComponent = *Pawn->CachedMember<UObject*>(("AbilitySystemComponent"));
				static auto EmoteClass = FindObject(("BlueprintGeneratedClass /Game/Abilities/Emotes/GAB_Emote_Generic.GAB_Emote_Generic_C"));

				TArray<FGameplayAbilitySpec<FGameplayAbilityActivationInfo, 0x50>> Specs;

				if (Engine_Version <= 422)
					Specs = (*AbilitySystemComponent->CachedMember<FGameplayAbilitySpecContainerOL>(("ActivatableAbilities"))).Items;
				else
					Specs = (*AbilitySystemComponent->CachedMember<FGameplayAbilitySpecContainerSE>(("ActivatableAbilities"))).Items;

				UObject* DefaultObject = EmoteClass->CreateDefaultObject();

				auto GameData = Helper::GetGameData();
				auto EmoteGameplayAbilityClass = EmoteClass; //  GameData->Member<UObject*>("EmoteGameplayAbility"); // its a tassetptr

				if (false)
				{
					for (int i = 0; i < Specs.Num(); i++)
					{
						auto& CurrentSpec = Specs[i];

						if (CurrentSpec.Ability == DefaultObject)
						{
							auto EmoteAbility = CurrentSpec.Ability;

							*EmoteAbility->Member<UObject*>("PlayerPawn") = Pawn;

							auto ActivationInfo = EmoteAbility->Member<FGameplayAbilityActivationInfo>(("CurrentActivationInfo"));

							struct FGameplayAbilityRepAnimMontage
							{
							public:
								UObject* AnimMontage;
							};

							auto RepAnimMontageInfo = Pawn->Member<FGameplayAbilityRepAnimMontage>("RepAnimMontageInfo");
							RepAnimMontageInfo->AnimMontage = Montage;

							*EmoteAbility->Member<UObject*>("CurrentMontage") = Montage;

							// EmoteAbility->ProcessEvent("PlayInitialEmoteMontage");

							// Helper::SetLocalRole(Pawn, ENetRole::ROLE_SimulatedProxy);
							if (PlayMontage)
							{

							}
						}
					}
				}
			}
		}
	}
	else
	{
		std::cout << "Controller: " << Controller << '\n';
		std::cout << "Emoteasset: " << EmoteAsset << '\n';
		std::cout << "Pawn: " << Pawn << '\n';
	}

	return false;
}

void replace_all(std::string& input, const std::string& from, const std::string& to) {
	size_t pos = 0;

	while ((pos = input.find(from, pos)) != std::string::npos) {
		input.replace(pos, from.size(), to);
		pos += to.size();
	}
}

inline bool ServerAttemptInteractHook(UObject* Controllera, UFunction* Function, void* Parameters)
{
	UObject* Controller = Controllera;

	if (Engine_Version >= 423)
	{
		Controller = Helper::GetOwnerOfComponent(Controllera);
	}

	// ProcessEventO(Controller, Function, Parameters);

	struct SAIParams {
		UObject* ReceivingActor;                                           // (Parm, ZeroConstructor, IsPlainOldData)
		UObject* InteractComponent;                                        // (Parm, ZeroConstructor, InstancedReference, IsPlainOldData)
		TEnumAsByte<ETInteractionType>                     InteractType;                                             // (Parm, ZeroConstructor, IsPlainOldData)
		UObject* OptionalObjectData;                                       // (Parm, ZeroConstructor, IsPlainOldData)
	};

	auto Params = (SAIParams*)Parameters;

	if (Params && Controller)
	{
		auto Pawn = *Controller->Member<UObject*>(("Pawn"));
		auto ReceivingActor = Params->ReceivingActor;

		if (ReceivingActor && basicLocationCheck(Pawn, ReceivingActor, 450.f))
		{
			auto ReceivingActorName = ReceivingActor->GetName(); // There has to be a better way, right?

			std::cout << ("ReceivingActorName: ") << ReceivingActorName << '\n';

			static auto BuildingContainerClass = FindObject("Class /Script/FortniteGame.BuildingContainer");

			if (ReceivingActor->IsA(BuildingContainerClass))
			{
				/* if (readBitfield(ReceivingActor, "bAlreadySearched"))
					return false;

				setBitfield(ReceivingActor, "bAlreadySearched", true, true); */

				auto Prop = GetProperty(ReceivingActor, "bAlreadySearched");

				auto offset = GetOffsetFromProp(Prop);

				if (offset == -1)
					return false;

				auto Actual = (__int64*)(__int64(ReceivingActor) + offset);

				static auto FieldMask = GetFieldMask(Prop);
				static auto BitIndex = GetBitIndex(Prop, FieldMask);

				if (!(bool)(*(uint8_t*)Actual & BitIndex))
					return false;

				uint8_t* Byte = (uint8_t*)Actual;

				if (((bool(1) << BitIndex) & *(bool*)(Actual)) != true)
				{
					*Byte = (*Byte & ~FieldMask) | (FieldMask);
				}

				static auto AlreadySearchedFn = ReceivingActor->Function(("OnRep_bAlreadySearched"));

				if (AlreadySearchedFn)
					ReceivingActor->ProcessEvent(AlreadySearchedFn);
			}

			static auto VendingMachineClass = FindObject("BlueprintGeneratedClass /Game/Athena/Items/Gameplay/VendingMachine/B_Athena_VendingMachine.B_Athena_VendingMachine_C");

			if (bIsPlayground && ReceivingActor->IsA(VendingMachineClass))
			{
				static auto WoodItemData = FindObject(("FortResourceItemDefinition /Game/Items/ResourcePickups/WoodItemData.WoodItemData"));
				static auto StoneItemData = FindObject(("FortResourceItemDefinition /Game/Items/ResourcePickups/StoneItemData.StoneItemData"));
				static auto MetalItemData = FindObject(("FortResourceItemDefinition /Game/Items/ResourcePickups/MetalItemData.MetalItemData"));

				auto CurrentMaterial = *ReceivingActor->Member<UObject*>("ActiveInputItem");

				float CostAmount = 0;

				auto MaterialGUID = Inventory::GetItemGuid(Inventory::FindItemInInventory(Controller, CurrentMaterial));
				auto Count = Inventory::GetCount(Inventory::GetItemInstanceFromGuid(Controller, MaterialGUID));

				TArray<__int64>* ItemCollections = ReceivingActor->Member<TArray<__int64>>(("ItemCollections")); // CollectorUnitInfo

				UObject* DefinitionToDrop = nullptr;

				if (ItemCollections)
				{
					static UObject* CollectorUnitInfoClass = FindObject(("ScriptStruct /Script/FortniteGame.CollectorUnitInfo"));
					static std::string CollectorUnitInfoClassName = ("ScriptStruct /Script/FortniteGame.CollectorUnitInfo");

					if (!CollectorUnitInfoClass)
					{
						CollectorUnitInfoClassName = ("ScriptStruct /Script/FortniteGame.ColletorUnitInfo"); // die fortnite
						CollectorUnitInfoClass = FindObject(CollectorUnitInfoClassName); // Wedc what this value is
					}

					static auto OutputItemOffset = FindOffsetStruct(CollectorUnitInfoClassName, ("OutputItem"));
					static auto InputCountOffset = FindOffsetStruct(CollectorUnitInfoClassName, ("InputCount"));

					std::cout << ("Offset: ") << OutputItemOffset << '\n';
					// ItemCollections->At(i).OutputItem = LootingTables::GetWeaponDef();
					// So this is equal to Array[1] + OutputItemOffset, but since the array is __int64, it doesn't calcuate it properly so we have to implement it ourselves

					int Index = 0;

					if (CurrentMaterial == StoneItemData)
						Index = 1;
					else if (CurrentMaterial == MetalItemData)
						Index = 2;

					DefinitionToDrop = *(UObject**)(__int64((__int64*)((__int64(ItemCollections->GetData()) + (GetSizeOfStruct(CollectorUnitInfoClass) * Index)))) + OutputItemOffset);
					CostAmount = ((FScalableFloat*)(__int64((__int64*)((__int64(ItemCollections->GetData()) + (GetSizeOfStruct(CollectorUnitInfoClass) * Index)))) + InputCountOffset))->Value;
				}

				std::cout << "Cost Amount: " << CostAmount << '\n';

				if (Count && *Count >= CostAmount) {
					Inventory::TakeItem(Controller, MaterialGUID, CostAmount);
				}
				else
					return false;

				// CostAmount

				if (CurrentMaterial)
				{
					std::cout << "CurrentMaterial Name: " << CurrentMaterial->GetFullName() << '\n';
				}

				if (DefinitionToDrop)
				{
					Helper::SummonPickup(Pawn, DefinitionToDrop, Helper::GetCorrectLocation(ReceivingActor), EFortPickupSourceTypeFlag::Container, EFortPickupSpawnSource::Unset);
				}
			}

			static auto PawnClass = FindObject(("BlueprintGeneratedClass /Game/Athena/PlayerPawn_Athena.PlayerPawn_Athena_C"));

			if (ReceivingActor->IsA(PawnClass))
			{
				auto InstigatorController = Controller;
				auto DeadPawn = ReceivingActor;
				auto DeadController = *DeadPawn->Member<UObject*>("Controller");

				DeadPawn->ProcessEvent("ReviveFromDBNO", &InstigatorController);
				DeadController->ProcessEvent("ClientOnPawnRevived", &InstigatorController);

				setBitfield(DeadPawn, "bIsDBNO", false);

				Helper::SetHealth(DeadPawn, 30);
			}

			if (ReceivingActorName.contains(("Vehicle")) || ReceivingActorName.contains("Cannon"))
			{
				Helper::SetLocalRole(Pawn, ENetRole::ROLE_Authority);
				Helper::SetLocalRole(ReceivingActor, ENetRole::ROLE_Authority);
				// Helper::SetLocalRole(*Controller->Member<UObject*>(("Pawn")), ENetRole::ROLE_AutonomousProxy);
				// Helper::SetLocalRole(ReceivingActor, ENetRole::ROLE_AutonomousProxy);

				// static auto StartupAbilitySet = *ReceivingActor->Member<UObject*>("StartupAbilitySet");
				// auto Abilities = GiveAbilitySet(Pawn, StartupAbilitySet);

				UObject* VehicleWeaponDefinition = nullptr;

				if (ReceivingActorName.contains("Ferret")) // plane
					VehicleWeaponDefinition = FindObject("FortWeaponRangedItemDefinition /Game/Athena/Items/Weapons/Ferret_Weapon.Ferret_Weapon");
				
				else if (ReceivingActorName.contains("Octopus")) // baller
					VehicleWeaponDefinition = FindObject("FortWeaponRangedItemDefinition /Game/Athena/Items/Weapons/Vehicles/WID_Octopus_Weapon.WID_Octopus_Weapon");

				else if (ReceivingActorName.contains("Cannon")) // cannon
					VehicleWeaponDefinition = FindObject("FortWeaponRangedItemDefinition /Game/Athena/Items/Weapons/Vehicles/ShipCannon_Weapon_InCannon.ShipCannon_Weapon_InCannon");

				else if (ReceivingActorName.contains("Ostrich")) // mech
					VehicleWeaponDefinition = FindObject("FortWeaponRangedItemDefinition /Game/Athena/Items/Weapons/Vehicles/WID_OstrichShotgunTest2.WID_OstrichShotgunTest2");

				std::cout << "goofy ahh\n";

				if (VehicleWeaponDefinition)
				{
					auto instnace = Inventory::GiveItem(Controller, VehicleWeaponDefinition, EFortQuickBars::Primary, 1, 1);
					*FFortItemEntry::GetLoadedAmmo(GetItemEntryFromInstance(instnace)) = INT32_MAX; // pro code
					Inventory::EquipInventoryItem(Controller, Inventory::GetItemGuid(instnace));
					std::cout << "vehicle weapon!\n";
				}		
			}

			if (Engine_Version >= 424 && ReceivingActorName.contains("Wumba")) // Workbench/Upgrade Bench
			{
				auto CurrentWeapon = Helper::GetCurrentWeapon(Pawn);
				auto CurrentWeaponDefinition = Helper::GetWeaponData(CurrentWeapon);
				auto CurrentRarity = CurrentWeaponDefinition->Member<EFortRarityC2>("Rarity");
				 
				// im stupid ok

				std::pair<std::string, std::string> thingToReplace;
				
				switch (*CurrentRarity)
				{
				case EFortRarityC2::Common:
					thingToReplace = std::make_pair("_C_", "_UC_");
					break;
				case EFortRarityC2::Uncommon:
					thingToReplace = std::make_pair("_UC_", "_R_");
					break;
				case EFortRarityC2::Rare:
					thingToReplace = std::make_pair("_R_", "_VR_");
					break;
				case EFortRarityC2::Epic:
					thingToReplace = std::make_pair("_VR_", "_SR_");
					break;
				default:
					thingToReplace = std::make_pair("NONE", "NONE");
					break;
				}

				if (thingToReplace.first == "NONE")
					return false;

				std::string newDefStr = CurrentWeaponDefinition->GetFullName();
				// newDefStr.replace(newDefStr.find(thingToReplace.first), thingToReplace.first.size(), thingToReplace.second);
				replace_all(newDefStr, thingToReplace.first, thingToReplace.second);

				auto newDef = FindObject(newDefStr);

				if (!newDef)
				{
					std::cout << "Unable to find to " << newDefStr << '\n';
					return false;
				}

				auto CostPerMat = ((int)(*CurrentRarity) + 1) * 50;

				static auto WoodItemData = FindObject(("FortResourceItemDefinition /Game/Items/ResourcePickups/WoodItemData.WoodItemData"));
				static auto StoneItemData = FindObject(("FortResourceItemDefinition /Game/Items/ResourcePickups/StoneItemData.StoneItemData"));
				static auto MetalItemData = FindObject(("FortResourceItemDefinition /Game/Items/ResourcePickups/MetalItemData.MetalItemData"));

				auto WoodGUID = Inventory::GetItemGuid(Inventory::FindItemInInventory(Controller, WoodItemData));
				auto StoneGUID = Inventory::GetItemGuid(Inventory::FindItemInInventory(Controller, StoneItemData));
				auto MetalGUID = Inventory::GetItemGuid(Inventory::FindItemInInventory(Controller, MetalItemData));

				Inventory::TakeItem(Controller, WoodGUID, CostPerMat);
				Inventory::TakeItem(Controller, StoneGUID, CostPerMat);
				Inventory::TakeItem(Controller, MetalGUID, CostPerMat);

				Inventory::TakeItem(Controller, *CurrentWeapon->Member<FGuid>("ItemEntryGuid"), 1, true);
				Inventory::GiveItem(Controller, newDef, EFortQuickBars::Primary, 1, 1);
			}

			if (ReceivingActorName.contains("Portapotty")) // dont work rn
			{
				return true;
			}

			// Looting::Tables::HandleSearch(ReceivingActor);
			LootingV2::HandleSearch(ReceivingActor);
		}
	}

	return false;
}

inline bool ServerSendZiplineStateHook(UObject* Pawn, UFunction* Function, void* Parameters)
{
	if (Pawn && Parameters)
	{
		struct FZiplinePawnState
		{
			UObject* Zipline;           // AFortAthenaZipline                                        // 0x0000(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
			bool                                               bIsZiplining;                                             // 0x0008(0x0001) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
			bool                                               bJumped;                                                  // 0x0009(0x0001) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
			unsigned char                                      UnknownData00[0x2];                                       // 0x000A(0x0002) MISSED OFFSET
			int                                                AuthoritativeValue;                                       // 0x000C(0x0004) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
			FVector                                     SocketOffset;                                             // 0x0010(0x000C) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
			float                                              TimeZipliningBegan;                                       // 0x001C(0x0004) (ZeroConstructor, IsPlainOldData, RepSkip, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
			float                                              TimeZipliningEndedFromJump;                               // 0x0020(0x0004) (ZeroConstructor, IsPlainOldData, RepSkip, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
			unsigned char                                      UnknownData01[0x4];                                       // 0x0024(0x0004) MISSED OFFSET
		};

		struct parms { FZiplinePawnState ZiplineState; };
		auto Params = (parms*)Parameters;

		static auto ZiplineOffset = FindOffsetStruct(("ScriptStruct /Script/FortniteGame.ZiplinePawnState"), ("Zipline"));
		static auto SocketOffsetOffset = FindOffsetStruct(("ScriptStruct /Script/FortniteGame.ZiplinePawnState"), ("SocketOffset"));

		auto Zipline = (UObject**)(__int64(&Params->ZiplineState) + ZiplineOffset);
		auto SocketOffset = (FVector*)(__int64(&Params->ZiplineState) + SocketOffsetOffset);

		// Helper::SetLocalRole(Pawn, ENetRole::ROLE_AutonomousProxy);
		// Helper::SetRemoteRole(Pawn, ENetRole::ROLE_Authority);

		struct FFortAnimInput_Zipline
		{
			unsigned char                                      bIsZiplining : 1;
		};

		if (Zipline && *Zipline)
		{
			// TWeakObjectPtr<class AFortPlayerPawn>  CurrentInteractingPawn
			
			TWeakObjectPtr<UObject>* CurrentInteractingPawn = (*Zipline)->Member<TWeakObjectPtr<UObject>>("CurrentInteractingPawn");

			if (CurrentInteractingPawn)
			{
				CurrentInteractingPawn->ObjectIndex = Pawn->InternalIndex;
				CurrentInteractingPawn->ObjectSerialNumber = GetSerialNumber(Pawn);
			}

			struct
			{
				UObject* Zipline;        // AFortAthenaZipline                                           // (ConstParm, Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
				UObject* SocketComponent;         // USceneComponent                                 // (Parm, ZeroConstructor, InstancedReference, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
				FName                                       SocketName;                                               // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
				FVector                                     SocketOffset;                                             // (Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
			} AFortPlayerPawn_BeginZiplining_Params{*Zipline, *(*Zipline)->Member<UObject*>("RootComponent"), FName(-1), *SocketOffset};

			// ^^ they eventually remove the last 2 params

			static auto BeginZiplining = Pawn->Function("BeginZiplining");

			if (BeginZiplining)
				Pawn->ProcessEvent(BeginZiplining, &AFortPlayerPawn_BeginZiplining_Params);

			/* static auto StartZipLining = (*Zipline)->Function("StartZipLining");

			if (StartZipLining)
				(*Zipline)->ProcessEvent(StartZipLining, &Pawn); */

			Helper::GetAnimInstance(Pawn)->Member<FFortAnimInput_Zipline>("ZiplineInput")->bIsZiplining = true;
			
			*Pawn->Member<FZiplinePawnState>("ZiplineState") = Params->ZiplineState;
			Pawn->ProcessEvent("OnRep_ZiplineState");

			// Helper::SetLocalRole(*Zipline, ENetRole::ROLE_AutonomousProxy);
			// Helper::SetLocalRole(*Zipline, ENetRole::ROLE_Authority); // UNTESTED
			// Helper::SetRemoteRole(*Zipline, ENetRole::ROLE_Authority);
		}
		else
		{
			bool bJumped = Params->ZiplineState.bJumped;

			static auto EndZiplining = Pawn->Function("EndZiplining");

			if (EndZiplining)
				Pawn->ProcessEvent(EndZiplining, &bJumped);

			Helper::GetAnimInstance(Pawn)->Member<FFortAnimInput_Zipline>("ZiplineInput")->bIsZiplining = false;

			*Pawn->Member<FZiplinePawnState>("ZiplineState") = Params->ZiplineState;
			Pawn->ProcessEvent("OnRep_ZiplineState");
			std::cout << ("Player is getting off of zipline!\n");
		}
	}

	return false;
}

inline bool ReceiveActorEndOverlapHook(UObject* Actor, UFunction*, void* Parameters)
{
	if (Actor)
	{
		struct parms { UObject* otherActor; };

		auto Params = (parms*)Parameters;

		std::cout << "Actor: " << Actor->GetFullName() << '\n';

		if (Params->otherActor && Actor->GetFullName().contains("Zipline"))
		{
			struct FFortAnimInput_Zipline
			{
				unsigned char                                      bIsZiplining : 1;
			};

			auto Pawn = Params->otherActor;
			Helper::GetAnimInstance(Pawn)->Member<FFortAnimInput_Zipline>("ZiplineInput")->bIsZiplining = false;
			bool bJumped = true;
			Pawn->ProcessEvent("EndZiplining", &bJumped);

			struct FZiplinePawnState
			{
				UObject* Zipline;           // AFortAthenaZipline                                        // 0x0000(0x0008) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
				bool                                               bIsZiplining;                                             // 0x0008(0x0001) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
				bool                                               bJumped;                                                  // 0x0009(0x0001) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
				unsigned char                                      UnknownData00[0x2];                                       // 0x000A(0x0002) MISSED OFFSET
				int                                                AuthoritativeValue;                                       // 0x000C(0x0004) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
				FVector                                     SocketOffset;                                             // 0x0010(0x000C) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
				float                                              TimeZipliningBegan;                                       // 0x001C(0x0004) (ZeroConstructor, IsPlainOldData, RepSkip, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
				float                                              TimeZipliningEndedFromJump;                               // 0x0020(0x0004) (ZeroConstructor, IsPlainOldData, RepSkip, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
				unsigned char                                      UnknownData01[0x4];                                       // 0x0024(0x0004) MISSED OFFSET
			};

			auto ZiplineState = Pawn->Member<FZiplinePawnState>("ZiplineState");
			ZiplineState->bIsZiplining = false;
			ZiplineState->SocketOffset = FVector();
			ZiplineState->Zipline = nullptr;
			ZiplineState->bJumped = true;
			ZiplineState->AuthoritativeValue = 0;
			*Actor->Member<UObject*>("PlayerPawn") = nullptr;

			Pawn->ProcessEvent("OnRep_ZiplineState");

			/* struct
			{
			public:
				UObject* OverlappedComponent;         // UPrimitiveComponent                      // 0x0(0x8)(BlueprintVisible, BlueprintReadOnly, Parm, ZeroConstructor, InstancedReference, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
				UObject* OtherActor;      // AActor                                   // 0x8(0x8)(BlueprintVisible, BlueprintReadOnly, Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
				UObject* OtherComp; // UPrimitiveComponent // 0x10(0x8)(BlueprintVisible, BlueprintReadOnly, Parm, ZeroConstructor, InstancedReference, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
				int32_t                                       OtherBodyIndex;                                    // 0x18(0x4)(BlueprintVisible, BlueprintReadOnly, Parm, ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash)
			} ABP_Athena_Environmental_ZipLine_C_HandleOnEndOverlap_Params{ *Actor->Member<UObject*>("RootComponent"), Params->otherActor, nullptr, 0};

			static auto HandleOnEndOverlap = Actor->Function("HandleOnEndOverlap");

			if (HandleOnEndOverlap)
				Actor->ProcessEvent(HandleOnEndOverlap, &ABP_Athena_Environmental_ZipLine_C_HandleOnEndOverlap_Params); */
		}
	}

	return false;
}

inline bool PlayButtonHook(UObject* Object, UFunction* Function, void* Parameters)
{
	LoadInMatch();
	return false;
}

inline bool ClientWasKickedHook(UObject* Controller, UFunction*, void* Params)
{
	std::cout << "ClientWasKicked!\n";

	if (FnVerDouble >= 16.00)
	{
		return true;
	}

	return false;
}

inline bool IsResurrectionChipAvailableHook(UObject* PlayerState, UFunction* Function, void* Parameters)
{
	struct IRCAParams { bool ret; };

	auto Params = (IRCAParams*)Parameters;

	if (Params)
	{
		Params->ret = true;
	}

	return true;
}

inline bool ServerClientPawnLoadedHook(UObject* Controller, UFunction* Function, void* Parameters)
{
	struct SCPLParams { bool bIsPawnLoaded; };
	auto Params = (SCPLParams*)Parameters;

	if (Parameters && Controller)
	{
		std::cout << ("bIsPawnLoaded: ") << Params->bIsPawnLoaded << '\n';

		auto Pawn = *Controller->Member<UObject*>(("Pawn"));

		if (Pawn)
		{
			auto bLoadingScreenDropped = *Controller->Member<bool>(("bLoadingScreenDropped"));
			if (bLoadingScreenDropped && !Params->bIsPawnLoaded)
			{
			}
			else
				std::cout << ("Loading screen is not dropped!\n");
		}
		else
			std::cout << ("Pawn is not valid!\n");
	}

	return false;
}

inline bool pawnClassHook(UObject* GameModeBase, UFunction*, void* Parameters)
{
	struct parms{ UObject* InController; UObject* Class; };
	std::cout << "called!\n";

	static auto PawnClass = FindObject(("BlueprintGeneratedClass /Game/Athena/PlayerPawn_Athena.PlayerPawn_Athena_C"));

	auto Params = (parms*)Parameters;
	Params->Class = PawnClass;

	return false;
}

inline bool AircraftExitedDropZoneHook(UObject* GameMode, UFunction* Function, void* Parameters)
{
	/*
	
	Function called: Function /Script/Engine.GameModeBase.FindPlayerStart
	Function called: Function /Script/Engine.GameModeBase.ChoosePlayerStart
	Function called: Function /Script/Engine.GameModeBase.MustSpectate
	Function called: Function /Script/Engine.GameModeBase.GetDefaultPawnClassForController
	Function called: Function /Script/Engine.GameModeBase.SpawnDefaultPawnFor
	Function called: Function /Script/Engine.GameModeBase.GetDefaultPawnClassForController
	Function called: Function /Script/Engine.Pawn.ReceiveUnpossessed
	Function called: Function /Script/Engine.Actor.ReceiveEndPlay
	Function called: Function /Script/Engine.Actor.ReceiveDestroyed

	*/

	return true;
}

inline bool ServerChoosePartHook(UObject* Pawn, UFunction* Function, void* Parameters)
{
	struct SCP_Params {
		TEnumAsByte<EFortCustomPartType> Part;
		UObject* ChosenCharacterPart;
	};

	auto Params = (SCP_Params*)Parameters;

	if (Params && (!Params->ChosenCharacterPart && Params->Part.Get() != EFortCustomPartType::Backpack))
		return true;

	return false;
}

inline bool OnDeathServerHook(UObject* BuildingActor, UFunction* Function, void* Parameters) // credits: Pro100kat
{
	if (BuildingActor && bStarted && !bRestarting)
	{
		if (bDoubleBuildFix)
		{
			static auto BuildingSMActorClass = FindObject(("Class /Script/FortniteGame.BuildingSMActor"));
			if (ExistingBuildings.size() > 0 && BuildingActor->IsA(BuildingSMActorClass))
			{
				for (int i = 0; i < ExistingBuildings.size(); i++)
				{
					auto Building = ExistingBuildings[i];

					if (!Building)
						continue;

					if (Building == BuildingActor)
					{
						ExistingBuildings.erase(ExistingBuildings.begin() + i);
						break;
					}
				}
			}
		}

		static auto BuildingContainerClass = FindObject(("Class /Script/FortniteGame.BuildingContainer"));

		if (BuildingActor->IsA(BuildingContainerClass) && !readBitfield(BuildingActor, "bAlreadySearched")) // bDoNotDropLootOnDestructionAActor
		{
			LootingV2::HandleSearch(BuildingActor);
		}
	}

	return false;
}

bool ServerUpdateVehicleInputStateReliableHook(UObject* Pawn, UFunction* Function, void* Parameters) // used for tricks like rolling
{
	if (Pawn)
	{
		struct FFortAthenaVehicleInputStateReliable
		{
			unsigned char                                      bIsSprinting : 1;                                         // 0x0000(0x0001) (NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
			unsigned char                                      bIsJumping : 1;                                           // 0x0000(0x0001) (NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
			unsigned char                                      bIsBraking : 1;                                           // 0x0000(0x0001) (NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
			unsigned char                                      bIsHonking : 1;                                           // 0x0000(0x0001) (NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
			unsigned char                                      bIgnoreForwardInAir : 1;                                  // 0x0000(0x0001) (NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
			unsigned char                                      bMovementModifier0 : 1;                                   // 0x0000(0x0001) (NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
			unsigned char                                      bMovementModifier1 : 1;                                   // 0x0000(0x0001) (NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
			unsigned char                                      bMovementModifier2 : 1;                                   // 0x0000(0x0001) (NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
		};

		struct parms { FFortAthenaVehicleInputStateReliable ReliableInput; };

		auto Params = (parms*)Parameters;

		static auto MulticastUpdateVehicleInputStateReliable = Pawn->Function("MulticastUpdateVehicleInputStateReliable");

		struct { FFortAthenaVehicleInputStateReliable ReliableInput; } newParams{Params->ReliableInput};

		if (MulticastUpdateVehicleInputStateReliable)
			Pawn->ProcessEvent(MulticastUpdateVehicleInputStateReliable, &newParams);
		else
			std::cout << "No MulticastUpdateVehicleInputStateReliable!\n";
		
		std::cout << "aa!\n";
	}

	return false;
}

bool ServerUpdateVehicleInputStateUnreliableHook(UObject* Pawn, UFunction* Function, void* Parameters) // used for turning
{
	struct FFortAthenaVehicleInputStateUnreliable
	{
		float                                              ForwardAlpha;                                             // 0x0000(0x0004) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
		float                                              RightAlpha;                                               // 0x0004(0x0004) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
		float                                              PitchAlpha;                                               // 0x0008(0x0004) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
		float                                              LookUpDelta;                                              // 0x000C(0x0004) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
		float                                              TurnDelta;                                                // 0x0010(0x0004) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
		float                                              SteerAlpha;                                               // 0x0014(0x0004) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
		float                                              GravityOffset;                                            // 0x0018(0x0004) (ZeroConstructor, IsPlainOldData, NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
		FVector                      MovementDir;                                              // 0x001C(0x000C) (NoDestructor, NativeAccessSpecifierPublic)
	};

	struct parms { FFortAthenaVehicleInputStateUnreliable InState; };
	auto Params = (parms*)Parameters;

	// static auto PitchAlphaOffset = FindOffsetStruct(("ScriptStruct /Script/FortniteGame.FortAthenaVehicleInputStateUnreliable"), ("PitchAlpha"));
	// auto PitchAlpha = (float*)(__int64(&Params->InState) + PitchAlphaOffset);

	static auto MovementDirOffset = FindOffsetStruct(("ScriptStruct /Script/FortniteGame.FortAthenaVehicleInputStateUnreliable"), ("MovementDir"));
	auto MovementDir = (FVector*)(__int64(&Params->InState) + MovementDirOffset);

	if (Pawn && Params && MovementDir)
	{
		auto Vehicle = Helper::GetVehicle(Pawn);

		if (Vehicle)
		{
			auto Rotation = Helper::GetActorRotation(Vehicle);
			auto Location = Helper::GetActorLocation(Vehicle);

			/* std::cout << ("ForwardAlpha: ") << Params->InState.ForwardAlpha << '\n';
			std::cout << ("RightAlpha: ") << Params->InState.RightAlpha << '\n';
			std::cout << ("PitchAlpha: ") << Params->InState.PitchAlpha << '\n';
			std::cout << ("LookUpDelta: ") << Params->InState.LookUpDelta << '\n';
			std::cout << ("TurnDelta: ") << Params->InState.TurnDelta << '\n';
			std::cout << ("SteerAlpha: ") << Params->InState.SteerAlpha << '\n';
			std::cout << ("GravityOffset: ") << Params->InState.GravityOffset << '\n';
			std::cout << ("MovementDir: ") << MovementDir->Describe() << '\n'; */

			auto newRotation = FRotator{ Rotation.Pitch, Rotation.Yaw, Rotation.Roll };

			// Helper::SetActorLocationAndRotation(Vehicle, Location, newRotation); 
		}
	}

	return false;
}

bool Server_SpawnProjectileHook(UObject* something, UFunction* Function, void* Parameters)
{
	if (something)
	{
		/* struct parm { FVector Loc; FRotator Direction; };
		auto Params = (parm*)Parameters;
		static auto ProjClass = FindObject("BlueprintGeneratedClass /Game/Athena/Items/Consumables/TowerGrenade/B_Prj_Athena_TowerGrenade.B_Prj_Athena_TowerGrenade_C");
		UObject* Proj = Easy::SpawnActor(ProjClass, Params->Loc, FRotator());// *something->Member<UObject*>("ProjectileReference"); // i think im stupid

		if (Proj)
		{
			std::cout << "Proj name: " << Proj->GetFullName() << '\n';

			// ClearAndBuild is useless
			// so is CreateBaseSection

			struct
			{
				FVector                                     ReferenceLocation;                                        // (BlueprintVisible, BlueprintReadOnly, Parm, IsPlainOldData)
			} AB_Prj_Athena_TowerGrenade_C_SpawnTires_Params{ Params->Loc };

			static auto SpawnTires = Proj->Function("SpawnTires");

			if (SpawnTires)
				Proj->ProcessEvent(SpawnTires, &AB_Prj_Athena_TowerGrenade_C_SpawnTires_Params);
			else
				std::cout << "No SpawnTires\n";

		}
		else
			std::cout << "No Proj!\n"; */

		/*
		
		static void SpawnActorsInPlayset(class AActor* WorldContextObject, class UFortPlaysetItemDefinition* Playset);
		struct FName GetPlaysetName();
		static struct FVector AdjustToFinalLocation(class UObject* WorldContextObject, class UFortPlaysetItemDefinition* Playset, struct FVector BaseLocation, struct FRotator Rotation)

		*/
	}

	return false;
}

bool UpdateControlStateHook(UObject* DoghouseVehicle, UFunction*, void* Parameters)
{
	struct FReplicatedControlState
	{
		FVector                   Up;                                                       // 0x0000(0x000C) (NoDestructor, NativeAccessSpecifierPublic)
		FVector                   Forward;                                                  // 0x000C(0x000C) (NoDestructor, NativeAccessSpecifierPublic)
		unsigned char                                      bIsEngineOn : 1;                                          // 0x0018(0x0001) (NoDestructor, HasGetValueTypeHash, NativeAccessSpecifierPublic)
		unsigned char                                      UnknownData00[0x3];                                       // 0x0019(0x0003) MISSED OFFSET
	};

	if (DoghouseVehicle && Parameters)
	{
		auto Params = *(FReplicatedControlState*)Parameters;

		std::cout << "Up: " << Params.Up.Describe() << '\n';
		std::cout << "Forward: " << Params.Forward.Describe() << '\n';

		auto oldRot = Helper::GetActorRotation(DoghouseVehicle);
		auto newRot = oldRot;
		Helper::SetActorLocationAndRotation(DoghouseVehicle, Helper::GetActorLocation(DoghouseVehicle), newRot);
	}

	return false;
}

bool balloonFunHook(UObject* ability, UFunction* Function, void* Parameters)
{
	if (ability)
	{
		std::cout << "balloon!\n";
		// ability->ProcessEvent("SpawnBalloon");
		auto Pawn = nullptr; // *ability->Member<UObject*>("PlayerPawn");
		std::cout << "Pawn: " << Pawn << '\n';
		ability->Member<TArray<UObject*>>("Balloons")->Add(Easy::SpawnActor(FindObject("Class /Script/FortniteGame.BuildingGameplayActorBalloon"), Helper::GetActorLocation(Pawn)));
	}

	return false;
}

bool boomboxHook(UObject* ability, UFunction* Function, void* Parameters)
{
	if (ability)
	{
		struct parm { FVector Loc; FRotator Direction; };
		auto Params = (parm*)Parameters;

		static auto ProjClass = FindObject("BlueprintGeneratedClass /Game/Athena/Items/Consumables/BoomBox/B_Proj_BoomBox.B_Proj_BoomBox_C");
		UObject* Proj = Easy::SpawnActor(ProjClass, Params->Loc, FRotator());// *something->Member<UObject*>("ProjectileReference"); // i think im stupid

		if (Proj)
		{
			std::cout << "Proj name: " << Proj->GetFullName() << '\n';
			static auto actualBoomBoxClass = FindObject("BlueprintGeneratedClass /Game/Athena/Items/Consumables/BoomBox/BGA_Athena_BoomBox.BGA_Athena_BoomBox_C");
			auto newBoomBox = Easy::SpawnActor(actualBoomBoxClass, Helper::GetActorLocation(Proj));
			*Proj->Member<UObject*>("SpawnedBGA") = newBoomBox;

			UObject* Pawn; // Helper::GetOwner(ability);
			ability->ProcessEvent("GetActivatingPawn", &Pawn);

			std::cout << "Pawn: " << Pawn << '\n';

			std::cout << "Pawn Name: " << Pawn->GetFullName() << '\n';

			Helper::InitializeBuildingActor(*Pawn->Member<UObject*>("Controller"), newBoomBox);
		}
	}

	return false;
}

bool throwableConsumablesHook(UObject* ability, UFunction*, void* Parameters)
{
	if (ability)
	{
		// ability->Server_SpawnProjectile
		// ability->ThrowConsumable
		
		auto memberNames = GetMemberNames(ability, true, false);

		for (auto& MemberName : memberNames)
			std::cout << "memberName: " << MemberName << '\n';

		std::cout << "throwable!\n";
	}

	return false;
}

bool riftItemHook(UObject* ability, UFunction* Function, void* Parameters)
{
	std::cout << "rift!\n";

	if (ability)
	{
		UObject* Pawn; // Helper::GetOwner(ability);
		ability->ProcessEvent("GetActivatingPawn", &Pawn);
		Helper::TeleportToSkyDive(Pawn, 11000);

		__int64* SkydiveAbilitySpec = nullptr;

		auto FindSkydiveAbility = [&SkydiveAbilitySpec](__int64* Spec) -> void {
			auto Ability = *GetAbilityFromSpec(Spec);
			if (Ability && Ability->GetFullName().contains("GA_Rift_Athena_Skydive_C"))
			{
				SkydiveAbilitySpec = Spec;
			}
		};

		auto ASC = *Pawn->CachedMember<UObject*>("AbilitySystemComponent");
		LoopSpecs(ASC, FindSkydiveAbility);

		if (SkydiveAbilitySpec)
		{
			std::cout << "foujd spec!\n";
			// std::cout << "found skydive ability: " << SkydiveAbilitySpec->GetFullName() << '\n';
			auto SkyDiveAbility = *GetAbilityFromSpec(SkydiveAbilitySpec);
			*SkyDiveAbility->Member<UObject*>("PlayerPawn") = Pawn;
			SkyDiveAbility->ProcessEvent("SetPlayerToSkydive");
			SkyDiveAbility->ProcessEvent("K2_ActivateAbility"); // does nothing
		}
		else
			std::cout << "failed to fmind skydive ability!\n";
	}

	return false;
}

__int64 (__fastcall* FailedToSpawnPawnO)(__int64 a1);

__int64 __fastcall FailedToSpawnPawnDetour(__int64 a1)
{
	std::cout << "FAiled to spawn a pawn: " << __int64(_ReturnAddress()) - (uintptr_t)GetModuleHandleW(0) << '\n';
	return FailedToSpawnPawnO(a1);
}

bool onpawnmotnageblendingoutHook(UObject* weapon, UFunction*, void*)
{
	if (weapon)
	{
		auto pawn = Helper::GetOwner(weapon);

		auto Controller = *pawn->Member<UObject*>(("Controller"));
		auto AmmoCount = weapon->Member<int>("AmmoCount");
		auto oldAmmoCount = *AmmoCount;
		auto entry = Inventory::GetEntryFromWeapon(Controller, weapon);
		(*FFortItemEntry::GetLoadedAmmo(entry)) -= 1; // = *AmmoCount;
		Inventory::Update(Controller, -1, true, (FFastArraySerializerItem*)entry);

		static auto OnRep_AmmoCount = weapon->Function("OnRep_AmmoCount");

		if (OnRep_AmmoCount)
			weapon->ProcessEvent(OnRep_AmmoCount, &oldAmmoCount);

		std::cout << "called!\n";
	}

	return false;
}

char (__fastcall* sutpidfucnO)(FFastArraySerializerSE::TFastArraySerializeHelper* a1, __int64 a2, int a3);

char __fastcall sutpidfucnDetour(FFastArraySerializerSE::TFastArraySerializeHelper* a1, __int64 a2, int a3)
{
	// oldmap size does not match blah blah
	std::cout << "faastararai!\n";

	std::cout << "AHH: " <<  a1->Struct->GetName() << '\n';// GetOwnerStruct()->GetName();
	
	// a1->ArraySerializer.MarkArrayDirty();

	return sutpidfucnO(a1, a2, a3);
}

bool RiftPortalActivateAbility(UObject* Ability, UObject* Func, void* Parameters)
{
	auto pawn = *Ability->Member("PlayerPawn");

	if (pawn)
	{
		static auto TeleportToSkyDiveFn = pawn->Function(("TeleportToSkyDive"));

		float heihr5 = 10000;

		if (TeleportToSkyDiveFn)
			pawn->ProcessEvent(TeleportToSkyDiveFn, &heihr5);
	}

	return false;
}

bool PlayerCanRestartHook(UObject* GameMode, UFunction*, void* Parameters)
{
	std::cout << "eehhg!~\n";
	return true;
}

bool MustSpectateHook(UObject* GameMode, UFunction*, void* Parameters)
{
	std::cout << "aefiu132q8f!\n";
	return false;
}

bool IsProjectileBeingKilledHook(UObject* Interface, UFunction* func, void* Parameters)
{
	ProcessEventO(Interface, func, Parameters);

	auto isBeingKilled = *(bool*)(Parameters);

	std::cout << "isBeingKilled: " << isBeingKilled << '\n';

	static auto GetInstigatorPlayerController = Interface->Function("GetInstigatorPlayerController");

	UObject* Controller = nullptr;

	if (GetInstigatorPlayerController)
		Interface->ProcessEvent(GetInstigatorPlayerController, &Controller);

	static auto GetFortProjectileMovementComponent = Interface->Function("GetFortProjectileMovementComponent");

	UObject* ProjectileMovementComponent = nullptr;

	if (GetFortProjectileMovementComponent)
		Interface->ProcessEvent(GetFortProjectileMovementComponent, &ProjectileMovementComponent);

	static auto GetCurrentLocation = Interface->Function("GetCurrentLocation");

	FVector CurrentLocation;

	if (GetCurrentLocation)
		Interface->ProcessEvent(GetCurrentLocation, &CurrentLocation);

	if (Controller)
	{
		auto Pawn = Helper::GetPawnFromController(Controller);
		Helper::SummonPickup(Pawn, *Helper::GetCurrentWeapon(Pawn)->Member<UObject*>("WeaponData"), CurrentLocation, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset);
	}

	return true;
}

void FinishInitializeUHooks()
{
	if (Engine_Version < 422)
		AddHook(("BndEvt__BP_PlayButton_K2Node_ComponentBoundEvent_1_CommonButtonClicked__DelegateSignature"), PlayButtonHook);

	if (Engine_Version >= 424)
		AddHook("Function /Script/FortniteGame.FortProjectileMovementInterface.IsProjectileBeingKilled", IsProjectileBeingKilledHook);

	AddHook("Function /Script/Engine.GameModeBase.MustSpectate", MustSpectateHook);
	AddHook("Function /Script/Engine.GameModeBase.PlayerCanRestart", PlayerCanRestartHook);
	// AddHook("Function /Script/FortniteGame.FortWeapon.OnPawnMontageBlendingOut", onpawnmotnageblendingoutHook);
	AddHook("Function /Game/Athena/SafeZone/SafeZoneIndicator.SafeZoneIndicator_C.OnSafeZoneStateChange", OnSafeZoneStateChangeHook);
	AddHook(("Function /Script/FortniteGame.BuildingActor.OnDeathServer"), OnDeathServerHook);
	AddHook(("Function /Script/Engine.GameMode.ReadyToStartMatch"), ReadyToStartMatchHook);
	AddHook("Function /Script/Engine.GameModeBase.GetDefaultPawnClassForController", pawnClassHook);
	// AddHook("Function /Script/Engine.Actor.ReceiveActorEndOverlap", ReceiveActorEndOverlapHook);

	// AddHook("Function /Script/FortniteGame.FortPlayerPawn.ServerUpdateVehicleInputStateReliable", ServerUpdateVehicleInputStateReliableHook);

	if (Engine_Version > 424)
	{
		// AddHook(("Function /Game/Athena/Items/Consumables/Parents/GA_Athena_Consumable_ThrowWithTrajectory_Parent.GA_Athena_Consumable_ThrowWithTrajectory_Parent_C.Server_SpawnProjectile"), throwableConsumablesHook); // wrong func
	}
	
	// AddHook("Function /Game/Athena/Items/Consumables/Grenade/GA_Athena_Grenade_WithTrajectory.GA_Athena_Grenade_WithTrajectory_C.Server_SpawnProjectile", boomboxHook);
	// AddHook("Function /Game/Athena/Items/Consumables/TowerGrenade/GA_Athena_TowerGrenadeWithTrajectory.GA_Athena_TowerGrenadeWithTrajectory_C.Server_SpawnProjectile", Server_SpawnProjectileHook);
	// AddHook("Function /Game/Athena/Items/Consumables/Balloons/GA_Athena_Balloons_Consumable_Passive.GA_Athena_Balloons_Consumable_Passive_C.K2_ActivateAbility", balloonFunHook);

	if (FnVerDouble < 9) // idk if right
		AddHook(("Function /Script/FortniteGame.FortPlayerControllerAthena.ServerAttemptAircraftJump"), ServerAttemptAircraftJumpHook);
	else if (Engine_Version < 424)
		AddHook(("Function /Script/FortniteGame.FortPlayerController.ServerAttemptAircraftJump"), ServerAttemptAircraftJumpHook);
	else
		AddHook(("Function /Script/FortniteGame.FortControllerComponent_Aircraft.ServerAttemptAircraftJump"), ServerAttemptAircraftJumpHook);

	// AddHook(("Function /Script/FortniteGame.FortPlayerController.ServerCheat"), ServerCheatHook); // Commands Hook
	// AddHook(("Function /Script/FortniteGame.FortPlayerController.ServerClientPawnLoaded"), ServerClientPawnLoadedHook);

	AddHook(("Function /Script/FortniteGame.FortPlayerControllerZone.ClientOnPawnDied"), ClientOnPawnDiedHook);
	// AddHook("Function /Script/FortniteGame.FortAthenaDoghouseVehicle.ServerUpdateControlState", UpdateControlStateHook);
	// AddHook(("Function /Script/FortniteGame.FortPlayerPawn.ServerSendZiplineState"), ServerSendZiplineStateHook);
	AddHook("Function /Script/Engine.PlayerController.ClientWasKicked", ClientWasKickedHook);
	// AddHook(("Function /Script/FortniteGame.FortPlayerPawn.ServerUpdateVehicleInputStateUnreliable"), ServerUpdateVehicleInputStateUnreliableHook);

	if (Engine_Version >= 420 && Engine_Version < 423)
	{
		AddHook(("Function /Script/FortniteGame.FortAthenaVehicle.ServerUpdatePhysicsParams"), ServerUpdatePhysicsParamsHook);
	}

	// if (PlayMontage)
	// AddHook(("Function /Script/FortniteGame.FortPlayerController.ServerPlayEmoteItem"), ServerPlayEmoteItemHook);

	if (Engine_Version < 423)
	{ // ??? Idk why we need the brackets
		AddHook(("Function /Script/FortniteGame.FortPlayerController.ServerAttemptInteract"), ServerAttemptInteractHook);
	}
	else
		AddHook(("Function /Script/FortniteGame.FortControllerComponent_Interaction.ServerAttemptInteract"), ServerAttemptInteractHook);

	AddHook(("Function /Script/FortniteGame.FortPlayerControllerZone.ServerAttemptExitVehicle"), ServerAttemptExitVehicleHook);

	AddHook(("Function /Script/FortniteGame.FortPlayerPawn.ServerChoosePart"), ServerChoosePartHook);
	AddHook("Function /Script/FortniteGame.FortPlayerPawn.ServerReviveFromDBNO", ServerReviveFromDBNOHook);

	for (auto& Func : FunctionsToHook)
	{
		if (!Func.first)
			std::cout << ("Detected null UFunction!\n");
	}

	std::cout << std::format("Hooked {} UFunctions!\n", std::to_string(FunctionsToHook.size()));
}

void* ProcessEventDetour(UObject* Object, UFunction* Function, void* Parameters)
{
	if (Object && Function)
	{
		if (bStarted && bListening && (bLogRpcs || bLogProcessEvent))
		{
			auto FunctionName = Function->GetFullName();
			// if (Function->FunctionFlags & 0x00200000 || Function->FunctionFlags & 0x01000000) // && FunctionName.find("Ack") == -1 && FunctionName.find("AdjustPos") == -1))

			if (bLogRpcs && (FunctionName.starts_with(("Server")) || FunctionName.starts_with(("Client")) || FunctionName.starts_with(("OnRep_"))))
			{
				if (!FunctionName.contains("ServerUpdateCamera") && !FunctionName.contains("ServerMove")
					&& !FunctionName.contains(("ServerUpdateLevelVisibility"))
					&& !FunctionName.contains(("AckGoodMove")))
				{
					std::cout << ("RPC Called: ") << FunctionName << '\n';
				}
			}

			if (bLogProcessEvent)
			{
				if (!strstr(FunctionName.c_str(), ("EvaluateGraphExposedInputs")) &&
					!strstr(FunctionName.c_str(), ("Tick")) &&
					!strstr(FunctionName.c_str(), ("OnSubmixEnvelope")) &&
					!strstr(FunctionName.c_str(), ("OnSubmixSpectralAnalysis")) &&
					!strstr(FunctionName.c_str(), ("OnMouse")) &&
					!strstr(FunctionName.c_str(), ("Pulse")) &&
					!strstr(FunctionName.c_str(), ("BlueprintUpdateAnimation")) &&
					!strstr(FunctionName.c_str(), ("BlueprintPostEvaluateAnimation")) &&
					!strstr(FunctionName.c_str(), ("BlueprintModifyCamera")) &&
					!strstr(FunctionName.c_str(), ("BlueprintModifyPostProcess")) &&
					!strstr(FunctionName.c_str(), ("Loop Animation Curve")) &&
					!strstr(FunctionName.c_str(), ("UpdateTime")) &&
					!strstr(FunctionName.c_str(), ("GetMutatorByClass")) &&
					!strstr(FunctionName.c_str(), ("UpdatePreviousPositionAndVelocity")) &&
					!strstr(FunctionName.c_str(), ("IsCachedIsProjectileWeapon")) &&
					!strstr(FunctionName.c_str(), ("LockOn")) &&
					!strstr(FunctionName.c_str(), ("GetAbilityTargetingLevel")) &&
					!strstr(FunctionName.c_str(), ("ReadyToEndMatch")) &&
					!strstr(FunctionName.c_str(), ("ReceiveDrawHUD")) &&
					!strstr(FunctionName.c_str(), ("OnUpdateDirectionalLightForTimeOfDay")) &&
					!strstr(FunctionName.c_str(), ("GetSubtitleVisibility")) &&
					!strstr(FunctionName.c_str(), ("GetValue")) &&
					!strstr(FunctionName.c_str(), ("InputAxisKeyEvent")) &&
					!strstr(FunctionName.c_str(), ("ServerTouchActiveTime")) &&
					!strstr(FunctionName.c_str(), ("SM_IceCube_Blueprint_C")) &&
					!strstr(FunctionName.c_str(), ("OnHovered")) &&
					!strstr(FunctionName.c_str(), ("OnCurrentTextStyleChanged")) &&
					!strstr(FunctionName.c_str(), ("OnButtonHovered")) &&
					!strstr(FunctionName.c_str(), ("ExecuteUbergraph_ThreatPostProcessManagerAndParticleBlueprint")) &&
					!strstr(FunctionName.c_str(), ("UpdateCamera")) &&
					!strstr(FunctionName.c_str(), ("GetMutatorContext")) &&
					!strstr(FunctionName.c_str(), ("CanJumpInternal")) && 
					!strstr(FunctionName.c_str(), ("OnDayPhaseChanged")) &&
					!strstr(FunctionName.c_str(), ("Chime")) && 
					!strstr(FunctionName.c_str(), ("ServerMove")) &&
					!strstr(FunctionName.c_str(), ("OnVisibilitySetEvent")) &&
					!strstr(FunctionName.c_str(), "ReceiveHit") &&
					!strstr(FunctionName.c_str(), "ReadyToStartMatch") && 
					!strstr(FunctionName.c_str(), "ClientAckGoodMove") &&
					!strstr(FunctionName.c_str(), "Prop_WildWest_WoodenWindmill_01") &&
					!strstr(FunctionName.c_str(), "ContrailCheck") &&
					!strstr(FunctionName.c_str(), "B_StockBattleBus_C") &&
					!strstr(FunctionName.c_str(), "Subtitles.Subtitles_C.") &&
					!strstr(FunctionName.c_str(), "/PinkOatmeal/PinkOatmeal_") &&
					!strstr(FunctionName.c_str(), "BP_SpectatorPawn_C") &&
					!strstr(FunctionName.c_str(), "FastSharedReplication") &&
					!strstr(FunctionName.c_str(), "OnCollisionHitEffects") &&
					!strstr(FunctionName.c_str(), "BndEvt__SkeletalMesh") &&
					!strstr(FunctionName.c_str(), ".FortAnimInstance.AnimNotify_") &&
					!strstr(FunctionName.c_str(), "OnBounceAnimationUpdate") &&
					!strstr(FunctionName.c_str(), "ShouldShowSoundIndicator") &&
					!strstr(FunctionName.c_str(), "Primitive_Structure_AmbAudioComponent_C") &&
					!strstr(FunctionName.c_str(), "PlayStoppedIdleRotationAudio"))
				{
					std::cout << ("Function called: ") << FunctionName << '\n';
				}
			}
		}

		for (auto& Func : FunctionsToHook)
		{
			if (Function == Func.first)
			{
				if (Func.second(Object, Function, Parameters)) // If the function returned true, then cancel default execution.
				{
					// std::cout << "ret true!\n";
					return 0;
				}
			}
		}
	}

	return ProcessEventO(Object, Function, Parameters);
}

__int64 __fastcall FixCrashDetour(int32_t* PossiblyNull, __int64 a2, int* a3)
{
	if (!PossiblyNull)
	{
		std::cout << "Prevented Crash!\n";
		return 0;
	}

	return FixCrash(PossiblyNull, a2, a3);
}

void __fastcall GetPlayerViewPointDetour(UObject* pc, FVector* a2, FRotator* a3)
{
	if (pc)
	{
		static auto fn = FindObject(("Function /Script/Engine.Controller.GetViewTarget"));
		UObject* TheViewTarget = nullptr; // *pc->Member<UObject*>(("Pawn"));
		pc->ProcessEvent(fn, &TheViewTarget);

		if (TheViewTarget)
		{
			if (a2)
			{
				auto Loc = Helper::GetActorLocation(TheViewTarget);
				*a2 = Loc;

				// if (bTraveled)
					// std::cout << std::format("X: {} Y: {} Z {}\n", Loc.X, Loc.Y, Loc.Z);
			}
			if (a3)
			{
				auto Rot = Helper::GetActorRotation(TheViewTarget);
				*a3 = Rot;
			}

			return;
		}
		// else
			// std::cout << ("unable to get viewpoint!\n"); // This will happen if someone leaves iirc
	}

	return GetPlayerViewPoint(pc, a2, a3);
}

__int64(__fastcall* idkbroke)(UObject* a1);

__int64 (__fastcall* ehehheO)(__int64 a1, __int64* a2);

__int64 __fastcall ehehheDetour(__int64 NetViewer, UObject* Connection)
{
	static auto Connection_ViewTargetOffset = GetOffset(Connection, "ViewTarget");
	static auto Connection_PlayerControllerOffset = GetOffset(Connection, "PlayerController");
	static auto Connection_OwningActorOffset = GetOffset(Connection, "OwningActor");

	auto Connection_ViewTarget = *(UObject**)(__int64(Connection) + Connection_ViewTargetOffset);
	auto Connection_PlayerController = *(UObject**)(__int64(Connection) + Connection_PlayerControllerOffset);

	static auto Viewer_ConnectionOffset = FindOffsetStruct("ScriptStruct /Script/Engine.NetViewer", "Connection");
	*(UObject**)(__int64(NetViewer) + Viewer_ConnectionOffset) = Connection;

	static auto Viewer_InViewerOffset = FindOffsetStruct("ScriptStruct /Script/Engine.NetViewer", "InViewer");
	*(UObject**)(__int64(NetViewer) + Viewer_InViewerOffset) = Connection_PlayerController ? Connection_PlayerController : *(UObject**)(__int64(Connection) + Connection_OwningActorOffset);

	static auto Viewer_ViewTargetOffset = FindOffsetStruct("ScriptStruct /Script/Engine.NetViewer", "ViewTarget");
	auto Viewer_ViewTarget = (UObject**)(__int64(NetViewer) + Viewer_ViewTargetOffset);
	*Viewer_ViewTarget = Connection_ViewTarget;

	static auto Viewer_ViewLocationOffset = FindOffsetStruct("ScriptStruct /Script/Engine.NetViewer", "ViewLocation");
	auto Viewer_ViewLocation = (FVector*)(__int64(NetViewer) + Viewer_ViewLocationOffset);

	if (*Viewer_ViewTarget)
		*(FVector*)(__int64(NetViewer) + Viewer_ViewLocationOffset) = Helper::GetActorLocation(*Viewer_ViewTarget);

	float CP, SP, CY, SY;

	FRotator ViewRotation = (*Viewer_ViewTarget) ? Helper::GetActorRotation(*Viewer_ViewTarget) : FRotator();

	SinCos(&SP, &CP, DegreesToRadians(ViewRotation.Pitch));
	SinCos(&SY, &CY, DegreesToRadians(ViewRotation.Yaw));

	static auto Viewer_ViewDirOffset = FindOffsetStruct("ScriptStruct /Script/Engine.NetViewer", "ViewDir");

	*(FVector*)(__int64(NetViewer) + Viewer_ViewDirOffset) = FVector(CP * CY, CP * SY, SP);

	return NetViewer;

	// return ehehheO(a1, a2);
}

void InitializeHooks()
{
	MH_CreateHook((PVOID)ProcessEventAddr, ProcessEventDetour, (void**)&ProcessEventO);
	MH_EnableHook((PVOID)ProcessEventAddr);

	if (Engine_Version >= 423)
	{
		MH_CreateHook((PVOID)FixCrashAddr, FixCrashDetour, (void**)&FixCrash);
		MH_EnableHook((PVOID)FixCrashAddr);
	}

	auto sig = FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 40 48 89 11 48 8B D9 48 8B 42 30 48 85 C0 75 07 48 8B 82 ? ? ? ? 48");

	if (!sig)
		sig = FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 40 48 89 11");

	bool bNewFlashingFix = false;

	if (sig) //Engine_Version == 423)
	{
		// fixes flashing

		// MH_CreateHook((PVOID)GetPlayerViewpointAddr, GetPlayerViewPointDetour, (void**)&GetPlayerViewPoint);
		// MH_EnableHook((PVOID)GetPlayerViewpointAddr);

		MH_CreateHook((PVOID)sig, ehehheDetour, (void**)&ehehheO);
		MH_EnableHook((PVOID)sig);
	}
	else
		std::cout << ("[WARNING] Could not fix flashing!\n");

	if (LP_SpawnPlayActorAddr && false) // bad time but eh
	{
		MH_CreateHook((PVOID)LP_SpawnPlayActorAddr, LP_SpawnPlayActorDetour, (void**)&LP_SpawnPlayActor);
		MH_EnableHook((PVOID)LP_SpawnPlayActorAddr);
	}
}
