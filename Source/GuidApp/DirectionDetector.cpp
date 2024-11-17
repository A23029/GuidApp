// Fill out your copyright notice in the Description page of Project Settings.


#include "DirectionDetector.h"
#include "IXRTrackingSystem.h"
#include "HeadMountedDisplayFunctionLibrary.h"

// Sets default values
ADirectionDetector::ADirectionDetector()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
    vYaw = 0;
}

// Called when the game starts or when spawned
void ADirectionDetector::BeginPlay()
{
	Super::BeginPlay();
    GetHMDOrientation();
}

// Called every frame
void ADirectionDetector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	GetHMDOrientation();
}

void ADirectionDetector::GetHMDOrientation()
{
    if (GEngine->XRSystem.IsValid())
    {
        FQuat Orientation;
        FVector Position;
        if (GEngine->XRSystem->GetCurrentPose(IXRTrackingSystem::HMDDeviceId, Orientation, Position))
        {
            HMDOrientation = Orientation.Rotator();
            if (HMDOrientation.Yaw < 0.0f)
            {
                HMDOrientation.Yaw += 360.0f;
            }
            CompassDirection = DetermineCompassDirection(HMDOrientation.Yaw);
            vYaw = HMDOrientation.Yaw;
            vPosX = Position.X;
            vPosY = Position.Y;
            vPosZ = Position.Z;
        }
    }
}

FString ADirectionDetector::DetermineCompassDirection(float Yaw)
{
    if (Yaw > 135.0f && Yaw < 225.0f )
    {
        return "North"; 
    }
    else if (Yaw >= 45.0f && Yaw <= 135.0f)
    {
        return "West";
    }
    else if (Yaw >= 225.0f && Yaw <= 315.0f)
    {
        return "East";
    }
    else 
    {
        return "south";
    }
}