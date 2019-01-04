// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "TestProjectGameMode.h"
#include "MyHUD.h"
#include "TestProjectPawn.h"

ATestProjectGameMode::ATestProjectGameMode()
{
	// set default pawn class to our flying pawn
	DefaultPawnClass = ATestProjectPawn::StaticClass();

	HUDClass = AMyHUD::StaticClass();
}
