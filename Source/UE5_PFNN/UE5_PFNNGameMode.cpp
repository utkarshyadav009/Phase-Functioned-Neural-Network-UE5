// Copyright Epic Games, Inc. All Rights Reserved.

#include "UE5_PFNNGameMode.h"
#include "UE5_PFNNCharacter.h"
#include "UObject/ConstructorHelpers.h"

AUE5_PFNNGameMode::AUE5_PFNNGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/PFNN_BP/PFNN_Character/PFNNCharacter_BP"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}


//"/Game/PFNN_BP/PFNN_Character/PFNNCharacter_BP"
//"/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"