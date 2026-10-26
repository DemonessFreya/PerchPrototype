// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PerchPrototypeCharacter.generated.h"

class APerchActor;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

// enum defining the state machine for perching
UENUM(BlueprintType)
enum class EPerchState : uint8
{
	WALKING UMETA(DisplayName = "Walking"),
	PERCHING UMETA(DisplayName = "Perching"),
	PERCHED UMETA(DisplayName = "Perched")
};

UCLASS()
class PERCHPROTOTYPE_API APerchPrototypeCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APerchPrototypeCharacter();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Raycast to perches and start perch
	UFUNCTION(BlueprintCallable, Category = "Perch")
	void TryStartPerch();

	// Perform perch
	UFUNCTION(BlueprintCallable, Category = "Perch")
	void PerchTo(float DeltaTime);
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Enchanced input
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* PerchInputAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* PerchInputMappingContext;

	// Handles perching when input triggered
	void HandlePerchInput(const FInputActionValue& Value);

private:
	// Define member variable for state machine
	UPROPERTY(VisibleAnywhere, Category = "Perch")
	EPerchState CurrentPerchState = EPerchState::WALKING;

	// The perch node that is selected in gameplay
	UPROPERTY(VisibleDefaultsOnly, Category = "Perch")
	APerchActor* TargetPerch;

	UPROPERTY(VisibleAnywhere, Category = "Perch")
	FVector StartPerchLocation; // player location when initiating perch
	UPROPERTY(VisibleAnywhere, Category = "Perch")
	float PerchProgress; // time elapsed during perch

};
