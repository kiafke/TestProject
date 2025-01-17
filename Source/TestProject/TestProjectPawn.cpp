// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "TestProjectPawn.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "DrawDebugHelpers.h"
#include "Components/TimelineComponent.h"
#include "Components/BoxComponent.h"
#include "DamagedInterface.h"

ATestProjectPawn::ATestProjectPawn()
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UStaticMesh> PlaneMesh;
		FConstructorStatics()
			: PlaneMesh(TEXT("/Game/Flying/Meshes/UFO.UFO"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;
	
	// Create static mesh component
	PlaneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaneMesh0"));
	PlaneMesh->SetStaticMesh(ConstructorStatics.PlaneMesh.Get());	// Set static mesh
	RootComponent = PlaneMesh;

	// Create a spring arm component
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm0"));
	SpringArm->SetupAttachment(RootComponent);	// Attach SpringArm to RootComponent
	SpringArm->TargetArmLength = 160.0f; // The camera follows at this distance behind the character	
	SpringArm->SocketOffset = FVector(0.f,0.f,60.f);
	SpringArm->bEnableCameraLag = true;	// Do not allow camera to lag
	SpringArm->CameraLagSpeed = 15.f;

	// Create camera component 
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera0"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);	// Attach the camera
	Camera->bUsePawnControlRotation = false; // Don't rotate camera with controller

	// Set handling parameters
	Acceleration = 5000.f;
	TurnSpeed = 100.f;
	MaxSpeed = 5000.f;
	MinSpeed = 2000.f;
	RotateSpeed = 5.f;
	CurrentForwardSpeed = 2000.f;
}

void ATestProjectPawn::Tick(float DeltaSeconds)
{
	const FVector LocalMove = FVector(CurrentForwardSpeed * DeltaSeconds, 0.f, 0.f);

	// Move plan forwards (with sweep so we stop when we collide with things)
	AddActorLocalOffset(LocalMove, true);

	// Calculate change in rotation this frame
	FRotator DeltaRotation(0,0,0);
	DeltaRotation.Pitch = CurrentPitchSpeed * DeltaSeconds;
	DeltaRotation.Yaw = CurrentYawSpeed * DeltaSeconds;
	DeltaRotation.Roll = CurrentRollSpeed * DeltaSeconds;

	// Rotate plane
	AddActorLocalRotation(DeltaRotation);

	


	

	//
	// Call any parent class Tick implementation
	Super::Tick(DeltaSeconds);
}

void ATestProjectPawn::NotifyHit(class UPrimitiveComponent* MyComp, class AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	// Deflect along the surface when we collide.
	FRotator CurrentRotation = GetActorRotation();
	SetActorRotation(FQuat::Slerp(CurrentRotation.Quaternion(), HitNormal.ToOrientationQuat(), 0.025f));
}


void ATestProjectPawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
    // Check if PlayerInputComponent is valid (not NULL)
	check(PlayerInputComponent);

	// Bind our control axis' to callback functions
	PlayerInputComponent->BindAxis("Thrust", this, &ATestProjectPawn::ThrustInput);
	PlayerInputComponent->BindAxis("MoveUp", this, &ATestProjectPawn::MoveUpInput);
	PlayerInputComponent->BindAxis("MoveRight", this, &ATestProjectPawn::MoveRightInput);
	



	// Set up gameplay key bindings

	PlayerInputComponent->BindAction("Fire", IE_Pressed,this, &ATestProjectPawn::Fire);
	PlayerInputComponent->BindAction("RotateLeft", IE_Repeat, this, &ATestProjectPawn::RotateLeft);
}

void ATestProjectPawn::ThrustInput(float Val)
{
	// Is there any input?
	bool bHasInput = !FMath::IsNearlyEqual(Val, 0.f);
	// If input is not held down, reduce speed
	float CurrentAcc = bHasInput ? (Val * Acceleration) : (-0.5f * Acceleration);
	// Calculate new speed
	float NewForwardSpeed = CurrentForwardSpeed + (GetWorld()->GetDeltaSeconds() * CurrentAcc);
	// Clamp between MinSpeed and MaxSpeed
	CurrentForwardSpeed = FMath::Clamp(NewForwardSpeed, MinSpeed, MaxSpeed);
}

