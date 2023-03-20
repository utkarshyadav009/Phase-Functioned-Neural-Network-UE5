// Copyright 2018 Sticks & Stones. All Rights Reserved.
#include "Animation/AnimComponents/TrajectoryComponent.h"

#include "Core/Character/PFNNCharacter.h"
#include "Utilities/PFNNHelperFunctions.h"

#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Runtime/NavigationSystem/Public/NavigationSystem.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <ThirdParty/glm/gtc/matrix_transform.hpp>
#include <ThirdParty/glm/gtc/type_ptr.hpp>
#include <ThirdParty/glm/gtx/transform.hpp>
#include <ThirdParty/glm/gtx/quaternion.hpp>


#include <fstream>

UTrajectoryComponent::UTrajectoryComponent(): ExtraStrafeSmooth(0), ExtraGaitSmooth(0), ExtraJointSmooth(0)
{
	PrimaryComponentTick.bCanEverTick = true;

	Width = 25.0f;
	TargetDirection = glm::vec3(0.0f,0.0f,1.0f);
	ExtraVelocitySmooth	= 0.9f;
	ExtraStrafeVelocity	= 0.9f;
	ExtraDirectionSmooth= 0.9f;
	ExtraStrafeSmooth	= 0.9f;
	ExtraGaitSmooth		= 0.1f;
	ExtraJointSmooth	= 0.5f;

	StrafeTarget= 0.0f;
	StrafeAmount= 0.0f;
	Responsive	= 0.0f;

	TargetVelocity = glm::vec3();

	Positions[LENGTH] =		{ glm::vec3(0.0f, 0.0f, 0.0f) };
	Rotations[LENGTH] =		{ glm::mat4(1.0f) };
	Directions[LENGTH] =	{ glm::vec3(0.0f, 0.0f, 1.0f) };
	Heights[LENGTH] =		{ 0.0f };
	GaitJog[LENGTH] =		{ 0.0f };
	GaitWalk[LENGTH] =		{ 0.0f };
	GaitBump[LENGTH] =		{ 0.0f };
	GaitJump[LENGTH] =		{ 0.0f };
	GaitStand[LENGTH] =		{ 0.0f };

	CurrentFrameInput = glm::vec2(0.0f);

	OwnerPawn = nullptr;

	bIsTrajectoryDebuggingEnabled = false;

}

UTrajectoryComponent::~UTrajectoryComponent()
{
}

void UTrajectoryComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerPawn = Cast<APawn>(GetOwner());
	IsValid(OwnerPawn);

	Directions[LENGTH] = { UPFNNHelperFunctions::XZYTranslationToXYZ(glm::vec3(OwnerPawn->GetActorForwardVector().X, OwnerPawn->GetActorForwardVector().Y, 0.0f)) };

	tickcounter = 0;
}

void UTrajectoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


	glm::vec3 Direction1 = glm::vec3(1.0f, 0.0, 0.0);
	glm::vec3 Direction2 = glm::vec3(0.0f, 0.0, 0.0f);
	
	glm::vec3 Mixed = MixDirections(Direction1, Direction2, 0.5f);

#ifdef WITH_EDITOR
	DrawDebugTrajectory();
#endif

}

glm::vec3 UTrajectoryComponent::GetRootPosition() const
{
	return UPFNNHelperFunctions::XZYTranslationToXYZ(FVector(GetOwner()->GetActorLocation().X * 0.01, 
															 GetOwner()->GetActorLocation().Y * 0.01, 
															 Heights[LENGTH/2]));
}

glm::vec3 UTrajectoryComponent::GetPreviousRootPosition() const
{
	return glm::vec3(
		Positions[LENGTH / 2 - 1].x,
		Heights[LENGTH / 2 - 1],
		Positions[LENGTH / 2 - 1].z
	);
}

glm::mat3 UTrajectoryComponent::GetRootRotation() const
{
	return Rotations[LENGTH / 2];
}

glm::mat3 UTrajectoryComponent::GetPreviousRootRotation() const
{
	return Rotations[LENGTH / 2 - 1];
}

