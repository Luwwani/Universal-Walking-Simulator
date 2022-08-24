#pragma once
#include <UE/structs.h>
#include <Gameplay/helper.h>
#include <Gameplay/abilities.h>

namespace Carmine {
	static bool SpanwedCarmine = false;
	void HandleCarmine(UObject* Controller) {
		if (!Controller || FnVerDouble >= 11.0)
			return;

		UObject* Pawn = Helper::GetPawnFromController(Controller);
		Helper::ChoosePart(Pawn, EFortCustomPartType::Body, FindObject("CustomCharacterPart /Game/Athena/Heroes/Meshes/Bodies/Dev_TestAsset_Body_M_XL.Dev_TestAsset_Body_M_XL"));
		Helper::ChoosePart(Pawn, EFortCustomPartType::Head, FindObject("CustomCharacterPart /Game/Athena/Heroes/Meshes/Heads/Dev_TestAsset_Head_M_XL.Dev_TestAsset_Head_M_XL"));
		*Pawn->Member<UObject*>("AnimBPOverride") = FindObject("AnimBlueprintGeneratedClass /Game/Characters/Player/Male/Male_Avg_Base/Gauntlet_Player_AnimBlueprint.Gauntlet_Player_AnimBlueprint_C", true);

		UObject* AS = FindObject("FortAbilitySet /Game/Athena/Items/Gameplay/BackPacks/CarminePack/AS_CarminePack.AS_CarminePack");
		auto GrantedAbilities = AS->Member<TArray<UObject*>>("GameplayAbilities");

		for (int i = 0; i < GrantedAbilities->Num(); i++) {
			GrantGameplayAbility(Pawn, GrantedAbilities->At(i));
		}

		//ApplyGameplayEffect(*Pawn->Member<UObject*>("AbilitySystemComponent"), FindObject("/Game/Athena/Items/Gameplay/BackPacks/CarminePack/GE_Carmine_FallDamageImmune.GE_Carmine_FallDamageImmune_C"));

		//UObject* Montage = FindObject("AnimMontage /Game/Animation/Game/MainPlayer/Skydive/Freefall/Custom/Jim/Transitions/Spawn_Montage.Spawn_Montage");
	}

	inline bool OnProjectileStop_Hook(UObject* Object, UFunction* Func, void* Params) {
		auto LocOffset = FindOffsetStruct("ScriptStruct /Script/Engine.HitResult", "Location");
		FVector* Loc = reinterpret_cast<FVector*>(__int64(Params) + LocOffset);
		if (Object->GetName().contains("AthenaSupplyDrop_Meteor_Gauntlet") && SpanwedCarmine == false) {
			Helper::SummonPickup(nullptr, FindObject("AthenaGadgetItemDefinition /Game/Athena/Items/Gameplay/BackPacks/CarminePack/AGID_CarminePack.AGID_CarminePack"), *Loc, EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource::Unset);
			SpanwedCarmine = true;
		}
		return true;
	}

	inline bool ReceiveBeginPlay_Hook(UObject* Object, UFunction* Func, void* Params) {
		if (SpanwedCarmine == true) {
			Helper::DestroyActor(Object);
			return false;
		}
		return true;
	}

	void InitCarmine() {
		AddHook("Function /Game/Athena/SupplyDrops/AthenaSupplyDrop.AthenaSupplyDrop_C.BndEvt__ProjectileMovement_K2Node_ComponentBoundEvent_0_OnProjectileStopDelegate__DelegateSignature", OnProjectileStop_Hook);
		AddHook("Function /Game/Athena/Playlists/Carmine/MeteorDrop/AthenaSupplyDrop_Meteor_Gauntlet.AthenaSupplyDrop_Meteor_Gauntlet_C.ReceiveBeginPlay", ReceiveBeginPlay_Hook);
	}

	void SpawnGauntlet() {
		UObject* Class = FindObject("/Game/Athena/Playlists/Carmine/MeteorDrop/AthenaSupplyDrop_Meteor_Gauntlet.AthenaSupplyDrop_Meteor_Gauntlet_C", true);
		FVector RandLocation;
		std::random_device rd; // obtain a random number from hardware
		std::mt19937 gen(rd()); // seed the generator

		std::uniform_int_distribution<> Xdistr(-40000, 128000);
		std::uniform_int_distribution<> Ydistr(-90000, 70000);
		std::uniform_int_distribution<> Zdistr(-1000, 20000); // doesnt matter

		RandLocation.X = Xdistr(gen);
		RandLocation.Y = Ydistr(gen);
		RandLocation.Z = Zdistr(gen);

		RandLocation = FVector{ 1250, 1818, 6000 };

		Easy::SpawnActor(Class, RandLocation, {});
	}
}

namespace Ashton {
	inline bool OnProjectileStop_Hook(UObject* Object, UFunction* Func, void* Params) {
		auto LocOffset = FindOffsetStruct("ScriptStruct /Script/Engine.HitResult", "Location");
		FVector* Loc = reinterpret_cast<FVector*>(__int64(Params) + LocOffset);
		UObject** ItemToSpawn = Object->Member<UObject*>("ItemDefToSpawn");
		if (ItemToSpawn && *ItemToSpawn) {
			std::cout << "\nGauntlet Spawned!\n";
			Helper::SummonPickup(nullptr, *ItemToSpawn, *Loc, EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource::Unset);
		}
		return true;
	}

	void InitAshton() {
		AddHook("Function /Game/Athena/SupplyDrops/AthenaSupplyDrop.AthenaSupplyDrop_C.BndEvt__ProjectileMovement_K2Node_ComponentBoundEvent_0_OnProjectileStopDelegate__DelegateSignature", OnProjectileStop_Hook);
	}

	void SpawnRandomStone() {
		//(TODO) Spawn Randomly on the Map
		UObject* Class = FindObject("/Game/Athena/Playlists/Ashton/Rocks/AthenaSupplyDrop_Rock_P.AthenaSupplyDrop_Rock_P_C", true);
		FVector RandLocation;
		std::random_device rd; // obtain a random number from hardware
		std::mt19937 gen(rd()); // seed the generator

		// CHAPTER 1

		std::uniform_int_distribution<> Xdistr(-40000, 128000);
		std::uniform_int_distribution<> Ydistr(-90000, 70000);
		std::uniform_int_distribution<> Zdistr(-1000, 20000); // doesnt matter

		RandLocation.X = Xdistr(gen);
		RandLocation.Y = Ydistr(gen);
		RandLocation.Z = Zdistr(gen);

		//RandLocation = FVector{ 1250, 1818, 6000 };

		Easy::SpawnActor(Class, RandLocation, {});
	}
}