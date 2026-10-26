// Fill out your copyright notice in the Description page of Project Settings.


#include "PerchPrototypeCharacter.h"
#include "PerchActor.h"
#include "GameFramework/PlayerController.h"
#include "DrawDebugHelpers.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

// Sets default values
APerchPrototypeCharacter::APerchPrototypeCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APerchPrototypeCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Activate input mapping context
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(PerchInputMappingContext, 0);
		}
	}
}

// Called every frame
void APerchPrototypeCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void APerchPrototypeCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Bind input action to HandlePerchInput
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (PerchInputAction)
		{
			EnhancedInputComponent->BindAction(PerchInputAction, ETriggerEvent::Started, this, &APerchPrototypeCharacter::HandlePerchInput);
		}
	}
}

void APerchPrototypeCharacter::HandlePerchInput(const FInputActionValue& Value)
{
	// Call raycast
	TryStartPerch();
}

void APerchPrototypeCharacter::TryStartPerch()
{
	// Only perch if not currently perching
	if (CurrentPerchState == EPerchState::PERCHING || CurrentPerchState == EPerchState::PERCHED)
	{
		return;
	}

	// Get player controller to see where camera is looking
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!IsValid(PC)) return;

	FVector CameraLocation;
	FRotator CameraRotation;
	PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

	// Define start of raycast
	FVector TraceStart = CameraLocation;
	FVector TraceEnd = TraceStart + (CameraRotation.Vector() * 5000.f);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this); // raycast can't hit player character

	float SphereRadius = 60.0f;
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(SphereRadius);

	// Perform sphere sweep
	bool bHit = GetWorld()->SweepSingleByChannel
	(
		HitResult,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		ECC_Visibility,
		SphereShape,
		QueryParams
	);

	// Check what was hit
	if (bHit && HitResult.GetActor())
	{
		// Try cast actor to APerchActor class
		if (APerchActor* HitPerch = Cast<APerchActor>(HitResult.GetActor()))
		{
			// Hit perch actor
			TargetPerch = HitPerch;
			StartPerchLocation = GetActorLocation();
			PerchProgress = 0.0f;
			CurrentPerchState = EPerchState::PERCHING;

			// Debug line to view where raycast is (hit perch actor)
			DrawDebugLine(GetWorld(), TraceStart, HitResult.Location, FColor::Green, false, 2.0f, 0, 2.0f);
		}
		else
		{
			// Debug line to view where raycast is (missed perch actor)
			DrawDebugLine(GetWorld(), TraceStart, HitResult.Location, FColor::Yellow, false, 2.0f, 0, 2.0f);
		}
	}
	else
	{
		// Debug line to view where raycast is (missed perch actor, out of bounds)
		DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Red, false, 2.0f, 0, 2.0f);
	}
}