void UTrajectoryComponent::TickGaits()
{
	//Updating of the gaits
	const float MovementCutOff = 0.01f;
	const float JogCuttoff = 0.5f;
	const auto TrajectoryLength = glm::length(TargetVelocity);
	//const auto NormalizedTrajectoryLength = glm::normalize(TrajectoryLength);
	if (TrajectoryLength < MovementCutOff) //Standing
	{
		const float StandingClampMin = 0.0f;
		const float StandingClampMax = 1.0f;
		const float StandingAmount = 1.0f - glm::clamp(glm::length(TargetVelocity) / 0.1f, StandingClampMin, StandingClampMax);
		GaitStand[LENGTH / 2] = glm::mix(GaitStand[LENGTH / 2], StandingAmount, ExtraGaitSmooth);
		GaitWalk[LENGTH / 2] = glm::mix(GaitWalk[LENGTH / 2], 0.0f, ExtraGaitSmooth);
		GaitJog	[LENGTH / 2] = glm::mix(GaitJog	[LENGTH / 2], 0.0f, ExtraGaitSmooth);
		GaitJump[LENGTH / 2] = glm::mix(GaitJump[LENGTH / 2], 0.0f, ExtraGaitSmooth);
		GaitBump[LENGTH / 2] = glm::mix(GaitBump[LENGTH / 2], 0.0f, ExtraGaitSmooth);

	}
	else if (glm::abs(TargetVelocity.x) > JogCuttoff || glm::abs(TargetVelocity.y) > JogCuttoff) //Jog
	{

		GaitStand[LENGTH / 2] = glm::mix(GaitStand[LENGTH / 2], 0.0f, ExtraGaitSmooth);
		GaitWalk[LENGTH / 2] = glm::mix(GaitWalk[LENGTH / 2], 0.0f, ExtraGaitSmooth);
		GaitJog	[LENGTH / 2] = glm::mix(GaitJog	[LENGTH / 2], 1.0f, ExtraGaitSmooth);
		GaitJump[LENGTH / 2] = glm::mix(GaitJump[LENGTH / 2], 0.0f, ExtraGaitSmooth);
		GaitBump[LENGTH / 2] = glm::mix(GaitBump[LENGTH / 2], 0.0f, ExtraGaitSmooth);
	}
	else //Walking
	{
		GaitStand[LENGTH / 2] = glm::mix(GaitStand[LENGTH / 2], 0.0f, ExtraGaitSmooth);
		GaitWalk[LENGTH / 2] = glm::mix(GaitWalk[LENGTH / 2], 1.0f, ExtraGaitSmooth);
		GaitJog	[LENGTH / 2] = glm::mix(GaitJog	[LENGTH / 2], 0.0f, ExtraGaitSmooth);
		GaitJump[LENGTH / 2] = glm::mix(GaitJump[LENGTH / 2], 0.0f, ExtraGaitSmooth);
		GaitBump[LENGTH / 2] = glm::mix(GaitBump[LENGTH / 2], 0.0f, ExtraGaitSmooth);
	}
	
	if (bIsTrajectoryDebuggingEnabled)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::Printf(TEXT("%f Gait stand "),	GaitStand[LENGTH / 2]));
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::Printf(TEXT("%f Gait walks "),	GaitWalk[LENGTH / 2]));
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::Printf(TEXT("%f Gait Jog "),		GaitJog[LENGTH / 2]));
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::Printf(TEXT("%f Gait Jump "),		GaitJump[LENGTH / 2]));
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::Printf(TEXT("%f Gait Bump "),		GaitBump[LENGTH / 2]));
	}

}

