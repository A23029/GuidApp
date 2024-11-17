// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DirectionDetector.generated.h"

UCLASS()
class GUIDAPP_API ADirectionDetector : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADirectionDetector();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "HoloLens")
	FString CompassDirection;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "HoloLens")
	float vYaw;//viewYaw
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "HoloLens")
	float vPosX;//viewPosition
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "HoloLens")
	float vPosY;//viewPosition
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "HoloLens")
	float vPosZ;//viewPosition
	
private:
	void GetHMDOrientation();
	FString DetermineCompassDirection(float Yaw);
	FRotator HMDOrientation;

};