void ATestProjectPawn::MoveUpInput(float Val)
{
	// Target pitch speed is based in input
	float TargetPitchSpeed = (Val * TurnSpeed * -1.f);

	// When steering, we decrease pitch slightly
	TargetPitchSpeed += (FMath::Abs(CurrentYawSpeed) * -0.2f);

	// Smoothly interpolate to target pitch speed
	CurrentPitchSpeed = FMath::FInterpTo(CurrentPitchSpeed, TargetPitchSpeed, GetWorld()->GetDeltaSeconds(), 2.f);
}

void ATestProjectPawn::MoveRightInput(float Val)
{
	// Target yaw speed is based on input
	float TargetYawSpeed = (Val * TurnSpeed);

	// Smoothly interpolate to target yaw speed
	CurrentYawSpeed = FMath::FInterpTo(CurrentYawSpeed, TargetYawSpeed, GetWorld()->GetDeltaSeconds(), 2.f);

	// Is there any left/right input?
	const bool bIsTurning = FMath::Abs(Val) > 0.2f;

	// If turning, yaw value is used to influence roll
	// If not turning, roll to reverse current roll value.
	//float TargetRollSpeed = bIsTurning ? (CurrentYawSpeed * 0.5f) : (GetActorRotation().Roll * -2.f);

	// Smoothly interpolate roll speed
	//CurrentRollSpeed = FMath::FInterpTo(CurrentRollSpeed, TargetRollSpeed, GetWorld()->GetDeltaSeconds(), 2.f);
}
void ATestProjectPawn::Fire()
{
		//
		//Hit contains information about what the raycast hit.
		FHitResult Hit;

		//The length of the ray in units.
		//For more flexibility you can expose a public variable in the editor
		float RayLength = 20000000;

		//The Origin of the raycast

		FVector StartLocation = PlaneMesh->GetSocketLocation("Gun");

		//FVector ActorForward = PlaneMesh->GetForwardVector();

		FRotator ActorRotation = PlaneMesh->GetSocketRotation("Gun");

		//FVector ForwardVector = FRotationMatrix(ActorRotation).GetScaledAxis(EAxis::X);
		//FVector ForwardVector= GetActorForwardVector();
		//FVector ForwardAmount = FVector(5000.f, 00.f, 00.f);

		FVector ForwardVector = FRotationMatrix(ActorRotation).GetScaledAxis(EAxis::X);
		//FVector StartLocation = PlaneMesh->GetSocketLocation("Gun");
		//The EndLocation of the raycast



		//FVector EndLocationRotator = ForwardVector.RotateAngleAxis(180, FVector(1, 0, 0));
		FVector EndLocation = StartLocation + (ForwardVector) * RayLength;

		//Collision parameters. The following syntax means that we don't want the trace to be complex
		FCollisionQueryParams CollisionParameters;
		//CollisionParameters.AddIgnoredActor(this);
		CollisionParameters.bTraceComplex = true;

		//Perform the line trace
		//The ECollisionChannel parameter is used in order to determine what we are looking for when performing the raycast
		if (GetWorld()->LineTraceSingleByChannel(Hit, StartLocation, EndLocation, ECollisionChannel::ECC_WorldStatic, CollisionParameters))
		{
			UE_LOG(LogTemp, Log, TEXT("Target hit!"))
		}

		//DrawDebugLine is used in order to see the raycast we performed
		//The boolean parameter used here means that we want the lines to be persistent so we can see the actual raycast
		//The last parameter is the width of the lines.
		DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::Red, true, -1, 0, 1.f);


}
void ATestProjectPawn::OnHit() {


}
void ATestProjectPawn::RotateLeft() {

	FRotator CurrentRotation = this->GetActorRotation();
	FRotator TargetRotation = CurrentRotation + FRotator(0.f, 0.f, 30.f);
	//this->SetActorRotation(FMath::(Lerp(CurrentRotation,FQuat(TargetRotation))))
	//GetOwner()->SetActorRotation(FMath::Lerp(CurrentRotation,)

}