void UTrajectoryComponent::PredictFutureTrajectory()
{
	//Predicting future trajectory
	glm::vec3 TrajectoryPositionsBlend[LENGTH] = { glm::vec3(0.0f) };
	TrajectoryPositionsBlend[LENGTH / 2] = Positions[LENGTH / 2];

	const float BiasPosition = Responsive ? glm::mix(2.0f, 2.0f, StrafeAmount) : glm::mix(0.5f, 1.0f, StrafeAmount);
	const float BiasDirection = Responsive ? glm::mix(5.0f, 3.0f, StrafeAmount) : glm::mix(2.0f, 0.5f, StrafeAmount);

	for (int i = LENGTH / 2 + 1; i < LENGTH; i++)
	{

		const float ScalePosition = (1.0f - powf(1.0f - (static_cast<float>(i - LENGTH / 2) / (LENGTH / 2)), BiasPosition));
		const float ScaleDirection = (1.0f - powf(1.0f - (static_cast<float>(i - LENGTH / 2) / (LENGTH / 2)), BiasDirection));

		TrajectoryPositionsBlend[i] = TrajectoryPositionsBlend[i - 1] + glm::mix(Positions[i] - Positions[i], TargetVelocity, ScalePosition);
		glm::vec3 DistanceFromPreviousNode = Positions[i] - Positions[i - 1];
		//TrajectoryPositionsBlend[i] = TrajectoryPositionsBlend[i-1] + glm::mix(DistanceFromPreviousNode, TargetVelocity, ScalePosition);

		//TODO: Add wall colision for future trajectory - 1519

		Directions[i] = MixDirections(Directions[i], TargetDirection, ScaleDirection);

		/*Heights[i] = Heights[LENGTH / 2];*/

		GaitStand[i]= GaitStand[LENGTH / 2];
		GaitWalk[i] = GaitWalk[LENGTH / 2];
		GaitJog[i]	= GaitJog [LENGTH / 2];
		GaitJump[i] = GaitJump[LENGTH / 2];
		GaitBump[i] = GaitBump[LENGTH / 2];
		
	}

	for (int i = LENGTH / 2 + 1; i < LENGTH; i++)
	{
		Positions[i] = TrajectoryPositionsBlend[i];
	}


}

void UTrajectoryComponent::TickRotations()
{
	for (int i = 0; i < LENGTH; i++)
	{
		Rotations[i] = glm::mat3(glm::rotate(atan2f(Directions[i].x, Directions[i].z), glm::vec3(0.0f, 1.0f, 0.0f)));
	}
}

void UTrajectoryComponent::TickHeights()
{
	const float DistanceOffsetHeight = 100.f;
	const float DistanceOffsetFloor = 450.f;
	//Trajectory height
	for (int i = LENGTH / 2; i < LENGTH; i++)
	{
		FCollisionQueryParams CollisionParams = FCollisionQueryParams(FName(TEXT("GroundGeometryTrace")), true, OwnerPawn);
		CollisionParams.AddIgnoredActor(GetOwner());

		FHitResult OutResult;

		const auto StartPoint = Positions[i] * 100.f;
		const auto EndPoint = Positions[i] * 100.f;

		FVector UStartPoint = UPFNNHelperFunctions::XYZTranslationToXZY(StartPoint);
		FVector UEndPoint = UPFNNHelperFunctions::XYZTranslationToXZY(EndPoint);

		UStartPoint.Z = OwnerPawn->GetActorLocation().Z + DistanceOffsetHeight;
		UEndPoint.Z = OwnerPawn->GetActorLocation().Z - DistanceOffsetFloor;

		if(OwnerPawn->GetWorld()->LineTraceSingleByChannel(OutResult, UStartPoint, UEndPoint, ECollisionChannel::ECC_WorldStatic, CollisionParams))
		{
			Positions[i].y = OutResult.Location.Z * 0.01f;
		}
	}
	//Convert position to world space


	//for (int i = 0; i < LENGTH; i += 10)
	//{
	//	Heights[LENGTH / 2] += (Positions[i].y / ((LENGTH) / 10));
	//}
}

void UTrajectoryComponent::UpdatePastTrajectory()
{
	for (int i = 0; i < LENGTH / 2; i++)
	{
		Positions[i]	= Positions [i + 1];
		Directions[i]	= Directions[i + 1];
		Rotations[i]	= Rotations [i + 1];
		Heights[i]		= Heights	[i + 1];
		GaitStand[i]	= GaitStand [i + 1];
		GaitWalk[i]		= GaitWalk  [i + 1];
		GaitJog[i]		= GaitJog	[i + 1];
		GaitBump[i]		= GaitBump	[i + 1];
	}
}

glm::vec3 UTrajectoryComponent::MixDirections(const glm::vec3 arg_XDirection, const glm::vec3 arg_YDirection,
	const float arg_Scalar)
{
	const glm::quat XQuat = glm::angleAxis(atan2f(arg_XDirection.x, arg_XDirection.z), glm::vec3(0.0f, 1.0f, 0.0f));
	const glm::quat YQuat = glm::angleAxis(atan2f(arg_YDirection.x, arg_YDirection.z), glm::vec3(0.0f, 1.0f, 0.0f));
	const glm::quat ZQuat = glm::slerp(XQuat, YQuat, arg_Scalar);
	return ZQuat * glm::vec3(0.0f, 0.0f, 1.0f);
}


