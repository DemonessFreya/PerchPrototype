// Fill out your copyright notice in the Description page of Project Settings.


#include "PerchPrototypeCharacter.h"
#include "PerchActor.h"
#include "GameFramework/PlayerController.h"
#include "DrawDebugHelpers.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

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

	PerchTo(DeltaTime);
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
	if (CurrentPerchState == EPerchState::PERCHING)
	{
		CurrentPerchState = EPerchState::WALKING;

		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);

		TargetPerch = nullptr;
		UE_LOG(LogTemp, Warning, TEXT("Broke off perch!"));
		return;
	}

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
	FVector CameraForward = CameraRotation.Vector();

	// Find all perch actors in level
	TArray<AActor*> AllPerches;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APerchActor::StaticClass(), AllPerches);

	APerchActor* BestPerch = nullptr;
	float BestScore = -1.0f; // Higher dot product means closer to crosshair
	FVector BestHitLocation = FVector::ZeroVector;

	float MaxRange = 5000.0f;
	// Adjust angle to make targeting window tighter or larger
	float MaxAngleCos = FMath::Cos(FMath::DegreesToRadians(5.0f));

	for (AActor* Actor : AllPerches)
	{
		APerchActor* Perch = Cast<APerchActor>(Actor);
		if (!IsValid(Perch)) continue;

		FVector PerchLocation = Perch->GetActorLocation();
		float DistSq = FVector::DistSquared(CameraLocation, PerchLocation);

		// Ignore perches outside of max distance
		if (DistSq > (MaxRange * MaxRange)) continue;

		FVector DirectionToPerch = (PerchLocation - CameraLocation).GetSafeNormal();
		float DotProduct = FVector::DotProduct(CameraForward, DirectionToPerch);

		// Ignore perches outside cone
		if (DotProduct < MaxAngleCos) continue;

		// Check line of sight
		FHitResult LoSHit;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		QueryParams.AddIgnoredActor(Perch);

		bool bHitSomething = GetWorld()->LineTraceSingleByChannel
		(
			LoSHit,
			CameraLocation,
			PerchLocation,
			ECC_Visibility,
			QueryParams
		);

		// If something was hit before reaching perch
		if (bHitSomething) continue;

		// Select best perch
		if (DotProduct > BestScore)
		{
			BestScore = DotProduct;
			BestPerch = Perch;
			BestHitLocation = PerchLocation;
		}
	}

	// Handle final selection
	if (BestPerch)
	{
		TargetPerch = BestPerch;
		StartPerchLocation = GetActorLocation();
		CurrentPerchState = EPerchState::PERCHING;

		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
		GetCharacterMovement()->Velocity = FVector::ZeroVector;

		// Green debug line showing successful target lock
		DrawDebugLine(GetWorld(), CameraLocation, BestHitLocation, FColor::Green, false, 2.0f, 0, 2.0f);
		UE_LOG(LogTemp, Warning, TEXT("Successfully locked onto perch via cone targeting!"));
	}
	else
	{
		// Red debug line showing no valid unblocked perches found
		DrawDebugLine(GetWorld(), CameraLocation, CameraLocation + (CameraForward * 1000.f), FColor::Red, false, 2.0f, 0, 2.0f);
		UE_LOG(LogTemp, Warning, TEXT("No valid unblocked perch found in range/cone."));
	}
}

void APerchPrototypeCharacter::PerchTo(float DeltaTime)
{
	// If we are currently flying toward a perch actor
	if (CurrentPerchState == EPerchState::PERCHING)
	{
		if (TargetPerch)
		{
			// Get where perch is
			FVector TargetLocation = TargetPerch->GetActorLocation() + FVector(0.f, 0.f, 75.f);

			// Interpolate to target vector
			FVector NewLocation = FMath::VInterpTo(GetActorLocation(), TargetLocation, DeltaTime, 10.0f);
			SetActorLocation(NewLocation);

			// Check if arrived (threshold: 15.0f)
			float DistanceToTarget = FVector::Dist(GetActorLocation(), TargetLocation);
			if (DistanceToTarget < 15.0f)
			{
				// Snap to perch position to avoid overshoot (replace with integral?)
				SetActorLocation(TargetLocation);

				GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);

				// Transition to perched state
				CurrentPerchState = EPerchState::WALKING;

				LaunchCharacter(FVector(0.f, 0.f, 300.f), true, true);

				TargetPerch = nullptr;

				UE_LOG(LogTemp, Warning, TEXT("Successfully Arrived!"));
			}
		}
		else
		{
			// Fallback to walking state if perching blocked somehow
			CurrentPerchState = EPerchState::WALKING;
			GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
		}
	}
}