//USES NATIVE CPP TO ENSURE IT CAN BE USED IN REFERENCE PROJECT
void UTrajectoryComponent::LogTrajectoryData(int arg_FrameCount)
{
	try 
	{
		std::ofstream fs;
		fs.open("UE4_Trajectory.log", std::ios::out);

		if (fs.is_open()) 
		{
			fs << "UE4_Implementation" << std::endl;
			fs << "TrajectoryLog Frame[" << arg_FrameCount << "]" << std::endl << std::endl;

			fs << "#Basic Variables" << std::endl;
			fs << "TargetDirection: " << TargetDirection.x << "X, " << TargetDirection.y << "Y, " << TargetDirection.z << "Z" << std::endl;
			fs << "TargetVelocity:  " << TargetVelocity.x << "X, " << TargetVelocity.y << "Y, " << TargetVelocity.z << "Z" << std::endl;
			fs << "Width:           " << Width << std::endl;
			fs << "StrafeAmount:    " << StrafeAmount << std::endl;
			fs << "StrafeTarget:    " << StrafeTarget << std::endl;
			fs << "Responsive:      " << Responsive << std::endl;
			fs << "#End Basic Variables" << std::endl << std::endl;

			fs << "#Extra Smoothing values" << std::endl;
			fs << "	ExtraVelocitySmooth:	" << ExtraVelocitySmooth << std::endl;
			fs << "	ExtraDirectionSmooth:   " << ExtraDirectionSmooth << std::endl;
			fs << "	ExtraStrafeVelocity:	" << ExtraStrafeVelocity << std::endl;
			fs << "	ExtraStrafeSmooth:      " << ExtraStrafeSmooth << std::endl;
			fs << "	ExtraGaitSmooth:        " << ExtraGaitSmooth << std::endl;
			fs << "	ExtraJointSmooth:       " << ExtraJointSmooth << std::endl;
			fs << "#End Extra Smoothing values" << std::endl << std::endl;

			fs << "#Positional Data" << std::endl;
			for (size_t i = 0; i < LENGTH; i++)
			{
				fs << "TrajectoryNode[" << i << "]" << std::endl;
				fs << " Position:  " << Positions[i].x << "X, " << Positions[i].y << "Y, " << Positions[i].z << "Z" << std::endl;
				fs << "	Direction: " << Directions[i].x << "X, " << Directions[i].y << "Y, " << Directions[i].z << "Z" << std::endl;
				for (size_t x = 0; x < 3; x++)
				{
				fs << "	Rotation:  " << Rotations[i][x].x << "X, " << Rotations[i][x].y << ", " << Rotations[i][x].z << std::endl;
				}
			}
			fs << "#End Positional Data" << std::endl << std::endl;

			fs << "#Gaits" << std::endl;
			for (size_t i = 0; i < LENGTH; i++)
			{
				fs << "Gait[" << i << "]" << std::endl;
				fs << "	GaitStand: " << GaitStand[i] << std::endl;
				fs << "	GaitWalk:  " << GaitWalk[i] << std::endl;
				fs << "	GaitJog:   " << GaitJog[i] << std::endl;
				fs << "	GaitJump:  " << GaitJump[i] << std::endl;
				fs << "	GaitBump:  " << GaitBump[i] << std::endl;
			}
			fs << "#End Gaits" << std::endl << std::endl;
		}
		else 
		{
			throw std::exception();
		}
		fs.close();
		return;
	}
	catch (std::exception e) 
	{
#ifdef WITH_EDITOR
		UE_LOG(LogTemp, Error, TEXT("Failed to Log Trajectory output data"));
#endif
	}
}

void UTrajectoryComponent::TickTrajectory()
{
	TickInput();
	CalculateTargetDirection();
	TickGaits();
	PredictFutureTrajectory();
	TickRotations();
	TickHeights();
}

void UTrajectoryComponent::TickInput()
{
	APFNNCharacter* PFNNCharacter = Cast<APFNNCharacter>(GetOwner());
	if (PFNNCharacter)
	{
		UPawnMovementComponent* MovementComponent = PFNNCharacter->GetMovementComponent();
		glm::vec3 FlippedVelocity = UPFNNHelperFunctions::XZYTranslationToXYZ(MovementComponent->Velocity);
		CurrentFrameInput = glm::vec2(FlippedVelocity.x, FlippedVelocity.z);
		CurrentFrameInput = glm::normalize(CurrentFrameInput);
	}

	if (glm::abs(CurrentFrameInput.x) + glm::abs(CurrentFrameInput.y) < 0.305f)
	{
		CurrentFrameInput = glm::vec2(0.0f);
	}
}

void UTrajectoryComponent::CalculateTargetDirection()
{
	glm::vec3 FlippedForward = UPFNNHelperFunctions::XZYTranslationToXYZ(glm::vec3(OwnerPawn->GetActorForwardVector().X, OwnerPawn->GetActorForwardVector().Y, 0.0f));
	glm::vec3 TrajectoryTargetDirectionNew = glm::normalize(FlippedForward);
	const glm::mat3 TrajectoryTargetRotation = glm::mat3(glm::rotate(atan2f(
		TrajectoryTargetDirectionNew.x,
		TrajectoryTargetDirectionNew.z), glm::vec3(0.0f, 1.0f, 0.0f)));

	const float TargetVelocitySpeed = OwnerPawn->GetVelocity().SizeSquared() / (OwnerPawn->GetMovementComponent()->GetMaxSpeed() * OwnerPawn->GetMovementComponent()->GetMaxSpeed()) / 7.5f; //7.5 is current training walking speed

	const glm::vec3 TrajectoryTargetVelocityNew = TargetVelocitySpeed * (glm::vec3(CurrentFrameInput.x, 0.0f, CurrentFrameInput.y));
	TargetVelocity = glm::mix(TargetVelocity, TrajectoryTargetVelocityNew, ExtraVelocitySmooth);
	StrafeAmount = glm::mix(StrafeAmount, StrafeTarget, ExtraStrafeSmooth);
	const glm::vec3 TrajectoryTargetVelocityDirection = glm::length(TargetVelocity) < 1e-05 ? TargetDirection : glm::normalize(TargetVelocity);
	TrajectoryTargetDirectionNew = MixDirections(TrajectoryTargetVelocityDirection, TrajectoryTargetDirectionNew, StrafeAmount);
	TargetDirection = TrajectoryTargetDirectionNew; MixDirections(TargetDirection, TrajectoryTargetDirectionNew, ExtraDirectionSmooth);
	
	if (bIsTrajectoryDebuggingEnabled)
	{
		FVector FlippedTargetDirectionNew = UPFNNHelperFunctions::XYZTranslationToXZY(TrajectoryTargetDirectionNew);
		FVector FlippedTargetDirection = UPFNNHelperFunctions::XYZTranslationToXZY(TargetDirection);
		FVector FlippedCurrentFrameInput = UPFNNHelperFunctions::XYZTranslationToXZY(glm::vec3(CurrentFrameInput.x, 0.0f, CurrentFrameInput.y));
		FVector FlippedTargetVelocityNew = UPFNNHelperFunctions::XYZTranslationToXZY(TrajectoryTargetVelocityNew);
		FVector FlippedTargetVelocity = UPFNNHelperFunctions::XYZTranslationToXZY(TargetVelocity);

		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, FString::Printf(TEXT("%s Current frame input"), *FlippedCurrentFrameInput.ToString()));
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Blue, FString::Printf(TEXT("%s TrajectoryTargetDirectionNew"), *FlippedTargetDirectionNew.ToString()));
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, FString::Printf(TEXT("%s TargetDirection"), *FlippedTargetDirection.ToString()));
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, FString::Printf(TEXT("%s TargetVelocityNew"), *FlippedTargetVelocityNew.ToString()));
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, FString::Printf(TEXT("%s TargetVelocity"), *FlippedTargetVelocity.ToString()));
		
		FVector ActorLoc = GetOwner()->GetActorLocation();

		FVector FrameInputActor = ActorLoc + FVector(0.0f, 0.0f, 200.0f);
		FVector TargetDirectionActor = ActorLoc + FVector(0.0f, 0.0f, 250.0f);
		FVector TargetVelocityActor = ActorLoc + FVector(0.0f, 0.0f, 225.0f);

		DrawDebugDirectionalArrow(GetWorld(), FrameInputActor, FrameInputActor + FlippedCurrentFrameInput * 100, 0.0f, FColor::White, false, -1, 0, 2);
		
		DrawDebugDirectionalArrow(GetWorld(),TargetDirectionActor, TargetDirectionActor + FlippedTargetDirectionNew * 100.0f, 0.0f, FColor::Blue, false, -1, 0, 2);
		DrawDebugDirectionalArrow(GetWorld(), TargetDirectionActor, TargetDirectionActor + FlippedTargetDirection * 100.0f, 0.0f, FColor::Green, false, -1, 0, 2);
		
		DrawDebugDirectionalArrow(GetWorld(), TargetVelocityActor, TargetVelocityActor + FlippedTargetVelocityNew * 1000.0f, 0.0f, FColor::Red, false, -1, 0, 2);
		DrawDebugDirectionalArrow(GetWorld(), TargetVelocityActor, TargetVelocityActor + FlippedTargetVelocity * 1000.0f, 0.0f, FColor::Yellow, false, -1, 0, 2);
	}
}

void UTrajectoryComponent::DrawDebugTrajectory()
{
	if (bIsTrajectoryDebuggingEnabled)
	{
		const glm::vec3 StartingPoint = Positions[0] * 100.0f;			//Get the leading point of the trajectory
		const glm::vec3 MidPoint = Positions[LENGTH / 2] * 100.0f;		//Get the mid point/player point of the trajectory
		const glm::vec3 EndingPoint = Positions[LENGTH - 1] * 100.0f;	//Get the ending point of the trajectory

		for (size_t i = 0; i < LENGTH; i++)
		{
			if (i % 10 == 0 || i == LENGTH-1) 
			{
				FVector DebugLocation = (UPFNNHelperFunctions::XYZTranslationToXZY(Positions[i]) * 100.0f);
				DrawDebugPoint(GetWorld(), DebugLocation, 7.5f, FColor::Red);

				FVector CrossProduct = FVector::CrossProduct(FVector(0.0f,0.0f,1.0f), UPFNNHelperFunctions::XYZTranslationToXZY(Directions[i])) * 50.0f;

				DrawDebugPoint(GetWorld(), DebugLocation + CrossProduct, 10.0f, FColor::Black);
				DrawDebugPoint(GetWorld(), DebugLocation - CrossProduct, 10.0f, FColor::Black);

			}
		}

		FVector DebugStartingPoint = UPFNNHelperFunctions::XYZTranslationToXZY(StartingPoint);
		FVector DebugMidPoint = UPFNNHelperFunctions::XYZTranslationToXZY(MidPoint);
		FVector DebugEndPoint = UPFNNHelperFunctions::XYZTranslationToXZY(EndingPoint);

		DrawDebugPoint(GetWorld(), DebugStartingPoint, 10.0f, FColor::Red);
		DrawDebugPoint(GetWorld(), DebugMidPoint, 10.0f, FColor::Red);
		DrawDebugPoint(GetWorld(), DebugEndPoint, 10.0f, FColor::Red);

		DrawDebugString(GetWorld(), DebugStartingPoint, TEXT("Past"), nullptr, FColor::Blue, 0.001f);
		DrawDebugString(GetWorld(), DebugMidPoint, TEXT("Current"), nullptr, FColor::Blue, 0.001f);
		DrawDebugString(GetWorld(), DebugEndPoint, TEXT("Future"), nullptr, FColor::Blue, 0.001f);
		
		const FVector StartingDirection = UPFNNHelperFunctions::XYZTranslationToXZY(Directions[0]);
		const FVector MidDirection = UPFNNHelperFunctions::XYZTranslationToXZY(Directions[LENGTH / 2]);
		const FVector EndDirection = UPFNNHelperFunctions::XYZTranslationToXZY(Directions[LENGTH - 1]);

		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Black, FString::Printf(TEXT("%s Past Direction"), *StartingDirection.ToString()));
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Purple, FString::Printf(TEXT("%s Past Position"), *DebugStartingPoint.ToString()));
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Black, FString::Printf(TEXT("%s Present Direction"), *MidDirection.ToString()));
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Purple, FString::Printf(TEXT("%s Present Position"), *DebugMidPoint.ToString()));
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Black, FString::Printf(TEXT("%s Future Direction"), *EndDirection.ToString()));
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Purple, FString::Printf(TEXT("%s Future Position"), *DebugEndPoint.ToString()));

	}
}

